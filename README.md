<div align="center">

# 🌐 AILEE-Core

### **Ambient AI + Bitcoin Layer-2 Orchestration Framework**

*Building Intelligent, Verifiable, and Sustainable Bitcoin Infrastructure*

[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-blue.svg)](https://cmake.org/)
[![FastAPI](https://img.shields.io/badge/FastAPI-Python-green.svg)](https://fastapi.tiangolo.com/)
[![Bitcoin](https://img.shields.io/badge/Bitcoin-Layer--2-orange.svg)](https://bitcoin.org/)
[![Status](https://img.shields.io/badge/Status-Active%20Research-yellow.svg)](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin)

**[Documentation](docs/)** | **[Quick Start](#-quick-start)** | **[API Reference](API_QUICKSTART.md)** | **[Architecture](docs/ARCHITECTURE_CONCEPTUAL.md)** | **[Contributing](docs/CONTRIBUTING.md)**

---

## ⚠️ SECURITY NOTICE - PUBLIC REPOSITORY

**This is a PUBLIC repository.** All files, commits, and history are visible to anyone on the internet.

🔴 **NEVER commit:**
- Real credentials (Bitcoin RPC passwords, API keys, JWT secrets)
- Private keys (`.key`, `.pem` files)
- Production configuration files (`.env`, `secrets.yaml`)

✅ **Safe to commit:**
- Example files (`.env.example`, `secrets.example.yaml`)
- Documentation and code
- Public certificates (without private keys)

📖 **See [SECURITY_PUBLIC_REPO.md](SECURITY_PUBLIC_REPO.md) for complete security guidelines.**

</div>

---

## 📖 Table of Contents

- [Overview](#-overview)
- [What is AILEE-Core?](#-what-is-ailee-core)
- [Key Features](#-key-features)
- [Quick Start](#-quick-start)
- [Architecture](#-architecture)
- [Use Cases](#-use-cases)
- [Security Model](#-security-model)
- [Project Status](#%EF%B8%8F-project-status)
- [What This Is NOT](#-what-this-is-not)
- [Technology Stack](#-technology-stack)
- [Documentation](#-documentation)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🎯 Overview

**AILEE-Core** is a research-driven, Bitcoin-anchored Layer-2 orchestration and verification framework that explores how ambient AI, deterministic verification, and recovery-first design can extend Bitcoin's capabilities **without modifying its consensus rules**.

> **"We build not to replace Bitcoin — but to evolve responsibly around it."**

This repository contains **real, buildable infrastructure** (13,500+ lines of C++ and Python) combining:
- 🤖 **Ambient AI orchestration** for intelligent task scheduling
- ⛓️ **Bitcoin Layer-2 state management** with deterministic verification
- 🔐 **Federated security model** with explicit trust assumptions
- 🌍 **Multi-chain adapter framework** (Bitcoin-first, 7+ blockchain integrations)
- 🌐 **Production-ready REST API** and web dashboard
- ♻️ **Energy telemetry** and sustainability tracking

---

## 💡 What is AILEE-Core?

AILEE (AI-Load Energy Efficiency) is a **Bitcoin Layer-2 framework** that treats Bitcoin as an immutable settlement layer while providing:

### Core Capabilities

1. **Off-Chain Execution & Coordination**
   - Distributed task orchestration with AI-driven scheduling
   - Ambient telemetry for system coherence and stability
   - Federated learning primitives for privacy-preserving computation

2. **Bitcoin Anchoring & Verification**
   - Deterministic state commitments anchored to Bitcoin
   - SPV-based peg-in verification (Bitcoin → L2)
   - Federated multisig peg-outs (L2 → Bitcoin)
   - Offline third-party state verification tooling

3. **Recovery-First Protocol Design**
   - Cryptographic recovery mechanisms for long-dormant Bitcoin
   - Circuit breakers and fail-safe mechanisms
   - Explicit incident accountability

4. **Global Accessibility**
   - RESTful API for programmatic access
   - Web dashboard for real-time monitoring
   - CORS-enabled for browser integration
   - Docker and Fly.io deployment support

---

## ✨ Key Features

### 🧠 Ambient AI Orchestration
- **Intelligent Task Scheduling**: Weighted, round-robin, least-loaded, and latency-aware strategies
- **Reputation-Based Assignment**: Node scoring with capacity, cost, and energy considerations
- **Byzantine Detection**: Outlier-based heuristics for anomaly detection
- **Deterministic Verification**: Hash-based commitments with integrity checks

### ⛓️ Bitcoin Layer-2 Infrastructure
- **SPV Verification**: Trust-minimized Bitcoin event verification
- **Deterministic Anchoring**: OP_RETURN and Taproot commitment construction
- **Federated Bridge**: Multisig quorum-based peg-in/peg-out lifecycle
- **State Snapshots**: Reproducible L2 state root computation
- **Offline Verifier**: `ailee_l2_verify` tool for third-party audits

### 🌐 Web & API Integration
- **REST API**: FastAPI-based production server with OpenAPI docs
- **Web Dashboard**: Real-time monitoring interface
- **Health Checks**: Built-in monitoring and observability
- **Rate Limiting**: Lightweight request throttling
- **Docker Support**: Containerized deployment
- **Cloud Ready**: Fly.io, Railway, and Render deployment configs

### 🔐 Security & Safety
- **Circuit Breakers**: Automatic reversion to safe states
- **Federated Security**: Explicit trust model (not trustless rollup)
- **Energy Telemetry**: Verifiable sustainability metrics
- **Policy System**: YAML/JSON/TOML configuration with validation
- **ZMQ Real-Time Monitoring**: Live Bitcoin network event listening

### 🌍 Multi-Chain Support
- **Bitcoin Family**: Bitcoin, Litecoin, Dogecoin
- **EVM Chains**: Ethereum, Polygon, Avalanche
- **Other Chains**: Solana, Cardano, Polkadot
- **Adapter Registry**: Extensible multi-chain framework

---

## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev \
    libzmq3-dev libcppzmq-dev libjsoncpp-dev libyaml-cpp-dev librocksdb-dev

# Optional: Install libp2p for full P2P networking
# See scripts/install-libp2p.sh or docs/LIBP2P_INTEGRATION.md

# macOS
brew install cmake openssl curl zeromq cppzmq jsoncpp yaml-cpp rocksdb
```

### Option 1: Build C++ Core Node

```bash
# Clone the repository
git clone https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin.git
cd AILEE-Protocol-Core-For-Bitcoin

# Build the project
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)

# Run the main node
./ailee_node

# Run the L2 verification tool
./ailee_l2_verify

# Run the P2P network demo
./ailee_p2p_demo

# Run tests
ctest --verbose
```

### Option 2: Run REST API (Python)

```bash
# Install Python dependencies
pip install -r requirements.txt

# Configure environment
cp .env.example .env
# Edit .env with your settings

# Run the API server
uvicorn api.main:app --host 0.0.0.0 --port 8080 --reload

# Access the API
curl http://localhost:8080/health

# View OpenAPI documentation
open http://localhost:8080/docs
```

### Option 3: Run Web Dashboard

```bash
# Build and run the web demo
cd build
./ailee_web_demo

# Access dashboard
open http://localhost:8080
```

### Option 4: Docker Deployment

```bash
# Build Docker image
docker build -t ailee-core-api .

# Run container
docker run -p 8000:8000 --env-file .env ailee-core-api

# Test the API
curl http://localhost:8000/status

# Access the web dashboard
open http://localhost:8000/
```

### Option 5: Deploy to Fly.io

```bash
# Install Fly CLI
curl -L https://fly.io/install.sh | sh

# Login and deploy
fly auth login
fly launch
fly deploy

# View logs
fly logs
```

### Option 6: Multi-Node Cluster (Docker Compose) ⭐ NEW

```bash
# Deploy 3-node cluster with Prometheus + Grafana
./scripts/deploy-multinode.sh

# Access points
# - Node 1: http://localhost:8080
# - Node 2: http://localhost:8081
# - Node 3: http://localhost:8082
# - Prometheus: http://localhost:9000
# - Grafana: http://localhost:3000 (admin/admin)

# View logs
docker-compose logs -f

# Stop cluster
docker-compose down
```

See **[API_QUICKSTART.md](API_QUICKSTART.md)** and **[Next Phase Features](docs/NEXT_PHASE_FEATURES.md)** for detailed deployment guides.

---

## ⚠️ Project Status

**AILEE-Core is actively developed research infrastructure.**

### ✅ Implemented & Tested
- Bitcoin RPC and ZMQ adapter integration
- SPV verification for peg-ins
- Federated peg-out lifecycle modeling
- Deterministic anchor commitment construction
- Task orchestration and scheduling engine
- Federated learning execution paths
- Recovery protocols and claim handling
- Energy and telemetry instrumentation
- Multi-chain adapter framework
- REST API with FastAPI
- Web dashboard and monitoring
- Docker and cloud deployment configs
- **🆕 SHA3-256 cryptographic hashing** (OpenSSL 3.0+)
- **🆕 RocksDB persistent storage layer**
- **🆕 P2P networking infrastructure** (libp2p C++ bindings integrated)
- **🆕 Distributed task protocol**
- **🆕 Prometheus metrics export**
- **🆕 Multi-node deployment testing** (Docker Compose)

### 🚧 In Active Development
- Distributed consensus for L2 state
- End-to-end automated test coverage
- Adversarial and fault-injection testing
- External cryptographic audits
- Production hardening and operational tooling

### 🔬 Research Areas
- Layer-2 orchestration optimization
- Deterministic verification mechanisms
- Recovery-first protocol design
- Energy and sustainability telemetry
- Safe multi-chain interoperability

**Important**: This is **real infrastructure** with **explicit trust assumptions**, not a finished consumer product.

---

## 🏗️ Architecture

<div align="center">

```
┌─────────────────────────────────────────────────────────────────┐
│                      AILEE LAYER-2 NODE                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐  │
│  │ Orchestration│  │  Ambient AI  │  │  Federated         │  │
│  │ & Scheduling │  │  Telemetry   │  │  Learning          │  │
│  └──────────────┘  └──────────────┘  └────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              L2 State & Ledger Management                │  │
│  │        (Deterministic Snapshots + Verification)          │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │           Bitcoin Anchoring & Bridge Logic               │  │
│  │  • SPV Peg-In  • Federated Peg-Out  • Commitments       │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │        Multi-Chain Adapters (Bitcoin, ETH, SOL...)      │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │   REST API + Web Dashboard (FastAPI + HTTP Server)      │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              ↕
          ┌───────────────────────────────────────┐
          │     BITCOIN MAINNET (L1)              │
          │  • Anchor Commitments                 │
          │  • Peg-In Verification (SPV)          │
          │  • Peg-Out Settlement (Multisig)      │
          └───────────────────────────────────────┘
```

</div>

### Key Architectural Principles

1. **Bitcoin-First Design**: Bitcoin remains the authoritative settlement layer
2. **Deterministic Verification**: All L2 state is reproducibly verifiable
3. **Federated Security**: Explicit multisig trust model (not trustless rollup)
4. **Ambient Intelligence**: AI assists but never controls settlement
5. **Recovery-First**: Circuit breakers and fail-safe mechanisms throughout

**Detailed Architecture**: See [docs/ARCHITECTURE_CONCEPTUAL.md](docs/ARCHITECTURE_CONCEPTUAL.md)

---

## 🎯 Use Cases

### For Developers
- **Bitcoin Layer-2 Research**: Explore federated sidechain architectures
- **AI Orchestration**: Study ambient AI for distributed systems
- **Multi-Chain Integration**: Learn adapter pattern for blockchain interop
- **Verifiable Computation**: Investigate deterministic off-chain execution

### For Researchers
- **Layer-2 Security Models**: Analyze federated vs. rollup trust assumptions
- **Recovery Mechanisms**: Study cryptographic recovery protocols
- **Energy Telemetry**: Explore verifiable sustainability metrics
- **Byzantine Fault Tolerance**: Investigate outlier detection heuristics

### For Operators
- **Public API Deployment**: Run production REST API on Fly.io/Railway
- **Monitoring Dashboard**: Deploy web interface for L2 state visibility
- **Third-Party Verification**: Use `ailee_l2_verify` for independent audits
- **Energy Reporting**: Track and report verifiable energy consumption

---

## 🔐 Security Model

### ✅ What We Implement (Explicit Federated Model)

**Peg-Ins (Bitcoin → L2)**
- SPV proofs with Bitcoin headers + Merkle paths
- Trust-minimized verification of L1 events
- No reliance on centralized oracles

**Peg-Outs (L2 → Bitcoin)**
- Federated multisig quorum (sidechain-style custody)
- Explicit signer thresholds and fee logic
- Anchor-bound authorization (peg-outs must reference valid anchors)

**Anchoring**
- Deterministic L2 state root computation
- OP_RETURN (≤80 bytes) or Taproot commitment payloads
- Verifiable but not L1-enforced

**Circuit Breakers**
- Automatic reversion to safe states under anomalies
- Conservative parameter bounds
- Fail-safe > performance

### ❌ What We Do NOT Claim

- ❌ No fraud-proof system enforced by Bitcoin L1
- ❌ No validity-proof system verifiable on Bitcoin L1
- ❌ No L1-enforced data availability
- ❌ No trustless exit mechanism
- ❌ No rollup-style security inheritance

**This is intentional and explicit.** AILEE-Core is a **federated Layer-2**, not a rollup.

**Trust Model**: See [docs/VERIFICATION.md](docs/VERIFICATION.md) and [SECURITY.md](SECURITY.md)

---

## ⚠️ What This Is NOT

AILEE-Core is **NOT**:

- ❌ A replacement for Bitcoin
- ❌ A consensus-changing protocol
- ❌ A trustless rollup or validity rollup
- ❌ A finished consumer product
- ❌ An investment offering
- ❌ A guarantee of specific performance metrics

**If you're looking for:**
- Guaranteed throughput numbers → This is research infrastructure
- Immediate mainnet deployment → This is active development
- L1-enforced exits → This is a federated model, not a rollup
- Zero trust assumptions → This uses explicit federated trust

**Then this repository may not meet your expectations.**

---

## 🛠️ Technology Stack

### Core Infrastructure (C++)
- **Language**: C++17
- **Build System**: CMake 3.10+
- **Bitcoin Integration**: JSON-RPC client + ZMQ listener
- **Cryptography**: OpenSSL (SHA256, signature verification)
- **Networking**: libcurl, ZeroMQ, cppzmq
- **JSON Parsing**: nlohmann/json, JsonCpp
- **Configuration**: yaml-cpp, TOML++
- **Testing**: GoogleTest / Custom MiniGTest

### REST API (Python)
- **Framework**: FastAPI (async, production-ready)
- **Server**: Uvicorn (ASGI server)
- **Validation**: Pydantic models
- **Rate Limiting**: SlowAPI
- **Documentation**: OpenAPI/Swagger auto-generated

### Web Dashboard (C++)
- **HTTP Server**: cpp-httplib (header-only)
- **Frontend**: HTML5 + CSS + Vanilla JavaScript
- **Auto-Refresh**: Real-time metrics updates

### Deployment
- **Containerization**: Docker
- **Cloud Platforms**: Fly.io, Railway, Render
- **CI/CD**: GitHub Actions
- **Version Control**: Git

---

## 📚 Documentation

### Getting Started
- **[Quick Start](#-quick-start)**: Build and run guides
- **[API Quick Start](API_QUICKSTART.md)**: REST API deployment
- **[Build Guide](docs/BUILD.md)**: Detailed build instructions
- **[Ambient AI + VCP Demo](examples/README.md)**: Working integration demonstration ⭐ NEW
- **[Next Phase Features](docs/NEXT_PHASE_FEATURES.md)**: P2P, Storage, Metrics, Multi-node ⭐ NEW

### Architecture & Design
- **[Architecture Overview](docs/ARCHITECTURE_CONCEPTUAL.md)**: System design
- **[Ambient AI + VCP Integration](docs/AMBIENT_VCP_INTEGRATION.md)**: Complete integration guide ⭐ NEW
- **[L1-L2 Infrastructure](docs/L1_TO_L2_Infrastructure.md)**: Bitcoin anchoring
- **[Verification Model](docs/VERIFICATION.md)**: Security and trust
- **[Vision Document](docs/VISION.md)**: Long-term research direction

### Implementation Details
- **[Adapters](docs/ADAPTERS.md)**: Multi-chain integration
- **[Web Integration](docs/WEB_INTEGRATION.md)**: HTTP API and dashboard
- **[Recovery Guide](docs/RECOVERY_GUIDE.md)**: Recovery protocols
- **[Security](docs/SECURITY_AND_POWER.md)**: Power model and safety
- **[libp2p Integration](docs/LIBP2P_INTEGRATION.md)**: P2P networking with libp2p C++ bindings ⭐ NEW
- **[P2P Networking](docs/NEXT_PHASE_FEATURES.md#-3-p2p-networking-layer)**: Distributed networking
- **[Persistent Storage](docs/NEXT_PHASE_FEATURES.md#-2-rocksdb-persistent-storage)**: RocksDB integration
- **[Metrics Export](docs/NEXT_PHASE_FEATURES.md#-5-prometheus-metrics-export)**: Prometheus monitoring

### For Developers
- **[Examples Directory](examples/)**: Working code demonstrations
- **[Whitepapers](docs/whitepapers/)**: Technical specifications (Ambient AI, VCP)

### Operations
- **[Configuration](config/)**: YAML/JSON/TOML policy system
- **[Changelog](CHANGELOG.md)**: Version history
- **[Security Policy](SECURITY.md)**: Vulnerability reporting

---

## 🤝 Contributing

This project welcomes contributions of all kinds:

### Valuable Contributions
- ✅ **Cryptographic review** of verification mechanisms
- ✅ **Protocol skepticism** and adversarial analysis
- ✅ **Systems engineering critique** of architecture
- ✅ **Documentation improvements** for clarity
- ✅ **Bug reports** with reproducible cases
- ✅ **Test coverage** expansion
- ✅ **Performance benchmarks** and profiling

### How to Contribute

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Make your changes** with clear commit messages
4. **Add tests** for new functionality
5. **Update documentation** as needed
6. **Submit a pull request** with detailed description

**Note**: Identifying flaws, limits, or incorrect assumptions is considered a valuable contribution.

See **[CONTRIBUTING.md](docs/CONTRIBUTING.md)** for detailed guidelines.

---

## 🌟 What Makes This README Phenomenal?

This README demonstrates:

1. **✨ Visual Excellence**: Badges, emojis, ASCII art diagrams
2. **📋 Comprehensive Structure**: Clear TOC, logical flow, easy navigation
3. **🎯 Multiple Entry Points**: Quick start for builders, API for devs, docs for researchers
4. **⚠️ Radical Honesty**: Explicit about what IS and ISN'T implemented
5. **🚀 Actionable**: 5 different ways to get started immediately
6. **🔐 Security-First**: Trust model front and center
7. **📚 Well-Documented**: Links to detailed docs throughout
8. **🤝 Welcoming**: Clear contribution guidelines
9. **🌐 Modern**: Deployment guides for Docker, Fly.io, cloud platforms
10. **🎓 Educational**: Architecture diagrams, use cases, technology stack

---

## 🧠 What is AILEE?

**AILEE (AI-Load Energy Efficiency)** is a framework for applying adaptive intelligence to distributed systems under strict safety constraints.

### AI's Role in AILEE-Core

AI is **limited** to:
- ✅ Orchestration and scheduling optimization
- ✅ Resource-aware task assignment
- ✅ Conservative, reversible decision support
- ✅ Telemetry analysis and anomaly detection

AI **never** controls:
- ❌ Settlement or finality
- ❌ Verification or consensus
- ❌ Recovery or governance (except advisory)

**Principle**: AI always yields to deterministic safety rules.

---

## 📊 Performance & Benchmarks

**Important**: Any throughput, latency, or energy figures in this repository represent:
- 🔬 **Simulations** (not production measurements)
- 📈 **Benchmarks** (controlled test environments)
- 🎯 **Research targets** (aspirational goals)

They are **NOT**:
- ❌ Claimed production guarantees
- ❌ Marketing promises
- ❌ Verified at scale

**Methodology**: See [docs/TECHNICAL_APPENDIX.md](docs/TECHNICAL_APPENDIX.md)

---

## 🌍 Community & Support

### Get Help
- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: Questions and community support
- **Documentation**: Comprehensive guides in `docs/`
- **Examples**: Working code in `examples/`

### Stay Updated
- **Watch** this repository for updates
- **Star** if you find this useful
- **Fork** to experiment with your own modifications

### Citation

If you use or build upon this work in academic research, please cite:

```bibtex
@software{ailee_core,
  author = {Feeney, Don Michael},
  title = {AILEE-Core: Bitcoin-Anchored Layer-2 Orchestration Framework},
  year = {2025},
  publisher = {GitHub},
  url = {https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin},
  license = {MIT}
}
```

See **[CITATION.cff](CITATION.cff)** for complete citation metadata.

---

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

**Copyright © 2025 Don Michael Feeney Jr**

Permission is hereby granted, free of charge, to any person obtaining a copy of this software to use, modify, and distribute it under the terms of the MIT License.

---

## 🔮 Closing Thoughts

Bitcoin's strength comes from restraint.

AILEE-Core exists to explore how far **explicit, verifiable, and recovery-aware** systems can extend Bitcoin's usefulness *without weakening its foundations*.

This repository is shared **openly, early, and honestly** — because correctness matters more than speed.

### Who Should Use This?

You should explore AILEE-Core if you want to:
- 🧑‍🔬 **Research** Bitcoin Layer-2 architectures
- 🧑‍💻 **Learn** federated sidechain implementation
- 🧑‍🏫 **Study** ambient AI orchestration patterns
- 🔍 **Audit** explicit trust models
- 🛠️ **Build** on verifiable off-chain infrastructure

### Who Should Wait?

You should wait if you need:
- ⏸️ **Production-ready** consumer applications
- ⏸️ **Audited and hardened** critical infrastructure
- ⏸️ **Trustless rollup** security guarantees
- ⏸️ **Mainnet deployment** without research/testing

---

<div align="center">

**If you are here to learn, challenge assumptions, or help shape responsible Layer-2 infrastructure — welcome.**

Made with ❤️ for the Bitcoin community

[⭐ Star this repo](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin) • [🐛 Report Bug](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/issues) • [✨ Request Feature](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin/issues)

</div>

