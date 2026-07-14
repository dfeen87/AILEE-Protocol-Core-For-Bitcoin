#pragma once

#include "l2/ExecutionContext.h"      // use canonical ExecutionEnvelope
#include "l3/GossipLayer.h"
#include "network/MainnetDiscovery.hpp"

namespace ailee {
namespace l3 {

// ---------------------------------------------------------
// Sync status enum + threshold
// ---------------------------------------------------------
enum class SyncStatus : uint8_t {
    IN_SYNC = 0,
    STALE = 1,
    NEEDS_RECOVERY = 2,
    AHEAD = 3,
    BEHIND = 4
};

constexpr uint64_t STALE_THRESHOLD = 8; // tune as needed

// ---------------------------------------------------------
// PeerSync state + summary
// ---------------------------------------------------------
struct PeerSyncState {
    l2::ExecutionEnvelope local_envelope;
    GossipEnvelope        remote_envelope;
    int64_t               coherence_delta;
    SyncStatus            sync_status;
};

struct PeerSyncSummary {
    uint64_t local_epoch;
    uint64_t remote_epoch;
    int64_t  delta;
    uint8_t  sync_status;
};

// ---------------------------------------------------------
// Main PeerSync API
// ---------------------------------------------------------
PeerSyncState compute_peer_sync(
    const l2::ExecutionEnvelope& local_envelope,
    const GossipEnvelope& remote_envelope);

PeerSyncSummary summarize_peer_sync(const PeerSyncState& state);

// ---------------------------------------------------------
// Bind mainnet discovery subsystem
// ---------------------------------------------------------
void bind_mainnet_discovery(MainnetDiscovery* discovery);

} // namespace l3
} // namespace ailee
