# Production Deployment Guide

## Quick Start: Deploy Production AILEE-Core

This guide shows how to deploy a production-ready Ambient AI + VCP network.

---

## Prerequisites

### System Requirements (Per Node)
- **OS**: Ubuntu 22.04 LTS or later
- **CPU**: 4+ cores (8+ recommended)
- **RAM**: 8GB minimum (16GB+ recommended)
- **Storage**: 100GB SSD (NVMe preferred)
- **Network**: 100 Mbps+ bandwidth, < 50ms latency

### Software Dependencies
```bash
# Install system packages
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    libssl-dev libcurl4-openssl-dev \
    libzmq3-dev libjsoncpp-dev \
    libyaml-cpp-dev \
    docker.io docker-compose \
    prometheus-node-exporter

# Optional: WasmEdge runtime (for production WASM execution)
curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | sudo bash

# Optional: RocksDB (for persistent storage)
sudo apt-get install -y librocksdb-dev
```

---

## Deployment Options

### Option 1: Single-Node Development (5 minutes)

```bash
# Clone repository
git clone https://github.com/dfeen87/AILEE-Protocol-Core-For-Bitcoin.git
cd AILEE-Protocol-Core-For-Bitcoin

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run demo
./ailee_ambient_vcp_demo --nodes 3 --tasks 10

# Output: 3-node mesh executing 10 tasks with telemetry
```

### Option 2: Docker Multi-Node (15 minutes)

Create `docker-compose.yml`:
```yaml
version: '3.8'

services:
  coordinator:
    image: ailee/coordinator:latest
    container_name: ailee-coordinator
    ports:
      - "8080:8080"      # HTTP API
      - "9090:9090"      # Prometheus metrics
    environment:
      - AILEE_ROLE=coordinator
      - AILEE_CLUSTER_ID=production-cluster
      - BITCOIN_RPC_URL=http://bitcoin:8332
    volumes:
      - coordinator-data:/data
    networks:
      - ailee-net
    restart: unless-stopped

  worker-1:
    image: ailee/worker:latest
    container_name: ailee-worker-1
    environment:
      - AILEE_ROLE=worker
      - AILEE_NODE_ID=worker-001
      - AILEE_DEVICE_CLASS=gateway
      - AILEE_COORDINATOR=coordinator:8080
    volumes:
      - worker-1-data:/data
    networks:
      - ailee-net
    depends_on:
      - coordinator
    restart: unless-stopped

  worker-2:
    image: ailee/worker:latest
    container_name: ailee-worker-2
    environment:
      - AILEE_ROLE=worker
      - AILEE_NODE_ID=worker-002
      - AILEE_DEVICE_CLASS=pc
      - AILEE_COORDINATOR=coordinator:8080
    volumes:
      - worker-2-data:/data
    networks:
      - ailee-net
    depends_on:
      - coordinator
    restart: unless-stopped

  worker-3:
    image: ailee/worker:latest
    container_name: ailee-worker-3
    environment:
      - AILEE_ROLE=worker
      - AILEE_NODE_ID=worker-003
      - AILEE_DEVICE_CLASS=smartphone
      - AILEE_COORDINATOR=coordinator:8080
    volumes:
      - worker-3-data:/data
    networks:
      - ailee-net
    depends_on:
      - coordinator
    restart: unless-stopped

  prometheus:
    image: prom/prometheus:latest
    container_name: ailee-prometheus
    ports:
      - "9091:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus-data:/prometheus
    networks:
      - ailee-net
    restart: unless-stopped

  grafana:
    image: grafana/grafana:latest
    container_name: ailee-grafana
    ports:
      - "3000:3000"
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
    volumes:
      - grafana-data:/var/lib/grafana
    networks:
      - ailee-net
    restart: unless-stopped

volumes:
  coordinator-data:
  worker-1-data:
  worker-2-data:
  worker-3-data:
  prometheus-data:
  grafana-data:

networks:
  ailee-net:
    driver: bridge
```

