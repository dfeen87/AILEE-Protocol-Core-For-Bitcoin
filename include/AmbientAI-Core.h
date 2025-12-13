// SPDX-License-Identifier: MIT
// AmbientAI-Core.h — Bulletproof Decentralized Crypto + Compute Internet Protocol
// Ambient nodes, ZK proofs, federated learning, tokenized incentives, safety, and mesh coordination

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <atomic>
#include <mutex>
#include <functional>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "zk_proofs.h" // ZK proof module

namespace ambient {

// ==================== CORE DATA MODELS ====================

struct NodeId {
    std::string pubkey;
    std::string region;
    std::string deviceClass;
};

struct EnergyProfile {
    double inputPowerW = 0.0;
    double wasteHeatRecoveredW = 0.0;
    double temperatureC = 0.0;
    double ambientTempC = 0.0;
    double carbonIntensity_gCO2_kWh = 0.0;
    double computeEfficiency_GFLOPS_W = 0.0;
};

struct ComputeProfile {
    double cpuUtilization = 0.0;
    double npuUtilization = 0.0;
    double gpuUtilization = 0.0;
    double availableMemMB = 0.0;
    double bandwidthMbps = 0.0;
    double latencyMs = 0.0;
    double instantaneousPower_GFLOPS = 0.0;
};

struct PrivacyBudget {
    double epsilon = 1.0;
    double delta   = 1e-5;
    double privacyBudgetRemaining = 1.0;
    bool homomorphicEncryptionEnabled = true;
    bool zeroKnowledgeProofEnabled = true;
};

struct TelemetrySample {
    NodeId node;
    EnergyProfile energy;
    ComputeProfile compute;
    std::chrono::system_clock::time_point timestamp;
    PrivacyBudget privacy;
    std::string cryptographicVerificationHash;
};

// ==================== FEDERATED LEARNING ====================

struct FederatedUpdate {
    std::string modelId;
    std::vector<float> gradient;
    PrivacyBudget privacy;
};

// ==================== ZK PROOFS ====================

struct ZKProofStub {
    std::string proofHash;
    std::string circuitId;
    bool verified = false;
    uint64_t timestampMs = 0;
};

// ==================== INCENTIVES & REPUTATION ====================

struct IncentiveRecord {
    std::string taskId;
    NodeId node;
    double rewardTokens = 0.0;
    bool distributed = false;
};

struct Reputation {
    NodeId node;
    double score = 0.0;
    uint64_t completedTasks = 0;
    uint64_t disputes = 0;
};

// ==================== SAFETY POLICY ====================

struct SafetyPolicy {
    double maxTemperatureC = 80.0;
    double maxLatencyMs    = 300.0;
    double maxBlockMB      = 8.0;
    int    maxErrorCount   = 25;
};

// ==================== AMBIENT NODE ====================

class AmbientNode {
public:
    explicit AmbientNode(NodeId id, SafetyPolicy policy)
        : id_(std::move(id)), policy_(policy) {}

    // Ingest telemetry with safety check
    void ingestTelemetry(const TelemetrySample& sample) {
        std::lock_guard<std::mutex> lock(mu_);
        lastSample_ = sample;
        safeMode_.store(sample.energy.temperatureC > policy_.maxTemperatureC ||
                        sample.compute.latencyMs > policy_.maxLatencyMs);
    }

    // Run federated local training
    FederatedUpdate runLocalTraining(const std::string& modelId,
                                     const std::vector<float>& miniBatch) {
        std::lock_guard<std::mutex> lock(mu_);
        FederatedUpdate up;
        up.modelId = modelId;
        up.privacy = lastSample_.privacy;
        float sum = 0.0f;
        for (auto v : miniBatch) sum += v;
        up.gradient = {sum};
        return up;
    }

    // ---------- ZK Proof integration ----------
    ZKProofStub verifyComputation(const std::string& taskId,
                                  const std::string& circuitId,
                                  const std::string& resultHash) {
        ailee::zk::ZKEngine zkEngine;
        auto proof = zkEngine.generateProof(taskId, resultHash);

        ZKProofStub p;
        p.circuitId = circuitId;
        p.proofHash = proof.proofData;
        p.verified  = zkEngine.verifyProof(proof);
        p.timestampMs = proof.timestampMs;
        return p;
    }

    // Accrue token reward
    IncentiveRecord accrueReward(const std::string& taskId, double tokens) const {
        IncentiveRecord rec;
        rec.taskId = taskId;
        rec.node = id_;
        rec.rewardTokens = tokens;
        rec.distributed = false;
        return rec;
    }

    // Update reputation
    void updateReputation(bool success, double deltaScore) {
        std::lock_guard<std::mutex> lock(mu_);
        if (success) {
            rep_.completedTasks++;
            rep_.score += deltaScore;
        } else {
            rep_.disputes++;
            rep_.score -= deltaScore;
        }
        if (rep_.score < 0.0) rep_.score = 0.0;
    }

    bool isSafeMode() const { return safeMode_.load(); }

    NodeId id() const { return id_; }
    Reputation reputation() const {
        std::lock_guard<std::mutex> lock(mu_);
        return rep_;
    }
    std::optional<TelemetrySample> last() const {
        std::lock_guard<std::mutex> lock(mu_);
        return lastSample_;
    }

private:
    NodeId id_;
    SafetyPolicy policy_;
    mutable std::mutex mu_;
    std::optional<TelemetrySample> lastSample_;
    Reputation rep_{id_, 0.0, 0, 0};
    std::atomic<bool> safeMode_{false};
};

// ==================== MESH COORDINATOR ====================

class MeshCoordinator {
public:
    using TaskFn = std::function<double(const AmbientNode&)>;

    explicit MeshCoordinator(std::string clusterId)
        : clusterId_(std::move(clusterId)) {}

    void registerNode(AmbientNode* node) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_.push_back(node);
    }

    AmbientNode* selectNodeForTask() {
        std::lock_guard<std::mutex> lock(mu_);
        AmbientNode* best = nullptr;
        double bestScore = -1.0;
        for (auto* n : nodes_) {
            auto last = n->last();
            if (!last.has_value() || n->isSafeMode()) continue;
            double score = last->compute.bandwidthMbps - last->compute.latencyMs * 0.1;
            if (score > bestScore) { bestScore = score; best = n; }
        }
        return best;
    }

    IncentiveRecord dispatchAndReward(const std::string& taskId, TaskFn fn, double baseRewardTokens) {
        AmbientNode* n = selectNodeForTask();
        if (!n) return IncentiveRecord{taskId, NodeId{"", "", ""}, 0.0, false};
        double multiplier = fn(*n);
        double reward = baseRewardTokens * multiplier;
        return n->accrueReward(taskId, reward);
    }

    std::string clusterId() const { return clusterId_; }

private:
    std::string clusterId_;
    mutable std::mutex mu_;
    std::vector<AmbientNode*> nodes_;
};

// ==================== SYSTEM & CLUSTER HELPERS ====================

struct ClusterMetrics {
    double avgLatency = 0.0;
    double totalBandwidth = 0.0;
    double totalComputePower = 0.0;
    double energyEfficiency = 0.0;
    double geographicDispersion = 0.0;
};

ClusterMetrics evaluateCluster(const std::vector<TelemetrySample>& cluster) {
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

double clusterObjectiveFunction(const ClusterMetrics& m,
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
