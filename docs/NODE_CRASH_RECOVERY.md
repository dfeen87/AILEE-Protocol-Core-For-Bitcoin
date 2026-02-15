# Node Crash Recovery Guide

This document provides detailed procedures for recovering from AILEE node crashes and ensuring data integrity.

---

## Table of Contents

1. [Overview](#overview)
2. [State Persistence Architecture](#state-persistence-architecture)
3. [Automatic Recovery Procedures](#automatic-recovery-procedures)
4. [Manual Recovery Procedures](#manual-recovery-procedures)
5. [Data Validation](#data-validation)
6. [Troubleshooting](#troubleshooting)

---

## Overview

AILEE nodes are designed with crash-resilience in mind. The system maintains persistent state through:

1. **L2 State Snapshots** - Append-only log of L2 state at each anchor point
2. **Reorg Detection Database** - RocksDB-backed storage of block hashes and anchor commitments
3. **Checkpoint-based Recovery** - Ability to restore to last known good state

### Recovery Guarantees

✅ **What is guaranteed**:
- Last anchored state is always recoverable
- Reorg detection state survives crashes
- Anchor commitments and their Bitcoin confirmations are preserved
- No corruption of historical snapshots (append-only)

❌ **What is not guaranteed**:
- In-flight transactions since last anchor may be lost
- Unanchored state changes require resubmission
- Temporary mempool state is not persisted

---

## State Persistence Architecture

### File Locations

```
/var/lib/ailee/
├── l2_snapshots.dat          # L2 state snapshots (append-only)
├── reorg_db/                 # RocksDB database for reorg detection
│   ├── CURRENT
│   ├── LOCK
│   ├── MANIFEST-*
│   └── *.sst
└── backups/                  # Optional backup directory
    └── l2_snapshots_*.dat.bak
```

### Snapshot Format

Each snapshot in `l2_snapshots.dat` contains:

```
SNAPSHOT v1
timestamp_ms <milliseconds>
balances <count>
balance "<peerId>" <amount>
...
escrows <count>
escrow "<taskId>" "<clientPeerId>" <amount> <locked> <createdAt>
...
pegins <count>
pegin "<pegId>" "<btcTxId>" <vout> <amount> "<btcSource>" "<aileeDest>" ...
...
pegouts <count>
pegout "<pegId>" "<aileeSource>" "<btcDest>" <burnAmount> <releaseAmount> ...
...
tasks <count>
task "<taskId>" <type> <priority> "<submitterId>" <submittedAt> ...
...
anchor "<l2StateRoot>" <timestamp> "<recoveryMetadata>" "<payload>" "<hash>"
END_SNAPSHOT
```

### RocksDB Schema

The reorg detection database uses the following key prefixes:

- `block:<height>` → Block hash at height
- `anchor:<anchorHash>` → Serialized AnchorCommitmentRecord
- `reorg:<eventId>` → Serialized ReorgEvent
- `reorg_counter` → Counter for reorg events

---

## Automatic Recovery Procedures

### On Normal Startup

When an AILEE node starts, it automatically:

1. **Opens the reorg detection database**:
   ```cpp
   ReorgDetector detector("/var/lib/ailee/reorg_db");
   detector.initialize(&err);
   ```

2. **Loads the latest L2 snapshot**:
   ```cpp
   auto snapshot = loadLatestSnapshotFromFile("/var/lib/ailee/l2_snapshots.dat", &err);
   ```

3. **Validates snapshot integrity**:
   ```cpp
   std::string recomputedRoot = computeL2StateRoot(snapshot);
   if (snapshot.anchor && snapshot.anchor->l2StateRoot != recomputedRoot) {
       // Corruption detected - fall back to backup
   }
   ```

4. **Restores system state**:
   ```cpp
   ledger.restoreFromSnapshot(snapshot.ledger);
   bridge.restoreFromSnapshot(snapshot.bridge);
   engine.restoreFromSnapshot(snapshot.orchestration);
   ```

5. **Resumes Bitcoin monitoring**:
   - Reconnects to Bitcoin RPC
   - Syncs from last known block height
   - Detects any reorgs that occurred during downtime

### Recovery from Clean Shutdown

If the node was shut down cleanly (SIGTERM/SIGINT handled):
- State is already persisted
- No special recovery needed
- Node resumes from last snapshot

### Recovery from Crash

If the node crashed (SIGKILL, power loss, kernel panic):

1. **Snapshot File Integrity**:
   - Append-only format means partial writes only affect the incomplete snapshot
   - Last complete `END_SNAPSHOT` marker indicates valid recovery point
   - Incomplete snapshots are ignored

2. **RocksDB Integrity**:
   - RocksDB uses WAL (Write-Ahead Log) for crash consistency
   - Incomplete writes are automatically rolled back
   - Database opens to last committed state

3. **State Divergence Check**:
   ```cpp
   // Compare snapshot timestamp with Bitcoin blockchain
   auto btcHeight = bitcoinClient.getBlockCount();
   auto snapshotAge = currentTime - snapshot.snapshotTimestampMs;
   
   if (snapshotAge > MAX_ACCEPTABLE_DIVERGENCE) {
       logWarning("Large gap between crash and recovery");
       // May need to re-anchor
   }
   ```

---

## Manual Recovery Procedures

### Procedure 1: Restore from Latest Snapshot

If automatic recovery fails:

```bash
# 1. Stop the node
sudo systemctl stop ailee-node

# 2. Verify snapshot file integrity
./ailee_l2_verify --snapshot /var/lib/ailee/l2_snapshots.dat

# 3. If verification fails, restore from backup
cp /var/lib/ailee/backups/l2_snapshots_latest.dat.bak \
   /var/lib/ailee/l2_snapshots.dat

# 4. Restart the node
sudo systemctl start ailee-node

# 5. Monitor logs
sudo journalctl -u ailee-node -f
```

### Procedure 2: Restore from Earlier Snapshot

If the latest snapshot is corrupted:

```bash
# Extract all snapshots
./ailee_extract_snapshots /var/lib/ailee/l2_snapshots.dat /tmp/snapshots/

# List available snapshots
ls -lh /tmp/snapshots/

# Verify each snapshot
for file in /tmp/snapshots/snapshot_*.dat; do
    echo "Verifying $file..."
    ./ailee_l2_verify --snapshot "$file"
done

# Use the latest valid snapshot
cp /tmp/snapshots/snapshot_12345.dat /var/lib/ailee/l2_snapshots.dat

# Restart node
sudo systemctl restart ailee-node
```

### Procedure 3: Rebuild Reorg Database

If the RocksDB database is corrupted:

```bash
# 1. Stop the node
sudo systemctl stop ailee-node

# 2. Backup corrupted database
mv /var/lib/ailee/reorg_db /var/lib/ailee/reorg_db.corrupted

# 3. Create fresh database
./ailee_init_reorg_db --path /var/lib/ailee/reorg_db

# 4. Resync block hashes from Bitcoin
./ailee_resync_blocks --start-height <last_known_height> \
                       --db-path /var/lib/ailee/reorg_db \
                       --bitcoin-rpc http://localhost:8332

# 5. Restart node
sudo systemctl start ailee-node
```

### Procedure 4: Complete Reset (Last Resort)

If all recovery attempts fail:

```bash
# 1. Stop the node
sudo systemctl stop ailee-node

# 2. Backup current state
mkdir -p /var/lib/ailee/recovery_$(date +%s)
cp -r /var/lib/ailee/l2_snapshots.dat \
      /var/lib/ailee/reorg_db \
      /var/lib/ailee/recovery_$(date +%s)/

# 3. Remove corrupted state
rm -f /var/lib/ailee/l2_snapshots.dat
rm -rf /var/lib/ailee/reorg_db

# 4. Initialize fresh state
./ailee_init_node --data-dir /var/lib/ailee

# 5. Restart node
sudo systemctl start ailee-node

# 6. Re-anchor current state
./ailee_create_anchor --l2-state-file /tmp/current_state.json
```

⚠️ **Warning**: Complete reset loses all L2 state. Only use if:
- All backups are corrupted
- Snapshot files are unrecoverable
- You have coordinated with other federation members
- Users have been notified about potential peg-out delays

---

## Data Validation

### Validate L2 Snapshot

```bash
./ailee_l2_verify --snapshot /var/lib/ailee/l2_snapshots.dat
```

Expected output on success:
```
Computed L2StateRoot: 7a3f8b2c...
Anchor verified: 7a3f8b2c...
Verification OK.
```

Expected output on failure:
```
Computed L2StateRoot: 7a3f8b2c...
Anchor verification failed: Anchor state root mismatch. expected=7a3f8b2c... anchor=9d5e6f1a...
<exit code 2>
```

### Validate Reorg Database

```cpp
#include "ReorgDetector.h"

// Open database
ReorgDetector detector("/var/lib/ailee/reorg_db");
if (!detector.initialize(&err)) {
    std::cerr << "Database corrupted: " << err << "\n";
    return 1;
}

// Check for recent reorgs
auto reorgs = detector.getRecentReorgHistory(10);
for (const auto& reorg : reorgs) {
    std::cout << "Reorg at height " << reorg.reorgHeight << "\n";
}

// Verify anchor statuses
auto pending = detector.getAnchorsByStatus(AnchorStatus::PENDING);
auto confirmed = detector.getAnchorsByStatus(AnchorStatus::CONFIRMED);
auto invalidated = detector.getAnchorsByStatus(AnchorStatus::INVALIDATED_REORG);

std::cout << "Pending: " << pending.size() << "\n";
std::cout << "Confirmed: " << confirmed.size() << "\n";
std::cout << "Invalidated: " << invalidated.size() << "\n";
```

### Cross-Validate with Bitcoin

```bash
# Get latest block from snapshot
SNAPSHOT_BLOCK=$(./ailee_get_snapshot_block /var/lib/ailee/l2_snapshots.dat)

# Get current Bitcoin block
BITCOIN_BLOCK=$(bitcoin-cli getblockcount)

# Calculate divergence
DIVERGENCE=$((BITCOIN_BLOCK - SNAPSHOT_BLOCK))

echo "Snapshot is $DIVERGENCE blocks behind Bitcoin"

# If divergence is large, may need to re-anchor
if [ $DIVERGENCE -gt 100 ]; then
    echo "Warning: Large divergence detected"
fi
```

---

## Troubleshooting

### Issue: Node won't start after crash

**Symptoms**:
- Node exits immediately after start
- Error message: "Failed to load snapshot"

**Solution**:
```bash
# Check snapshot file permissions
ls -l /var/lib/ailee/l2_snapshots.dat

# Should be readable by ailee user
sudo chown ailee:ailee /var/lib/ailee/l2_snapshots.dat

# Verify snapshot integrity
./ailee_l2_verify --snapshot /var/lib/ailee/l2_snapshots.dat

# If corrupted, restore from backup
sudo -u ailee cp /var/lib/ailee/backups/l2_snapshots_*.dat.bak \
                  /var/lib/ailee/l2_snapshots.dat
```

### Issue: Reorg database locked

**Symptoms**:
- Error message: "IO error: lock /var/lib/ailee/reorg_db/LOCK"

**Cause**:
- Previous instance didn't exit cleanly
- LOCK file left behind

**Solution**:
```bash
# Verify no ailee process is running
ps aux | grep ailee

# If no process, remove stale lock
sudo rm /var/lib/ailee/reorg_db/LOCK

# Restart node
sudo systemctl start ailee-node
```

### Issue: Snapshot file growing too large

**Symptoms**:
- `l2_snapshots.dat` exceeds several GB
- Slow startup times

**Solution**:
```bash
# Prune old snapshots (keep last N)
./ailee_prune_snapshots --file /var/lib/ailee/l2_snapshots.dat \
                        --keep 1000 \
                        --output /var/lib/ailee/l2_snapshots_pruned.dat

# Verify pruned file
./ailee_l2_verify --snapshot /var/lib/ailee/l2_snapshots_pruned.dat

# Replace if valid
sudo systemctl stop ailee-node
mv /var/lib/ailee/l2_snapshots.dat /var/lib/ailee/l2_snapshots_old.dat
mv /var/lib/ailee/l2_snapshots_pruned.dat /var/lib/ailee/l2_snapshots.dat
sudo systemctl start ailee-node
```

### Issue: State root mismatch after recovery

**Symptoms**:
- Verification shows: "Anchor state root mismatch"

**Cause**:
- Snapshot file corruption
- Bug in state computation
- Concurrent modification during crash

**Solution**:
```bash
# Try previous snapshot
./ailee_extract_snapshots /var/lib/ailee/l2_snapshots.dat /tmp/snapshots/

# Find last valid snapshot
for i in $(seq 10 -1 1); do
    snapshot="/tmp/snapshots/snapshot_${i}.dat"
    if ./ailee_l2_verify --snapshot "$snapshot" > /dev/null 2>&1; then
        echo "Found valid snapshot: $snapshot"
        cp "$snapshot" /var/lib/ailee/l2_snapshots.dat
        break
    fi
done

# Restart node
sudo systemctl restart ailee-node
```

---

## Best Practices

### 1. Regular Backups

Set up automated backups:

```bash
# /etc/cron.hourly/ailee-backup
#!/bin/bash
BACKUP_DIR=/var/lib/ailee/backups
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Backup snapshot file
cp /var/lib/ailee/l2_snapshots.dat \
   $BACKUP_DIR/l2_snapshots_$TIMESTAMP.dat.bak

# Keep only last 24 backups
ls -t $BACKUP_DIR/l2_snapshots_*.dat.bak | tail -n +25 | xargs rm -f

# Backup reorg database
tar -czf $BACKUP_DIR/reorg_db_$TIMESTAMP.tar.gz \
         -C /var/lib/ailee reorg_db/

# Keep only last 24 reorg DB backups
ls -t $BACKUP_DIR/reorg_db_*.tar.gz | tail -n +25 | xargs rm -f
```

### 2. Monitoring

Monitor file sizes and growth:

```bash
# /etc/cron.daily/ailee-monitor
#!/bin/bash
SNAPSHOT_SIZE=$(stat -c%s /var/lib/ailee/l2_snapshots.dat)
SNAPSHOT_SIZE_MB=$((SNAPSHOT_SIZE / 1024 / 1024))

if [ $SNAPSHOT_SIZE_MB -gt 5000 ]; then
    echo "Warning: Snapshot file is ${SNAPSHOT_SIZE_MB}MB - consider pruning"
fi

# Check reorg DB size
REORG_SIZE=$(du -sm /var/lib/ailee/reorg_db | cut -f1)
if [ $REORG_SIZE -gt 1000 ]; then
    echo "Warning: Reorg DB is ${REORG_SIZE}MB - consider pruning"
fi
```

### 3. Testing Recovery

Periodically test recovery procedures:

```bash
# Test recovery on staging environment
./test_recovery.sh --mode crash --data-dir /tmp/ailee_test
./test_recovery.sh --mode reorg --reorg-depth 3
./test_recovery.sh --mode corruption --corrupt-snapshot
```

---

## See Also

- [L2_PROTOCOL_SPECIFICATION.md](./L2_PROTOCOL_SPECIFICATION.md) - Complete L2 protocol spec
- [VERIFICATION.md](./VERIFICATION.md) - Verification model
- [BACKUP_RECOVERY.md](./BACKUP_RECOVERY.md) - Backup strategies

---

*Last Updated: 2025-02-15*  
*Version: 1.0.0*
