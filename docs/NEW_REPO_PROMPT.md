# New Repository Prompt: Ambient AI + VCP System

Use this prompt with an AI coding assistant (like GitHub Copilot, Claude, ChatGPT Code Interpreter, or Cursor) to build a fresh implementation of the Ambient AI + Verifiable Computation Protocol system.

---

## Complete System Prompt

```markdown
I want you to help me build a production-ready implementation of an **Ambient AI + Verifiable Computation Protocol (VCP)** system. This will be a decentralized compute network that orchestrates AI workloads across heterogeneous edge devices with cryptographic verification and Bitcoin Layer-2 settlement.

## SYSTEM OVERVIEW

Build a decentralized compute network with these capabilities:

1. **Ambient Node Mesh**: Heterogeneous devices form a self-organizing network
2. **Intelligent Orchestration**: Health-based task assignment with reputation scoring
3. **WASM Execution Engine**: Secure sandboxed computation with resource limits
4. **Zero-Knowledge Proofs**: Cryptographic verification of execution correctness
5. **Federated Learning**: Privacy-preserving multi-node model training
6. **Token Incentives**: Reward system tied to Bitcoin Layer-2
7. **Energy Telemetry**: Verifiable sustainability metrics

## ARCHITECTURE

### Core Components

#### 1. Ambient Node
**Responsibility**: Individual compute node with telemetry and health tracking

**Features**:
- Telemetry collection (energy, compute, privacy budgets)
- Health scoring based on:
  - Bandwidth (40% weight)
  - Latency (30% weight)
  - Compute efficiency (20% weight)
  - Reputation (10% weight)
- Safety circuit breakers (temperature, latency thresholds)
- Reputation tracking (completed tasks, disputes)
- Byzantine fault detection

**API**:
```rust
pub struct AmbientNode {
    pub id: NodeId,
    pub telemetry: TelemetrySample,
    pub reputation: Reputation,
    safety_policy: SafetyPolicy,
}

impl AmbientNode {
    fn ingest_telemetry(&mut self, sample: TelemetrySample);
    fn health_score(&self) -> f64;
    fn is_safe_mode(&self) -> bool;
    fn update_reputation(&mut self, success: bool, delta: f64);
}
```

#### 2. WASM Execution Engine
**Responsibility**: Secure sandboxed execution with resource limits

**Features**:
- Multi-runtime support (WasmEdge, Wasmer, WAVM)
- Resource limits:
  - Memory: 512MB default
  - Timeout: 30 seconds
  - Max instructions: 10 billion
  - Gas metering enabled
- Capabilities: No filesystem, no network, crypto allowed
- Determinism checking for reproducibility
- Execution trace recording for ZK proofs

**API**:
```rust
pub struct WasmEngine {
    runtime: WasmRuntime,
    limits: SandboxLimits,
}

impl WasmEngine {
    fn execute(&self, call: WasmCall) -> Result<WasmResult>;
    fn execute_with_trace(&self, call: WasmCall) -> Result<(WasmResult, ExecutionTrace)>;
    fn verify_determinism(&self, module_hash: &str, inputs: &[u8]) -> bool;
}
```

#### 3. ZK Proof System
**Responsibility**: Generate and verify proofs of correct execution

**Features**:
- Execution trace to proof conversion
- Universal verifier (works for any WASM program)
- Proof anchoring to blockchain
- Efficient verification (< 1s for most proofs)

**Technology Stack Options**:
- **Option A**: RISC Zero zkVM (production-ready, WASM-compatible)
- **Option B**: Plonky2 (ultra-fast proving, custom circuits)
- **Option C**: SP1 (emerging, WASM-native)

**API**:
```rust
pub struct ZKProver {
    proving_key: ProvingKey,
    verification_key: VerificationKey,
}

impl ZKProver {
    fn generate_proof(&self, trace: ExecutionTrace) -> Result<ZKProof>;
    fn verify_proof(&self, proof: &ZKProof, public_inputs: &[u8]) -> bool;
    fn proof_size(&self, proof: &ZKProof) -> usize;
}
```

#### 4. Mesh Coordinator
**Responsibility**: Network-wide node management and task assignment

**Features**:
- Node registry with health tracking
- Task assignment strategies:
  - Weighted (capacity-based)
  - Round-robin
  - Least-loaded
  - Latency-aware
- Proof verification
- Reward distribution
- Byzantine consensus

**API**:
```rust
pub struct MeshCoordinator {
    cluster_id: String,
    nodes: Vec<AmbientNode>,
    strategy: TaskAssignmentStrategy,
}

