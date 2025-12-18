# TECHNICAL_APPENDIX.md
## AILEE-Core Technical Appendix
**Scope:** Architectural definitions, contracts, invariants, and implementation notes  
**Applies to:** AILEE-Core-Bitcoin v1.2.x  
**Status:** Non-normative (documentation); describes current design and intended constraints

---

## 1. Purpose

This appendix provides technical grounding for AILEE-Core’s architecture. It exists to:

- formalize key design contracts (what must remain true)
- document trust boundaries between modules
- define adapter semantics (authoritative vs observational)
- clarify telemetry and efficiency metrics (AILEE η)
- make assumptions explicit (including bounded defaults)

This appendix does **not** introduce protocol changes, consensus rules, or execution semantics. It documents how the repository is structured and how the system is intended to be evaluated.

---

## 2. Repository Architecture at a Glance

AILEE-Core is organized around stable boundaries:

- `include/` — public interfaces and stable headers
- `src/` — implementations
- `config/` — configuration loaders/types/hot-reload support
- `docs/` — architecture, verification, security, recovery, sustainability
- `docs/whitepapers/` — long-form research artifacts
- `examples/` — demonstration binaries (non-production entrypoints)
- `tests/` — test harnesses validating boundaries and contracts

**Design intent:** reviewers should be able to audit behavior and boundaries by reading docs and interfaces before inspecting implementation details.

---

## 3. Trust Boundaries and Authority Layers

AILEE-Core treats “authority” as a first-class engineering concept.

### 3.1 Authority Layers

1. **Deterministic execution & core logic**
   - Ledger operations, invariant enforcement, deterministic transformations
   - Intended to remain stable and auditable

2. **Chain interaction (L1 adapters)**
   - Authoritative chain adapters may query state and optionally broadcast (if not read-only)
   - Responsible for translating chain-native concepts into normalized representations

3. **Orchestration (coordination, scheduling, mesh optimization)**
   - Coordinates work across adapters and runtime modules
   - Must not violate adapter contracts or falsifiability bounds

4. **Observability (telemetry, metrics, AILEE observational adapters)**
   - Read-only: measures, normalizes, reports
   - Must not alter transaction validity, consensus behavior, or routing

5. **Security & ZK verification**
   - Verifies proofs and enforces safety controls (e.g., circuit breakers)
   - Must remain explicit and conservative

### 3.2 Critical Rule

> Observation may inform decisions; observation must not silently become control.

This rule is enforced by interface design and module separation.

---

## 4. Core Components and Their Roles

This section maps major headers/implementations to responsibilities.

### 4.1 Ledger Layer
- `include/Ledger.h`
- `src/core/Ledger.cpp`

**Role:** canonical representation of internal events/state transitions.  
**Invariants:** deterministic, auditable, idempotent where applicable.

### 4.2 Orchestration Layer
- `include/Orchestrator.h`
- `src/orchestration/Orchestrator.cpp`
- `src/orchestration/mesh_optimizer.cpp`

**Role:** coordinates execution, scheduling, and optional optimization decisions.  
**Constraint:** orchestration cannot override or mutate chain truth; it consumes normalized state.

### 4.3 Runtime / Execution Layer
- `src/runtime/WasmEngine.h`
- `src/runtime/interpreter.cpp`

**Role:** controlled execution of bounded logic, where applicable.  
**Constraint:** runtime must preserve deterministic behavior within declared limits.

### 4.4 Telemetry and Metrics
- `include/metrics.h`
- `src/telemetry/ailee_energy_telemetry.h`
- `src/telemetry/metrics.cpp`

**Role:** observational reporting and instrumentation.  
**Constraint:** telemetry must not be required for correctness; the system must remain correct without telemetry.

### 4.5 Security, Policies, and Proofs
- `include/policies.h`
- `src/security/ZKVerifier.h`
- `src/security/ailee_circuit_breaker.h`
- `src/security/policies.cpp`
- `src/security/zk_proofs.cpp`
- `include/zk_proofs.h`

**Role:** explicit trust controls, proof verification, and policy enforcement gates.  
**Constraint:** security enforcement must be explicit, testable, and conservative.

---

## 5. Adapter Model

AILEE-Core uses two distinct adapter classes:

### 5.1 Chain Adapters (Authoritative)
Examples:
- `src/l1/BitcoinAdapter.cpp`
- `src/l1/EthereumAdapter.cpp`
- `src/l1/LitecoinAdapter.cpp`
- `src/l1/DogecoinAdapter.cpp`
- etc.

**Definition:** chain adapters may:
- query chain state
- normalize transactions and headers
- optionally broadcast transactions (if permitted by config; not in read-only mode)

**Contract:**
- must not claim to alter consensus
- must handle reorgs and idempotency where applicable
- must be robust to network errors and retries

### 5.2 AILEE Observational Adapters (Read-Only)
These adapters exist to normalize external conditions into load and efficiency signals.
They:
- do not broadcast
- do not reorder transactions
- do not modify consensus behavior
- can be attached or omitted without changing correctness

**Attachment model:** observational adapters are attached to an authoritative chain adapter (e.g., BitcoinAdapter) rather than registered as independent “chains.”

---

## 6. Adapter Registry and Bootstrap

- `src/l1/AdapterRegistry.cpp`
- `tests/AdapterRegistryTests.cpp`

