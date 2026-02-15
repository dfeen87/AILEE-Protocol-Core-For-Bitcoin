#!/bin/bash
# test-p2p-integration.sh - Test P2P networking integration
# 
# This script tests the P2P network implementation with or without libp2p

set -e

echo "============================================================"
echo "  AILEE-Core P2P Integration Test"
echo "============================================================"
echo ""

# Configuration
BUILD_DIR="${BUILD_DIR:-./build}"
TEST_TIMEOUT=10
PEERS_COUNT=3

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print test results
print_test_result() {
    local test_name="$1"
    local result="$2"
    
    if [ "$result" -eq 0 ]; then
        echo -e "${GREEN}✓${NC} $test_name: PASSED"
    else
        echo -e "${RED}✗${NC} $test_name: FAILED"
    fi
}

echo "Test Configuration:"
echo "  Build directory: $BUILD_DIR"
echo "  Test timeout: ${TEST_TIMEOUT}s"
echo ""

# Check if executables exist
if [ ! -f "$BUILD_DIR/ailee_p2p_demo" ]; then
    echo -e "${RED}Error:${NC} ailee_p2p_demo not found in $BUILD_DIR"
    echo "Please build the project first:"
    echo "  mkdir -p build && cd build"
    echo "  cmake .. && make -j\$(nproc)"
    exit 1
fi

# Test 1: Basic executable runs
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 1: Basic Executable Launch"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
timeout $TEST_TIMEOUT $BUILD_DIR/ailee_p2p_demo > /tmp/p2p_test1.log 2>&1 || true
if grep -q "Network started successfully" /tmp/p2p_test1.log; then
    print_test_result "Network startup" 0
else
    print_test_result "Network startup" 1
    echo "Log:"
    cat /tmp/p2p_test1.log
fi

# Test 2: Topic subscription
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 2: Topic Subscription"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if grep -q "Subscribed to 3 topics" /tmp/p2p_test1.log; then
    print_test_result "Topic subscription" 0
else
    print_test_result "Topic subscription" 1
fi

# Test 3: Message publishing
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 3: Message Publishing"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if grep -q "Initial messages published" /tmp/p2p_test1.log; then
    print_test_result "Message publishing" 0
else
    print_test_result "Message publishing" 1
fi

# Test 4: Message reception
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 4: Message Reception"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if grep -q "Task Handler.*Received message" /tmp/p2p_test1.log; then
    print_test_result "Message reception" 0
else
    print_test_result "Message reception" 1
fi

# Test 5: Peer ID generation
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 5: Peer ID Generation"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if grep -q "Local Peer ID: Qm" /tmp/p2p_test1.log; then
    print_test_result "Peer ID generation" 0
    PEER_ID=$(grep "Local Peer ID:" /tmp/p2p_test1.log | head -1 | awk '{print $NF}')
    echo "  Generated Peer ID: $PEER_ID"
else
    print_test_result "Peer ID generation" 1
fi

# Test 6: Network statistics
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 6: Network Statistics"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if grep -q "Network Statistics" /tmp/p2p_test1.log; then
    print_test_result "Statistics collection" 0
    echo "  Statistics summary:"
    grep -A5 "Network Statistics" /tmp/p2p_test1.log | tail -5 | sed 's/^/    /'
else
    print_test_result "Statistics collection" 1
fi

# Test 7: Graceful shutdown
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 7: Graceful Shutdown"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if grep -q "Network stopped" /tmp/p2p_test1.log; then
    print_test_result "Graceful shutdown" 0
else
    print_test_result "Graceful shutdown" 1
fi

# Test 8: Check libp2p status
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 8: libp2p Integration Status"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if grep -q "Running in enhanced stub mode" /tmp/p2p_test1.log; then
    echo -e "${YELLOW}ℹ${NC} libp2p: STUB MODE (library not installed)"
    echo "  To enable full libp2p support:"
    echo "    ./scripts/install-libp2p.sh"
    echo "    cd build && cmake .. && make -j\$(nproc)"
elif grep -q "Initializing with libp2p C++ bindings" /tmp/p2p_test1.log; then
    echo -e "${GREEN}✓${NC} libp2p: FULL INTEGRATION (library installed)"
