# AILEE-Core Next Phase Features

This document describes the newly implemented features for the AILEE-Core Bitcoin Layer-2 framework.

## 🎯 Overview

The following features have been implemented as part of the next development phase:

1. ✅ **SHA3-256 Migration** (OpenSSL 3.0+)
2. ✅ **RocksDB Persistent Storage**
3. ✅ **P2P Networking Layer** (libp2p integration)
4. ✅ **Task Distribution Protocol**
5. ✅ **Prometheus Metrics Export**
6. ✅ **Multi-Node Deployment Testing**

---

## 📦 1. SHA3-256 Migration

### Description
Migrated cryptographic hashing from SHA-256 to SHA3-256 using OpenSSL 3.0+ APIs with automatic fallback to SHA-256 for older OpenSSL versions.

### Files Modified
- `src/security/HashProofSystem.cpp` - Updated to use EVP_sha3_256()
- `src/security/HashProofSystem.h` - Updated documentation

### Usage
```cpp
#include "security/HashProofSystem.h"

using namespace ailee::security;

// SHA3-256 is now used automatically
std::string hash = HashProofSystem::sha3_256("data to hash");
```

### Requirements
- OpenSSL 3.0+ for SHA3-256 support
- Falls back to SHA-256 on older OpenSSL versions

---

## 💾 2. RocksDB Persistent Storage

### Description
Production-ready persistent storage layer using RocksDB for storing nodes, tasks, proofs, and telemetry data.

### Files Added
- `src/storage/PersistentStorage.h` - Storage interface
- `src/storage/PersistentStorage.cpp` - RocksDB implementation

### Features
- ACID guarantees
- Configurable cache and write buffers
- Snappy compression
- Bloom filters for efficient lookups

### Usage
```cpp
#include "storage/PersistentStorage.h"

using namespace ailee::storage;

PersistentStorage::Config config;
config.dbPath = "./data/ailee.db";
config.blockCacheSizeMB = 512;

PersistentStorage storage(config);

// Store data
storage.put("key", "value");

// Retrieve data
auto value = storage.get("key");
if (value) {
    std::cout << "Value: " << *value << std::endl;
}
```

### Requirements
- RocksDB library (`librocksdb-dev` on Ubuntu)
- Compiled with `-DAILEE_HAS_ROCKSDB=1`

---

## 🌐 3. P2P Networking Layer

### Description
Peer-to-peer networking layer designed for libp2p integration, providing node discovery and pub/sub messaging.

### Files Added
- `src/network/P2PNetwork.h` - P2P network interface
- `src/network/P2PNetwork.cpp` - Stub implementation (to be replaced with libp2p)

### Features
- Peer discovery (mDNS, DHT, bootstrap peers)
- Pub/Sub messaging
- Direct peer-to-peer communication
- Connection management
- Network statistics

### Usage
```cpp
#include "network/P2PNetwork.h"

using namespace ailee::network;

P2PConfig config;
config.listenAddress = "/ip4/0.0.0.0/tcp/4001";
config.bootstrapPeers = {"/ip4/bootstrap.example.com/tcp/4001"};

P2PNetwork network(config);
network.start();

// Subscribe to topics
network.subscribe("tasks", [](const NetworkMessage& msg) {
    std::cout << "Received message from: " << msg.senderId << std::endl;
});

// Publish messages
std::vector<uint8_t> payload = {1, 2, 3};
network.publish("tasks", payload);
```

### Note
Current implementation is a stub. For production use, integrate actual libp2p C++ bindings.

---

## 📋 4. Task Distribution Protocol

### Description
Distributed task coordination protocol for multi-node task execution with priority-based scheduling.

### Files Added
- `src/orchestration/DistributedTaskProtocol.h` - Protocol interface
- `src/orchestration/DistributedTaskProtocol.cpp` - Implementation

### Features
- Priority-based task queue
- Automatic task execution
- Fault tolerance with retries
- Task lifecycle management
- Event callbacks
- Statistics and monitoring

### Usage
```cpp
#include "orchestration/DistributedTaskProtocol.h"

using namespace ailee::orchestration;

auto network = std::make_shared<network::P2PNetwork>();
network->start();

DistributedTaskProtocol::Config config;
config.nodeId = "node-1";
config.maxConcurrentTasks = 10;

DistributedTaskProtocol protocol(network, config);
protocol.start();

// Register task executor
protocol.registerExecutor(TaskType::COMPUTATION, [](const DistributedTask& task) {
    // Execute task
    TaskResult result;
    result.taskId = task.taskId;
    result.success = true;
    return result;
});

// Distribute task
DistributedTask task;
task.taskId = "task-123";
task.type = TaskType::COMPUTATION;
task.priority = TaskPriority::HIGH;
protocol.distributeTask(task);
```

---

## 📊 5. Prometheus Metrics Export

### Description
Comprehensive metrics export in Prometheus text format for monitoring and observability.

### Files Added
- `src/metrics/PrometheusExporter.h` - Metrics interface
- `src/metrics/PrometheusExporter.cpp` - Implementation

### Features
- Counter metrics (monotonically increasing)
- Gauge metrics (can go up or down)
- Histogram metrics (distribution of values)
- Pre-configured AILEE-Core metrics
- Thread-safe metric updates

### Metrics Exported
- **Node metrics**: Total nodes, active peers
- **Task metrics**: Tasks created/completed/failed, pending/running tasks, task duration
- **Network metrics**: Bytes/messages sent/received
- **Storage metrics**: Operations, latency, size
- **Bitcoin metrics**: Block height, transactions
- **System metrics**: Uptime, memory, CPU usage

