# Backup and Recovery Procedures

## Table of Contents
- [Overview](#overview)
- [What to Backup](#what-to-backup)
- [Backup Procedures](#backup-procedures)
- [Recovery Procedures](#recovery-procedures)
- [Disaster Recovery](#disaster-recovery)
- [Testing](#testing)

---

## Overview

This document describes backup and recovery procedures for AILEE-Core production deployments. Proper backup and recovery procedures are **critical** for protecting user funds and maintaining system integrity.

### Recovery Time Objectives (RTO) and Recovery Point Objectives (RPO)

| Component | RTO Target | RPO Target | Priority |
|-----------|------------|------------|----------|
| Federation Keys | < 1 hour | 0 (no data loss) | **CRITICAL** |
| L2 State Database | < 4 hours | < 15 minutes | **HIGH** |
| Configuration | < 1 hour | < 1 hour | **HIGH** |
| Logs & Metrics | < 24 hours | < 1 day | **MEDIUM** |

---

## What to Backup

### 1. Federation Signer Private Keys ⚠️ **CRITICAL**

**Location:**
- HSM-backed keys: Managed by HSM vendor backup procedures
- File-based keys: `/etc/ailee/keys/` or configured key path

**Backup Frequency:** Immediately after generation, then verify weekly

**Backup Method:**
```bash
# CRITICAL: Encrypt keys before backing up
# Use GPG with strong passphrase stored in separate secure location

# Backup individual signer key
gpg --symmetric --cipher-algo AES256 \
    --output /secure/backup/signer-01.key.gpg \
    /etc/ailee/keys/signer-01.key

# Store passphrase in separate secure location (e.g., password manager)
# Geographic distribution: Store backups in multiple physical locations
```

**Storage:**
- Primary: Hardware Security Module (HSM)
- Backup 1: Encrypted USB drive in secure safe (geographic location 1)
- Backup 2: Encrypted USB drive in secure safe (geographic location 2)
- Backup 3: Encrypted cloud backup with customer-managed encryption keys

**Recovery Test:** Quarterly

### 2. L2 State Database

**Location:**
- RocksDB data: `/data/` or configured data path
- Contains L2 transaction history, state roots, and anchor commitments

**Backup Frequency:** Every 15 minutes (incremental), Daily (full)

**Backup Method:**

```bash
# Incremental backup script
#!/bin/bash
set -e

BACKUP_DIR="/backup/l2-state"
DATA_DIR="/data/node-1"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Create backup directory
mkdir -p "$BACKUP_DIR/incremental"

# Use RocksDB checkpoint API (preferred) or rsync
# Option 1: Application-level checkpoint
ailee-cli create-checkpoint --output "$BACKUP_DIR/incremental/checkpoint-$TIMESTAMP"

# Option 2: Filesystem-level backup (requires brief read lock)
rsync -av --link-dest="$BACKUP_DIR/incremental/latest" \
    "$DATA_DIR/" \
    "$BACKUP_DIR/incremental/snapshot-$TIMESTAMP/"

ln -sfn "snapshot-$TIMESTAMP" "$BACKUP_DIR/incremental/latest"

# Compress old backups (older than 24 hours)
find "$BACKUP_DIR/incremental" -type d -mtime +1 -name "snapshot-*" | \
    while read dir; do
        tar -czf "$dir.tar.gz" -C "$(dirname $dir)" "$(basename $dir)"
        rm -rf "$dir"
    done

# Upload to cloud storage
aws s3 sync "$BACKUP_DIR/incremental/" \
    "s3://ailee-backups/l2-state/incremental/" \
    --storage-class STANDARD_IA
```

**Daily Full Backup:**
```bash
#!/bin/bash
set -e

BACKUP_DIR="/backup/l2-state/full"
DATA_DIR="/data/node-1"
DATE=$(date +%Y%m%d)

# Create full snapshot
ailee-cli create-checkpoint --output "$BACKUP_DIR/full-$DATE"

# Compress
tar -czf "$BACKUP_DIR/full-$DATE.tar.gz" \
    -C "$BACKUP_DIR" "full-$DATE"
rm -rf "$BACKUP_DIR/full-$DATE"

# Upload to cold storage
aws s3 cp "$BACKUP_DIR/full-$DATE.tar.gz" \
    "s3://ailee-backups/l2-state/full/" \
    --storage-class GLACIER

# Retention: Keep daily backups for 30 days, then monthly for 1 year
find "$BACKUP_DIR" -name "full-*.tar.gz" -mtime +30 -delete
```

### 3. Configuration Files

**Files to Backup:**
- `/etc/ailee/config.yaml`
- `/etc/ailee/secrets.yaml` (encrypted)
- `.env` files
- Docker compose configurations
- Nginx/reverse proxy configs

**Backup Frequency:** After each change, minimum daily

**Backup Method:**
```bash
#!/bin/bash
# Configuration backup script

BACKUP_DIR="/backup/config"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "$BACKUP_DIR"

# Backup all config files
tar -czf "$BACKUP_DIR/config-$TIMESTAMP.tar.gz" \
    /etc/ailee/ \
    /opt/ailee-core/.env \
    /opt/ailee-core/docker-compose.yml

# Upload to version control (without secrets)
# Store encrypted secrets separately
```

### 4. Audit Logs

**Location:** `/var/log/ailee/`

**Backup Frequency:** Hourly (rotation), Daily (archival)

**Retention:** 90 days online, 7 years archived (compliance)

**Backup Method:**
```bash
# Use logrotate for automatic log rotation and backup
# /etc/logrotate.d/ailee

/var/log/ailee/*.log {
    hourly
    rotate 168  # 7 days of hourly logs
    compress
    delaycompress
    missingok
    notifempty
    create 0640 ailee ailee
    sharedscripts
    postrotate
        # Upload to centralized log storage
        aws s3 sync /var/log/ailee/ \
            s3://ailee-logs/$(hostname)/ \
            --exclude "*.log"
    endscript
}
```

---

## Backup Procedures

### Automated Backup Schedule

```bash
# /etc/cron.d/ailee-backup

# Incremental L2 state backup every 15 minutes
*/15 * * * * root /opt/ailee-core/scripts/backup-l2-incremental.sh

# Full L2 state backup daily at 2 AM
0 2 * * * root /opt/ailee-core/scripts/backup-l2-full.sh

# Configuration backup after changes and daily at 3 AM
0 3 * * * root /opt/ailee-core/scripts/backup-config.sh

# Verify backups daily at 4 AM
0 4 * * * root /opt/ailee-core/scripts/verify-backups.sh

# Key backup verification weekly on Sunday at 1 AM
0 1 * * 0 root /opt/ailee-core/scripts/verify-key-backups.sh
```

### Manual Backup (Before Major Changes)

```bash
# Before upgrading or making major changes
sudo /opt/ailee-core/scripts/backup-all.sh

# This will:
# 1. Create full L2 state snapshot
# 2. Backup all configuration
# 3. Export current metrics
# 4. Create recovery point
```

### Backup Verification

```bash
#!/bin/bash
# verify-backups.sh

set -e

BACKUP_DIR="/backup"
LOG_FILE="/var/log/ailee/backup-verification.log"

echo "[$(date)] Starting backup verification" >> "$LOG_FILE"

# Verify L2 state backup
LATEST_BACKUP=$(find "$BACKUP_DIR/l2-state/incremental" -name "snapshot-*" -type d | sort -r | head -n1)
if [ -z "$LATEST_BACKUP" ]; then
    echo "[$(date)] ERROR: No L2 state backup found!" >> "$LOG_FILE"
    exit 1
fi

# Check backup age (should be < 30 minutes old)
BACKUP_AGE=$(($(date +%s) - $(stat -c %Y "$LATEST_BACKUP")))
if [ $BACKUP_AGE -gt 1800 ]; then
    echo "[$(date)] ERROR: Latest backup is $((BACKUP_AGE/60)) minutes old" >> "$LOG_FILE"
    exit 1
fi

# Verify backup integrity
if ! ailee-cli verify-checkpoint "$LATEST_BACKUP"; then
    echo "[$(date)] ERROR: Backup integrity check failed" >> "$LOG_FILE"
    exit 1
fi

echo "[$(date)] Backup verification successful" >> "$LOG_FILE"
```

---

## Recovery Procedures

### 1. Federation Key Recovery

**Scenario:** Signer node hardware failure or key corruption

**Procedure:**

```bash
# Step 1: Secure the environment
# Ensure recovery is happening in secure, offline environment

# Step 2: Retrieve encrypted backup
# From secure storage location
gpg --decrypt /secure/backup/signer-01.key.gpg > /tmp/signer-01.key

# Step 3: Verify key integrity
# Compare public key hash with recorded value
openssl ec -in /tmp/signer-01.key -pubout | sha256sum

# Step 4: Import to HSM (preferred) or secure file location
# For HSM:
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
    --login --write-object /tmp/signer-01.key \
    --type privkey --id 01 --label "ailee-signer-01"

# For file-based (temporary):
install -m 0400 -o ailee -g ailee /tmp/signer-01.key /etc/ailee/keys/

# Step 5: Securely delete temporary file
shred -vfz -n 10 /tmp/signer-01.key

# Step 6: Verify signer functionality
ailee-cli test-signer --id signer-01

# Step 7: Document recovery in audit log
echo "[$(date)] Signer key recovered for signer-01" >> /var/log/ailee/key-operations.log
```

### 2. L2 State Database Recovery

**Scenario:** Database corruption or node failure

**Procedure:**

```bash
#!/bin/bash
# recover-l2-state.sh

set -e

# Step 1: Stop the node
systemctl stop ailee-node

# Step 2: Backup current (possibly corrupted) state
mv /data/node-1 /data/node-1.corrupted.$(date +%Y%m%d_%H%M%S)

# Step 3: Download latest full backup
aws s3 cp s3://ailee-backups/l2-state/full/full-$(date +%Y%m%d).tar.gz \
    /tmp/l2-state-backup.tar.gz

# Step 4: Extract backup
tar -xzf /tmp/l2-state-backup.tar.gz -C /data/

# Step 5: Apply incremental backups
# Download all incremental backups since the full backup
aws s3 sync s3://ailee-backups/l2-state/incremental/ \
    /tmp/incremental-backups/ \
    --exclude "*" --include "snapshot-*"

# Apply each incremental in order
for snapshot in $(ls -1 /tmp/incremental-backups/snapshot-* | sort); do
    echo "Applying incremental: $snapshot"
    ailee-cli apply-checkpoint "$snapshot" --target /data/node-1
done

# Step 6: Verify state integrity
ailee-cli verify-state /data/node-1

# Step 7: Start the node
systemctl start ailee-node

# Step 8: Monitor sync status
ailee-cli sync-status

echo "Recovery complete. Node is syncing."
```

### 3. Configuration Recovery

**Scenario:** Configuration file loss or corruption

**Procedure:**

```bash
# Step 1: Stop services
systemctl stop ailee-node ailee-api

# Step 2: Restore configuration from backup
LATEST_CONFIG=$(ls -1 /backup/config/config-*.tar.gz | sort -r | head -n1)
tar -xzf "$LATEST_CONFIG" -C /

# Step 3: Restore secrets from secure backup
# Decrypt and restore secrets.yaml
gpg --decrypt /secure/backup/secrets.yaml.gpg > /etc/ailee/secrets.yaml
chmod 0400 /etc/ailee/secrets.yaml

# Step 4: Validate configuration
ailee-cli validate-config

# Step 5: Restart services
systemctl start ailee-node ailee-api

# Step 6: Verify operation
ailee-cli health-check
```

---

## Disaster Recovery

### Complete Node Failure

**Scenario:** Complete loss of node (hardware failure, datacenter outage)

**Procedure:**

1. **Provision new hardware/VM** matching production specifications
2. **Install base system** (OS, dependencies)
3. **Recover configuration** from backup
4. **Recover keys** from secure backup (if not using HSM)
5. **Recover L2 state** from most recent backup
6. **Sync from network** to catch up to current state
7. **Verify** state consistency with other nodes
8. **Resume** normal operation

**Estimated Recovery Time:** 2-4 hours (depending on state size)

### Federation Compromise

**Scenario:** Multiple federation keys compromised

**Emergency Procedure:**

1. **HALT all operations immediately**
2. **Activate incident response team**
3. **Freeze all peg-ins/peg-outs**
4. **Notify users** through all communication channels
5. **Forensic analysis** to determine scope of compromise
6. **Generate new keys** using secure ceremony
7. **Migrate funds** to new multisig address (if possible)
8. **Update all nodes** with new federation configuration
9. **Resume operations** only after full security review

---

## Testing

### Backup Test Schedule

| Test Type | Frequency | Purpose |
|-----------|-----------|---------|
| Backup verification | Daily | Ensure backups are being created |
| Config recovery | Weekly | Verify config can be restored |
| Key recovery | Monthly | Test key restoration procedure |
| Full node recovery | Quarterly | End-to-end disaster recovery |
| Disaster recovery drill | Bi-annually | Full team exercise |

### Test Procedure (Quarterly Full Recovery)

```bash
# Quarterly disaster recovery test
# Perform on separate test environment

# 1. Simulate failure
# Stop test node and delete data

# 2. Execute recovery procedure
# Follow full recovery procedure documented above

# 3. Verify recovery
# - L2 state matches other nodes
# - All services operational
# - Federation signing works
# - API endpoints responding

# 4. Measure metrics
# - Recovery time (should meet RTO)
# - Data loss (should meet RPO)

# 5. Document results
# Update runbook with any issues or improvements
```

### Recovery Metrics to Track

```bash
# Create recovery metrics dashboard

# Time to detect failure
# Time to initiate recovery
# Time to restore keys
# Time to restore state
# Time to full operation
# Data loss (in blocks/transactions)
# Success/failure rate
```

---

## Best Practices

1. **Test backups regularly** - Untested backups are useless
2. **Encrypt sensitive data** - Always encrypt keys and sensitive configuration
3. **Geographic distribution** - Store backups in multiple locations
4. **Automate everything** - Manual processes fail under pressure
5. **Document procedures** - Update runbooks after each incident
6. **Monitor backup status** - Alert on backup failures immediately
7. **Version control** - Track configuration changes
8. **Audit trail** - Log all backup and recovery operations
9. **Separate storage** - Don't store backups on same infrastructure
10. **Regular drills** - Practice makes perfect

---

## Emergency Contacts

Maintain an up-to-date list of emergency contacts:

- **System Administrator:** [contact info]
- **Database Administrator:** [contact info]
- **Security Team Lead:** [contact info]
- **Federation Operators:** [contact info]
- **Cloud Provider Support:** [contact info]
- **HSM Vendor Support:** [contact info]

---

## Appendix: Backup Scripts

See `/opt/ailee-core/scripts/backup/` for complete backup automation scripts:

- `backup-l2-incremental.sh` - Incremental state backup
- `backup-l2-full.sh` - Full daily backup
- `backup-config.sh` - Configuration backup
- `backup-keys.sh` - Key backup (manual, requires operator)
- `verify-backups.sh` - Automated verification
- `recover-node.sh` - Automated recovery script

---

**Last Updated:** 2026-02-15  
**Document Owner:** AILEE-Core DevOps Team  
**Review Schedule:** Quarterly
