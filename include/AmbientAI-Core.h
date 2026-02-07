// SPDX-License-Identifier: MIT
// AmbientAI-Core.h — Legacy compatibility header for AmbientAI.
// This header now forwards to AmbientAI.h to avoid duplicate type definitions.

#pragma once

#include "AmbientAI.h"

namespace ambient {

// ==================== SYSTEM & CLUSTER HELPERS ====================

struct ClusterMetrics {
    double avgLatency = 0.0;
    double totalBandwidth = 0.0;
    double totalComputePower = 0.0;
    double energyEfficiency = 0.0;
    double geographicDispersion = 0.0;
};

inline ClusterMetrics evaluateCluster(const std::vector<TelemetrySample>& cluster) {
    ClusterMetrics metrics = {};
    if (cluster.empty()) return metrics;

    for (const auto& node : cluster) {
        metrics.avgLatency += node.compute.latencyMs;
        metrics.totalBandwidth += node.compute.bandwidthMbps;
        metrics.totalComputePower += node.compute.instantaneousPower_GFLOPS;
        metrics.energyEfficiency += node.energy.computeEfficiency_GFLOPS_W;
    }

    metrics.avgLatency /= cluster.size();
    metrics.energyEfficiency /= cluster.size();
    metrics.geographicDispersion = std::sqrt(static_cast<double>(cluster.size()));
    return metrics;
}

inline double clusterObjectiveFunction(const ClusterMetrics& m,
                                       double w_latency = 0.3,
                                       double w_bandwidth = 0.2,
                                       double w_compute = 0.3,
                                       double w_energy = 0.2) {
    double latencyTerm = w_latency * m.avgLatency;
    double bandwidthTerm = w_bandwidth / std::max(1.0, m.totalBandwidth);
    double computeTerm = -w_compute * m.totalComputePower;
    double energyTerm = w_energy / std::max(0.01, m.energyEfficiency);
    return latencyTerm + bandwidthTerm + computeTerm + energyTerm;
}

} // namespace ambient
