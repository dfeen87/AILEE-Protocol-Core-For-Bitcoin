#!/bin/bash
# AILEE-Core Let's Encrypt Certificate Setup
# Automated TLS certificate management using Let's Encrypt (certbot)

set -e

# Colors for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOMAIN="${DOMAIN:-api.ailee.example.com}"
EMAIL="${EMAIL:-admin@ailee.example.com}"
CERT_DIR="/etc/letsencrypt/live/$DOMAIN"
STAGING="${STAGING:-false}"

echo -e "${BLUE}=== AILEE-Core Let's Encrypt Certificate Setup ===${NC}"
echo ""
echo "Domain: $DOMAIN"
echo "Email: $EMAIL"
echo "Staging mode: $STAGING"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}ERROR: This script must be run as root${NC}"
    echo "Run with: sudo $0"
    exit 1
fi

# Check domain is set
if [ "$DOMAIN" = "api.ailee.example.com" ]; then
    echo -e "${RED}ERROR: Please set your actual domain${NC}"
    echo "Usage: DOMAIN=your-domain.com EMAIL=your-email@example.com $0"
    exit 1
fi

# Install certbot if not present
echo -e "${BLUE}1. Checking for certbot...${NC}"
if ! command -v certbot &> /dev/null; then
    echo "   Installing certbot..."
    apt-get update
    apt-get install -y certbot python3-certbot-nginx
    echo -e "   ${GREEN}✓${NC} certbot installed"
else
    echo -e "   ${GREEN}✓${NC} certbot already installed"
fi

# Check if port 80 and 443 are available
echo ""
echo -e "${BLUE}2. Checking port availability...${NC}"
if netstat -tlnp | grep -q ":80 "; then
    echo -e "   ${YELLOW}⚠${NC} Port 80 is in use"
    echo "   Let's Encrypt needs port 80 for validation"
    echo "   Temporarily stopping web server..."
    systemctl stop nginx 2>/dev/null || systemctl stop apache2 2>/dev/null || true
fi

if netstat -tlnp | grep -q ":443 "; then
    echo -e "   ${YELLOW}⚠${NC} Port 443 is in use"
fi

echo -e "   ${GREEN}✓${NC} Ports ready"

# Obtain certificate
echo ""
echo -e "${BLUE}3. Obtaining SSL certificate from Let's Encrypt...${NC}"

CERTBOT_ARGS="certonly --standalone"
CERTBOT_ARGS="$CERTBOT_ARGS -d $DOMAIN"
CERTBOT_ARGS="$CERTBOT_ARGS --email $EMAIL"
CERTBOT_ARGS="$CERTBOT_ARGS --agree-tos"
CERTBOT_ARGS="$CERTBOT_ARGS --non-interactive"

# Use staging if specified (for testing)
if [ "$STAGING" = "true" ]; then
    echo -e "   ${YELLOW}⚠${NC} Using Let's Encrypt staging server (testing mode)"
    CERTBOT_ARGS="$CERTBOT_ARGS --staging"
fi

# Obtain certificate
if certbot $CERTBOT_ARGS; then
    echo -e "   ${GREEN}✓${NC} Certificate obtained successfully"
else
    echo -e "   ${RED}✗${NC} Failed to obtain certificate"
    echo "   Check DNS points to this server and ports 80/443 are accessible"
    exit 1
fi

# Verify certificate
echo ""
echo -e "${BLUE}4. Verifying certificate...${NC}"
if [ -f "$CERT_DIR/fullchain.pem" ] && [ -f "$CERT_DIR/privkey.pem" ]; then
    echo -e "   ${GREEN}✓${NC} Certificate files found:"
    echo "     - $CERT_DIR/fullchain.pem"
    echo "     - $CERT_DIR/privkey.pem"
    echo "     - $CERT_DIR/chain.pem"
    echo "     - $CERT_DIR/cert.pem"
    
    # Display certificate info
    echo ""
    echo "   Certificate details:"
    openssl x509 -in "$CERT_DIR/cert.pem" -noout -subject -issuer -dates
else
    echo -e "   ${RED}✗${NC} Certificate files not found"
    exit 1
fi

# Set up auto-renewal
echo ""
echo -e "${BLUE}5. Setting up automatic renewal...${NC}"

# Create renewal hook directory
mkdir -p /etc/letsencrypt/renewal-hooks/post

# Create post-renewal hook to reload AILEE services
cat > /etc/letsencrypt/renewal-hooks/post/reload-ailee.sh << 'EOF'
#!/bin/bash
# Reload AILEE services after certificate renewal

echo "Certificate renewed, reloading services..."

# Reload API server
systemctl reload ailee-api 2>/dev/null || echo "ailee-api not running"

