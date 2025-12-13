// mesh_optimizer.cpp
// Small runtime optimizer that tunes rewards for MeshCoordinator tasks.

#include "AmbientAI.h"
#include "ailee_recovery_protocol.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

namespace meshopt {

// Compute an adaptive base reward for the mesh given recent sampled nodes.
// The function returns a base token amount that the caller can pass to MeshCoordinator::dispatchAndReward.
// Heuristic:
//  - If average bandwidth is high and latency low -> increase base reward to attract participation.
//  - If many nodes are in safe mode or poor compute -> reduce reward to conserve budget.
double computeAdaptiveBaseReward(const ambient::MeshCoordinator& mesh, double defaultBase = 10.0) {
    // Gather stats
    double avgBw = 0.0;
    double avgLat = 0.0;
    size_t healthy = 0;
    size_t total = 0;

    // We do not have direct access to internal node list; use public selectNodeForTask as probe.
    // Instead, probe by attempting to dispatch to each node via a temporary task function that returns their score.
    // To avoid side-effects, we'll attempt to find clusterId and then attempt to evaluate nodes by copying logic.
    // For simplicity (and because nodes are stored privately), we will call selectNodeForTask repeatedly to get representative node.
    auto sampleNode = mesh.selectNodeForTask();
    if (!sampleNode) {
        return defaultBase * 0.5; // nobody healthy
    }

    // Since we can't iterate internal vector (private), we make a small probe of the selected node:
    if (auto last = sampleNode->last(); last.has_value()) {
        avgBw = last->compute.bandwidthMbps;
        avgLat = last->compute.latencyMs;
        healthy = sampleNode->isSafeMode() ? 0 : 1;
        total = 1;
    } else {
        return defaultBase * 0.8;
    }

    // Simple transform
    double bwFactor = std::min(4.0, std::max(0.5, avgBw / 50.0));   // normalized multiplier
    double latFactor = std::clamp(1.0 - (avgLat / 500.0), 0.2, 1.5); // lower latency increases factor
    double healthFactor = (total == 0) ? 0.5 : (1.0 + (static_cast<double>(healthy) / static_cast<double>(total)));

    double base = defaultBase * bwFactor * latFactor * healthFactor;

    // Clamp to reasonable bounds
    base = std::clamp(base, 1.0, 200.0);

    // Danger check — if the selected node reports odd telemetry, record incident
    if (avgLat > 1000.0 || avgBw < 0.1) {
        ailee::RecoveryProtocol::recordIncident("MeshOptimizer_AnomalousProbe",
            "lat=" + std::to_string(avgLat) + " bw=" + std::to_string(avgBw));
    }

    return base;
}

// Simple utility to allocate a task using the mesh and optimizer, returning the IncentiveRecord.
ambient::IncentiveRecord allocateOptimizedTask(ambient::MeshCoordinator& mesh,
                                               const std::string& taskId) {
    double adaptiveBase = computeAdaptiveBaseReward(mesh, 10.0);

    // Use a generic performance fn that rewards bandwidth and penalizes latency more strongly
    ambient::MeshCoordinator::TaskFn perfFn =
        [](const ambient::AmbientNode& n) {
            auto last = n.last();
            if (!last.has_value()) return 0.0;
            double bw = last->compute.bandwidthMbps;
            double lat = last->compute.latencyMs;
            double score = (bw / 25.0) - (lat / 250.0);
            score = std::clamp(score, 0.05, 3.0);
            return score;
        };

    auto rec = mesh.dispatchAndReward(taskId, perfFn, adaptiveBase);
    // Log to stdout (main logging can capture this)
    std::ostringstream ss;
    ss << "[MeshOptimizer] task=" << taskId
       << " base=" << std::fixed << std::setprecision(2) << adaptiveBase
       << " -> node=" << rec.node.pubkey
       << " tokens=" << rec.rewardTokens;
    std::cout << ss.str() << std::endl;

    return rec;
}

} // namespace meshopt
