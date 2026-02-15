#!/bin/bash
# Generate development TLS certificates for AILEE-Core
# WARNING: DO NOT use these in production!
# For production, use Let's Encrypt or commercial CA certificates

set -e

# Configuration
CERT_DIR="${CERT_DIR:-/etc/ailee/certs}"
VALIDITY_DAYS="${VALIDITY_DAYS:-365}"

# Colors for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "=== AILEE-Core Development Certificate Generator ==="
echo ""
echo -e "${YELLOW}⚠️  WARNING: These are self-signed certificates for development only!${NC}"
echo -e "${YELLOW}⚠️  DO NOT use in production!${NC}"
echo ""
echo "Certificate directory: $CERT_DIR"
echo "Validity period: $VALIDITY_DAYS days"
echo ""

# Create certificate directory
mkdir -p "$CERT_DIR"

# Check if OpenSSL is available
if ! command -v openssl &> /dev/null; then
    echo -e "${RED}ERROR: OpenSSL is not installed${NC}"
    echo "Install with: sudo apt-get install openssl"
    exit 1
fi

# 1. Generate Certificate Authority (CA)
echo "1. Generating Certificate Authority..."
openssl req -x509 -newkey rsa:4096 -sha256 -days "$VALIDITY_DAYS" \
    -nodes -keyout "$CERT_DIR/ca.key" -out "$CERT_DIR/ca.crt" \
    -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core-Dev/CN=AILEE-Dev-CA" \
    2>/dev/null

chmod 600 "$CERT_DIR/ca.key"
echo -e "   ${GREEN}✓${NC} CA certificate: $CERT_DIR/ca.crt"
echo -e "   ${GREEN}✓${NC} CA private key: $CERT_DIR/ca.key"

# 2. Generate API Server Certificate
echo ""
echo "2. Generating API Server Certificate..."
openssl req -newkey rsa:4096 -nodes \
    -keyout "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.csr" \
    -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core-Dev/CN=localhost" \
    2>/dev/null

# Create Subject Alternative Name (SAN) configuration
cat > "$CERT_DIR/server.ext" << EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = ailee-node-1
DNS.3 = ailee-node-2
DNS.4 = ailee-node-3
DNS.5 = *.ailee.local
DNS.6 = *.ailee-net
IP.1 = 127.0.0.1
IP.2 = ::1
IP.3 = 172.20.0.10
IP.4 = 172.20.0.11
IP.5 = 172.20.0.12
EOF

# Sign server certificate with CA
openssl x509 -req -in "$CERT_DIR/server.csr" \
    -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" \
    -CAcreateserial -out "$CERT_DIR/server.crt" \
    -days "$VALIDITY_DAYS" -sha256 \
    -extfile "$CERT_DIR/server.ext" \
    2>/dev/null

chmod 600 "$CERT_DIR/server.key"
echo -e "   ${GREEN}✓${NC} Server certificate: $CERT_DIR/server.crt"
echo -e "   ${GREEN}✓${NC} Server private key: $CERT_DIR/server.key"

# 3. Generate Client Certificate (for mutual TLS)
echo ""
echo "3. Generating Client Certificate (for mTLS)..."
openssl req -newkey rsa:4096 -nodes \
    -keyout "$CERT_DIR/client.key" \
    -out "$CERT_DIR/client.csr" \
    -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core-Dev/CN=ailee-client" \
    2>/dev/null

# Create client certificate extensions
cat > "$CERT_DIR/client.ext" << EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
extendedKeyUsage = clientAuth
EOF

openssl x509 -req -in "$CERT_DIR/client.csr" \
    -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" \
    -CAcreateserial -out "$CERT_DIR/client.crt" \
    -days "$VALIDITY_DAYS" -sha256 \
    -extfile "$CERT_DIR/client.ext" \
    2>/dev/null