Deploy:
```bash
docker-compose up -d
docker-compose logs -f

# Access dashboard: http://localhost:3000 (admin/admin)
# Access API: http://localhost:8080/health
```

### Option 3: Kubernetes Production (60 minutes)

```bash
# Apply manifests
kubectl apply -f kubernetes/namespace.yaml
kubectl apply -f kubernetes/configmap.yaml
kubectl apply -f kubernetes/coordinator-deployment.yaml
kubectl apply -f kubernetes/worker-deployment.yaml
kubectl apply -f kubernetes/service.yaml
kubectl apply -f kubernetes/ingress.yaml

# Scale workers
kubectl scale deployment ailee-worker --replicas=10

# Check status
kubectl get pods -n ailee-production
kubectl logs -f deployment/ailee-coordinator -n ailee-production
```

---

## Production Configuration

### Coordinator Config (`config/coordinator.yaml`)
```yaml
coordinator:
  cluster_id: "production-cluster"
  listen_address: "0.0.0.0:8080"
  max_nodes: 10000
  task_timeout: 30s
  
orchestration:
  strategy: "weighted"  # weighted, round-robin, latency-aware
  health_check_interval: 30s
  byzantine_threshold: 0.33
  
storage:
  backend: "rocksdb"
  path: "/data/coordinator"
  backup_interval: 1h
  
bitcoin_l2:
  rpc_url: "${BITCOIN_RPC_URL}"
  network: "mainnet"
  commitment_interval: 100  # blocks
  
security:
  require_tls: true
  max_task_size_mb: 10
  rate_limit_per_node: 100  # tasks/minute
```

### Worker Config (`config/worker.yaml`)
```yaml
worker:
  node_id: "${AILEE_NODE_ID}"
  device_class: "${AILEE_DEVICE_CLASS}"
  region: "${AILEE_REGION}"
  coordinator_url: "${AILEE_COORDINATOR}"
  
resources:
  max_memory_mb: 2048
  max_cpu_percent: 80
  max_concurrent_tasks: 5
  
telemetry:
  report_interval: 10s
  enable_energy_tracking: true
  enable_carbon_tracking: true
  
wasm:
  runtime: "wasmedge"  # wasmedge, wasmer, wavm
  max_execution_time: 30s
  max_memory_mb: 512
  enable_gas_metering: true
  
privacy:
  differential_privacy: true
  epsilon: 1.0
  delta: 1e-5
```

---

## Monitoring & Observability

### Prometheus Metrics

Key metrics exposed on `/metrics`:
```
# Node metrics
ailee_node_health_score{node_id, device_class, region}
ailee_node_reputation{node_id}
ailee_node_tasks_completed{node_id}
ailee_node_power_watts{node_id}
ailee_node_temperature_celsius{node_id}

# Task metrics
ailee_tasks_submitted_total
ailee_tasks_completed_total
ailee_tasks_failed_total
ailee_task_execution_duration_seconds{quantile}

# Proof metrics
ailee_proofs_generated_total
ailee_proofs_verified_total
ailee_proof_generation_duration_seconds{quantile}

# System metrics
ailee_coordinator_nodes_active
ailee_coordinator_tasks_queued
ailee_storage_size_bytes
```

### Grafana Dashboards

Import dashboards from `monitoring/grafana/`:
- `node-health.json` - Node health and telemetry
- `task-execution.json` - Task throughput and latency
- `network-overview.json` - Overall network status

---

## Security Hardening

### TLS Configuration
```bash
# Generate certificates
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes

# Configure in coordinator.yaml
security:
  tls_cert: "/certs/cert.pem"
  tls_key: "/certs/key.pem"
  require_client_certs: true
```

### Firewall Rules
```bash
# Allow only necessary ports
sudo ufw allow 8080/tcp   # API
sudo ufw allow 9090/tcp   # Metrics
sudo ufw deny 5432/tcp    # Block database externally
sudo ufw enable
```

