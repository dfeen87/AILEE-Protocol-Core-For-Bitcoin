# AILEE-Core Examples

This directory contains working examples and demonstrations of AILEE-Core's capabilities.

## Available Examples

### 1. **AmbientVCP_Demo.cpp** - Ambient AI + VCP Integration

**What it demonstrates:**
- End-to-end workflow from task submission to settlement
- Node health scoring and intelligent task assignment
- WASM sandbox execution with resource limits
- ZK proof generation and verification
- Energy telemetry and efficiency tracking
- Token reward distribution based on performance

**How to build:**
```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
make ailee_ambient_vcp_demo
```

**How to run:**
```bash
# Run with default settings (3 nodes, 5 tasks)
./ailee_ambient_vcp_demo

# Custom configuration
./ailee_ambient_vcp_demo --nodes 5 --tasks 10

# Quiet mode (less output)
./ailee_ambient_vcp_demo --quiet
```

**Expected output:**
```
╔══════════════════════════════════════════════════════════════════╗
║        Ambient AI + VCP Integration Demo                        ║
║        Decentralized Verifiable Computation on Bitcoin L2       ║
╚══════════════════════════════════════════════════════════════════╝

=== Initializing Ambient Node Mesh ===
  ✓ Node 0 (gateway): Health=0.78, Power=85W, Latency=22ms
  ✓ Node 1 (smartphone): Health=0.65, Power=42W, Latency=38ms
  ✓ Node 2 (pc): Health=0.82, Power=95W, Latency=15ms

=== Executing Tasks ===

--- Task: task_001 ---
Description: ML Inference: Image Classification (cat.jpg)
  → Selected: node_2 (Health: 0.820)
  → Executing in WASM sandbox...
  → ZK Proof: ✓ VERIFIED
  → Reward: 0.1200 AILEE tokens
  → Metrics:
      Execution Time: 245.3 ms
      Memory Used: 28 MB
      Instructions: 5847293
      Gas Consumed: 584729
  → Energy Impact:
      Power: 105 W
      Temperature: 58 °C
      Efficiency: 7.8 GFLOPS/W

[... more tasks ...]

=== Mesh Summary ===

node_0:
  Device: gateway (us-west)
  Reputation: 0.520
  Tasks Completed: 42
  Current Power: 87 W
  Health Score: 0.785

[... more nodes ...]

Network Totals:
  Total Tasks: 128
  Total Power Draw: 242 W
  Average Power per Node: 80.7 W
```

**What it showcases:**
- ✅ Multi-factor node health scoring (bandwidth, latency, efficiency, reputation)
- ✅ Intelligent orchestration based on node capabilities
- ✅ Simulated WASM execution with realistic metrics
- ✅ ZK proof workflow (generation and verification)
- ✅ Energy-aware computing with telemetry
- ✅ Reputation-based incentive system
- ✅ Real-time adaptation to node performance

**Architecture diagram:**
```
Client Task
    ↓
Orchestrator (Health-Based Selection)
    ↓
Ambient Node (Best Match)
    ↓
WASM Engine (Sandboxed Execution)
    ↓
ZK Proof Generator
    ↓
Verification & Settlement
    ↓
Reputation Update + Token Reward
```

---

### 2. **WebServerDemo.cpp** - HTTP Dashboard

**What it demonstrates:**
- Real-time web dashboard for monitoring
- REST API for system status
- Metrics visualization

**How to run:**
```bash
./ailee_web_demo
# Visit: http://localhost:8080
```

---

## Integration with Main System

The demo shows how the following components work together:

1. **AmbientAI.h/cpp** - Node mesh coordination
   - Telemetry collection
   - Health scoring
   - Reputation tracking

2. **WasmEngine.h** - Secure execution
   - Resource limits
   - Gas metering
   - Deterministic results

3. **zk_proofs.cpp** - Verification
   - Proof generation
   - Universal verification
   - Trust minimization

4. **Orchestrator.cpp** - Task assignment
   - Strategy selection
   - Load balancing
   - Fault tolerance

## Next Steps

After running the demo:

1. **Understand the Architecture**
   - Read: `docs/AMBIENT_VCP_INTEGRATION.md`
   - Review: Component interfaces in `include/`

2. **Implement Real WASM Runtime**
   - Replace `SimulatedWasmEngine` with WasmEdge integration
   - Add actual WASM module loading from IPFS
   - Implement execution trace recording

3. **Add Real ZK Proofs**
   - Integrate RISC Zero or Plonky2
   - Generate proofs from execution traces
   - Implement universal verification

4. **Deploy Multi-Node Network**
   - Use Docker Compose for local testing
   - Deploy to cloud with Kubernetes
   - Connect to Bitcoin L2 testnet

5. **Build Your Own Tasks**
   - Create WASM modules for your use case
   - Test with the demo infrastructure
   - Measure performance and costs

## Further Reading

- **Ambient AI Whitepaper**: `docs/whitepapers/AMBIENT_AI.md`
- **VCP Specification**: `docs/whitepapers/VCP.md`
- **Integration Guide**: `docs/AMBIENT_VCP_INTEGRATION.md`
- **Build Instructions**: `docs/BUILD.md`
- **API Reference**: `API_QUICKSTART.md`

## Questions?

Open an issue on GitHub or check the documentation in `docs/`.

## Contributing

Want to add more examples? PRs welcome!

Ideas for new examples:
- Federated learning workflow
- Byzantine fault injection testing
- Performance benchmarking suite
- Multi-chain integration demo
- Energy optimization showcase