impl MeshCoordinator {
    fn register_node(&mut self, node: AmbientNode);
    fn select_node_for_task(&self, requirements: TaskRequirements) -> Option<&AmbientNode>;
    fn dispatch_and_reward(&mut self, task: Task) -> Result<IncentiveRecord>;
}
```

#### 5. Federated Learning Protocol
**Responsibility**: Privacy-preserving multi-node model training

**Features**:
- Local training with proof-of-training generation
- Secure aggregation of model deltas
- Differential privacy guarantees (ε, δ budgets)
- Poisoning attack detection

**API**:
```rust
pub struct FederatedLearner {
    aggregator: Aggregator,
    privacy_budget: PrivacyBudget,
}

impl FederatedLearner {
    fn local_train(&self, task: FLTask, data: &[u8]) -> Result<(ModelDelta, Proof)>;
    fn aggregate_deltas(&self, deltas: Vec<ModelDelta>) -> Result<GlobalModel>;
    fn verify_training_proof(&self, proof: &Proof) -> bool;
}
```

## TECHNICAL REQUIREMENTS

### Programming Language: Rust
**Why Rust?**
- Memory safety without garbage collection
- Zero-cost abstractions
- Strong type system prevents bugs
- Excellent async/await support
- Best-in-class WASM tooling

### Key Dependencies
```toml
[dependencies]
# WASM Runtime
wasmedge-sdk = "0.13"  # or wasmer = "4.0"

# ZK Proofs
risc0-zkvm = "0.21"    # or plonky2 = "0.2"

# Networking
libp2p = "0.53"
tokio = "1.35"

# Bitcoin
bitcoin = "0.31"
rust-bitcoin = "0.31"

# Cryptography
ring = "0.17"
sha3 = "0.10"

# Serialization
serde = { version = "1.0", features = ["derive"] }
bincode = "1.3"

# Async
tokio = { version = "1.35", features = ["full"] }
async-trait = "0.1"

