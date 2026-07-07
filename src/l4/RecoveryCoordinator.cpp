#include "l4/RecoveryCoordinator.h"
#include <cstring>

namespace ailee {
namespace l4 {

std::vector<RecoveryRequest> build_recovery_requests(const ClusterView& view) {
    std::vector<RecoveryRequest> requests;

    for (const auto& node : view.nodes) {
        bool has_eligible_peer = false;
        for (const auto& sync_state : node.peer_sync_states) {
            if (sync_state.sync_status == l3::SyncStatus::IN_SYNC ||
                sync_state.sync_status == l3::SyncStatus::AHEAD) {
                has_eligible_peer = true;
                break;
            }
        }

        if (!has_eligible_peer) {
            continue;
        }

        if (node.peer_sync_states.empty()) {
            continue;
        }

        const auto& latest_sync = node.peer_sync_states.back();

        if (latest_sync.sync_status == l3::SyncStatus::NEEDS_RECOVERY) {
            RecoveryRequest req;
            std::memset(&req, 0, sizeof(RecoveryRequest));
            req.requester_node_id_hash = node.node_id_hash;
            req.target_epoch = node.last_envelope.context.l1_height;
            std::memcpy(req.target_state_root, node.last_envelope.context.state_root_hash, 32);
            req.reason = RecoveryReason::STATE_MISMATCH;
            requests.push_back(req);
        } else if (latest_sync.sync_status == l3::SyncStatus::BEHIND) {
            uint64_t local_epoch = latest_sync.local_envelope.context.l1_height;
            uint64_t remote_epoch = latest_sync.remote_envelope.remote_summary.epoch_height;
            if (remote_epoch > local_epoch && (remote_epoch - local_epoch) >= 8) {
                RecoveryRequest req;
                std::memset(&req, 0, sizeof(RecoveryRequest));
                req.requester_node_id_hash = node.node_id_hash;
                req.target_epoch = node.last_envelope.context.l1_height;
                std::memcpy(req.target_state_root, node.last_envelope.context.state_root_hash, 32);
                req.reason = RecoveryReason::BEHIND;
                requests.push_back(req);
            }
        } else if (latest_sync.sync_status == l3::SyncStatus::STALE) {
            RecoveryRequest req;
            std::memset(&req, 0, sizeof(RecoveryRequest));
            req.requester_node_id_hash = node.node_id_hash;
            req.target_epoch = node.last_envelope.context.l1_height;
            std::memcpy(req.target_state_root, node.last_envelope.context.state_root_hash, 32);
            req.reason = RecoveryReason::STALE;
            requests.push_back(req);
        }
    }

    return requests;
}

std::vector<RecoveryResponse> match_recovery_responses(
    const ClusterView& view,
    const std::vector<RecoveryRequest>& requests
) {
    std::vector<RecoveryResponse> responses;

    for (const auto& req : requests) {
        const ClusterNodeState* best_provider = nullptr;

        for (const auto& node : view.nodes) {
            if (node.node_id_hash == req.requester_node_id_hash) {
                continue;
            }

            if (node.peer_sync_states.empty()) {
                continue;
            }

            const auto& latest_sync = node.peer_sync_states.back();
            if (latest_sync.sync_status != l3::SyncStatus::IN_SYNC &&
                latest_sync.sync_status != l3::SyncStatus::AHEAD) {
                continue;
            }

            if (node.last_envelope.context.l1_height < req.target_epoch) {
                continue;
            }

            bool is_zero_root = true;
            for (int i = 0; i < 32; ++i) {
                if (node.last_envelope.context.state_root_hash[i] != 0) {
                    is_zero_root = false;
                    break;
                }
            }
            if (is_zero_root) {
                continue;
            }

            if (!best_provider) {
                best_provider = &node;
            } else {
                if (node.node_id_hash < best_provider->node_id_hash) {
                    best_provider = &node;
                } else if (node.node_id_hash == best_provider->node_id_hash) {
                    int cmp = std::memcmp(node.last_envelope.context.state_root_hash,
                                          best_provider->last_envelope.context.state_root_hash, 32);
                    if (cmp < 0) {
                        best_provider = &node;
                    }
                }
            }
        }

        if (best_provider) {
            RecoveryResponse resp;
            std::memset(&resp, 0, sizeof(RecoveryResponse));
            resp.requester_node_id_hash = req.requester_node_id_hash;
            resp.provider_node_id_hash = best_provider->node_id_hash;
            resp.epoch = best_provider->last_envelope.context.l1_height;
            std::memcpy(resp.state_root, best_provider->last_envelope.context.state_root_hash, 32);
            std::memcpy(&resp.payload, &best_provider->engine_state.recovery, sizeof(l2::RecoveryPayload));
            resp.full_recovery = true;
            responses.push_back(resp);
        }
    }

    return responses;
}

void apply_recovery(ClusterNodeState& node, const RecoveryResponse& response) {
    if (!response.full_recovery) {
        return;
    }

    // Overwrite engine_state recovery fields
    node.engine_state.epoch.l1_height = response.epoch;
    std::memcpy(node.engine_state.state_root.root_hash, response.state_root, 32);

    // Update context
    node.engine_state.context.l1_height = response.epoch;
    std::memcpy(node.engine_state.context.state_root_hash, response.state_root, 32);

    // Update last_envelope
    node.last_envelope.context.l1_height = response.epoch;
    std::memcpy(node.last_envelope.context.state_root_hash, response.state_root, 32);

    // Attach response.payload (assume it goes to engine_state.recovery)
    std::memcpy(&node.engine_state.recovery, &response.payload, sizeof(l2::RecoveryPayload));

    // Reset peer_sync_states
    node.peer_sync_states.clear();

    // Increment step_counter
    node.step_counter++;
}

} // namespace l4
} // namespace ailee
