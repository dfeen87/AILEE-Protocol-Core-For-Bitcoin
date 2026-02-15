# AILEE Layer-2 Protocol Specification

**Definitive Answers to Critical L2 Protocol Questions**

This document provides explicit, technically precise answers to the questions that distinguish a proper Layer-2 protocol from an "anchored app."

---

## Table of Contents

1. [What Exactly is Anchored?](#1-what-exactly-is-anchored)
2. [How is Anchoring Verified?](#2-how-is-anchoring-verified)
3. [Failure Scenario Handling](#3-failure-scenario-handling)
4. [Protocol vs. Anchored App](#4-protocol-vs-anchored-app)
5. [Security Guarantees](#5-security-guarantees)

---

## 1. What Exactly is Anchored?

### 1.1 State Root Composition

AILEE anchors a **deterministic L2 state root** to Bitcoin. This state root is a cryptographic commitment (SHA-256 hash) to the complete L2 state, computed from:

```
L2StateRoot = SHA256(Canonical(
    LedgerState || BridgeState || OrchestrationState
))
```

#### Components:

**A. Ledger State**
- **Balances**: Sorted list of all account balances `(peerId, balance)`
- **Escrows**: Sorted list of all task escrows `(taskId, clientPeerId, amount, locked, createdAt)`

**B. Bridge State**
- **Peg-Ins**: Sorted list of all peg-in operations
  - `pegId, btcTxId, vout, btcAmount, btcSource, aileeDest, confirmations, status, timestamps`
- **Peg-Outs**: Sorted list of all peg-out operations
  - `pegId, aileeSource, btcDest, aileeBurnAmount, btcReleaseAmount, status, anchorCommitmentHash, timestamps`

**C. Orchestration State**
- **Tasks**: Sorted list of all queued tasks
  - `taskId, taskType, priority, submitterId, submittedAtMs, payloadHash, anchorCommitmentHash`

### 1.2 Canonical Serialization

All components are sorted lexicographically by their primary identifier before serialization to ensure deterministic ordering. The canonical format is:

```
L2STATE|v1
balances:<count>
balance:<peerId>:<balance>
...
escrows:<count>
escrow:<taskId>:<clientPeerId>:<amount>:<locked>:<createdAt>
...
pegins:<count>
pegin:<pegId>:<btcTxId>:<vout>:<btcAmount>:<btcSource>:<aileeDest>:<status>:<confirmations>:<initiatedTime>:<completedTime>
...
pegouts:<count>
pegout:<pegId>:<aileeSource>:<btcDest>:<aileeBurnAmount>:<btcReleaseAmount>:<status>:<anchorCommitmentHash>:<initiatedTime>:<completedTime>
...
tasks:<count>
task:<taskId>:<taskType>:<priority>:<submitterId>:<submittedAtMs>:<payloadHash>:<anchorCommitmentHash>
...
```

This canonical form is hashed with SHA-256 to produce the L2StateRoot.

### 1.3 Anchor Commitment Structure

The anchor commitment itself contains:

```cpp
struct AnchorSnapshot {
    std::string l2StateRoot;          // The computed state root
    uint64_t timestampMs;             // Millisecond timestamp of anchor creation
    std::string recoveryMetadata;     // Recovery-related metadata
    std::string payload;              // Full payload: "stateRoot:timestamp:metadata"
    std::string hash;                 // SHA256 of the payload
};
```

### 1.4 Bitcoin Encoding

The anchor is committed to Bitcoin in one of two ways:

**Option A: OP_RETURN (≤80 bytes)**
```
OP_RETURN <commitment>
```
Where `commitment` is either:
- The full payload (if ≤80 bytes), or
- `"AILEE" || SHA256(payload)` (37 bytes)

**Option B: Taproot Commitment**
```
OP_FALSE OP_IF <commitment> OP_ENDIF
```
Embedded in a Tapscript leaf.

### 1.5 What is NOT Anchored

The following are explicitly **not** included in the state root:
- Individual transaction contents (only task payload hashes)
- Full transaction history (only current state)
- Telemetry data beyond what's in orchestration state
- Off-chain data or external oracle inputs
- Pending mempool transactions not yet in tasks

---

## 2. How is Anchoring Verified?

### 2.1 Deterministic Replay Model

**Anyone can independently recompute the state root** given:
1. A snapshot file containing the L2 state
2. The `computeL2StateRoot()` function (deterministic)
3. No Bitcoin RPC access required

#### Verification Process:

```bash
# Load snapshot from disk
snapshot = loadLatestSnapshotFromFile("l2_snapshots.dat")

# Recompute state root deterministically
computedRoot = computeL2StateRoot(snapshot)

# Verify anchor commitment
assert(snapshot.anchor.l2StateRoot == computedRoot)
assert(SHA256(snapshot.anchor.payload) == snapshot.anchor.hash)

# Verify all peg-outs reference this anchor
for pegout in snapshot.bridge.pegouts:
    assert(pegout.anchorCommitmentHash == snapshot.anchor.hash)
```

### 2.2 Third-Party Verification Tool

The `ailee_l2_verify` tool provides offline verification:

```bash
./ailee_l2_verify --snapshot /path/to/l2_snapshots.dat
```

This tool:
1. Loads the latest snapshot from disk
2. Recomputes the L2StateRoot from scratch
3. Validates the anchor commitment hash
4. Verifies all peg-outs reference the correct anchor
5. Returns exit code 0 (success), 1 (error), or 2 (verification failed)

**Key Property**: Verification is **purely local** and **deterministic**. No network access, no trusted parties, no proprietary data.

### 2.3 Reproducibility Guarantees

Given identical inputs, the verification process produces identical outputs across:
- Different machines
- Different operating systems
- Different points in time
- Different operators

This is achieved through:
- Deterministic sorting of all state components
- Fixed serialization format
- Standard SHA-256 hashing
- No random or time-dependent operations in state root computation

### 2.4 Merkle Roots vs. State Roots

**AILEE uses a state root, not a Merkle root.**

- **Merkle Root**: Cryptographic commitment to a set of transactions (Bitcoin blocks use this)
- **State Root**: Cryptographic commitment to the entire current state (Ethereum uses this)

AILEE's state root commits to:
- Current balances (not transaction history)
- Current bridge state (not all historical peg operations)
- Current task queue (not all historical tasks)

This is more similar to Ethereum's state root than Bitcoin's transaction Merkle root.

### 2.5 Batched Commitments

Anchors are created on-demand, typically:
- After significant state changes (large peg-out)
- Before critical operations (governance decisions)
- On a regular schedule (e.g., every N blocks)

Each anchor commits to the **complete state at that moment**, not just incremental changes.

---

## 3. Failure Scenario Handling

### 3.1 Node Crash Recovery

**Scenario**: Your AILEE node crashes mid-operation.

**Recovery Procedure**:

1. **State Persistence**: L2 state snapshots are appended to disk after each anchor:
   ```
   /var/lib/ailee/l2_snapshots.dat
   ```

2. **On Restart**:
   ```cpp
   // Load the latest valid snapshot
   auto snapshot = loadLatestSnapshotFromFile("l2_snapshots.dat", &err);
   
   // Restore ledger balances
   ledger.restoreFromSnapshot(snapshot.ledger);
   
   // Restore bridge state
   bridge.restoreFromSnapshot(snapshot.bridge);
   
   // Restore orchestration state
   engine.restoreFromSnapshot(snapshot.orchestration);
   ```

3. **Consistency Check**:
   ```cpp
   // Recompute state root to verify integrity
   std::string recomputedRoot = computeL2StateRoot(snapshot);
   if (snapshot.anchor && snapshot.anchor->l2StateRoot != recomputedRoot) {
       // Snapshot corruption detected - fall back to previous snapshot
       recoverFromBackup();
   }
   ```

**Guarantees**:
- Last anchored state is always recoverable
- Snapshots are write-once, append-only (no corruption of old snapshots)
- Multiple snapshots allow rollback to earlier states

**Data Loss**:
- Changes since last snapshot are lost
- In-flight transactions not yet anchored may need resubmission
- This is **intentional** - only anchored states are considered durable

### 3.2 Bitcoin Reorg Handling

**Scenario**: A Bitcoin block containing an anchor transaction is reorganized out of the chain.

**Detection**:

```cpp
class ReorgDetector {
    // Track block hashes at each height
    std::map<uint64_t, std::string> blockHashAtHeight;
    
    bool detectReorg(uint64_t height, const std::string& newBlockHash) {
        auto it = blockHashAtHeight.find(height);
        if (it != blockHashAtHeight.end() && it->second != newBlockHash) {
            // Reorg detected at this height
            return true;
        }
        blockHashAtHeight[height] = newBlockHash;
        return false;
    }
};
```

**Response**:

1. **Invalidate Affected Anchors**:
   ```cpp
   void handleReorg(uint64_t reorgHeight) {
       // Mark all anchors at or after reorg height as invalid
       for (auto& anchor : anchorCommitments) {
           if (anchor.bitcoinHeight >= reorgHeight) {
               anchor.status = AnchorStatus::INVALIDATED_REORG;
               anchor.confirmations = 0;
           }
       }
   }
   ```

2. **Halt Affected Peg-Outs**:
   ```cpp
   // Any peg-out referencing an invalidated anchor must be re-anchored
   for (auto& pegout : pendingPegouts) {
       if (getAnchorStatus(pegout.anchorCommitmentHash) == INVALIDATED_REORG) {
           pegout.status = PegOutStatus::PENDING_REANCHOR;
       }
   }
   ```

3. **Re-anchor if Necessary**:
   ```cpp
   // Create a new anchor commitment in the reorganized chain
   AnchorSnapshot newAnchor = createAnchorCommitment(currentL2State);
   broadcastAnchorToBitcoin(newAnchor);
   ```

**Safety Rules**:
- Peg-outs wait for N confirmations (default: 6) before finalization
- Reorgs beyond 6 blocks trigger circuit breaker (emergency halt)
- State root remains valid - only the Bitcoin commitment is invalidated

### 3.3 Orphaned Anchor Transaction

**Scenario**: An anchor transaction is broadcast but never confirmed (stuck in mempool or rejected).

**Detection**:

```cpp
class AnchorMonitor {
    void monitorAnchorConfirmations() {
        for (auto& anchor : pendingAnchors) {
            auto age = currentTime - anchor.broadcastTime;
            
            if (age > MAX_ANCHOR_PENDING_TIME && anchor.confirmations == 0) {
                // Anchor is orphaned
                handleOrphanedAnchor(anchor);
            }
        }
    }
};
```

**Response**:

1. **Retry with Higher Fee**:
   ```cpp
   void handleOrphanedAnchor(AnchorSnapshot& anchor) {
       // Attempt to re-broadcast with RBF (Replace-By-Fee)
       if (anchor.retryCount < MAX_RETRIES) {
           rebroadcastWithHigherFee(anchor);
           anchor.retryCount++;
       } else {
           // Mark as failed and create new anchor
           anchor.status = AnchorStatus::FAILED_ORPHANED;
           createNewAnchorCommitment();
       }
   }
   ```

2. **Update Dependent Operations**:
   ```cpp
   // Peg-outs referencing orphaned anchor must wait for new anchor
   updatePegoutAnchorReferences(oldAnchorHash, newAnchorHash);
   ```

**Guarantees**:
- The L2 state root remains valid regardless of anchor status
- Peg-outs are never finalized without a confirmed anchor
- Failed anchors can be re-created with the same state root

### 3.4 Conflicting L2 States

**Scenario**: Two different nodes produce different state roots for the "same" L2 state.

**Detection**:

```cpp
struct StateConsensus {
    std::map<std::string, std::vector<NodeId>> stateRootVotes;
    
    void recordStateRoot(NodeId node, const std::string& stateRoot) {
        stateRootVotes[stateRoot].push_back(node);
    }
    
    bool hasConflict() {
        return stateRootVotes.size() > 1;
    }
    
    std::string getCanonicalStateRoot() {
        // Return state root with majority support
        auto maxIt = std::max_element(
            stateRootVotes.begin(), stateRootVotes.end(),
            [](const auto& a, const auto& b) {
                return a.second.size() < b.second.size();
            }
        );
        return maxIt->first;
    }
};
```

**Response**:

1. **Halt Operations**:
   ```cpp
   if (stateConsensus.hasConflict()) {
       // Trigger circuit breaker
       circuitBreaker.halt("State conflict detected");
       
       // Log conflict details
       logStateConflict(stateConsensus.getAllStateRoots());
       
       // Notify operators
       alertOperators("Manual intervention required: state conflict");
   }
   ```

2. **Manual Investigation**:
   - Compare snapshot files from conflicting nodes
   - Identify divergence point in state history
   - Determine root cause (bug, malicious node, data corruption)

3. **Resolution**:
   - **If bug**: Fix bug, all nodes recompute from last valid checkpoint
   - **If malicious**: Remove malicious node from federation
   - **If corruption**: Restore from backup snapshot

**Prevention**:
- Deterministic state computation (same inputs → same outputs)
- Snapshot validation before broadcasting
- Peer verification of state roots before anchoring

**Trade-off**:
- AILEE uses a **federated model** with explicit trust in validator nodes
- Not a trustless rollup - conflicts require governance intervention
- This is **by design** for operational flexibility

---

## 4. Protocol vs. Anchored App

### 4.1 What Makes AILEE a Protocol?

| Criterion | Anchored App | AILEE Protocol | ✓ |
|-----------|--------------|----------------|---|
| **Deterministic State** | May be opaque | ✓ Fully specified canonical format | ✓ |
| **Independent Verification** | Trust required | ✓ Anyone can verify with `ailee_l2_verify` | ✓ |
| **State Replay** | Usually impossible | ✓ Deterministic from snapshot | ✓ |
| **Failure Handling** | Often undefined | ✓ Explicit reorg/crash procedures | ✓ |
| **Public Spec** | Often proprietary | ✓ Complete specification in this doc | ✓ |
| **Commitment Format** | May be custom/opaque | ✓ Standard Bitcoin OP_RETURN/Taproot | ✓ |
| **Recovery Procedures** | Manual/ad-hoc | ✓ Automated with fallbacks | ✓ |

### 4.2 Remaining Limitations (Honest Assessment)

AILEE is a **federated sidechain protocol**, not a trustless rollup. Limitations:

- ❌ **No fraud proofs**: Invalid state transitions are not enforced by Bitcoin
- ❌ **No validity proofs**: State correctness is not proven on Bitcoin L1
- ❌ **No trustless exits**: Peg-outs require federation approval
- ❌ **No data availability guarantees**: L2 state is not posted to Bitcoin
- ⚠️ **Federated trust**: Users trust the validator federation

**Why these trade-offs?**
- Bitcoin L1 cannot verify complex state transitions (no smart contracts)
- Rollup-style DA would bloat Bitcoin blockchain
- Federated model enables operational flexibility and faster iteration
- Explicit trust model is more honest than hidden assumptions

---

## 5. Security Guarantees

### 5.1 What AILEE Guarantees

✅ **Verifiable State Commitments**
- Any party can independently compute the state root from a snapshot
- Anchor commitments are cryptographically binding

✅ **Peg-In Security**
- SPV proofs verify Bitcoin transactions without full node
- Multi-confirmation requirement (default: 6 blocks)
- Merkle proof validation ensures transaction inclusion

✅ **Peg-Out Authorization**
- All peg-outs must reference a confirmed anchor
- Federation multisig (10/15 threshold) prevents single-point compromise
- Anchor-bound authorization prevents unauthorized exits

✅ **Crash Recovery**
- Last anchored state always recoverable from disk
- Append-only snapshot log prevents corruption
- Multiple recovery checkpoints for redundancy

✅ **Reorg Resilience**
- Automatic detection and handling of Bitcoin reorgs
- Anchor invalidation and re-anchoring protocol
- Peg-out holds until anchor is re-confirmed

### 5.2 What AILEE Does NOT Guarantee

❌ **Liveness Under Federation Failure**
- If <10 federation signers are available, peg-outs halt
- This is a **Byzantine failure mode**, not a protocol failure

❌ **Censorship Resistance**
- Federation can censor peg-out requests
- This is an **explicit trade-off** for operational control

❌ **State Validity Enforcement**
- Bitcoin L1 does not verify L2 state transitions
- Invalid states can be committed (but are detectable off-chain)

❌ **Data Availability**
- L2 state is not posted to Bitcoin
- Users must maintain snapshot files for verification
- Data availability depends on node operators, not Bitcoin

### 5.3 Security Model Summary

**Trust Assumptions**:
1. At least 10/15 federation signers are honest
2. At least one archival node maintains complete snapshot history
3. Bitcoin L1 remains secure (PoW consensus)

**Attack Resistance**:
- ✓ Up to 5 malicious federation signers
- ✓ Bitcoin reorgs up to 6 blocks (circuit breaker beyond that)
- ✓ Node crashes with snapshot recovery
- ✓ Network partitions with eventual consistency

**Attack Vulnerabilities**:
- ✗ Majority federation collusion (>10/15 malicious signers)
- ✗ All archival nodes losing snapshot data simultaneously
- ✗ Bitcoin L1 compromise (out of scope)

---

## 6. Comparison with Other L2 Approaches

| Feature | AILEE | Rollup | Lightning | RSK Sidechain |
|---------|-------|--------|-----------|---------------|
| **State Commitment** | State root | State root | N/A | State root |
| **Bitcoin Verification** | Anchor only | Fraud/validity proofs | Channel state | Merge mining |
| **Trust Model** | Federated | Trustless | Peer-to-peer | Federated |
| **Exit Security** | Multisig | L1-enforced | Timelocked | Multisig |
| **Data Availability** | Off-chain | On-chain | Off-chain | On-chain |
| **Deterministic Replay** | ✓ Yes | ✓ Yes | N/A | ✓ Yes |

**AILEE's Position**: Federated sidechain with explicit verification procedures and honest security trade-offs.

---

## 7. Conclusion

### 7.1 Is AILEE a "Real" L2 Protocol?

**Yes**, because it provides:
1. ✓ **Explicit anchoring specification** (state root composition)
2. ✓ **Independent verification** (deterministic replay)
3. ✓ **Defined failure handling** (crashes, reorgs, conflicts)
4. ✓ **Public specification** (this document)
5. ✓ **Working implementation** (code matches spec)

### 7.2 Is AILEE a Trustless L2?

**No**, it uses a **federated security model**. This is:
- Explicitly documented
- Necessary given Bitcoin's limitations
- Honest about trade-offs
- Appropriate for the use case

### 7.3 Key Takeaway

**AILEE is a well-specified federated Layer-2 protocol, not an "anchored app."**

The difference lies in:
- Deterministic, verifiable state commitments
- Complete public specification
- Explicit failure handling procedures
- Independent verification tooling
- Honest security model documentation

This specification provides the clarity needed to evaluate AILEE as a serious L2 protocol rather than vaporware.

---

## References

- [L2State.h](../include/L2State.h) - State structures
- [L2State.cpp](../src/l2/L2State.cpp) - State computation implementation
- [ailee_l2_verify.cpp](../src/tools/ailee_l2_verify.cpp) - Verification tool
- [VERIFICATION.md](./VERIFICATION.md) - Verification model
- [L1_TO_L2_Infrastructure.md](./L1_TO_L2_Infrastructure.md) - Bridge architecture

---

*This specification is versioned and maintained as the authoritative source for AILEE L2 protocol behavior.*

**Version**: 1.0.0  
**Last Updated**: 2025-02-15  
**Status**: Active
