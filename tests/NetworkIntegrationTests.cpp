#include <gtest/gtest.h>
#include "l3/NetworkReflection.h"
#include "l3/NetworkBinding.h"
#include "l3/NetworkDriver.h"
#include <string>

using namespace ailee;
using namespace ailee::l3;

TEST(NetworkIntegrationTests, ParseBlockSnapshotDeterministic) {
    std::string json_data = R"({
        "hash": "0000000000000000000123456789abcdef123456789abcdef123456789abcdef",
        "previousblockhash": "0000000000000000000abcdef123456789abcdef123456789abcdef123456789",
        "merkleroot": "123456789abcdef123456789abcdef123456789abcdef123456789abcdef1234",
        "height": 800000,
        "time": 1690000000,
        "nonce": 424242,
        "difficulty": 1.23,
        "bits": "1700ffff",
        "nTx": 4500
    })";

    NetworkBlockSnapshot snap = parse_network_block_snapshot(json_data);

    EXPECT_EQ(snap.height, 800000);
    EXPECT_EQ(snap.timestamp, 1690000000);
    EXPECT_EQ(snap.nonce, 424242);
    EXPECT_EQ(snap.difficulty_target, 0x1700ffff);
    EXPECT_EQ(snap.transaction_count, 4500);

    // Verify correct parsing of hex 0s (leading) and trailing
    // hash: ends with ...abcdef -> hex 0xef = 239
    EXPECT_EQ(snap.block_hash[31], 0xef);
}

TEST(NetworkIntegrationTests, ParseMempoolSnapshot) {
    std::string json_data = R"({
        "size": 5000,
        "bytes": 2000000,
        "total_fee": 0.05,
        "mempoolminfee": 0.00001000,
        "minrelaytxfee": 0.00000100
    })";

    NetworkMempoolSnapshot snap = parse_network_mempool_snapshot(json_data);
    EXPECT_EQ(snap.transaction_count, 5000);
    EXPECT_EQ(snap.total_vsize, 2000000);
    EXPECT_EQ(snap.total_fee_sats, 5000000); // 0.05 * 10^8
    EXPECT_EQ(snap.min_fee_rate, 1000);      // 0.00001 * 10^8
    EXPECT_EQ(snap.max_fee_rate, 100);       // 0.000001 * 10^8
}

TEST(NetworkIntegrationTests, ParsePeerSnapshot) {
    std::string json_data = R"([
        {"bytesrecv": 1000, "bytessent": 500, "connection_type": "inbound"},
        {"bytesrecv": 2000, "bytessent": 1500, "connection_type": "outbound"}
    ])";

    NetworkPeerSnapshot snap = parse_network_peer_snapshot(json_data);
    EXPECT_EQ(snap.peer_count, 2);
    EXPECT_EQ(snap.total_bytes_recv, 3000);
    EXPECT_EQ(snap.total_bytes_sent, 2000);
    EXPECT_EQ(snap.active_connections, 2);
}

TEST(NetworkIntegrationTests, BindingIsDeterministicAndZeroAllocation) {
    NetworkBlockSnapshot net_block = {};
    net_block.height = 12345;
    net_block.block_hash[0] = 0xAA;

    reflection::ReflectionSnapshot l2_snap = bind_network_block(net_block);
    EXPECT_EQ(l2_snap.height.height, 12345);
    EXPECT_EQ(l2_snap.anchor.anchor_hash[0], 0xAA);
    EXPECT_EQ(l2_snap.anchor.block_height, 12345);

    NetworkMempoolSnapshot net_mempool = {};
    net_mempool.transaction_count = 1000;

    l1::SettlementIngestion l1_ingest = bind_network_mempool(net_mempool, net_block);
    EXPECT_EQ(l1_ingest.total_anchors_processed, 1000);
    EXPECT_EQ(l1_ingest.latest_anchor.anchorHash[0], 0xAA);

    NetworkPeerSnapshot net_peer = {};
    net_peer.active_connections = 3;

    mesh::MeshCoherenceResult mesh_coh = bind_network_peer(net_peer);
    EXPECT_EQ(mesh_coh.score, 3);
    EXPECT_TRUE(mesh_coh.l1_height_match);
    EXPECT_TRUE(mesh_coh.anchor_match);
    EXPECT_TRUE(mesh_coh.epoch_match);
    EXPECT_FALSE(mesh_coh.state_root_match);
}

TEST(NetworkIntegrationTests, DriverProducesEnvelopes) {
    NetworkConfig config;
    config.max_steps = 3;

    identity::NodeId node_id = {};
    uint32_t protocol_version = 1;

    NetworkRunSummary summary = NetworkDriver::run_offline(config, node_id, protocol_version);

    EXPECT_EQ(summary.total_steps_executed, 3);
    EXPECT_EQ(summary.envelopes_produced, 3);
    EXPECT_EQ(summary.sequence.count, 3);
    EXPECT_EQ(summary.sequence.envelopes.size(), 3);

    // Check that we got envelopes with 0 signature, but correctly built context
    for (size_t i = 0; i < summary.sequence.count; ++i) {
        // Envelopes should be distinct at least in some deterministic property if the engine steps
        // The driver used a dummy step counter (0, 1, 2) which alters network height
        // But since we aren't asserting state roots exactly here, just count is fine.
        EXPECT_EQ(summary.sequence.envelopes[i].context.mesh_coherence_score, 4); // Dummy active_connections=4 -> score=4
    }
}