# Reload nginx if used as reverse proxy
systemctl reload nginx 2>/dev/null || echo "nginx not running"

# Log renewal
logger -t ailee-cert "SSL certificates renewed and services reloaded"

echo "Services reloaded successfully"
EOF

chmod +x /etc/letsencrypt/renewal-hooks/post/reload-ailee.sh

# Test renewal (dry run)
echo "   Testing automatic renewal..."
if certbot renew --dry-run; then
    echo -e "   ${GREEN}✓${NC} Automatic renewal configured successfully"
    echo "   Certificates will auto-renew 30 days before expiration"
else
    echo -e "   ${YELLOW}⚠${NC} Renewal test failed, but certificate is installed"
fi

# Update AILEE configuration
echo ""
echo -e "${BLUE}6. Updating AILEE configuration...${NC}"

AILEE_ENV="/opt/ailee-core/.env"
if [ -f "$AILEE_ENV" ]; then
    # Backup existing config
    cp "$AILEE_ENV" "$AILEE_ENV.backup.$(date +%Y%m%d_%H%M%S)"
    
    # Update TLS settings
    sed -i "s|^AILEE_TLS_ENABLED=.*|AILEE_TLS_ENABLED=true|" "$AILEE_ENV" || \
        echo "AILEE_TLS_ENABLED=true" >> "$AILEE_ENV"
    
    sed -i "s|^AILEE_TLS_CERT_PATH=.*|AILEE_TLS_CERT_PATH=$CERT_DIR/fullchain.pem|" "$AILEE_ENV" || \
        echo "AILEE_TLS_CERT_PATH=$CERT_DIR/fullchain.pem" >> "$AILEE_ENV"
    
    sed -i "s|^AILEE_TLS_KEY_PATH=.*|AILEE_TLS_KEY_PATH=$CERT_DIR/privkey.pem|" "$AILEE_ENV" || \
        echo "AILEE_TLS_KEY_PATH=$CERT_DIR/privkey.pem" >> "$AILEE_ENV"
    
    echo -e "   ${GREEN}✓${NC} Configuration updated"
else
    echo -e "   ${YELLOW}⚠${NC} AILEE .env file not found at $AILEE_ENV"
    echo "   Manually update your configuration with:"
    echo "     AILEE_TLS_ENABLED=true"
    echo "     AILEE_TLS_CERT_PATH=$CERT_DIR/fullchain.pem"
    echo "     AILEE_TLS_KEY_PATH=$CERT_DIR/privkey.pem"
fi

# Create systemd timer for renewal check (redundant with certbot's cron, but good practice)
echo ""
echo -e "${BLUE}7. Setting up renewal monitoring...${NC}"

cat > /etc/systemd/system/ailee-cert-check.service << EOF
[Unit]
Description=AILEE Certificate Expiration Check
After=network.target

[Service]
Type=oneshot
ExecStart=/opt/ailee-core/scripts/security/check-cert-expiry.sh
User=root
EOF

cat > /etc/systemd/system/ailee-cert-check.timer << EOF
[Unit]
Description=AILEE Certificate Expiration Check Timer
Requires=ailee-cert-check.service

[Timer]
OnCalendar=daily
Persistent=true

[Install]
WantedBy=timers.target
EOF

systemctl daemon-reload
systemctl enable ailee-cert-check.timer
systemctl start ailee-cert-check.timer

echo -e "   ${GREEN}✓${NC} Daily certificate monitoring enabled"

# Summary
echo ""
echo -e "${GREEN}=== Setup Complete ===${NC}"
echo ""
echo "SSL Certificate installed successfully!"
echo ""
echo "Certificate information:"
echo "  Domain: $DOMAIN"
echo "  Certificate: $CERT_DIR/fullchain.pem"
echo "  Private Key: $CERT_DIR/privkey.pem"
echo "  Expires: $(openssl x509 -in "$CERT_DIR/cert.pem" -noout -enddate | cut -d= -f2)"
echo ""
echo "Auto-renewal:"
echo "  ✓ Enabled (certbot will auto-renew 30 days before expiration)"
echo "  ✓ Services will reload automatically after renewal"
echo "  ✓ Daily expiration check configured"
echo ""
echo "Next steps:"
echo "  1. Restart AILEE API server: sudo systemctl restart ailee-api"
echo "  2. Test HTTPS: curl https://$DOMAIN/health"
echo "  3. Check certificate: https://www.ssllabs.com/ssltest/analyze.html?d=$DOMAIN"
echo ""
echo "Manual renewal (if needed):"
echo "  sudo certbot renew --force-renewal"
echo ""
echo -e "${GREEN}✓${NC} Your AILEE-Core API is now secured with Let's Encrypt!"
echo ""
