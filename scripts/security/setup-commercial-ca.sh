#!/bin/bash
# AILEE-Core Commercial CA Certificate Setup
# Guide for installing certificates from DigiCert, GlobalSign, or other commercial CAs

set -e

# Colors for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOMAIN="${DOMAIN:-api.ailee.example.com}"
CERT_DIR="/etc/ailee/certs"
CSR_DIR="/tmp/ailee-csr"

echo -e "${BLUE}=== AILEE-Core Commercial CA Certificate Setup ===${NC}"
echo ""
echo "Domain: $DOMAIN"
echo "Certificate directory: $CERT_DIR"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}ERROR: This script must be run as root${NC}"
    echo "Run with: sudo $0"
    exit 1
fi

# Create directories
mkdir -p "$CERT_DIR"
mkdir -p "$CSR_DIR"

echo -e "${BLUE}Step 1: Generate Private Key and CSR${NC}"
echo ""
echo "This will generate:"
echo "  1. Private key (keep this secure!)"
echo "  2. Certificate Signing Request (CSR) to submit to your CA"
echo ""

# Check if key already exists
if [ -f "$CERT_DIR/server.key" ]; then
    echo -e "${YELLOW}⚠${NC} Private key already exists at $CERT_DIR/server.key"
    read -p "Generate new key? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Using existing private key"
        GENERATE_KEY=false
    else
        GENERATE_KEY=true
    fi
else
    GENERATE_KEY=true
fi

if [ "$GENERATE_KEY" = true ]; then
    echo "Generating 4096-bit RSA private key..."
    openssl genrsa -out "$CERT_DIR/server.key" 4096
    chmod 600 "$CERT_DIR/server.key"
    echo -e "${GREEN}✓${NC} Private key generated: $CERT_DIR/server.key"
fi

# Gather CSR information
echo ""
echo "Enter information for Certificate Signing Request:"
read -p "Country Code (2 letters, e.g., US): " COUNTRY
read -p "State/Province: " STATE
read -p "City/Locality: " CITY
read -p "Organization Name: " ORG
read -p "Organizational Unit (e.g., IT Department): " OU
read -p "Common Name (domain, e.g., api.ailee.example.com): " CN
read -p "Email Address: " EMAIL

# Additional SANs (Subject Alternative Names)
echo ""
echo "Subject Alternative Names (SANs) - additional domains for this certificate"
echo "Enter additional domains (one per line, press Enter with empty line when done):"
SANS=()
while true; do
    read -p "SAN: " san
    if [ -z "$san" ]; then
        break
    fi
    SANS+=("$san")
done

# Create OpenSSL config for CSR with SANs
cat > "$CSR_DIR/openssl.cnf" << EOF
[req]
default_bits = 4096
prompt = no
default_md = sha256
distinguished_name = dn
req_extensions = v3_req

[dn]
C=$COUNTRY
ST=$STATE
L=$CITY
O=$ORG
OU=$OU
CN=$CN
emailAddress=$EMAIL

[v3_req]
keyUsage = keyEncipherment, dataEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = $CN
EOF

# Add additional SANs
counter=2
for san in "${SANS[@]}"; do
    echo "DNS.$counter = $san" >> "$CSR_DIR/openssl.cnf"
    ((counter++))
done

# Generate CSR
echo ""
echo "Generating Certificate Signing Request..."
openssl req -new -key "$CERT_DIR/server.key" \
    -out "$CSR_DIR/server.csr" \
    -config "$CSR_DIR/openssl.cnf"

echo -e "${GREEN}✓${NC} CSR generated: $CSR_DIR/server.csr"