# Metrics
prometheus = "0.13"
```

### Performance Targets
- **Task Assignment Latency**: < 100ms
- **WASM Execution**: Native performance (< 2x slowdown)
- **Proof Generation**: < 10s for most tasks
- **Proof Verification**: < 1s
- **Throughput**: 1000+ concurrent tasks
- **Node Capacity**: 10,000+ nodes per cluster

### Security Requirements
- **WASM Sandbox**: Zero filesystem/network access by default
- **Memory Isolation**: Each task in separate memory space
- **Timeout Enforcement**: Hard limits on execution time
- **Cryptographic Verification**: All inputs/outputs hashed
- **mTLS**: Encrypted node communication
- **Byzantine Tolerance**: 33% malicious actors

## REPOSITORY STRUCTURE

```
ambient-vcp/
├── README.md                 # Comprehensive documentation
├── Cargo.toml               # Workspace configuration
├── Cargo.lock
├── docker-compose.yml       # Local multi-node deployment
├── kubernetes/              # K8s manifests
│   ├── node-deployment.yaml
│   ├── coordinator-deployment.yaml
│   └── service.yaml
│
├── crates/
│   ├── ambient-node/        # Node implementation
│   │   ├── src/
│   │   │   ├── lib.rs
│   │   │   ├── telemetry.rs
│   │   │   ├── health.rs
│   │   │   └── reputation.rs
│   │   └── Cargo.toml
│   │
│   ├── wasm-engine/         # Execution runtime
│   │   ├── src/
│   │   │   ├── lib.rs
│   │   │   ├── sandbox.rs
│   │   │   ├── limits.rs
│   │   │   └── trace.rs
│   │   └── Cargo.toml
│   │
│   ├── zk-prover/           # Proof generation
│   │   ├── src/
│   │   │   ├── lib.rs
│   │   │   ├── prover.rs
│   │   │   ├── verifier.rs
│   │   │   └── circuit.rs
│   │   └── Cargo.toml
│   │
│   ├── mesh-coordinator/    # Task orchestration
│   │   ├── src/
│   │   │   ├── lib.rs
│   │   │   ├── registry.rs
│   │   │   ├── assignment.rs
│   │   │   └── settlement.rs
│   │   └── Cargo.toml
│   │
│   ├── federated-learning/  # FL protocol
│   │   ├── src/
│   │   │   ├── lib.rs
│   │   │   ├── aggregator.rs
│   │   │   ├── privacy.rs
│   │   │   └── poisoning.rs
│   │   └── Cargo.toml
│   │
│   ├── bitcoin-anchor/      # L2 settlement
│   │   ├── src/
│   │   │   ├── lib.rs
│   │   │   ├── commitment.rs
│   │   │   ├── peg.rs
│   │   │   └── verifier.rs
│   │   └── Cargo.toml
│   │
│   └── cli/                 # Command-line interface
│       ├── src/
│       │   └── main.rs
│       └── Cargo.toml
│
├── examples/
│   ├── hello-compute/       # Simple task example
│   ├── image-classify/      # ML inference demo
│   ├── federated-mnist/     # FL training demo
│   └── local-mesh/          # 3-node simulation
│
├── wasm-modules/            # Example WASM programs
│   ├── inference.wasm
│   ├── training.wasm
│   └── analytics.wasm
│
├── benchmarks/              # Performance tests
│   ├── throughput.rs
│   ├── latency.rs
│   └── proof-generation.rs
│
├── tests/                   # Integration tests
│   ├── integration.rs
│   ├── byzantine.rs
│   └── failover.rs
│
├── dashboard/               # Web UI (TypeScript + React)
│   ├── package.json
│   ├── src/
│   │   ├── App.tsx
│   │   ├── NodeMap.tsx
│   │   └── MetricsDashboard.tsx
│   └── public/
│
└── docs/                    # Architecture documentation
    ├── ARCHITECTURE.md
    ├── API_REFERENCE.md
    ├── DEPLOYMENT.md
    ├── SECURITY.md
    └── CONTRIBUTING.md
```

## DELIVERABLES (PHASE 1)

Please build the following in order:

### 1. Core Infrastructure (Week 1)
- [ ] `ambient-node` crate with telemetry and health scoring
- [ ] `wasm-engine` crate with WasmEdge integration
- [ ] `mesh-coordinator` crate with node registry
- [ ] Basic CLI for starting nodes

### 2. Execution & Verification (Week 2)
- [ ] WASM execution with resource limits
- [ ] Execution trace recording
- [ ] ZK proof generation (can use placeholder initially)
- [ ] Proof verification

### 3. Demo Application (Week 3)
- [ ] 3-node local mesh simulation
- [ ] Simple compute task (e.g., hash calculation)
- [ ] End-to-end workflow: submit → execute → verify → reward
- [ ] Metrics dashboard (basic web UI)

### 4. Documentation (Week 4)
- [ ] README with architecture diagrams
- [ ] Build instructions
- [ ] API documentation (auto-generated from code)
- [ ] Deployment guide (Docker + K8s)

## EXAMPLE CODE SNIPPETS

### Node Initialization
```rust
use ambient_node::AmbientNode;
use ambient_node::{NodeId, SafetyPolicy, TelemetrySample};

#[tokio::main]
async fn main() -> Result<()> {
    let node_id = NodeId::new("node-001", "us-west", "gateway");
    
    let policy = SafetyPolicy {
        max_temperature_c: 85.0,
        max_latency_ms: 100.0,
        max_block_mb: 8.0,
        max_error_count: 25,
    };
    
    let mut node = AmbientNode::new(node_id, policy);
    
    // Start telemetry collection
    let telemetry = collect_system_telemetry().await?;
    node.ingest_telemetry(telemetry);
    
    println!("Node health score: {}", node.health_score());
    
    Ok(())
}
```

### WASM Execution
```rust
use wasm_engine::{WasmEngine, WasmCall, SandboxLimits};

