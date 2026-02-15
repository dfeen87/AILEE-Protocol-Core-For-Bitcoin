# Federation Key Management Best Practices

## Table of Contents
- [Overview](#overview)
- [Key Generation](#key-generation)
- [Key Storage](#key-storage)
- [Key Distribution](#key-distribution)
- [Key Rotation](#key-rotation)
- [Key Recovery](#key-recovery)
- [HSM Integration](#hsm-integration)
- [Multi-Party Computation](#multi-party-computation)
- [Operational Security](#operational-security)

---

## Overview

Federation keys are **the most critical security component** of AILEE-Core. These keys:

- Control the multisig Bitcoin vault (10-of-15 threshold)
- Authorize peg-ins and peg-outs
- Sign L2 state commitments
- Validate recovery operations

**Key compromise = potential loss of all funds under federation control**

This guide covers best practices for generating, storing, distributing, rotating, and recovering federation signer keys.

---

## Key Generation

### Security Requirements

| Requirement | Rationale |
|-------------|-----------|
| **Offline generation** | Prevents network-based key theft |
| **Air-gapped system** | Eliminates remote attack surface |
| **Auditable process** | Enables verification and accountability |
| **Witnessed ceremony** | Prevents insider attacks |
| **Tamper-evident** | Detects unauthorized access |
| **Deterministic backup** | Enables reliable recovery |

### Key Generation Ceremony

**Participants:**
- 3+ independent key custodians per signer
- 2+ security auditors
- 1 ceremony coordinator
- Legal counsel (for documentation)

**Equipment Required:**
- Air-gapped computer (never connected to network)
- Clean OS installation (verified hash)
- Hardware Security Module (HSM) or hardware wallet
- Tamper-evident bags/seals
- Faraday bag (RF shielding)
- Video recording equipment
- Notary public (optional, for legal documentation)

**Procedure:**

```bash
#!/bin/bash
# Federation Key Generation Ceremony Script
# Run on air-gapped computer with clean OS

set -e

echo "=== AILEE Federation Key Generation Ceremony ==="
echo "Date: $(date)"
echo "Signer ID: $1"
echo ""

# Validate environment
echo "1. Environment validation..."

# Check system has never been online
if ping -c 1 -W 1 8.8.8.8 &> /dev/null; then
    echo "ERROR: System has network connectivity!"
    echo "Disconnect all network interfaces and try again."
    exit 1
fi

# Check no wireless interfaces
if ls /sys/class/net/ | grep -qE '^wl'; then
    echo "ERROR: Wireless interface detected!"
    echo "Physically remove wireless card."
    exit 1
fi

echo "   ✓ System is air-gapped"

# Generate entropy from hardware RNG
echo ""
echo "2. Gathering entropy..."
echo "   Collecting from /dev/hwrng..."
head -c 32 /dev/hwrng > /tmp/entropy.bin
echo "   ✓ Hardware entropy collected"

# Generate key pair using OpenSSL (for Ed25519)
echo ""
echo "3. Generating Ed25519 key pair..."
SIGNER_ID=$1
KEY_DIR="/secure/keys/$SIGNER_ID"
mkdir -p "$KEY_DIR"

# Generate private key
openssl genpkey -algorithm ED25519 -out "$KEY_DIR/private.pem"

# Extract public key
openssl pkey -in "$KEY_DIR/private.pem" -pubout -out "$KEY_DIR/public.pem"

# Generate key fingerprint
FINGERPRINT=$(openssl pkey -in "$KEY_DIR/public.pem" -pubin -outform DER | sha256sum | cut -d' ' -f1)

echo "   ✓ Key pair generated"
echo "   Fingerprint: $FINGERPRINT"

# Create key metadata
cat > "$KEY_DIR/metadata.json" << EOF
{
  "signer_id": "$SIGNER_ID",
  "generated_at": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "generated_by": "$(whoami)",
  "fingerprint": "$FINGERPRINT",
  "algorithm": "Ed25519",
  "purpose": "AILEE Federation Signer",
  "ceremony_version": "1.0"
}
EOF

echo "   ✓ Metadata created"

# Encrypt private key for backup
echo ""
echo "4. Creating encrypted backup..."
echo "Enter backup encryption passphrase (will not be echoed):"
read -s PASSPHRASE
echo "Confirm passphrase:"
read -s PASSPHRASE_CONFIRM

if [ "$PASSPHRASE" != "$PASSPHRASE_CONFIRM" ]; then
    echo "ERROR: Passphrases do not match"
    exit 1
fi

# Encrypt with AES-256-CBC
openssl enc -aes-256-cbc -salt -pbkdf2 -iter 100000 \
    -in "$KEY_DIR/private.pem" \
    -out "$KEY_DIR/private.pem.enc" \
    -pass pass:"$PASSPHRASE"

echo "   ✓ Encrypted backup created"

# Create multiple backup copies
echo ""
echo "5. Creating backup copies..."
for i in {1..3}; do
    BACKUP_DIR="/media/usb$i/$SIGNER_ID"
    mkdir -p "$BACKUP_DIR"
    cp "$KEY_DIR/private.pem.enc" "$BACKUP_DIR/"
    cp "$KEY_DIR/public.pem" "$BACKUP_DIR/"
    cp "$KEY_DIR/metadata.json" "$BACKUP_DIR/"
    echo "   ✓ Backup $i created"
done

# Create paper wallet (QR codes)
echo ""
echo "6. Generating paper backup..."
# Generate QR codes for both encrypted and public keys
qrencode -o "$KEY_DIR/private_qr.png" < "$KEY_DIR/private.pem.enc"
qrencode -o "$KEY_DIR/public_qr.png" < "$KEY_DIR/public.pem"
echo "   ✓ QR codes generated"

# Print ceremony report
echo ""
echo "7. Generating ceremony report..."
cat > "$KEY_DIR/ceremony_report.txt" << EOF
AILEE FEDERATION KEY GENERATION CEREMONY REPORT

Signer ID: $SIGNER_ID
Date: $(date -u +%Y-%m-%dT%H:%M:%SZ)
Ceremony Coordinator: $(whoami)

PUBLIC KEY FINGERPRINT:
$FINGERPRINT

PUBLIC KEY:
$(cat "$KEY_DIR/public.pem")

PARTICIPANTS:
(To be filled in manually)
1. Custodian 1: _________________ Signature: _________________
2. Custodian 2: _________________ Signature: _________________
3. Custodian 3: _________________ Signature: _________________
4. Auditor 1:   _________________ Signature: _________________
5. Auditor 2:   _________________ Signature: _________________

VERIFICATION CHECKLIST:
□ System confirmed air-gapped (no network connectivity)
□ Clean OS installation verified (hash: ________________)
□ Hardware RNG entropy used
□ Key pair generated successfully
□ Public key fingerprint verified
□ Encrypted backup created
□ 3 backup copies created on separate USB drives
□ Paper wallet QR codes generated
□ Backup passphrases recorded separately
□ USB drives sealed in tamper-evident bags
□ Video recording of ceremony archived

BACKUP LOCATIONS:
Backup 1: _________________ (Geographic location)
Backup 2: _________________ (Geographic location)
Backup 3: _________________ (Geographic location)

PASSPHRASE CUSTODIANS:
Shard 1: _________________ (Name, organization)
Shard 2: _________________ (Name, organization)
Shard 3: _________________ (Name, organization)

NOTES:


CERTIFICATION:
I certify that this key generation ceremony was conducted according to
AILEE Federation Key Management procedures and that all security
requirements were met.

Ceremony Coordinator: _________________ Date: _________

Legal Witness: _________________ Date: _________

EOF

echo "   ✓ Ceremony report generated"

# Security cleanup
echo ""
echo "8. Security cleanup..."
shred -vfz -n 10 /tmp/entropy.bin 2>/dev/null || true
unset PASSPHRASE
unset PASSPHRASE_CONFIRM
echo "   ✓ Temporary files securely deleted"

echo ""
echo "=== KEY GENERATION COMPLETE ==="
echo ""
echo "Public key fingerprint: $FINGERPRINT"
echo ""
echo "NEXT STEPS:"
echo "1. Print ceremony report and have all participants sign"
echo "2. Seal USB backup drives in tamper-evident bags"
echo "3. Store backups in geographically distributed secure locations"
echo "4. Record ceremony on video and archive securely"
echo "5. Distribute public key to all federation members"
echo "6. Securely delete all key material from this computer"
echo ""
echo "⚠️  CRITICAL: Verify backup before deleting original!"
echo ""
```

### Post-Ceremony Checklist

- [ ] All participants have signed ceremony report
- [ ] 3+ backup copies created and stored separately
- [ ] Backup locations are geographically distributed
- [ ] Each backup is in tamper-evident packaging
- [ ] Passphrase shards distributed to independent custodians
- [ ] Public key distributed to all federation members
- [ ] Public key fingerprint published (blockchain, newspaper, etc.)
- [ ] Video recording archived
- [ ] All key material securely deleted from generation computer
- [ ] Generation computer securely wiped or destroyed

---

## Key Storage

### Storage Options

| Method | Security | Cost | Complexity | Recommended For |
|--------|----------|------|------------|-----------------|
| **HSM (Hardware Security Module)** | ★★★★★ | High | Medium | Production (Required) |
| **Hardware Wallet** | ★★★★☆ | Low | Low | Development, small deployments |
| **Encrypted File** | ★★☆☆☆ | None | Low | Development only (NOT production) |
| **Paper Wallet** | ★★★☆☆ | None | Low | Cold storage backup |
| **MPC (Multi-Party Computation)** | ★★★★★ | Very High | High | Enterprise (Advanced) |

### HSM (Hardware Security Module)

**Recommended HSMs:**
- **Thales Luna Network HSM** - Enterprise grade
- **Entrust nShield** - High security
- **AWS CloudHSM** - Cloud-native
- **YubiHSM 2** - Cost-effective

**HSM Configuration:**

```bash
# Example: YubiHSM 2 setup

# 1. Initialize HSM
yubihsm-shell

# 2. Create authentication key
yubihsm> put authkey 0 0 "AILEE Admin" all \
         sign-pkcs,decrypt-pkcs,exportable-under-wrap 0102030405060708

# 3. Generate signing key on HSM
yubihsm> generate asymmetrickey 0 1 "AILEE Signer 01" \
         all eddsa sign-eddsa

# 4. Export public key
yubihsm> get publickey 1

# 5. Backup (encrypted)
yubihsm> export wrapped 1 0 backup.bin
```

**PKCS#11 Integration:**

```c++
// C++ example using PKCS#11
#include <pkcs11.h>

// Load PKCS#11 module
CK_FUNCTION_LIST_PTR functions;
C_GetFunctionList(&functions);

// Initialize
functions->C_Initialize(NULL);

// Open session
CK_SESSION_HANDLE session;
functions->C_OpenSession(slot_id, CKF_SERIAL_SESSION | CKF_RW_SESSION,
                        NULL, NULL, &session);

// Login with PIN
functions->C_Login(session, CKU_USER, pin, pin_len);

// Find private key
CK_OBJECT_HANDLE private_key;
// ... search for key

// Sign data
CK_MECHANISM mechanism = {CKM_EDDSA, NULL, 0};
functions->C_SignInit(session, &mechanism, private_key);
functions->C_Sign(session, data, data_len, signature, &signature_len);

// Cleanup
functions->C_Logout(session);
functions->C_CloseSession(session);
functions->C_Finalize(NULL);
```

### Encrypted File Storage (Development Only)

**Never use for production!**

```bash
# Generate key
openssl genpkey -algorithm ED25519 -out private.pem

# Encrypt with strong passphrase
openssl enc -aes-256-cbc -salt -pbkdf2 -iter 100000 \
    -in private.pem -out private.pem.enc

# Secure file permissions
chmod 400 private.pem.enc

# Store passphrase in separate location
# Use password manager or split into Shamir shares
```

---

## Key Distribution

### Secure Distribution Protocol

**Objective:** Distribute public keys to all federation members without exposing private keys

**Method 1: In-Person Distribution**
1. Generate keys in ceremony (see above)
2. Print public key + fingerprint on paper
3. Hand-deliver to each federation member
4. Each member verifies fingerprint independently
5. Publish fingerprint in multiple public locations

**Method 2: Cryptographically Verified Distribution**
```bash
# 1. Signer publishes public key and fingerprint
cat public.pem | sha256sum
# Output: abc123...

# 2. Signer signs fingerprint with existing identity key
echo "abc123..." | openssl dgst -sha256 -sign identity.pem > fingerprint.sig

# 3. Publish to multiple channels:
# - Blockchain (Bitcoin OP_RETURN)
# - Public website over HTTPS
# - Twitter/social media
# - Email to all members (PGP signed)
# - Printed in newspaper (for timestamp proof)

# 4. Each member verifies consistency across all channels
```

**Method 3: Multi-Channel Verification**
```bash
# Publish fingerprint in multiple independent channels
# All should match (if one differs, compromise suspected)

# Channel 1: Bitcoin blockchain
bitcoin-cli sendrawtransaction $(bitcoin-cli createrawtransaction \
    '[]' '{"data":"'$(echo -n "AILEE-SIGNER-01:$FINGERPRINT" | xxd -p -c 256)'"}')

# Channel 2: Commit to GitHub
git commit -m "Add signer 01 public key: $FINGERPRINT"

# Channel 3: Tweet
twitter post "AILEE Signer 01 key fingerprint: $FINGERPRINT"

# Channel 4: Email (PGP signed)
echo "$FINGERPRINT" | gpg --clearsign --local-user signer@ailee.example

# Channel 5: DNS TXT record
# signer01.ailee.example TXT "$FINGERPRINT"
```

---

## Key Rotation

### When to Rotate

**Mandatory rotation:**
- Scheduled rotation (every 12 months recommended)
- Suspected compromise
- Signer departure from federation
- Security audit recommendation
- Regulatory requirement

**Rotation procedure:**

```bash
#!/bin/bash
# Federation Key Rotation

set -e

echo "=== AILEE Federation Key Rotation ==="
echo "This will rotate keys for signer: $1"
echo ""

OLD_SIGNER_ID="$1"
NEW_SIGNER_ID="${1}-rotated-$(date +%Y%m%d)"

# Step 1: Generate new key (using ceremony script)
echo "1. Generating new key..."
./generate-federation-key.sh "$NEW_SIGNER_ID"

# Step 2: Distribute new public key
echo "2. Distributing new public key..."
NEW_PUBKEY=$(cat "/secure/keys/$NEW_SIGNER_ID/public.pem")
NEW_FINGERPRINT=$(openssl pkey -in "/secure/keys/$NEW_SIGNER_ID/public.pem" \
                  -pubin -outform DER | sha256sum | cut -d' ' -f1)

# Publish to multiple channels (see distribution section)

# Step 3: Update federation configuration
echo "3. Updating federation configuration..."
# Update config.yaml with new public key
# Requires quorum approval from other signers

# Step 4: Test new key
echo "4. Testing new key..."
# Sign test message
echo "Test message" | openssl dgst -sha256 -sign \
    "/secure/keys/$NEW_SIGNER_ID/private.pem" > test.sig

# Step 5: Grace period (run both old and new keys in parallel)
echo "5. Starting grace period (30 days)..."
echo "   Old and new keys will both be valid"
echo "   Monitor for any issues"

# Step 6: After grace period, deprecate old key
echo ""
echo "After 30 days:"
echo "  1. Remove old key from federation config"
echo "  2. Securely destroy old key"
echo "  3. Update documentation"

echo ""
echo "New key fingerprint: $NEW_FINGERPRINT"
echo "Rotation initiated: $(date)"
```

---

## Key Recovery

See [BACKUP_RECOVERY.md](BACKUP_RECOVERY.md) for detailed procedures.

**Quick recovery checklist:**
- [ ] Retrieve encrypted backup from secure storage
- [ ] Assemble passphrase shards from custodians
- [ ] Decrypt backup
- [ ] Verify public key fingerprint
- [ ] Import to HSM or secure storage
- [ ] Test signing operation
- [ ] Securely delete temporary files
- [ ] Document recovery in audit log

---

## HSM Integration

### HSM Selection Criteria

**For production deployments:**

| Criteria | Requirement |
|----------|-------------|
| **FIPS 140-2 Level** | Level 3 minimum (Level 4 for high-value) |
| **Algorithm Support** | Ed25519 or ECDSA secp256k1 |
| **Key Backup** | Encrypted export/import |
| **Availability** | Redundant HSMs in different locations |
| **Monitoring** | Tamper detection, audit logging |
| **Cost** | $1,000-$50,000+ per HSM |

### HSM Deployment Models

**1. On-Premise HSM**
- Physical HSM in datacenter
- Full control, highest security
- Requires physical security measures
- Example: Thales Luna SA

**2. Cloud HSM**
- HSM-as-a-Service
- Lower operational overhead
- Trust in cloud provider required
- Example: AWS CloudHSM, Azure Dedicated HSM

**3. HSM-backed Key Management Service**
- Fully managed service
- Easiest to use
- Less control over key material
- Example: AWS KMS, Google Cloud KMS

### HSM High Availability

```yaml
# HSM cluster configuration for high availability

hsm_cluster:
  primary:
    type: "thales_luna"
    host: "hsm-1.ailee.internal"
    partition: "ailee-prod-1"
    backup_partition: "ailee-prod-1-backup"
  
  failover:
    - type: "thales_luna"
      host: "hsm-2.ailee.internal"
      partition: "ailee-prod-2"
      location: "datacenter-2"
    
    - type: "aws_cloudhsm"
      cluster_id: "cluster-abc123"
      region: "us-east-1"
      backup_region: "us-west-2"

  # Automatic failover on primary unavailability
  failover_timeout_ms: 5000
  retry_attempts: 3
```

---

## Multi-Party Computation (MPC)

**Advanced:** For highest security, consider MPC key management.

### MPC Benefits

- **No single point of compromise** - Key never exists in one place
- **Threshold signing** - t-of-n parties must cooperate to sign
- **Proactive security** - Keys can be refreshed without changing public key
- **Auditability** - All signing operations logged by all parties

### MPC Providers

- **Fireblocks** - Enterprise MPC-based key management
- **ZenGo** - Consumer MPC wallet
- **Sepior** - TSM (Threshold Signature Scheme) platform
- **Unbound Security** - PKCS#11-compatible MPC

### MPC Implementation Example

```python
# Simplified MPC threshold signing (conceptual)

from mpc_library import MPCParty, ThresholdSignature

# Each signer runs their own MPC party
party = MPCParty(
    party_id=1,
    threshold=10,  # 10-of-15
    total_parties=15
)

# Distributed key generation (DKG)
# Each party generates their share independently
key_share = party.generate_key_share()

# To sign, collect signature shares from threshold parties
message = "Bitcoin transaction to sign"
signature_share = party.sign(message, key_share)

# Combine shares to create final signature
# (requires 10 out of 15 shares)
shares = collect_shares_from_parties(threshold=10)
final_signature = ThresholdSignature.combine(shares)

# Verify signature
assert verify_signature(message, final_signature, public_key)
```

---

## Operational Security

### Access Control

**Principle of Least Privilege:**
- Only key custodians have access to keys
- No single person has all access
- All access logged and audited
- Regular access reviews

**Multi-Person Control:**
```yaml
# Key operation approval requirements

operations:
  key_generation:
    required_approvals: 3
    approvers: [custodian_1, custodian_2, custodian_3]
  
  key_backup:
    required_approvals: 2
    approvers: [custodian_1, security_officer]
  
  key_rotation:
    required_approvals: 5
    approvers: [all_custodians]
  
  key_recovery:
    required_approvals: 3
    approvers: [custodian_1, custodian_2, legal_counsel]
```

### Audit Logging

```bash
# Log all key operations
logger -t ailee-key-mgmt \
       "Operation: key_access, Signer: $SIGNER_ID, User: $(whoami), Time: $(date -u +%Y-%m-%dT%H:%M:%SZ)"

# Centralized logging
# Send to SIEM (Security Information and Event Management)
rsyslog -> Splunk/ELK Stack

# Alerts on suspicious activity
# - Key access outside business hours
# - Multiple failed access attempts
# - Key access from unusual location
# - Simultaneous access by multiple users
```

### Security Monitoring

```yaml
# Security monitoring configuration

monitoring:
  key_access_logs:
    enabled: true
    retention_days: 2555  # 7 years
    alert_on:
      - failed_authentication
      - after_hours_access
      - unusual_location
      - multiple_concurrent_access
  
  hsm_health:
    enabled: true
    check_interval_seconds: 60
    alert_on:
      - tamper_detected
      - temperature_anomaly
      - battery_low
      - connection_loss
  
  audit_trail:
    blockchain_anchoring: true
    blockchain: "bitcoin"
    anchor_interval_hours: 24
```

---

## Incident Response

### Key Compromise Response

**CRITICAL:** If key compromise is suspected:

1. **IMMEDIATELY** - Stop all federation operations
2. **ALERT** - Notify all federation members
3. **FREEZE** - Halt all peg-ins and peg-outs
4. **INVESTIGATE** - Forensic analysis
5. **ROTATE** - Emergency key rotation
6. **MIGRATE** - Move funds to new multisig (if possible)
7. **COMMUNICATE** - Public disclosure to users
8. **REVIEW** - Post-incident analysis

**Incident Response Contacts:**
- Security Team: security@ailee.example
- Emergency Hotline: +1-XXX-XXX-XXXX
- Legal: legal@ailee.example

---

## Compliance and Auditing

### Regular Security Audits

- **Quarterly:** Key access logs review
- **Bi-annually:** Physical security audit
- **Annually:** Full key management audit
- **After incidents:** Forensic analysis

### Compliance Requirements

Different jurisdictions may require:
- SOC 2 Type II certification
- ISO 27001 compliance
- PCI DSS compliance (if handling card data)
- GDPR compliance (if processing EU data)
- Cryptocurrency exchange regulations

---

## Additional Resources

- **NIST SP 800-57**: Key Management Recommendations
- **FIPS 140-2**: Security Requirements for Cryptographic Modules
- **HSM Vendor Documentation**: Specific HSM setup guides
- **Bitcoin Developer Guide**: Multisig best practices
- **CryptoCurrency Security Standard (CCSS)**: Industry standards

---

**Last Updated:** 2026-02-15  
**Document Owner:** AILEE-Core Security Team  
**Review Schedule:** Quarterly
