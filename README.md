# 🌐 AILEE-Core  
## Bitcoin Layer-2 Orchestration, Verification, and Recovery Framework

### *(Research-Driven • Buildable • Explicit Trust Model)*

> **"We build not to replace Bitcoin — but to evolve responsibly around it."**

---

## ⚠️ Project Status (Read First)

**AILEE-Core is an actively developed, research-driven Bitcoin Layer-2 implementation.**

This repository contains **real, buildable infrastructure** combined with ongoing research components.  
Some subsystems are production-quality; others are intentionally exploratory.

AILEE-Core **does not modify Bitcoin consensus**, does **not claim trustless exits**, and does **not present itself as a rollup-style L2**.

All trust assumptions are explicit.

---

## What AILEE-Core Is

**AILEE-Core is a Bitcoin-anchored Layer-2 orchestration and verification framework** designed to operate *alongside* Bitcoin without altering its consensus rules.

It provides:

- Off-chain execution and coordination
- A canonical Layer-2 state boundary
- Deterministic Bitcoin anchoring primitives
- Explicit peg-in / peg-out lifecycle modeling
- Recovery-first protocol design
- Federated security (not trustless rollup security)

Bitcoin remains the immutable settlement layer.  
AILEE-Core lives *around* it — never above it.

---

## What "Layer-2" Means in This Project

AILEE-Core follows a **Bitcoin-native Layer-2 model**, similar in class to federated sidechains rather than rollups.

### Canonical L2 State Boundary

Layer-2 state includes:

- Ledger balances and transaction state
- Orchestration and task execution state
- Telemetry commitments and verification artifacts
- Recovery and governance metadata

### Canonical L1 Anchoring Boundary

Bitcoin is used for:

- Verifiable anchoring of Layer-2 state via deterministic commitments
- SPV-based verification of peg-in events
- Recovery reference and dispute context

Anchor commitments are **constructed deterministically** and exposed as payloads.  
They are **not automatically broadcast on-chain** and introduce no new consensus rules.

This boundary is explicitly documented in:

- `docs/L1_TO_L2_Infrastructure.md`
- `docs/VERIFICATION.md`

---

## Security Model (Explicit and Honest)

AILEE-Core uses a **federated security model**.

### Implemented Today

**Peg-ins:**
- Verified using SPV proofs (Bitcoin headers + Merkle proofs)
- Trust-minimized verification of L1 events off-chain

**Peg-outs:**
- Executed via federated multisig quorum
- Explicit signer thresholds and fee logic
- Sidechain-style custody model (not trustless exits)

**Anchoring:**
- Deterministic hash commitments of L2 state + metadata
- Verifiable but not L1-enforced

### Not Implemented (By Design, Today)

- No fraud-proof system enforced by Bitcoin
- No validity-proof system verifiable on Bitcoin
- No L1-enforced data availability
- No trustless exit mechanism

Because of this, AILEE-Core **does not claim rollup-style security inheritance**.

This is intentional and explicit.

---

## 🚧 Current State of the Repository

This repository should be viewed as:

- A **real Layer-2 system with explicit trust assumptions**
- A **research platform for orchestration, recovery, and verification**
- A **buildable, auditable codebase under active iteration**

### Implemented Components

- Bitcoin adapter with RPC and ZMQ support
- SPV verification for peg-ins
- Federated peg-out lifecycle modeling
- Deterministic anchor commitment construction
- Orchestration and task scheduling engine
- Federated learning–ready execution paths
- Recovery protocols and claim handling
- Energy and telemetry instrumentation
- Multi-chain adapter abstractions (Bitcoin-first; others exploratory)
- Policy-driven configuration system (YAML / JSON / TOML)

### Still Evolving

- Distributed consensus for L2 state
- Persistent replicated storage
- End-to-end automated test coverage
- Adversarial and fault-injection testing
- External cryptographic and protocol audits
- Operational tooling and deployment hardening

---

## ❌ What This Project Is Not

AILEE-Core is **not**:

- A replacement for Bitcoin
- A consensus-changing protocol
- A rollup or trustless L2
- A finished consumer product
- A marketing demo with unverified claims

If you are looking for:

- Guaranteed throughput numbers
- Immediate mainnet deployment
- L1-enforced exits

👉 This repository is **not claiming those properties**.

---

## 🧠 What Is AILEE?

**AILEE (AI-Load Energy Efficiency)** is a framework for applying adaptive intelligence to distributed systems under strict safety constraints.

Within AILEE-Core, AI is limited to:

- Orchestration and scheduling
- Resource-aware optimization
- Conservative, reversible decision support

**AI never controls settlement.  
AI never overrides verification.  
AI always yields to deterministic safety rules.**

---

## 🏗️ High-Level Architecture (Implemented Boundaries)

```
┌─────────────────────────────────────────────────────────────┐
│                   AILEE LAYER-2 (Off-Chain)                 │
│─────────────────────────────────────────────────────────────│
│  Orchestration · Ledger · Verification · Recovery          │
│  Telemetry · Policies · Federated Learning Primitives      │
│─────────────────────────────────────────────────────────────│
│ Deterministic Bitcoin Anchor Commitments (No Auto-Broadcast)│
└─────────────────────────────────────────────────────────────┘
                ↓ Immutable Reference ↓
              BITCOIN MAINNET (Unmodified)
```

---

## 🔬 Active Research Areas

- Layer-2 orchestration and scheduling
- Deterministic verification and anchoring
- Recovery-first protocol design
- Federated security models
- Energy and sustainability telemetry
- Circuit breakers and fail-safe mechanisms
- Safe multi-chain interoperability boundaries

---

## 📊 Performance & Metrics

Any throughput, latency, or energy figures referenced in this repository represent:

- Simulations
- Benchmarks
- Research targets

They are **not claimed production guarantees**.

---

## 🤝 Contributing

This project welcomes:

- Cryptographic review
- Protocol skepticism
- Systems engineering critique
- Adversarial thinking
- Documentation improvements

Identifying flaws, limits, or incorrect assumptions is considered a contribution.

---

## ⚙️ Configuration & Policy System

The policy system supports:

- **YAML** (via `yaml-cpp`)
- **JSON** (via `nlohmann/json`)
- **TOML** (via `toml++`)

All formats feed a shared validation pipeline for:
`signals`, `metrics`, `policies`, `pipelines`, and `outputs`.

---

## 📄 License

MIT License — free to use, modify, and study.

---

## Closing Note

Bitcoin's strength comes from restraint.

AILEE-Core exists to explore how far **explicit, verifiable, and recovery-aware** systems can extend Bitcoin's usefulness *without weakening its foundations*.

This repository is shared openly, early, and honestly — because correctness matters more than speed.

If you are here to learn, challenge assumptions, or help shape responsible Layer-2 infrastructure — welcome.