# Display CSR
echo ""
echo -e "${BLUE}Step 2: Submit CSR to Certificate Authority${NC}"
echo ""
echo "Your Certificate Signing Request (CSR):"
echo "========================================"
cat "$CSR_DIR/server.csr"
echo "========================================"
echo ""
echo "This CSR has been saved to: $CSR_DIR/server.csr"
echo ""
echo "Instructions for popular CAs:"
echo ""
echo -e "${BLUE}DigiCert:${NC}"
echo "  1. Go to https://www.digicert.com/account/login.php"
echo "  2. Request New Certificate → SSL/TLS Certificates"
echo "  3. Paste the CSR above"
echo "  4. Complete domain validation"
echo "  5. Download certificate files"
echo ""
echo -e "${BLUE}GlobalSign:${NC}"
echo "  1. Go to https://www.globalsign.com/"
echo "  2. Navigate to SSL/TLS Certificates"
echo "  3. Submit the CSR above"
echo "  4. Complete validation process"
echo "  5. Download certificate"
echo ""
echo -e "${BLUE}Sectigo (formerly Comodo):${NC}"
echo "  1. Go to https://www.sectigo.com/"
echo "  2. Choose SSL/TLS certificate product"
echo "  3. Paste CSR in order form"
echo "  4. Complete DCV (Domain Control Validation)"
echo "  5. Download certificate bundle"
echo ""

# Wait for user to obtain certificate
echo -e "${BLUE}Step 3: Install Certificate${NC}"
echo ""
echo "After receiving your certificate from the CA, you will typically get:"
echo "  1. Your domain certificate (e.g., certificate.crt or domain.crt)"
echo "  2. Intermediate certificate(s) (e.g., intermediate.crt)"
echo "  3. Root CA certificate (e.g., root.crt)"
echo ""
read -p "Do you have the certificate files ready? (y/N): " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "When you receive the certificates, run:"
    echo "  sudo $0 --install"
    echo ""
    echo "Or manually install:"
    echo "  1. Copy your certificate to $CERT_DIR/server.crt"
    echo "  2. Copy intermediate(s) to $CERT_DIR/intermediate.crt"
    echo "  3. Create full chain: cat server.crt intermediate.crt > fullchain.crt"
    echo "  4. Update AILEE configuration to use the certificates"
    exit 0
fi

# Install certificates
echo ""
echo "Certificate installation:"
read -p "Path to your domain certificate file: " CERT_FILE
read -p "Path to intermediate certificate(s) file: " INTERMEDIATE_FILE

if [ ! -f "$CERT_FILE" ]; then
    echo -e "${RED}ERROR: Certificate file not found: $CERT_FILE${NC}"
    exit 1
fi

if [ ! -f "$INTERMEDIATE_FILE" ]; then
    echo -e "${YELLOW}⚠${NC} Intermediate file not found: $INTERMEDIATE_FILE"
    echo "Proceeding without intermediate certificates (not recommended)"
    INTERMEDIATE_FILE=""
fi

# Copy certificates
cp "$CERT_FILE" "$CERT_DIR/server.crt"
chmod 644 "$CERT_DIR/server.crt"
echo -e "${GREEN}✓${NC} Domain certificate installed: $CERT_DIR/server.crt"

if [ -n "$INTERMEDIATE_FILE" ]; then
    cp "$INTERMEDIATE_FILE" "$CERT_DIR/intermediate.crt"
    chmod 644 "$CERT_DIR/intermediate.crt"
    echo -e "${GREEN}✓${NC} Intermediate certificate installed: $CERT_DIR/intermediate.crt"
    
    # Create full chain
    cat "$CERT_DIR/server.crt" "$CERT_DIR/intermediate.crt" > "$CERT_DIR/fullchain.crt"
    chmod 644 "$CERT_DIR/fullchain.crt"
    echo -e "${GREEN}✓${NC} Full chain created: $CERT_DIR/fullchain.crt"
else
    # No intermediate, just copy server cert as fullchain
    cp "$CERT_DIR/server.crt" "$CERT_DIR/fullchain.crt"
fi

# Verify certificate
echo ""
echo -e "${BLUE}Step 4: Verify Certificate${NC}"
echo ""
echo "Certificate information:"
openssl x509 -in "$CERT_DIR/server.crt" -noout -subject -issuer -dates

# Verify certificate matches private key
SERVER_MODULUS=$(openssl x509 -noout -modulus -in "$CERT_DIR/server.crt" | openssl md5)
KEY_MODULUS=$(openssl rsa -noout -modulus -in "$CERT_DIR/server.key" | openssl md5)