chmod 600 "$CERT_DIR/client.key"
echo -e "   ${GREEN}✓${NC} Client certificate: $CERT_DIR/client.crt"
echo -e "   ${GREEN}✓${NC} Client private key: $CERT_DIR/client.key"

# 4. Generate P2P Node Certificates
echo ""
echo "4. Generating P2P Node Certificates..."
for i in {1..3}; do
    NODE_NAME="ailee-node-$i"
    
    openssl req -newkey rsa:4096 -nodes \
        -keyout "$CERT_DIR/node-$i.key" \
        -out "$CERT_DIR/node-$i.csr" \
        -subj "/C=US/ST=Development/L=Dev/O=AILEE-Core-Dev/CN=$NODE_NAME" \
        2>/dev/null
    
    # Create node certificate extensions
    cat > "$CERT_DIR/node-$i.ext" << EOF
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
extendedKeyUsage = serverAuth, clientAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = $NODE_NAME
DNS.2 = $NODE_NAME.ailee-net
IP.1 = 172.20.0.$((9+i))
EOF
    
    openssl x509 -req -in "$CERT_DIR/node-$i.csr" \
        -CA "$CERT_DIR/ca.crt" -CAkey "$CERT_DIR/ca.key" \
        -CAcreateserial -out "$CERT_DIR/node-$i.crt" \
        -days "$VALIDITY_DAYS" -sha256 \
        -extfile "$CERT_DIR/node-$i.ext" \
        2>/dev/null
    
    chmod 600 "$CERT_DIR/node-$i.key"
    echo -e "   ${GREEN}✓${NC} Node $i certificate: $CERT_DIR/node-$i.crt"
done

# 5. Cleanup temporary files
echo ""
echo "5. Cleaning up temporary files..."
rm -f "$CERT_DIR"/*.csr "$CERT_DIR"/*.ext "$CERT_DIR"/*.srl
echo -e "   ${GREEN}✓${NC} Cleanup complete"

# 6. Set ownership (if running as root)
if [ "$EUID" -eq 0 ]; then
    echo ""
    echo "6. Setting ownership..."
    chown -R ailee:ailee "$CERT_DIR" 2>/dev/null || \
        echo -e "   ${YELLOW}⚠${NC}  User 'ailee' not found, skipping ownership change"
else
    echo ""
    echo -e "6. ${YELLOW}⚠${NC}  Not running as root, skipping ownership change"
    echo "   Run manually if needed: sudo chown -R ailee:ailee $CERT_DIR"
fi

# 7. Create combined certificate file (cert + key) for some applications
echo ""
echo "7. Creating combined certificate files..."
cat "$CERT_DIR/server.crt" "$CERT_DIR/server.key" > "$CERT_DIR/server-combined.pem"
chmod 600 "$CERT_DIR/server-combined.pem"
echo -e "   ${GREEN}✓${NC} Combined server cert: $CERT_DIR/server-combined.pem"

# Summary
echo ""
echo "=== Certificate Generation Complete ==="
echo ""
echo "Generated certificates:"
echo "  • Certificate Authority (CA)"
echo "  • API Server certificate (with SANs for localhost, docker network)"
echo "  • Client certificate (for mutual TLS)"
echo "  • P2P Node certificates (nodes 1-3)"
echo ""
echo "Certificate details:"
openssl x509 -in "$CERT_DIR/server.crt" -noout -subject -issuer -dates
echo ""
echo "To view full certificate:"
echo "  openssl x509 -in $CERT_DIR/server.crt -text -noout"
echo ""
echo "To verify certificate:"
echo "  openssl verify -CAfile $CERT_DIR/ca.crt $CERT_DIR/server.crt"
echo ""
echo "To test HTTPS connection:"
echo "  curl --cacert $CERT_DIR/ca.crt https://localhost:8443/health"
echo ""
echo -e "${GREEN}✓${NC} Certificates ready for development use"
echo -e "${YELLOW}⚠${NC}  Remember: Use proper CA-signed certificates in production!"
echo ""