### Rate Limiting
```yaml
security:
  rate_limits:
    global: 10000  # tasks/minute across all nodes
    per_node: 100  # tasks/minute per node
    per_ip: 500    # requests/minute per IP
```

---

## Backup & Recovery

### Automated Backups
```bash
# Add to crontab
0 2 * * * /usr/local/bin/ailee-backup.sh

# Backup script
#!/bin/bash
DATE=$(date +%Y%m%d)
tar -czf /backups/ailee-${DATE}.tar.gz /data/coordinator /data/workers
aws s3 cp /backups/ailee-${DATE}.tar.gz s3://ailee-backups/
```

### Disaster Recovery
```bash
# Restore from backup
tar -xzf /backups/ailee-20260215.tar.gz -C /

# Restart services
docker-compose down
docker-compose up -d

# Verify recovery
curl http://localhost:8080/health
```

---

## Performance Tuning

### System Limits
```bash
# /etc/sysctl.conf
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864
fs.file-max = 1000000

# Apply
sudo sysctl -p
```

### Resource Allocation
```yaml
# Kubernetes resource requests/limits
resources:
  requests:
    memory: "2Gi"
    cpu: "1000m"
  limits:
    memory: "8Gi"
    cpu: "4000m"
```

---

## Troubleshooting

### Common Issues

**Issue**: Nodes not connecting to coordinator
```bash
# Check logs
docker logs ailee-coordinator
docker logs ailee-worker-1

# Verify network
docker network inspect ailee-net
ping coordinator

# Check firewall
sudo ufw status
```

**Issue**: Tasks timing out
```bash
# Increase timeout in config
task_timeout: 60s  # was 30s

# Check node resources
docker stats

# Review task complexity
# Large WASM modules may need more time
```

**Issue**: High memory usage
```bash
# Reduce cache size in config
wasm:
  module_cache_size_mb: 256  # was 512

# Enable compression
storage:
  enable_compression: true

# Add swap if needed
sudo fallocate -l 4G /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

---

## Scaling Strategy

### Horizontal Scaling
```bash
# Add more workers
kubectl scale deployment ailee-worker --replicas=50

# Add coordinator replicas (with Raft consensus)
kubectl scale statefulset ailee-coordinator --replicas=3
```

### Vertical Scaling
```yaml
# Increase resources per pod
resources:
  limits:
    memory: "16Gi"  # was 8Gi
    cpu: "8000m"    # was 4000m
```

---

## Cost Optimization

### Cloud Provider Estimates (Monthly)

**AWS (us-east-1)**:
- 1 coordinator (c5.2xlarge): ~$250
- 10 workers (t3.large): ~$600
- RDS PostgreSQL (db.t3.medium): ~$100
- S3 storage + egress: ~$50
- **Total**: ~$1000/month

**GCP (us-central1)**:
- 1 coordinator (n2-standard-8): ~$230
- 10 workers (e2-standard-2): ~$550
- Cloud SQL: ~$90
- Storage + egress: ~$50
- **Total**: ~$920/month

### Spot/Preemptible Instances
Save 60-90% on worker nodes:
```yaml
# Kubernetes node selector
nodeSelector:
  node-lifecycle: spot
```

---

## Production Checklist

Before going live:

- [ ] TLS enabled for all communication
- [ ] Automated backups configured
- [ ] Monitoring dashboards set up
- [ ] Alerting rules configured
- [ ] Rate limiting enabled
- [ ] Resource limits set
- [ ] Firewall rules applied
- [ ] Documentation complete
- [ ] Incident response plan ready
- [ ] On-call rotation established
- [ ] Security audit completed
- [ ] Load testing passed
- [ ] Disaster recovery tested
- [ ] Compliance review done

---

**Ready to deploy? Start with Docker Compose for testing, then graduate to Kubernetes for production.**

Support: Open an issue on GitHub or contact the team.
