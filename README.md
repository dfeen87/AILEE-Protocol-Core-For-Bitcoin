# 🌐 AILEE-Core  
## Ambient AI + Bitcoin Layer-2 Orchestration, Verification, and Recovery Framework

### *(Research-Driven • Buildable • Explicit Trust Model)*

> **"We build not to replace Bitcoin — but to evolve responsibly around it."**

---

## ⚠️ Project Status

**AILEE-Core is an actively developed, research-driven Bitcoin Layer-2 implementation.**

This repository contains **real, buildable infrastructure** combined with ongoing research components.  
Some subsystems are production-quality; others are intentionally exploratory.

AILEE-Core **does not modify Bitcoin consensus**, does **not claim trustless exits**, and does **not present itself as a rollup-style L2**.

All trust assumptions are explicit.

**Cloning AILEE-Core means inheriting the obligation to operate a deterministic, auditable, federated Bitcoin Layer-2 — including custody risk, verification correctness, explicit trust disclosure, and real incident accountability.**

Yes — cloning AILEE-Core gives you a real Bitcoin Layer-2 implementation, with an explicit federated trust model and verifiable anchoring, not a rollup-style trustless L2.

---

## What AILEE-Core Is

**AILEE-Core is a Bitcoin-anchored Layer-2 orchestration and verification framework** designed to operate *alongside* Bitcoin without altering its consensus rules.

It treats **Ambient AI orchestration** as a first-class subsystem: telemetry-aware nodes coordinate distributed AI workloads, produce deterministic verification artifacts, and feed those commitments into the L2 state boundary.

It provides:

- Off-chain execution and coordination
- Ambient AI telemetry, verification, and orchestration primitives
- A canonical Layer-2 state boundary
- Deterministic Bitcoin anchoring primitives
- Explicit peg-in / peg-out lifecycle modeling
- Recovery-first protocol design
- Federated security (not trustless rollup security)

AILEE-Core supports offline third-party verification of Layer-2 state via deterministic snapshots, Bitcoin-valid anchor payload construction, and anchor-bound peg-out authorization under an explicit federated trust model.

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

## 🔍 What You Get When You Clone This Repo (Code-Based Analysis)

### High-Level System Scope

This codebase implements an **AILEE-Core Node** in C++: a single process that integrates multiple subsystems—**task orchestration**, **Ambient telemetry + verification**, **Layer-2 bridge state + anchoring**, **federated learning primitives**, and **Bitcoin Core connectivity**—wired through a central runtime/entrypoint.

### Core Runtime Entrypoint & Configuration

The main node runtime sets up:
- Structured logging + signal handling
- A configurable node profile (TPS simulation knobs, circuit breaker thresholds, NetFlow settings, orchestration settings, metrics)
- Integration points for orchestration, FL, AmbientAI, L2/bridge logic, and Bitcoin adapters

This reflects an intended deployment model where these components run together in one coordinated node process.

### Distributed Task Orchestration

The orchestration layer provides:
- Scheduling strategies (weighted, round-robin, least-loaded, lowest-latency, etc.)
- Task/resource definitions, node metrics, latency/reputation interfaces
- Concrete scoring/assignment logic factoring: reputation, latency, capacity, cost, region preference, and "green energy" bonuses
- Assignment records including expected latency/cost and backup-worker selection

### Ambient Telemetry + Deterministic Verification + FL Update Plumbing

The Ambient subsystem includes:
- Telemetry serialization and integrity checks
- Deterministic proof/commitment wiring for telemetry verification (hash-based commitments rather than real ZK rollup proofs)
- Local federated update generation with proof bytes/hash fields and verification flags
- Node-utility / game-theory style scoring and basic byzantine detection heuristics (outlier-based)

### Federated Learning Primitives

The federated learning layer defines:
- Privacy budgeting (DP accounting), aggregation strategies (FedAvg, FedProx, Krum, etc.), and compression options
- Job/task structures (incentives, deadlines, proof requirements)
- Local update payloads (proof bytes/hash, DP accounting, optional encrypted delta hooks)
- Aggregator interfaces and aggregation result structures

### L2 State Snapshots + Offline Verification Tooling

The repo includes an **audit-friendly snapshot mechanism** so third parties can verify state without trusting a running node:
- Deterministic canonicalization and **reproducible L2StateRoot** computation
- Append-only snapshot persistence + latest snapshot loading
- An offline verifier tool (`ailee_l2_verify`) that recomputes roots and checks anchor/binding rules locally

### Bitcoin-Friendly Anchoring Primitives (Off-Chain Payload Generation)

Anchoring is implemented as **deterministic commitment construction** (no auto-broadcast):
- Anchor payload builders that emit either:
  - **OP_RETURN script payloads (≤80 bytes)**, or
  - A **Taproot commitment script fragment**
- Payloads bind `L2StateRoot` + timestamp + recovery metadata into a Bitcoin-consumable commitment
- Anchor hashes can be threaded through orchestration and recovery structures for traceability

### Bridge / Exit Authorization Model (Federated, Explicit)

Bridge/peg logic exists with explicit lifecycle structures. Exits are **not trustless**:
- Peg-outs are enforced to reference a **registered anchorCommitmentHash**
- Code rejects peg-outs that reference unknown anchors or anchors mismatching the authorizing state root
- The custody model remains **federated multisig quorum** (sidechain-class trust model), not rollup-style L1-enforced exits

### L2 NetFlow Relay (Bandwidth Relay Mesh)

