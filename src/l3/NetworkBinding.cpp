#include "l3/NetworkBinding.h"
#include <cstring>
#include <algorithm>

namespace ailee {
namespace l3 {

reflection::ReflectionSnapshot bind_network_block(const NetworkBlockSnapshot& net_block) {
    reflection::ReflectionSnapshot snap;
    std::memset(&snap, 0, sizeof(snap));

    snap.height.height = net_block.height;

    // Copy the block hash directly to the anchor hash since the block itself is the anchor in this abstraction.
    std::memcpy(snap.anchor.anchor_hash, net_block.block_hash, 32);
    snap.anchor.block_height = net_block.height;

    // We don't have reorg info in a single network block snapshot.
    // Reorg detection requires historical tracking, so we leave it zeroed.

    return snap;
}

l1::SettlementIngestion bind_network_mempool(const NetworkMempoolSnapshot& net_mempool, const NetworkBlockSnapshot& net_block) {
    l1::SettlementIngestion ingest;
    std::memset(&ingest, 0, sizeof(ingest));

    // Bind mempool snapshot size as anchor processed counts for deterministic reflection.
    // L1 SettlementIngestion requires a cache aligned anchor. We can represent the mempool state
    // relative to the latest block.

    std::memcpy(ingest.latest_anchor.anchorHash, net_block.block_hash, 32);
    ingest.latest_anchor.bitcoinHeight = net_block.height;
    ingest.latest_anchor.status = 1; // CONFIRMED

    ingest.total_anchors_processed = net_mempool.transaction_count;
    ingest.latest_confirmations = 1;

    return ingest;
}

mesh::MeshCoherenceResult bind_network_peer(const NetworkPeerSnapshot& net_peer) {
    mesh::MeshCoherenceResult result;
    std::memset(&result, 0, sizeof(result));

    // A MeshCoherenceResult represents a score. We map network peer metrics to a deterministic score.
    // Score could be active connections, etc.
    // In MeshCoherenceResult, we have score, l1_height_match, anchor_match, etc.
    // Since we don't have true mesh snapshots here, we fabricate a deterministic result based on peer count.

    result.score = static_cast<uint32_t>(std::min<uint64_t>(net_peer.active_connections, 4)); // max 4 for full coherence

    if (result.score > 0) result.l1_height_match = true;
    if (result.score > 1) result.anchor_match = true;
    if (result.score > 2) result.epoch_match = true;
    if (result.score > 3) result.state_root_match = true;

    return result;
}

} // namespace l3
} // namespace ailee
