# Verification & Falsifiability

This document defines the authoritative verification boundaries, measurable claims,
and falsifiability conditions for the AILEE-Core-Bitcoin framework.
Architectural concepts and exploratory designs are documented separately.


The purpose of this document is to clearly separate:
- What is claimed
- What is measurable
- What is simulated
- What is explicitly **out of scope**

---

## 1. Scope of Claims

AILEE-CORE-BITCOIN makes the following claims:

- Transaction throughput can be **modeled and optimized at the systems layer** without modifying Bitcoin consensus rules.
- Latency and throughput constraints emerge from **network topology, mempool dynamics, block propagation, and load distribution**, not from block size alone.
- Energy efficiency can be evaluated using an AI-assisted **Load–Energy Efficiency metric (η)** applied to transaction routing, batching, and temporal load shaping.
- Optimization gains are **situational and bounded**, not universal.

AILEE-CORE-BITCOIN does **not** claim:
- Infinite or unbounded TPS
- Guaranteed throughput improvements under all conditions
- Replacement of proof-of-work
- Changes to block size, block interval, or consensus rules
- Centralized transaction control

---

## 2. Bitcoin Invariants (Non-Negotiable)

All analysis and modeling in this repository explicitly respects the following Bitcoin invariants:

- Proof-of-Work consensus remains unchanged
- Block size and block interval constraints are respected
- Full node validation rules are not modified
- Decentralization assumptions are preserved
- Adversarial network conditions are assumed possible

Any optimization that violates these constraints is **out of scope by definition**.

---

## 3. Layer-2 Boundary & Anchoring

This repository treats AILEE-Core as a Bitcoin-adjacent Layer-2 with **off-chain execution**
and **deterministic commitments**. The canonical boundaries are:

### 3.1 Canonical L2 State
The canonical L2 state is summarized as a deterministic **L2StateRoot** computed from:
- **Ledger balances:** sorted account balances and escrows from the L2 ledger.
- **Pending bridge state:** peg-in and peg-out records (including anchor references).
- **Orchestration task state:** queued tasks with stable identifiers and payload hashes.

The L2StateRoot is computed by canonicalizing the above data in a stable order and
hashing the result. This ensures reproducibility across nodes given identical inputs.

### 3.2 L1 Anchoring Boundary
- **Bitcoin adapter commitments:** deterministic anchor hashes produced from
  `(L2StateRoot || timestamp || recovery metadata)` and exposed as verifiable payloads.
  Payloads are encoded as either:
  - an **OP_RETURN** payload (≤ 80 bytes), or
  - a **Taproot commitment script fragment**.
- **Recovery hooks:** recovery claims and dispute artifacts may reference the anchor hash,
  but do not broadcast on-chain transactions from within the L2 runtime.

Anchors are purely commitment artifacts: they bind L2 state to Bitcoin-friendly script
payloads without asserting on-chain finality or fraud/validity proofs.

### 3.3 Anchor-Bound Exit Authorization
Peg-out requests are required to reference an **AnchorCommitmentHash**. The bridge
enforces that the referenced anchor matches the L2StateRoot that authorizes the exit.
Federation signers still approve peg-outs, but the anchor check is enforced prior to
signing and completion.

These boundaries define what is considered verifiable L2 state and what is only
anchored to Bitcoin via deterministic commitments.

---

## 4. Measurable Metrics

AILEE-CORE-BITCOIN evaluates system behavior using measurable and falsifiable metrics:

### 4.1 Throughput Modeling
- Transactions per second (TPS) as a function of:
  - Network latency distribution
  - Mempool congestion
  - Transaction batching strategies
  - Geographic node distribution

### 4.2 Latency Behavior
- End-to-end transaction confirmation latency
- Block propagation delays
- Queueing effects under burst load

### 4.3 Energy Efficiency (AILEE η)
- Energy cost per confirmed transaction
- Load-adjusted efficiency under variable demand
- Comparative analysis across routing and batching strategies

All metrics are expressed as **relative improvements or bounded efficiencies**, not absolute guarantees.

---

## 5. Simulation vs. Real-World Deployment

The framework distinguishes clearly between levels of maturity:

### Modeled
- Mathematical throughput and latency models
- Energy efficiency equations
- Load-distribution strategies

### Simulated
- Network topology variations
- Transaction burst scenarios
- Queueing and propagation effects

### Deployable (Non-Consensus)
- Transaction batching heuristics
- Routing and submission timing strategies
- Load-aware transaction scheduling

### Conceptual
- Large-scale AI-assisted coordination across heterogeneous nodes
- Global optimization strategies requiring broad adoption

Claims never cross these boundaries without explicit labeling.

---

## 6. Failure Modes & Limitations

AILEE-CORE-BITCOIN provides **no benefit** under certain conditions, including but not limited to:

- Low network congestion
- Homogeneous node distribution
- Adversarial latency manipulation
- Strictly local transaction submission with no batching
- Environments where transaction ordering cannot be influenced

Optimization gains degrade gracefully and do not introduce failure amplification.

---

## 7. Falsifiability Conditions

The framework is considered falsified if:

- Modeled efficiency gains cannot be reproduced under stated assumptions
- Latency improvements disappear under controlled simulation
- Energy efficiency metrics fail to outperform baseline strategies
- Claimed improvements require consensus changes

Each claim is intentionally framed to allow independent challenge.

---

## 8. Reproducibility & Independent Evaluation

To reproduce or challenge AILEE-CORE-BITCOIN claims, an evaluator requires:

- Access to transaction timing data
- Network latency measurements
- Energy usage estimates (node-level or modeled)
- Controlled load scenarios

No proprietary data or privileged network access is assumed.

Independent verification is performed locally using persisted L2 state snapshots.
The verification flow is:
1. Load a snapshot from disk.
2. Recompute the L2StateRoot from ledger, bridge, and orchestration state.
3. Verify the anchor commitment hash against the recomputed root.
4. Verify that all peg-outs reference the anchor commitment that matches the root.

These checks are deterministic and require **no Bitcoin RPC access**.
Use `ailee_l2_verify --snapshot <path>` for a local, offline verification pass.

---

## 9. Design Philosophy

AILEE-CORE-BITCOIN prioritizes:
- Constraint-respecting optimization
- Systems-level realism
- Explicit limits over speculative promises
- Scientific falsifiability over marketing claims

This repository is intended as a **reference framework**, not a mandate.

---

## 10. Non-Goals

This project explicitly does not:
- modify Bitcoin consensus rules
- alter transaction validity or ordering guarantees
- introduce centralized coordination or control
- promise universal throughput or energy improvements
- replace protocol-level scaling mechanisms

AILEE-Core-Bitcoin focuses on systems-level modeling,
observation, and optimization within existing constraints.

It does **not** claim trustless exits; peg-out approvals remain federated.

---

## 11. Status

This document reflects the current state of the framework and will evolve only when:
- New measurable claims are introduced
- Verification boundaries materially change

Documentation updates are treated as stability signals, not feature expansion.
