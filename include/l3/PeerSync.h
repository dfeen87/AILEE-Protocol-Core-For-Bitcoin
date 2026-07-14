#pragma once

#include "l2/ExecutionEnvelope.h"
#include "l3/GossipLayer.h"
#include "network/MainnetDiscovery.hpp"   // <-- added

namespace ailee {
namespace l3 {

// ---------------------------------------------------------
// PeerSync state + summary (unchanged)
// ---------------------------------------------------------
struct PeerSyncState {
    l2::ExecutionEnvelope local_envelope;
    GossipEnvelope remote_envelope;
    int64_t coherence_delta;
    SyncStatus sync_status;
};

struct PeerSyncSummary {
    uint64_t local_epoch;
    uint64_t remote_epoch;
    int64_t delta;
    uint8_t sync_status;
};

// ---------------------------------------------------------
// Main PeerSync API (unchanged)
// ---------------------------------------------------------
PeerSyncState compute_peer_sync(
    const l2::ExecutionEnvelope& local_envelope,
    const GossipEnvelope& remote_envelope);

PeerSyncSummary summarize_peer_sync(const PeerSyncState& state);

// ---------------------------------------------------------
// NEW: Bind mainnet discovery subsystem
// ---------------------------------------------------------
void bind_mainnet_discovery(MainnetDiscovery* discovery);

} // namespace l3
} // namespace ailee
