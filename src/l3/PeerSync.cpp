#include "l3/PeerSync.h"
#include <cstring>
#include <algorithm>

#include "protocol/ProtocolFrame.hpp"   // <-- added
#include <openssl/sha.h>                // <-- added

namespace ailee {
namespace l3 {

// ---------------------------------------------------------
// Deterministic signature verification for ProtocolFrame
// ---------------------------------------------------------
static bool verify_signature(const ProtocolFrame& pf)
{
    // Recreate the signed data exactly as sign_frame() does
    std::string data = pf.frame_id + pf.type + pf.version +
                       pf.node_id + std::to_string(pf.timestamp) +
                       pf.payload;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), hash);

    std::string hex;
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    static const char* digits = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex.push_back(digits[(hash[i] >> 4) & 0xF]);
        hex.push_back(digits[hash[i] & 0xF]);
    }

    return (hex == pf.signature);
}

// ---------------------------------------------------------
// Main PeerSync computation
// ---------------------------------------------------------
PeerSyncState compute_peer_sync(
    const l2::ExecutionEnvelope& local_envelope,
    const GossipEnvelope& remote_envelope
) {
    PeerSyncState state;
    std::memset(&state, 0, sizeof(state));

    // ---------------------------------------------------------
    // Signature verification (if remote envelope contains a frame)
    // ---------------------------------------------------------
    if (remote_envelope.has_protocol_frame) {
        const ProtocolFrame& pf = remote_envelope.protocol_frame;

        bool sig_ok = verify_signature(pf);

        if (!sig_ok) {
            // Deterministic fallback for invalid frames
            state.sync_status = SyncStatus::NEEDS_RECOVERY;
            state.coherence_delta = -999; // signal invalid frame
            return state;
        }
    }

    // ---------------------------------------------------------
    // Existing sync logic (unchanged)
    // ---------------------------------------------------------

    // Safely copy envelopes to state
    std::memcpy(&state.local_envelope, &local_envelope, sizeof(l2::ExecutionEnvelope));
    std::memcpy(&state.remote_envelope, &remote_envelope, sizeof(GossipEnvelope));

    uint64_t local_epoch = state.local_envelope.context.l1_height;
    uint64_t remote_epoch = state.remote_envelope.remote_summary.epoch_height;

    // Compare roots securely without memory leaks
    bool roots_match = (std::memcmp(
        state.local_envelope.context.state_root_hash,
        state.remote_envelope.remote_summary.state_root_hash,
        32
    ) == 0);

    // Compute coherence delta
    uint64_t local_coherence = state.local_envelope.context.mesh_coherence_score;
    uint64_t remote_coherence = state.remote_envelope.remote_summary.coherence_score;
    state.coherence_delta = static_cast<int64_t>(local_coherence) - static_cast<int64_t>(remote_coherence);

    // Determine sync status deterministically based on defined rules
    if ((remote_epoch + STALE_THRESHOLD < local_epoch) || (local_epoch + STALE_THRESHOLD < remote_epoch)) {
        state.sync_status = SyncStatus::STALE;
    } else if (local_epoch == remote_epoch && roots_match) {
        state.sync_status = SyncStatus::IN_SYNC;
    } else if (local_epoch == remote_epoch && !roots_match) {
        state.sync_status = SyncStatus::NEEDS_RECOVERY;
    } else if (local_epoch > remote_epoch) {
        state.sync_status = SyncStatus::AHEAD;
    } else {
        state.sync_status = SyncStatus::BEHIND;
    }

    return state;
}

// ---------------------------------------------------------
// Summary builder (unchanged)
// ---------------------------------------------------------
PeerSyncSummary summarize_peer_sync(const PeerSyncState& state) {
    PeerSyncSummary summary;
    std::memset(&summary, 0, sizeof(summary));

    summary.local_epoch = state.local_envelope.context.l1_height;
    summary.remote_epoch = state.remote_envelope.remote_summary.epoch_height;
    summary.delta = state.coherence_delta;
    summary.sync_status = static_cast<uint8_t>(state.sync_status);

    return summary;
}

} // namespace l3
} // namespace ailee
