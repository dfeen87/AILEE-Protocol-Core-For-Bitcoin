#include "l4/ClusterSim.h"
#include "l4/RecoveryCoordinator.h"
#include "l4/MeshAnchor.h"
#include "l4/StateRootPropagation.h"
#include "l4/DeterministicTransport.h"
#include "l2/DeterministicEngine.h"
#include "l3/GossipLayer.h"
#include "l3/PeerSync.h"
#include "l4/DeterministicScheduler.h"
#include "NodeIdentity.h"
#include <cstring>
#include <iostream>

namespace ailee {
namespace l4 {

ClusterView run_cluster_simulation(
    const std::vector<ClusterNodeState>& initial_nodes,
    const std::vector<std::pair<size_t, size_t>>& gossip_schedule,
    uint64_t max_steps
) {
    ClusterView view = {};
    view.nodes = initial_nodes;
    view.total_nodes = initial_nodes.size();
    view.total_steps = 0;

    std::vector<l2::DeterministicEngine> engines;
    engines.reserve(view.nodes.size());
    for (const auto& node : view.nodes) {
        engines.emplace_back(node.engine_state);
    }

    DeterministicScheduler scheduler;

    // max_steps now represents full cluster ticks (which consist of 9 sub-ticks each, so total_ticks = max_steps * 9)
    uint64_t total_ticks = max_steps * 9;
    for (uint64_t i = 0; i < total_ticks; ++i) {
        scheduler.run_tick(view, gossip_schedule, engines);
    }

    return view;
}

ClusterCoherenceSummary compute_cluster_coherence(const ClusterView& view) {
    ClusterCoherenceSummary summary = {};
    std::memset(&summary, 0, sizeof(summary));

    if (view.total_nodes == 0) {
        return summary;
    }

    std::vector<RecoveryRequest> pending_reqs = build_recovery_requests(view);
    std::vector<RecoveryResponse> matched_resps = match_recovery_responses(view, pending_reqs);

    summary.unrecoverable_nodes_count = pending_reqs.size() - matched_resps.size();

    for (const auto& node : view.nodes) {
        if (!node.peer_sync_states.empty()) {
            const l3::PeerSyncState& latest_sync = node.peer_sync_states.back();

            switch (latest_sync.sync_status) {
                case l3::SyncStatus::IN_SYNC:
                    summary.in_sync_count++;
                    break;
                case l3::SyncStatus::AHEAD:
                    summary.ahead_count++;
                    break;
                case l3::SyncStatus::BEHIND:
                    summary.behind_count++;
                    break;
                case l3::SyncStatus::NEEDS_RECOVERY:
                    summary.needs_recovery_count++;
                    break;
                case l3::SyncStatus::STALE:
                    summary.stale_count++;
                    break;
            }

            bool was_bad = false;
            for (const auto& st : node.peer_sync_states) {
                if (st.sync_status == l3::SyncStatus::NEEDS_RECOVERY || st.sync_status == l3::SyncStatus::BEHIND) {
                    was_bad = true;
                }
                if (was_bad && st.sync_status == l3::SyncStatus::IN_SYNC) {
                    summary.recovered_nodes_count++;
                    break;
                }
            }
        } else {
            // Assume stale if no sync state
            summary.stale_count++;
        }

        if (node.state_root_status == StateRootStatus::CONSISTENT) {
            summary.consistent_state_root_nodes++;
        } else if (node.state_root_status == StateRootStatus::INCONSISTENT) {
            summary.inconsistent_state_root_nodes++;
        }
    }

    summary.global_coherence_score = (summary.in_sync_count * 100) / view.total_nodes;

    return summary;
}

} // namespace l4
} // namespace ailee
