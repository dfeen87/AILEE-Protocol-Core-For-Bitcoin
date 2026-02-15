# TLS/SSL Configuration Guide

## Table of Contents
- [Overview](#overview)
- [Why TLS is Critical](#why-tls-is-critical)
- [Quick Start](#quick-start)
- [Certificate Generation](#certificate-generation)
- [API Server TLS](#api-server-tls)
- [Bitcoin RPC TLS](#bitcoin-rpc-tls)
- [P2P Network TLS](#p2p-network-tls)
- [Certificate Management](#certificate-management)
- [Troubleshooting](#troubleshooting)

---

## Overview

This guide covers TLS/SSL configuration for all AILEE-Core components:

1. **API Server** - HTTPS for REST API
2. **Bitcoin RPC** - Secure connections to Bitcoin Core
3. **P2P Network** - Encrypted node-to-node communication

**Production Requirement:** All external connections MUST use TLS with valid certificates.

**For production deployments:**
- ✅ Use **Let's Encrypt** (free, automated, trusted by all browsers)
- ✅ Use **Commercial CA** (DigiCert, GlobalSign, Sectigo for enterprise)
- ❌ **NEVER** use self-signed certificates in production

**For development/testing only:**
- Use self-signed certificates (see end of this guide)
- Not trusted by browsers (will show warnings)
- Not suitable for any public-facing deployment

---

## Why TLS is Critical

### Security Risks Without TLS

| Risk | Impact | Mitigation |
|------|--------|------------|
| **Credential theft** | RPC credentials sent in plaintext | TLS encryption |
| **Man-in-the-middle attacks** | Attacker intercepts/modifies data | Certificate verification |
| **Data exposure** | Transaction data visible to network observers | End-to-end encryption |
| **Session hijacking** | API authentication tokens stolen | HTTPS-only cookies |
| **Replay attacks** | Captured requests replayed | TLS + nonce/timestamp |

### Compliance Requirements

- **PCI DSS:** Requires TLS 1.2+ for transmission of cardholder data
- **SOC 2:** Requires encrypted data in transit
- **GDPR:** Requires appropriate technical measures for data protection

---

## Quick Start

### Production Certificates (REQUIRED for Public Deployments)

For production, use certificates from a trusted Certificate Authority (CA):

**Option 1: Let's Encrypt (Recommended - Free & Automated)**

Let's Encrypt provides free, automated certificates trusted by all browsers.

```bash
# Quick setup with automated script
cd /opt/ailee-core
sudo DOMAIN=api.yourdomain.com EMAIL=admin@yourdomain.com \
    ./scripts/security/setup-letsencrypt.sh

# This will:
# 1. Install certbot
# 2. Obtain SSL certificate from Let's Encrypt
# 3. Configure automatic renewal (every 60 days)
# 4. Update AILEE configuration
# 5. Set up monitoring

# Certificates installed at:
# /etc/letsencrypt/live/api.yourdomain.com/fullchain.pem
# /etc/letsencrypt/live/api.yourdomain.com/privkey.pem
```

**Benefits of Let's Encrypt:**
- ✅ **Free** - No cost for certificates
- ✅ **Automated** - Auto-renewal every 60 days
- ✅ **Trusted** - Accepted by all major browsers
- ✅ **Quick** - Setup in minutes
- ✅ **Secure** - Modern best practices

**Requirements:**
- Domain name pointing to your server
- Port 80 accessible (for verification)
- Port 443 accessible (for HTTPS)

**Option 2: Commercial CA (Enterprise)**

For enterprise deployments requiring extended validation or specific compliance:

```bash
# Generate CSR and obtain certificate from commercial CA
cd /opt/ailee-core
sudo DOMAIN=api.yourdomain.com \
    ./scripts/security/setup-commercial-ca.sh

# This will:
# 1. Generate private key and CSR
# 2. Guide you through CA submission
# 3. Help install received certificates
# 4. Configure AILEE to use certificates
```

**Commercial CA Options:**
- **DigiCert** - Industry leader, highest trust
- **GlobalSign** - Worldwide coverage
- **Sectigo** (formerly Comodo) - Cost-effective
- **GoDaddy** - Easy integration

---

## Certificate Generation

### Development Certificate Script

Create `/opt/ailee-core/scripts/generate-dev-certs.sh`:

```bash
#!/bin/bash
# Generate development TLS certificates
# WARNING: DO NOT use these in production!

set -e

CERT_DIR="/etc/ailee/certs"
mkdir -p "$CERT_DIR"

# Certificate validity (days)
VALIDITY_DAYS=365

echo "=== Generating AILEE-Core Development Certificates ==="
echo "Directory: $CERT_DIR"
echo "Validity: $VALIDITY_DAYS days"
echo ""
echo "⚠️  WARNING: These are self-signed certificates for development only!"
echo "⚠️  DO NOT use in production!"
echo ""

# 1. Generate CA (Certificate Authority)
echo "1. Generating Certificate Authority..."
openssl req -x509 -newkey rsa:4096 -sha256 -days $VALIDITY_DAYS \
    -nodes -keyout "$CERT_DIR/ca.key" -out "$CERT_DIR/ca.crt" \
    -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core/CN=AILEE-CA" \
    2>/dev/null
chmod 600 "$CERT_DIR/ca.key"
echo "   ✓ CA certificate: $CERT_DIR/ca.crt"

# 2. Generate Server Certificate
echo "2. Generating API Server Certificate..."
openssl req -newkey rsa:4096 -nodes \
    -keyout "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.csr" \
    -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core/CN=localhost" \
    2>/dev/null

# Create SAN configuration
cat > "$CERT_DIR/server.ext" << EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = ailee-node-1
DNS.3 = *.ailee.local
IP.1 = 127.0.0.1
IP.2 = ::1
EOF

# Sign server certificate with CA
openssl x509 -req -in "$CERT_DIR/server.csr" \
    -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" \
    -CAcreateserial -out "$CERT_DIR/server.crt" \
    -days $VALIDITY_DAYS -sha256 \
    -extfile "$CERT_DIR/server.ext" \
    2>/dev/null

chmod 600 "$CERT_DIR/server.key"
echo "   ✓ Server certificate: $CERT_DIR/server.crt"
echo "   ✓ Server private key: $CERT_DIR/server.key"

# 3. Generate Client Certificate (for mutual TLS)
echo "3. Generating Client Certificate..."
openssl req -newkey rsa:4096 -nodes \
    -keyout "$CERT_DIR/client.key" \
    -out "$CERT_DIR/client.csr" \
    -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core/CN=client" \
    2>/dev/null

openssl x509 -req -in "$CERT_DIR/client.csr" \
    -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" \
    -CAcreateserial -out "$CERT_DIR/client.crt" \
    -days $VALIDITY_DAYS -sha256 \
    2>/dev/null

chmod 600 "$CERT_DIR/client.key"
echo "   ✓ Client certificate: $CERT_DIR/client.crt"

# 4. Generate P2P Node Certificates
echo "4. Generating P2P Node Certificates..."
for i in {1..3}; do
    openssl req -newkey rsa:4096 -nodes \
        -keyout "$CERT_DIR/node-$i.key" \
        -out "$CERT_DIR/node-$i.csr" \
        -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core/CN=ailee-node-$i" \
        2>/dev/null
    
    openssl x509 -req -in "$CERT_DIR/node-$i.csr" \
        -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" \
        -CAcreateserial -out "$CERT_DIR/node-$i.crt" \
        -days $VALIDITY_DAYS -sha256 \
        2>/dev/null
    
    chmod 600 "$CERT_DIR/node-$i.key"
done
echo "   ✓ Node certificates generated (node-1, node-2, node-3)"

# 5. Cleanup CSR files
rm -f "$CERT_DIR"/*.csr "$CERT_DIR"/*.ext "$CERT_DIR"/*.srl

# 6. Set ownership
chown -R ailee:ailee "$CERT_DIR" 2>/dev/null || true

echo ""
echo "=== Certificate Generation Complete ==="
echo ""
echo "To verify certificates:"
echo "  openssl x509 -in $CERT_DIR/server.crt -text -noout"
echo ""
echo "To test HTTPS connection:"
echo "  curl --cacert $CERT_DIR/ca.crt https://localhost:8080/health"
echo ""
```

Make it executable:
```bash
chmod +x /opt/ailee-core/scripts/generate-dev-certs.sh
```

---

## API Server TLS

### Configuration

Edit `.env`:
```bash
# Enable TLS
AILEE_TLS_ENABLED=true
AILEE_TLS_CERT_PATH=/etc/ailee/certs/server.crt
AILEE_TLS_KEY_PATH=/etc/ailee/certs/server.key
AILEE_TLS_CA_PATH=/etc/ailee/certs/ca.crt
```

### Python API Implementation

The API server in `api/main.py` needs TLS support. Run with uvicorn:

```bash
# Development (self-signed)
uvicorn api.main:app \
    --host 0.0.0.0 \
    --port 8443 \
    --ssl-keyfile /etc/ailee/certs/server.key \
    --ssl-certfile /etc/ailee/certs/server.crt \
    --ssl-ca-certs /etc/ailee/certs/ca.crt

# Production (Let's Encrypt)
uvicorn api.main:app \
    --host 0.0.0.0 \
    --port 443 \
    --ssl-keyfile /etc/letsencrypt/live/api.ailee.example.com/privkey.pem \
    --ssl-certfile /etc/letsencrypt/live/api.ailee.example.com/fullchain.pem
```

### Systemd Service with TLS

Create `/etc/systemd/system/ailee-api.service`:

```ini
[Unit]
Description=AILEE-Core REST API (HTTPS)
After=network.target

[Service]
Type=simple
User=ailee
Group=ailee
WorkingDirectory=/opt/ailee-core
Environment="AILEE_TLS_ENABLED=true"
ExecStart=/usr/bin/uvicorn api.main:app \
    --host 0.0.0.0 \
    --port 8443 \
    --ssl-keyfile /etc/ailee/certs/server.key \
    --ssl-certfile /etc/ailee/certs/server.crt
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

### Testing API TLS

```bash
# Test with curl
curl --cacert /etc/ailee/certs/ca.crt https://localhost:8443/health

# Expected output:
# {"status":"healthy","timestamp":"..."}

# Test certificate details
openssl s_client -connect localhost:8443 -showcerts

# Test TLS version
openssl s_client -connect localhost:8443 -tls1_3
```

---

## Bitcoin RPC TLS

### Bitcoin Core TLS Configuration

Bitcoin Core doesn't natively support TLS. Use stunnel or nginx as a TLS proxy:

#### Option 1: stunnel (Recommended for single node)

Install stunnel:
```bash
sudo apt-get install stunnel4
```

Configure `/etc/stunnel/bitcoin-rpc.conf`:
```ini
# Client mode (AILEE node connecting to Bitcoin Core)
client = yes

[bitcoin-rpc]
accept = 127.0.0.1:18332
connect = bitcoin.example.com:8332
CAfile = /etc/ailee/certs/bitcoin-ca.crt
cert = /etc/ailee/certs/client.crt
key = /etc/ailee/certs/client.key
verify = 2
```

Start stunnel:
```bash
sudo systemctl enable stunnel4
sudo systemctl start stunnel4
```

Update AILEE configuration:
```bash
BITCOIN_RPC_URL=https://127.0.0.1:18332
BITCOIN_RPC_USE_TLS=true
BITCOIN_RPC_VERIFY_TLS=true
```

#### Option 2: nginx Reverse Proxy (Recommended for multiple nodes)

Install nginx:
```bash
sudo apt-get install nginx
```

Configure `/etc/nginx/sites-available/bitcoin-rpc`:
```nginx
upstream bitcoin_rpc {
    server 127.0.0.1:8332;
}

server {
    listen 18332 ssl;
    server_name bitcoin-rpc.internal;

    # TLS configuration
    ssl_certificate /etc/ailee/certs/server.crt;
    ssl_certificate_key /etc/ailee/certs/server.key;
    ssl_client_certificate /etc/ailee/certs/ca.crt;
    ssl_verify_client optional;

    # Strong TLS settings
    ssl_protocols TLSv1.3;
    ssl_ciphers 'TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256';
    ssl_prefer_server_ciphers on;

    # Security headers
    add_header Strict-Transport-Security "max-age=31536000" always;

    location / {
        proxy_pass http://bitcoin_rpc;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        
        # Authentication
        proxy_set_header Authorization $http_authorization;
        proxy_pass_header Authorization;
    }
}
```

Enable and start:
```bash
sudo ln -s /etc/nginx/sites-available/bitcoin-rpc /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

### Testing Bitcoin RPC TLS

```bash
# Test with curl
curl --cacert /etc/ailee/certs/ca.crt \
     --user bitcoinrpc:password \
     -X POST https://localhost:18332 \
     -H 'Content-Type: application/json' \
     -d '{"jsonrpc":"1.0","id":"test","method":"getblockcount","params":[]}'

# Expected output:
# {"result":XXXXXX,"error":null,"id":"test"}
```

---

## P2P Network TLS

### libp2p TLS Configuration

For P2P node-to-node communication, configure TLS in the P2P network layer.

Create `/etc/ailee/p2p-tls.yaml`:
```yaml
# P2P TLS Configuration
tls:
  enabled: true
  
  # Node certificate and key
  cert_path: /etc/ailee/certs/node-1.crt
  key_path: /etc/ailee/certs/node-1.key
  
  # CA for peer verification
  ca_path: /etc/ailee/certs/ca.crt
  
  # Peer verification
  verify_peers: true
  
  # Minimum TLS version
  min_version: "TLSv1.3"
  
  # Allowed peer certificates (optional, for whitelist)
  allowed_peers:
    - /etc/ailee/certs/node-2.crt
    - /etc/ailee/certs/node-3.crt
```

### Testing P2P TLS

```bash
# Check P2P connection security
ailee-cli p2p-status

# Expected output should show "encrypted: true"

# Test peer connection
openssl s_client -connect peer-node:4001 -showcerts
```

---

## Certificate Management

### Certificate Rotation

**Recommendation:** Rotate certificates every 90 days for production

#### Automated Rotation with Let's Encrypt

```bash
# Let's Encrypt auto-renews at 60 days
# Add post-renewal hook: /etc/letsencrypt/renewal-hooks/post/reload-ailee.sh

#!/bin/bash
systemctl reload ailee-api
systemctl reload nginx
```

#### Manual Rotation

```bash
# 1. Generate new certificate (as shown in Certificate Generation)

# 2. Update configuration with new certificate paths

# 3. Reload services (zero-downtime)
systemctl reload ailee-api
systemctl reload nginx

# 4. Verify new certificate
openssl s_client -connect localhost:8443 -showcerts | grep "Not After"
```

### Certificate Expiration Monitoring

Create `/opt/ailee-core/scripts/check-cert-expiry.sh`:

```bash
#!/bin/bash
# Monitor certificate expiration

CERT_PATH="/etc/ailee/certs/server.crt"
WARNING_DAYS=30

# Get expiration date
EXPIRY=$(openssl x509 -enddate -noout -in "$CERT_PATH" | cut -d= -f2)
EXPIRY_EPOCH=$(date -d "$EXPIRY" +%s)
NOW_EPOCH=$(date +%s)
DAYS_LEFT=$(( ($EXPIRY_EPOCH - $NOW_EPOCH) / 86400 ))

echo "Certificate: $CERT_PATH"
echo "Expires: $EXPIRY"
echo "Days remaining: $DAYS_LEFT"

if [ $DAYS_LEFT -lt $WARNING_DAYS ]; then
    echo "⚠️  WARNING: Certificate expires in $DAYS_LEFT days!"
    # Send alert (email, Slack, PagerDuty, etc.)
    exit 1
fi

echo "✓ Certificate valid"
```

Add to cron:
```bash
# Check certificate expiration daily
0 8 * * * /opt/ailee-core/scripts/check-cert-expiry.sh
```

---

## Troubleshooting

### Common Issues

#### 1. "Certificate verify failed"

**Problem:** Client can't verify server certificate

**Solution:**
```bash
# Check certificate chain
openssl verify -CAfile /etc/ailee/certs/ca.crt /etc/ailee/certs/server.crt

# If using self-signed, ensure CA is trusted
# For curl testing:
curl --cacert /etc/ailee/certs/ca.crt https://localhost:8443/health

# For system-wide trust:
sudo cp /etc/ailee/certs/ca.crt /usr/local/share/ca-certificates/ailee-ca.crt
sudo update-ca-certificates
```

#### 2. "Wrong host name in certificate"

**Problem:** Certificate CN/SAN doesn't match hostname

**Solution:**
```bash
# Check certificate SAN
openssl x509 -in /etc/ailee/certs/server.crt -text -noout | grep -A1 "Subject Alternative Name"

# Regenerate certificate with correct SAN (see generation script)
```

#### 3. "Permission denied" reading key file

**Problem:** Private key file permissions too restrictive

**Solution:**
```bash
# Set correct ownership and permissions
sudo chown ailee:ailee /etc/ailee/certs/server.key
sudo chmod 600 /etc/ailee/certs/server.key
```

#### 4. "TLS handshake timeout"

**Problem:** Firewall blocking TLS port or certificate issues

**Solution:**
```bash
# Check port is listening
sudo netstat -tlnp | grep 8443

# Test TLS handshake
openssl s_client -connect localhost:8443 -debug

# Check firewall
sudo ufw status
sudo ufw allow 8443/tcp
```

### Debug TLS Connections

```bash
# Verbose TLS debugging
openssl s_client -connect localhost:8443 -debug -msg -showcerts

# Check supported TLS versions
for v in tls1 tls1_1 tls1_2 tls1_3; do
    echo -n "$v: "
    openssl s_client -connect localhost:8443 -$v < /dev/null 2>&1 | grep "Protocol"
done

# Check cipher suites
openssl s_client -connect localhost:8443 -cipher 'HIGH:!aNULL:!MD5' < /dev/null
```

---

## Security Best Practices

1. **Use TLS 1.3** - Disable TLS 1.0, 1.1, 1.2 if possible
2. **Strong Ciphers Only** - Use AEAD ciphers (AES-GCM, ChaCha20-Poly1305)
3. **Certificate Pinning** - Pin expected certificates in configuration
4. **Short-lived Certificates** - 90 days or less
5. **Automated Renewal** - Use Let's Encrypt or automated PKI
6. **Monitor Expiration** - Alert 30 days before expiry
7. **Secure Private Keys** - 600 permissions, encrypted storage
8. **Mutual TLS** - Require client certificates for sensitive endpoints
9. **HSTS Headers** - Force HTTPS for browsers
10. **Regular Audits** - Scan with SSL Labs, testssl.sh

### SSL Labs Testing (for public APIs)

```bash
# Test your public API
https://www.ssllabs.com/ssltest/analyze.html?d=api.ailee.example.com

# Goal: A+ rating
```

### testssl.sh (for any endpoint)

```bash
# Install testssl
git clone https://github.com/drwetter/testssl.sh.git
cd testssl.sh

# Test your endpoint
./testssl.sh https://localhost:8443

# Review findings and fix any warnings
```

---

## Production Checklist

Before going to production:

- [ ] Production certificates from trusted CA installed
- [ ] TLS 1.3 enabled, older versions disabled
- [ ] Strong cipher suites configured
- [ ] Certificate expiration monitoring in place
- [ ] Automated certificate renewal configured
- [ ] All RPC connections use TLS
- [ ] P2P network encryption enabled
- [ ] Private keys stored securely (600 permissions minimum)
- [ ] Certificate rotation procedure documented
- [ ] TLS testing completed (SSL Labs A+ or testssl.sh clean)
- [ ] Backup of certificates and keys secured
- [ ] Firewall rules configured correctly
- [ ] Monitoring and alerting configured

---

## Appendix: Self-Signed Certificates (Development Only)

⚠️ **WARNING: NEVER use self-signed certificates in production!**

Self-signed certificates are **ONLY** for:
- Local development
- Internal testing  
- Offline environments

**Why NOT for production:**
- ❌ Not trusted by browsers (security warnings)
- ❌ No certificate revocation
- ❌ Manual management required
- ❌ Professional/compliance issues

**For development/testing:**

```bash
# Generate self-signed certificates
cd /opt/ailee-core
sudo ./scripts/security/generate-dev-certs.sh

# This creates:
# - /etc/ailee/certs/ca.crt (self-signed CA)
# - /etc/ailee/certs/server.crt
# - /etc/ailee/certs/server.key
# - /etc/ailee/certs/client.crt (for mTLS testing)
```

**Testing with self-signed certs:**

```bash
# Test with curl (specify CA)
curl --cacert /etc/ailee/certs/ca.crt https://localhost:8443/health

# Or ignore verification (insecure, testing only)
curl -k https://localhost:8443/health
```

**For production, use Let's Encrypt or a commercial CA!**

---

**Last Updated:** 2026-02-15  
**Document Owner:** AILEE-Core Security Team  
**Review Schedule:** Quarterly