### Usage
```cpp
#include "metrics/PrometheusExporter.h"

using namespace ailee::metrics;

// Get AILEE metrics singleton
auto& metrics = AILEEMetrics::getInstance();

// Update metrics
metrics.tasksTotal->increment();
metrics.tasksPending->set(5);
metrics.taskDuration->observe(1.5); // 1.5 seconds

// Render Prometheus metrics
auto& exporter = PrometheusExporter::getInstance();
std::string metricsText = exporter.renderMetrics();
```

### HTTP Endpoint
Expose metrics via HTTP endpoint (add to your web server):

```cpp
// In your HTTP server
server.Get("/metrics", [](const Request& req, Response& res) {
    auto& exporter = PrometheusExporter::getInstance();
    res.set_content(exporter.renderMetrics(), "text/plain; version=0.0.4");
});
```

---

## 🚀 6. Multi-Node Deployment Testing

### Description
Docker Compose configuration for deploying and testing a multi-node AILEE cluster with monitoring.

### Files Added
- `docker-compose.yml` - Multi-node cluster configuration
- `Dockerfile.node` - Node container image
- `config/prometheus.yml` - Prometheus scrape configuration
- `scripts/deploy-multinode.sh` - Deployment script

### Cluster Components
- **3 AILEE Nodes**: Bootstrap node + 2 peers
- **Prometheus**: Metrics collection and storage
- **Grafana**: Visualization and dashboards

### Usage

#### Start the Cluster
```bash
# Using the deployment script
./scripts/deploy-multinode.sh

# Or manually
docker-compose up -d
```

#### Access Points
- Node 1 API: http://localhost:8080
- Node 2 API: http://localhost:8081
- Node 3 API: http://localhost:8082
- Node 1 Metrics: http://localhost:9090/metrics
- Node 2 Metrics: http://localhost:9091/metrics
- Node 3 Metrics: http://localhost:9092/metrics
- Prometheus: http://localhost:9000
- Grafana: http://localhost:3000 (admin/admin)

#### View Logs
```bash
# All services
docker-compose logs -f

# Specific node
docker-compose logs -f ailee-node-1

# Prometheus
docker-compose logs -f prometheus
```

#### Stop the Cluster
```bash
# Stop without removing data
docker-compose down

# Stop and remove all data
docker-compose down -v
```

### Network Configuration
- Network: `172.20.0.0/16`
- Node 1: `172.20.0.10` (bootstrap)
- Node 2: `172.20.0.11`
- Node 3: `172.20.0.12`

---

## 🔧 Build Instructions

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install -y \
    build-essential cmake \
    libssl-dev libcurl4-openssl-dev \
    libzmq3-dev libcppzmq-dev \
    libjsoncpp-dev libyaml-cpp-dev \
    librocksdb-dev

# macOS
brew install cmake openssl curl zeromq cppzmq jsoncpp yaml-cpp rocksdb
```

### Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Build Flags
- `AILEE_HAS_ROCKSDB=1` - RocksDB support enabled
- `AILEE_HAS_ZMQ=1` - ZeroMQ support enabled
- `AILEE_HAS_JSONCPP=1` - JsonCpp support enabled

---

## 📈 Performance Considerations

### Storage
- **Write Buffer**: 64MB default (configurable)
- **Block Cache**: 512MB default (configurable)
- **Compression**: Snappy (fast compression)

### Network
- **Max Peers**: 50 default (configurable)
- **Pub/Sub**: Efficient broadcast messaging
- **Connection Management**: Automatic reconnection

### Tasks
- **Max Concurrent**: 10 default (configurable)
- **Priority Queue**: Fair scheduling with priority levels
- **Timeout**: 300s default (configurable)

---

## 🧪 Testing

### Unit Tests
```bash
cd build
cmake .. -DBUILD_TESTS=ON
make
ctest --verbose
```

### Integration Tests
```bash
# Start multi-node cluster
./scripts/deploy-multinode.sh

# Run integration tests
# TODO: Add integration test suite
```

---

## 📚 Additional Resources

- [Architecture Overview](../docs/ARCHITECTURE_CONCEPTUAL.md)
- [API Documentation](../API_QUICKSTART.md)
- [Contributing Guide](../docs/CONTRIBUTING.md)
- [Security Model](../SECURITY.md)

---

## ⚠️ Production Notes

1. **SHA3-256**: Requires OpenSSL 3.0+. Falls back to SHA-256 on older versions.
2. **libp2p**: Current P2P implementation is a stub. Integrate actual libp2p for production.
3. **RocksDB**: Ensure proper disk space and backup strategy.
4. **Prometheus**: Configure retention and storage limits.
5. **Multi-Node**: Test thoroughly before production deployment.

---

## 🔄 Migration Guide

### From SHA-256 to SHA3-256
No code changes required. The migration is transparent with automatic fallback.

### Adding Persistent Storage
```cpp
// Before: In-memory storage
std::map<std::string, std::string> data;

// After: Persistent storage
PersistentStorage storage;
storage.put("key", "value");
```

### Enabling Metrics
```cpp
// Add to your main loop
auto& metrics = AILEEMetrics::getInstance();
metrics.uptimeSeconds->set(getCurrentUptime());

// Expose HTTP endpoint
server.Get("/metrics", [](const Request& req, Response& res) {
    auto& exporter = PrometheusExporter::getInstance();
    res.set_content(exporter.renderMetrics(), "text/plain; version=0.0.4");
});
```

---

## 🎓 Examples

See the `examples/` directory for complete working examples of each feature.

---

**Last Updated**: 2026-02-15  
**Version**: 1.3.0  
**Status**: Production Ready (SHA3, RocksDB, Metrics) / Beta (P2P, Tasks)
