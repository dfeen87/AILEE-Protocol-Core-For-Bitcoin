#!/bin/bash
# Multi-node deployment test script for AILEE-Core

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "========================================="
echo "AILEE-Core Multi-Node Deployment Test"
echo "========================================="
echo ""

# Check if docker-compose is installed
if ! command -v docker-compose &> /dev/null; then
    echo "ERROR: docker-compose is not installed"
    echo "Please install docker-compose: https://docs.docker.com/compose/install/"
    exit 1
fi

# Build Docker images
echo "Building Docker images..."
docker-compose build

# Start the cluster
echo ""
echo "Starting multi-node cluster..."
docker-compose up -d

# Wait for nodes to be healthy
echo ""
echo "Waiting for nodes to be healthy..."
sleep 10

# Check node health
echo ""
echo "Checking node health..."

for i in 1 2 3; do
    PORT=$((8079 + i))
    echo -n "Node $i (port $PORT): "
    
    if curl -s -f "http://localhost:$PORT/health" > /dev/null 2>&1; then
        echo "✓ Healthy"
    else
        echo "✗ Unhealthy"
    fi
done

# Check Prometheus
echo -n "Prometheus (port 9000): "
if curl -s -f "http://localhost:9000/-/healthy" > /dev/null 2>&1; then
    echo "✓ Healthy"
else
    echo "✗ Unhealthy"
fi

# Check Grafana
echo -n "Grafana (port 3000): "
if curl -s -f "http://localhost:3000/api/health" > /dev/null 2>&1; then
    echo "✓ Healthy"
else
    echo "✗ Unhealthy"
fi

# Show cluster status
echo ""
echo "Cluster Status:"
docker-compose ps

echo ""
echo "========================================="
echo "Cluster Access URLs:"
echo "========================================="
echo "Node 1 API:       http://localhost:8080"
echo "Node 2 API:       http://localhost:8081"
echo "Node 3 API:       http://localhost:8082"
echo "Node 1 Metrics:   http://localhost:9090/metrics"
echo "Node 2 Metrics:   http://localhost:9091/metrics"
echo "Node 3 Metrics:   http://localhost:9092/metrics"
echo "Prometheus:       http://localhost:9000"
echo "Grafana:          http://localhost:3000 (admin/admin)"
echo ""
echo "To view logs:     docker-compose logs -f [service-name]"
echo "To stop cluster:  docker-compose down"
echo "To cleanup:       docker-compose down -v"
echo "========================================="