**Role:** thread-safe registration and retrieval of chain adapters by `Chain` enum.  
**Key property:** registry remains chain-centric; observational adapters attach to chain adapters.

**Bootstrap principle:** register only what has concrete implementation; stubs may exist but must remain clearly labeled.

---

## 7. Normalized Data Contracts

AILEE-Core normalizes disparate chain concepts into stable internal structures.

Common examples (names may vary by header):
- `NormalizedTx`
- `TxIn`, `TxOut`
- `BlockHeader`
- `UnitSpec`
- `AdapterConfig`, callbacks (Tx/Block/Energy/Error)

**Contract goals:**
- stable identifiers (`chainTxId`, `normalizedId`)
- consistent unit semantics (e.g., sats, decimals)
- deterministic conversions where possible

**Non-goal:** perfect semantic equivalence across chains. Normalization is a practical interface, not a universal ontology.

---

## 8. Telemetry: Energy, Efficiency, and AILEE η

### 8.1 Telemetry is Not Control
Energy telemetry is emitted periodically and consumed by monitoring and evaluation tooling.

Telemetry is designed to be:
- bounded
- optional
- non-blocking

### 8.2 AILEE η (Efficiency)
AILEE η is used as a normalized efficiency indicator. The precise mathematical form may evolve, but must always satisfy:

- **bounded:** `0 < η ≤ 1` (or clearly documented equivalent)
- **monotonic with respect to load:** as load increases, η should not increase without justification
- **non-magical:** derived from declared inputs (load, measured power, latency, etc.)

### 8.3 Default Load Estimate
Some environments may lack sufficient signals to compute load. In those cases, a bounded default may be used.

Example constant (documented in code):
- `kDefaultLoadEstimate = 0.5`

**Meaning:** placeholder load estimate for telemetry enrichment only.  
**Constraint:** defaults must not be used to claim performance; they exist to keep telemetry pipelines functional.

**Future derivations (examples):**
- mempool depth and arrival rate
- measured queue lengths
- observed propagation skew
- empirical workload rates from replay logs or simulation engine outputs

---

## 9. Verification, Falsifiability, and Claims Hygiene

This repository’s verification posture is documented in:
- `docs/VERIFICATION.md` (or `docs/verification.md` depending on naming)

The technical intent is:

- separate demonstrable claims from aspirational research
- define conditions under which claims fail
- avoid protocol-level promises without implementation and evidence

**Hard constraint:** documentation must not imply consensus modification unless explicitly implemented and validated.

---

## 10. L1↔L2 Infrastructure Notes

AILEE-Core includes L2-oriented components such as:
- `src/l2/ailee_sidechain_bridge.h`
- `src/l2/ailee_gold_bridge.h`
- `src/l2/ailee_tps_engine.h`

**Interpretation:**
- These modules represent infrastructure scaffolding and integration surfaces.
- They are not a statement of consensus modification.
- They must be interpreted within the repository’s declared non-goals and verification boundaries.

---

## 11. Configuration System (Config Hygiene)

Config layer components include:
- `config/config.yaml`
- `src/config_loader.*`
- `config/config_types.h`
- `config/config_hot_reload.*`
- `src/core/Config.h`

**Rules:**
- configuration must not silently change security posture
- defaults must be explicit and documented
- experimental toggles must be labeled and constrained
- read-only modes must be enforced at adapter boundaries (e.g., broadcast blocked)

---

## 12. Operational Safety and Recovery

Operational guidance lives in:
- `docs/RECOVERY_GUIDE.md`
- `docs/SECURITY_AND_POWER.md`
- `docs/SAFETY_AND_SUSTAINABILITY.md`

**Engineering intent:**
- define how systems behave under partial failure
- document idempotent operations where possible
- ensure recovery protocols are explicit, conservative, and auditable

---

## 13. Examples and Demos

Examples include:
- `examples/AILEE_IntegrationDemo.cpp`
- `examples/OrchestratorDemo.cpp`
- governance demo sources

**Non-goal:** examples are not normative.  
**Rule:** examples must never be treated as production entrypoints.

---

## 14. Testing Philosophy

- `tests/AdapterRegistryTests.cpp`

Testing focuses on:
- boundaries and contracts
- determinism and thread-safety where applicable
- adapter registration/retrieval correctness
- regression protection for core trust surfaces

**Future test targets (recommended):**
- telemetry emission boundedness
- read-only enforcement in adapters
- reorg-handling invariants (where implemented)

---

## 15. Glossary

- **Authoritative adapter:** may interact with a chain and optionally broadcast (if permitted).
- **Observational adapter:** read-only adapter producing signals/telemetry; never broadcasts.
- **η (eta):** normalized efficiency indicator; bounded and monotonic with respect to load.
- **Trust boundary:** a module separation where assumptions change and must be explicit.
- **Falsifiability:** defined conditions under which a claim is considered invalid.

---

## 16. Appendix: “Undeniable” Criteria

AILEE-Core is considered “undeniable” when:

- claims are bounded and falsifiable
- non-goals are explicit
- authority boundaries are explicit and preserved in code
- observational logic does not mutate execution
- core correctness does not depend on telemetry
- documentation matches implementation reality

This appendix exists to keep those conditions stable over time.