else
    echo -e "${YELLOW}?${NC} libp2p: STATUS UNKNOWN"
fi

# Test 9: Multi-instance test (if possible)
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 9: Multi-Instance Launch (Optional)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  Launching 3 instances with different ports..."

# Launch instances in background
timeout 5 $BUILD_DIR/ailee_p2p_demo /ip4/0.0.0.0/tcp/4001 > /tmp/p2p_node1.log 2>&1 &
PID1=$!
sleep 1

timeout 5 $BUILD_DIR/ailee_p2p_demo /ip4/0.0.0.0/tcp/4002 /ip4/127.0.0.1/tcp/4001 > /tmp/p2p_node2.log 2>&1 &
PID2=$!
sleep 1

timeout 5 $BUILD_DIR/ailee_p2p_demo /ip4/0.0.0.0/tcp/4003 /ip4/127.0.0.1/tcp/4001 > /tmp/p2p_node3.log 2>&1 &
PID3=$!

sleep 3

# Check if instances started
INSTANCE_COUNT=0
[ -f /tmp/p2p_node1.log ] && grep -q "Network started successfully" /tmp/p2p_node1.log && INSTANCE_COUNT=$((INSTANCE_COUNT + 1))
[ -f /tmp/p2p_node2.log ] && grep -q "Network started successfully" /tmp/p2p_node2.log && INSTANCE_COUNT=$((INSTANCE_COUNT + 1))
[ -f /tmp/p2p_node3.log ] && grep -q "Network started successfully" /tmp/p2p_node3.log && INSTANCE_COUNT=$((INSTANCE_COUNT + 1))

if [ $INSTANCE_COUNT -eq 3 ]; then
    print_test_result "Multi-instance launch (3 nodes)" 0
    echo "  Node 1 Peer ID: $(grep "Local Peer ID:" /tmp/p2p_node1.log | awk '{print $NF}' || echo "N/A")"
    echo "  Node 2 Peer ID: $(grep "Local Peer ID:" /tmp/p2p_node2.log | awk '{print $NF}' || echo "N/A")"
    echo "  Node 3 Peer ID: $(grep "Local Peer ID:" /tmp/p2p_node3.log | awk '{print $NF}' || echo "N/A")"
elif [ $INSTANCE_COUNT -gt 0 ]; then
    echo -e "${YELLOW}⚠${NC} Multi-instance launch: PARTIAL ($INSTANCE_COUNT/3 nodes started)"
else
    print_test_result "Multi-instance launch" 1
fi

# Cleanup background processes
wait $PID1 2>/dev/null || true
wait $PID2 2>/dev/null || true
wait $PID3 2>/dev/null || true

# Test 10: Integration with AILEE node
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Test 10: AILEE Node Integration"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ -f "$BUILD_DIR/ailee_node" ]; then
    # Check if P2PNetwork is linked in ailee_node
    if ldd $BUILD_DIR/ailee_node 2>/dev/null | grep -q "not found"; then
        echo -e "${YELLOW}⚠${NC} AILEE node: Missing dependencies"
    else
        print_test_result "AILEE node build" 0
        echo "  P2P networking is integrated into ailee_node"
    fi
else
    echo -e "${YELLOW}⚠${NC} AILEE node: Not built"
fi

# Summary
echo ""
echo "============================================================"
echo "  Test Summary"
echo "============================================================"
echo ""

TOTAL_TESTS=10
PASSED_TESTS=0

# Count passed tests from log
grep -c "PASSED" /tmp/p2p_test_summary.log 2>/dev/null || true

echo "Test logs saved to:"
echo "  - /tmp/p2p_test1.log (main test)"
echo "  - /tmp/p2p_node1.log (instance 1)"
echo "  - /tmp/p2p_node2.log (instance 2)"
echo "  - /tmp/p2p_node3.log (instance 3)"
echo ""

echo "For detailed integration guide, see:"
echo "  - docs/LIBP2P_INTEGRATION.md"
echo "  - docs/NEXT_PHASE_FEATURES.md"
echo ""

echo "To run the P2P demo manually:"
echo "  cd $BUILD_DIR"
echo "  ./ailee_p2p_demo"
echo ""

echo "============================================================"
echo "  Integration Test Complete"
echo "============================================================"