if [ "$SERVER_MODULUS" = "$KEY_MODULUS" ]; then
    echo -e "${GREEN}✓${NC} Certificate matches private key"
else
    echo -e "${RED}✗${NC} ERROR: Certificate does not match private key!"
    exit 1
fi

# Verify certificate chain
if [ -n "$INTERMEDIATE_FILE" ]; then
    if openssl verify -CAfile "$CERT_DIR/intermediate.crt" "$CERT_DIR/server.crt" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Certificate chain verified"
    else
        echo -e "${YELLOW}⚠${NC} Could not verify certificate chain"
        echo "   This may be normal if root CA is not included"
    fi
fi

# Update AILEE configuration
echo ""
echo -e "${BLUE}Step 5: Update AILEE Configuration${NC}"
echo ""

AILEE_ENV="/opt/ailee-core/.env"
if [ -f "$AILEE_ENV" ]; then
    # Backup existing config
    cp "$AILEE_ENV" "$AILEE_ENV.backup.$(date +%Y%m%d_%H%M%S)"
    
    # Update TLS settings
    sed -i "s|^AILEE_TLS_ENABLED=.*|AILEE_TLS_ENABLED=true|" "$AILEE_ENV" || \
        echo "AILEE_TLS_ENABLED=true" >> "$AILEE_ENV"
    
    sed -i "s|^AILEE_TLS_CERT_PATH=.*|AILEE_TLS_CERT_PATH=$CERT_DIR/fullchain.crt|" "$AILEE_ENV" || \
        echo "AILEE_TLS_CERT_PATH=$CERT_DIR/fullchain.crt" >> "$AILEE_ENV"
    
    sed -i "s|^AILEE_TLS_KEY_PATH=.*|AILEE_TLS_KEY_PATH=$CERT_DIR/server.key|" "$AILEE_ENV" || \
        echo "AILEE_TLS_KEY_PATH=$CERT_DIR/server.key" >> "$AILEE_ENV"
    
    echo -e "   ${GREEN}✓${NC} Configuration updated"
else
    echo -e "   ${YELLOW}⚠${NC} AILEE .env file not found at $AILEE_ENV"
    echo "   Manually update your configuration with:"
    echo "     AILEE_TLS_ENABLED=true"
    echo "     AILEE_TLS_CERT_PATH=$CERT_DIR/fullchain.crt"
    echo "     AILEE_TLS_KEY_PATH=$CERT_DIR/server.key"
fi

# Set up expiration monitoring
echo ""
echo -e "${BLUE}Step 6: Certificate Expiration Monitoring${NC}"
echo ""

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
echo "  Domain: $CN"
echo "  Certificate: $CERT_DIR/fullchain.crt"
echo "  Private Key: $CERT_DIR/server.key"
echo "  Expires: $(openssl x509 -in "$CERT_DIR/server.crt" -noout -enddate | cut -d= -f2)"
echo ""
echo "Important files:"
echo "  CSR (for renewal): $CSR_DIR/server.csr"
echo "  Private key: $CERT_DIR/server.key (KEEP SECURE!)"
echo "  Certificate: $CERT_DIR/server.crt"
echo "  Full chain: $CERT_DIR/fullchain.crt"
echo ""
echo "Next steps:"
echo "  1. Restart AILEE API server: sudo systemctl restart ailee-api"
echo "  2. Test HTTPS: curl https://$CN/health"
echo "  3. Check certificate: https://www.ssllabs.com/ssltest/analyze.html?d=$CN"
echo ""
echo "Certificate renewal:"
echo "  Commercial CA certificates typically last 1-2 years"
echo "  Set a calendar reminder 30 days before expiration"
echo "  Renewal process:"
echo "    1. Reuse existing private key (DO NOT regenerate)"
echo "    2. Generate new CSR with same key: sudo $0"
echo "    3. Submit CSR to CA for renewal"
echo "    4. Install new certificate"
echo ""
echo -e "${GREEN}✓${NC} Your AILEE-Core API is now secured with a commercial CA certificate!"
echo ""