async fn execute_task(module_path: &str, inputs: &[u8]) -> Result<Vec<u8>> {
    let limits = SandboxLimits::default();
    let engine = WasmEngine::new(limits)?;
    
    let call = WasmCall {
        function_name: "run_inference".to_string(),
        input_bytes: inputs.to_vec(),
        ..Default::default()
    };
    
    let result = engine.execute(call).await?;
    
    if !result.success {
        return Err(Error::ExecutionFailed(result.error));
    }
    
    Ok(result.output_bytes)
}
```

### ZK Proof Generation
```rust
use zk_prover::{ZKProver, ExecutionTrace};

async fn prove_execution(trace: ExecutionTrace) -> Result<ZKProof> {
    let prover = ZKProver::new()?;
    
    let proof = prover.generate_proof(trace).await?;
    
    // Verify locally
    assert!(prover.verify_proof(&proof, &trace.public_inputs));
    
    Ok(proof)
}
```

### Mesh Coordination
```rust
use mesh_coordinator::{MeshCoordinator, TaskAssignmentStrategy};

async fn orchestrate_task(task: Task) -> Result<TaskResult> {
    let mut coordinator = MeshCoordinator::new("cluster-001");
    
    // Register nodes
    for node in discover_nodes().await? {
        coordinator.register_node(node);
    }
    
    // Select best node
    let selected = coordinator
        .select_node_for_task(task.requirements)
        .ok_or(Error::NoAvailableNodes)?;
    
    println!("Selected node: {} (health: {})", 
        selected.id, selected.health_score());
    
    // Execute and reward
    let incentive = coordinator.dispatch_and_reward(task).await?;
    
    Ok(TaskResult {
        output: incentive.result,
        reward: incentive.reward_tokens,
    })
}
```

## TESTING REQUIREMENTS

### Unit Tests
```rust
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_node_health_scoring() {
        let node = create_test_node();
        assert!(node.health_score() > 0.0);
        assert!(node.health_score() <= 1.0);
    }
    
    #[tokio::test]
    async fn test_wasm_execution() {
        let result = execute_simple_wasm().await.unwrap();
        assert_eq!(result, expected_output());
    }
    
    #[test]
    fn test_zk_proof_verification() {
        let (proof, public_inputs) = generate_test_proof();
        assert!(verify_proof(&proof, &public_inputs));
    }
}
```

### Integration Tests
```rust
#[tokio::test]
async fn test_end_to_end_workflow() {
    // 1. Start coordinator
    let coordinator = start_coordinator().await;
    
    // 2. Start 3 nodes
    let nodes = start_test_nodes(3).await;
    
    // 3. Submit task
    let task = Task::new("test-task", simple_wasm_module());
    let result = coordinator.submit_task(task).await.unwrap();
    
    // 4. Verify result
    assert!(result.proof_verified);
    assert_eq!(result.output, expected_output());
}
```

### Byzantine Tests
```rust
#[tokio::test]
async fn test_byzantine_detection() {
    let mut coordinator = MeshCoordinator::new("test");
    
    // Add malicious node
    let mut malicious = AmbientNode::new(...);
    malicious.set_behavior(Byzantine::CorruptOutput);
    coordinator.register_node(malicious);
    
    // Submit task
    let result = coordinator.submit_task(task).await;
    
    // Should detect corruption via proof verification
    assert!(!result.proof_verified);
    assert!(malicious.reputation.score < 0.5);
}
```

## DESIGN PRINCIPLES

1. **Safety First**: Use Rust's type system to prevent bugs at compile time
2. **Explicit Trust**: Document all trust assumptions clearly
3. **Verifiable Everything**: Cryptographic proofs for all claims
4. **Energy Conscious**: Track and optimize power consumption
5. **Byzantine Resilient**: Assume 33% of nodes may be malicious
6. **Privacy Preserving**: Federated learning with differential privacy
7. **Bitcoin Native**: Anchor commitments to Bitcoin for settlement

## WHAT TO AVOID

- ❌ Don't claim "trustless" (this is a federated model, not a rollup)
- ❌ Don't implement novel cryptography (use audited libraries)
- ❌ Don't promise specific TPS numbers (provide benchmarks instead)
- ❌ Don't build a new blockchain (use Bitcoin L2)
- ❌ Don't add AI hype without substance
- ❌ Don't over-engineer (start with MVP, iterate based on feedback)

## DOCUMENTATION REQUIREMENTS

### README.md
- [ ] System overview with architecture diagram
- [ ] Quick start (< 5 minutes to first task)
- [ ] Build instructions
- [ ] Deployment guide (local, Docker, K8s)
- [ ] API reference links
- [ ] Contributing guidelines
- [ ] Security disclosure policy

### ARCHITECTURE.md
- [ ] Component diagram
- [ ] Data flow diagrams
- [ ] Trust model explanation
- [ ] Security boundaries
- [ ] Performance characteristics

### API_REFERENCE.md
- [ ] Auto-generated from code docs
- [ ] Examples for each API
- [ ] Error handling guide

## SUCCESS CRITERIA

The system is ready when:

1. ✅ A developer can start 3 local nodes in < 5 minutes
2. ✅ Tasks execute successfully with ZK proof verification
3. ✅ Byzantine nodes are detected and penalized
4. ✅ All code has > 80% test coverage
5. ✅ Documentation is comprehensive and accurate
6. ✅ Performance meets targets (< 100ms task assignment)
7. ✅ Security audit identifies no critical issues

## NEXT STEPS AFTER MVP

1. **Production Hardening**
   - External security audit
   - Chaos engineering tests
   - Production monitoring setup

2. **Advanced Features**
   - Federated learning implementation
   - Multi-chain support beyond Bitcoin
   - Advanced scheduling algorithms

3. **Ecosystem Development**
   - SDK for task developers
   - Marketplace for WASM modules
   - Node operator tooling

---

## YOUR TASK

Please build this system step-by-step, starting with:

1. **Project Setup**: Create Cargo workspace with all crates
2. **Core Node**: Implement `ambient-node` with telemetry
3. **WASM Engine**: Integrate WasmEdge with resource limits
4. **Coordinator**: Build mesh coordinator with node registry
5. **Demo**: Create end-to-end 3-node simulation

For each step:
- Write clean, well-documented Rust code
- Include comprehensive tests
- Provide example usage
- Update documentation

Let's begin with the project setup. Please create the complete directory structure and Cargo.toml files.
```

