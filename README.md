# AILEE Protocol Core for Bitcoin: **Ambient AI + Bitcoin Layer-2 Orchestration Framework**

*Building Intelligent, Verifiable, and Sustainable Bitcoin Infrastructure*

[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.10+-blue.svg)](https://cmake.org/)
[![FastAPI](https://img.shields.io/badge/FastAPI-Python-green.svg)](https://fastapi.tiangolo.com/)
[![Bitcoin](https://img.shields.io/badge/Bitcoin-Layer--2-orange.svg)](https://bitcoin.org/)
[![Status](https://img.shields.io/badge/Status-Production--Ready%20Framework-green.svg)](https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin)

**[Documentation](docs/)** | **[Quick Start](#-quick-start)** | **[API Reference](API_QUICKSTART.md)** | **[Architecture](docs/ARCHITECTURE_CONCEPTUAL.md)** | **[Contributing](docs/CONTRIBUTING.md)**

---

## 📖 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
- [Testing & Validation](#-testing--validation)
- [Quick Start](#-quick-start)
- [Architecture](#-architecture)
- [Security Model](#-security-model)
- [Technology Stack](#-technology-stack)
- [Documentation](#-documentation)
- [Contributing](#-contributing)
- [License](#-license)

---

## 🎯 Overview

**AILEE-Core** is a **secure and powerful** Bitcoin Layer-2 orchestration and verification framework. It leverages ambient AI for intelligent task scheduling and a robust, recovery-first design to extend Bitcoin's capabilities without modifying its consensus rules.

We are confident in this technology's ability to provide a scalable, energy-efficient, and verifiable Layer 2 solution. By anchoring state deterministically to Bitcoin and employing advanced reorg protection, AILEE ensures that your L2 operations remain secure even in the volatile environment of a proof-of-work blockchain.

> **Note**: This repository provides a **Production-Ready Architecture**. While the core logic for orchestration, reorg protection, and state management is complete, certain cryptographic primitives (like ZK proofs) are currently implemented as mocks to facilitate high-throughput testing. These must be replaced with real circuits (e.g., libsnark, Halo2) before mainnet deployment.

### Why AILEE?

1.  **Secure by Design**: Built-in reorg protection and circuit breakers ensure system integrity.
2.  **Powerful Orchestration**: Ambient AI optimizes task distribution across a decentralized mesh.
3.  **Verifiable**: Zero-Knowledge (ZK) proofs and deterministic state commitments allow for trust-minimized verification.
4.  **Bitcoin-Native**: Respects Bitcoin as the ultimate settlement layer.

---

## ✨ Key Features

### 🛡️ Secure Reorg Protection (New)
- **RocksDB-Backed Persistence**: Tracks Bitcoin block history and reorg events with high performance.
- **Deep Reorg Detection**: Automatically detects and handles deep blockchain reorganizations (>6 blocks).
- **State Safety**: Prevents L2 state corruption by halting block production during detected instability.
- **Integration**: Tightly coupled with the Block Producer to ensure no unsafe blocks are minted during L1 turbulence.

### 🧠 Ambient AI Orchestration
- **Intelligent Task Scheduling**: Weighted, round-robin, least-loaded, and latency-aware strategies.
- **Reputation-Based Assignment**: Node scoring with capacity, cost, and energy considerations.
- **Byzantine Detection**: Outlier-based heuristics for anomaly detection.
- **Deterministic Verification**: Hash-based commitments with integrity checks.

### ⛓️ Bitcoin Layer-2 Infrastructure
- **SPV Verification**: Trust-minimized Bitcoin event verification.
- **Deterministic Anchoring**: OP_RETURN and Taproot commitment construction.
- **Federated Bridge**: Multisig quorum-based peg-in/peg-out lifecycle.
- **Offline Verifier**: `ailee_l2_verify` tool for third-party audits.

### 🌐 Web & API Integration
- **REST API**: FastAPI-based production server with OpenAPI docs.
- **Web Dashboard**: Real-time monitoring interface.
- **Health Checks**: Built-in monitoring and observability.

---

## 🧪 Testing & Validation

**Confidence requires verification.** While AILEE-Core is built with powerful and secure primitives, the responsibility of operating financial infrastructure demands rigorous and continuous testing.

We stress the importance of the following validation steps before any mainnet deployment:

1.  **Unit & Integration Testing**: Run the full test suite (`ctest`) to verify all components, including the new Reorg Detector and Adapter Registry.
2.  **Adversarial Simulation**: Use the `PerformanceSimulator` to stress-test the network under high load and simulated attacks.
3.  **Reorg Simulation**: Test the node's behavior against a simulated Bitcoin regtest network where you intentionally invalidate blocks to trigger the Reorg Detector.
4.  **Audit**: While the architecture is sound, third-party cryptographic audits are recommended for the federation multisig setup.

> **"Trust, but verify."** - The core philosophy of Bitcoin and AILEE.

---

## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev \
    libzmq3-dev libcppzmq-dev libjsoncpp-dev libyaml-cpp-dev librocksdb-dev

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

# Check logs to verify Reorg Detector initialization
tail -f logs/ailee.log
```

### Option 2: Run Tests

```bash
cd build
ctest --verbose
```

---

## 🏗️ Architecture

<div align="center">

```
┌─────────────────────────────────────────────────────────────────┐
│                      AILEE LAYER-2 NODE                         │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐  │
│  │ Orchestration│  │  Reorg       │  │  Federated         │  │
│  │ & Scheduling │  │  Detector    │  │  Learning          │  │
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

---

## 🔐 Security Model

### ✅ Explicit Federated Model

**Peg-Ins (Bitcoin → L2)**
- SPV proofs with Bitcoin headers + Merkle paths
- Trust-minimized verification of L1 events

**Peg-Outs (L2 → Bitcoin)**
- Federated multisig quorum (sidechain-style custody)
- Explicit signer thresholds and fee logic

**Reorg Safety**
- **Automated Detection**: The `ReorgDetector` monitors L1 block headers.
- **Safety Halt**: Block production pauses if a deep reorg is detected.
- **State Recovery**: L2 state can be rolled back to the last finalized anchor.

---

## ⚠️ What This Is NOT

AILEE-Core is **NOT**:
- ❌ A replacement for Bitcoin
- ❌ A consensus-changing protocol
- ❌ A trustless rollup (it is a federated sidechain)

---

## 🛠️ Technology Stack

### Core Infrastructure (C++)
- **Language**: C++17
- **Build System**: CMake 3.10+
- **Persistence**: RocksDB (State & Reorg History)
- **Bitcoin Integration**: JSON-RPC client + ZMQ listener
- **Cryptography**: OpenSSL (SHA256, signature verification)
- **Networking**: libcurl, ZeroMQ, cppzmq
- **JSON Parsing**: nlohmann/json, JsonCpp
- **Configuration**: yaml-cpp, TOML++

### REST API (Python)
- **Framework**: FastAPI (async, production-ready)
- **Server**: Uvicorn (ASGI server)

---

## 🌍 Community & Support

### Get Help
- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: Questions and community support
- **Documentation**: Comprehensive guides in `docs/`

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

---

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

**Copyright © 2025 Don Michael Feeney Jr**

Permission is hereby granted, free of charge, to any person obtaining a copy of this software to use, modify, and distribute it under the terms of the MIT License.