The NetFlow subsystem models:
- Relay nodes, tunnels, and a mesh coordinator for node registration and relay selection
- Bandwidth distribution and accounting, including reward attribution (with a proof-hash field)
- Hybrid tunneling concepts (WireGuard/onion/hybrid) and active tunnel tracking

### Bitcoin Core RPC Integration

The repo contains a production-oriented Bitcoin Core JSON-RPC client:
- Raw transaction broadcast + block count fetching
- libcurl-based networking with retries, timeouts, and basic auth support

### What This Does *Not* Give You (By Code Inspection)

- No rollup-style fraud proofs or validity proofs enforced by Bitcoin L1
- No trustless exits / L1-enforced dispute resolution
- No demonstrated distributed consensus layer for the L2 ledger (beyond deterministic snapshots + verification tooling)

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

## 🌐 World Wide Web Integration

**AILEE-Core now includes full web integration** for global accessibility:

### Web Dashboard & REST API

- **Interactive Dashboard**: Real-time web-based monitoring interface
- **RESTful API**: Complete HTTP API for node status, metrics, and operations
- **CORS Enabled**: Browser-friendly for web application integration
- **Auto-refresh**: Live updates of node performance and state

### Quick Start

```bash
# Build and run the web demo
mkdir build && cd build
cmake .. && make
./ailee_web_demo

# Access the dashboard at http://localhost:8080
```

### API Endpoints

- `GET /api/status` - Node status and health
- `GET /api/metrics` - Performance metrics
- `GET /api/l2/state` - Layer-2 state information
- `GET /api/orchestration/tasks` - Task orchestration
- `POST /api/orchestration/submit` - Submit new tasks
- `GET /api/anchors/latest` - Bitcoin anchor status

**Full Documentation**: See [`docs/WEB_INTEGRATION.md`](docs/WEB_INTEGRATION.md)

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

## 🚀 Deploying AILEE-Core as a Public REST API

AILEE-Core now includes a **production-ready REST API** built with FastAPI, designed for deployment on cloud platforms like Fly.io, Railway, and Render.

### Features

- **Deterministic Endpoints**: All computations are reproducible and safe
- **Read-Only Operations**: No state mutation (except bounded internal tracking)
- **OpenAPI Documentation**: Auto-generated API docs at `/docs`
- **Health Checks**: Built-in monitoring endpoints
- **Rate Limiting**: Lightweight request throttling
- **CORS Support**: Browser-friendly configuration
- **Optional JWT Auth**: Disabled by default for easy testing

### API Endpoints

- `GET /health` - Service health check
- `GET /status` - Node metadata (version, uptime, config)
- `POST /trust/score` - Compute trust score for input
- `POST /trust/validate` - Validate model output with confidence
- `GET /l2/state` - Current Layer-2 state snapshot
- `GET /l2/anchors` - Bitcoin anchor history
- `GET /metrics` - Node performance metrics

### Building Locally

```bash
# Install Python dependencies
pip install -r requirements.txt

# Run the API server
uvicorn api.main:app --host 0.0.0.0 --port 8080 --reload

# Access the API
curl http://localhost:8080/health

# View OpenAPI documentation
open http://localhost:8080/docs
```

### Running with Docker

```bash
# Build the Docker image
docker build -t ailee-core-api .

# Run the container
docker run -p 8080:8080 --env-file .env ailee-core-api

# Test the API
curl http://localhost:8080/status
```

### Deploying to Fly.io

**Prerequisites**: Install the [Fly CLI](https://fly.io/docs/hands-on/install-flyctl/)

```bash
# Login to Fly.io
fly auth login

# Create a new app (first time only)
fly apps create ailee-core-api

# Set environment secrets
fly secrets set AILEE_NODE_ID=your-node-id
fly secrets set AILEE_JWT_SECRET=$(openssl rand -base64 32)

# Deploy the application
fly deploy

# Check deployment status
fly status

# View logs
fly logs

# Scale the application
fly scale count 2  # Run 2 instances
fly scale vm shared-cpu-1x --memory 512  # Increase memory
```

### Configuration

Copy `.env.example` to `.env` and configure:

```bash
cp .env.example .env
```

**Key Environment Variables:**

- `AILEE_NODE_ID` - Unique node identifier
- `AILEE_ENV` - Environment (production/staging/development)
- `AILEE_LOG_LEVEL` - Logging verbosity (info/debug/warning)
- `AILEE_JWT_SECRET` - JWT signing secret (if auth enabled)
- `AILEE_JWT_ENABLED` - Enable JWT authentication (default: false)

### Security Notes

- JWT authentication is **disabled by default** for ease of deployment
- Enable JWT in production by setting `AILEE_JWT_ENABLED=true`
- Generate secure secrets: `python -c "import secrets; print(secrets.token_urlsafe(32))"`
- Rate limiting is enabled by default (100 requests per 60 seconds)
- All endpoints are read-only or bounded-write
- No blockchain node connection required

### Production Checklist

- [ ] Set unique `AILEE_NODE_ID`
- [ ] Enable and configure JWT auth for production
- [ ] Review rate limiting settings
- [ ] Configure CORS origins for your domain
- [ ] Set up monitoring and alerting
- [ ] Review logs regularly
- [ ] Test health check endpoint
- [ ] Verify OpenAPI documentation

---

## 📄 License

MIT License — free to use, modify, and study.

---

## Closing Note

Bitcoin's strength comes from restraint.

AILEE-Core exists to explore how far **explicit, verifiable, and recovery-aware** systems can extend Bitcoin's usefulness *without weakening its foundations*.

This repository is shared openly, early, and honestly — because correctness matters more than speed.

If you are here to learn, challenge assumptions, or help shape responsible Layer-2 infrastructure — welcome.