---

## How to Use This Prompt

1. **Copy the entire prompt above** (from "I want you to help me build..." to "...Cargo.toml files.")

2. **Paste into your AI assistant** of choice:
   - GitHub Copilot Chat
   - Claude (Anthropic)
   - ChatGPT Code Interpreter
   - Cursor IDE
   - Any other AI coding assistant

3. **The AI will**:
   - Create the complete project structure
   - Implement each component with Rust
   - Write comprehensive tests
   - Generate documentation
   - Provide deployment guides

4. **Follow up prompts** you can use:
   - "Now implement the WASM engine crate"
   - "Add ZK proof generation using RISC Zero"
   - "Create a 3-node local demo"
   - "Write integration tests for Byzantine scenarios"
   - "Generate API documentation"

## Why Start Fresh vs. Integrate Here?

### Start Fresh (New Repo) If:
- ✅ You want a clean, modern Rust implementation
- ✅ You prefer async/await patterns throughout
- ✅ You want best-in-class WASM tooling (WasmEdge, Wasmer)
- ✅ You need production-grade ZK proofs (RISC Zero, Plonky2)
- ✅ You want strong type safety and memory safety guarantees

### Integrate Here (This Repo) If:
- ✅ You want to build on existing C++ infrastructure
- ✅ You value the current Bitcoin L2 integration
- ✅ You prefer the existing multi-chain adapters
- ✅ You want to extend rather than replace

## Expected Timeline

Using the prompt above with a capable AI assistant:

- **Week 1**: Core infrastructure (nodes, WASM engine, coordinator)
- **Week 2**: Execution & verification (traces, ZK proofs)
- **Week 3**: Demo application (3-node mesh, web UI)
- **Week 4**: Documentation & polish

**Total**: ~4 weeks to production-ready MVP

## Support

Questions? Issues? Want to contribute?

- Open an issue in the original repo
- Tag @dfeen87 for specific questions
- Join discussions about the architecture

---

**Note**: This prompt is designed to work with modern AI coding assistants. The more specific your follow-up questions, the better the results.
