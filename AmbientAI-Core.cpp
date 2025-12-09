// SPDX-License-Identifier: MIT
// AmbientAI-Core.cpp — Enhanced Ambient AI infrastructure for AILEE-Core
// Implements telemetry, ZK-proof verification, federated learning, Byzantine fault tolerance,
// cluster intelligence, dynamic incentives, and full diagnostics.

#include "AmbientAI.h"
#include "zk_proofs.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <numeric>
#include <deque>
#include <random>

namespace ambient {

// ============================================================================
// RANDOM & UTILITY HELPERS
// ============================================================================
inline double randomNoise(double scale = 1.0) {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(-scale, scale);
    return dist(gen);
}

inline uint64_t timestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// ============================================================================
// TELEMETRY HISTORY & NODE MODEL
// ============================================================================
struct NodeTelemetryHistory {
    std::deque<TelemetrySample> history;
    size_t maxSamples = 100;

    void addSample(const TelemetrySample& sample) {
        if (history.size() >= maxSamples) history.pop_front();
        history.push_back(sample);
    }

    double avgLatency() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) sum += s.compute.latencyMs;
        return sum / history.size();
    }

    double avgCompute() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) sum += s.compute.cpuUtilization + s.compute.npuUtilization + s.compute.gpuUtilization;
        return sum / history.size();
    }

    double avgEnergyEfficiency() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) sum += s.energy.inputPowerW > 0 ? s.compute.cpuUtilization / s.energy.inputPowerW : 0.0;
        return sum / history.size();
    }

    double avgPrivacyBudget() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) sum += s.privacy.epsilon;
        return sum / history.size();
    }
};

// ============================================================================
// AMBIENT NODE IMPLEMENTATION
// ============================================================================
class EnhancedAmbientNode : public AmbientNode {
public:
    explicit EnhancedAmbientNode(NodeId id, SafetyPolicy policy)
        : AmbientNode(id, policy) {}

    void ingestTelemetry(const TelemetrySample& sample) {
        std::lock_guard<std::mutex> lock(mu_);
        lastSample_ = sample;
        history_.addSample(sample);

        if (sample.energy.temperatureC > policy_.maxTemperatureC ||
            sample.compute.latencyMs > policy_.maxLatencyMs) {
            safeMode_.store(true);
        } else {
            safeMode_.store(false);
        }

        // Generate cryptographic verification hash automatically
        auto proof = zkEngine_.generateProof(id_.pubkey + std::to_string(timestampMs()), std::to_string(sample.compute.cpuUtilization));
        lastZKProof_ = proof;
    }

    FederatedUpdate runLocalTraining(const std::string& modelId, const std::vector<float>& miniBatch) {
        std::lock_guard<std::mutex> lock(mu_);
        FederatedUpdate up;
        up.modelId = modelId;
        up.privacy = lastSample_.privacy;

        float sum = std::accumulate(miniBatch.begin(), miniBatch.end(), 0.0f);
        // Add differential privacy noise
        sum += static_cast<float>(randomNoise(1.0 / lastSample_.privacy.epsilon));
        up.gradient = {sum};
        return up;
    }

    ZKProofStub verifyComputation(const std::string& taskId,
                                  const std::string& circuitId,
                                  const std::string& resultHash) {
        auto proof = zkEngine_.generateProof(taskId, resultHash);
        ZKProofStub p;
        p.circuitId = circuitId;
        p.proofHash = proof.proofData;
        p.verified = zkEngine_.verifyProof(proof);
        p.timestampMs = proof.timestampMs;
        lastZKProof_ = p;
        return p;
    }

    NodeTelemetryHistory getHistory() const { return history_; }

private:
    NodeTelemetryHistory history_;
    mutable std::mutex mu_;
    ailee::zk::ZKEngine zkEngine_;
    ZKProofStub lastZKProof_;
};

// ============================================================================
// MESH COORDINATOR (Cluster-level orchestration)
// ============================================================================
class EnhancedMeshCoordinator : public MeshCoordinator {
public:
    using TaskFn = std::function<double(const EnhancedAmbientNode&)>;

    explicit EnhancedMeshCoordinator(std::string clusterId)
        : MeshCoordinator(clusterId) {}

    void registerNode(EnhancedAmbientNode* node) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_.push_back(node);
    }

    EnhancedAmbientNode* selectNodeForTask() {
        std::lock_guard<std::mutex> lock(mu_);
        EnhancedAmbientNode* best = nullptr;
        double bestScore = -1.0;

        for (auto* n : nodes_) {
            auto last = n->last();
            if (!last.has_value()) continue;
            if (n->isSafeMode()) continue;

            double efficiency = n->getHistory().avgEnergyEfficiency();
            double latency = n->getHistory().avgLatency();
            double privacy = n->getHistory().avgPrivacyBudget();
            double reputation = n->reputation().score;

            // Weighted score: higher efficiency, reputation, privacy; lower latency
            double score = efficiency * 0.4 + reputation * 0.3 + privacy * 0.2 - latency * 0.1;
            if (score > bestScore) { bestScore = score; best = n; }
        }
        return best;
    }

    IncentiveRecord dispatchAndReward(const std::string& taskId, TaskFn fn, double baseRewardTokens) {
        EnhancedAmbientNode* n = selectNodeForTask();
        if (!n) return IncentiveRecord{taskId, NodeId{"", "", ""}, 0.0, false};

        double multiplier = fn(*n);
        double reward = baseRewardTokens * multiplier;
        return n->accrueReward(taskId, reward);
    }

private:
    mutable std::mutex mu_;
    std::vector<EnhancedAmbientNode*> nodes_;
};

// ============================================================================
// BYZANTINE FAULT DETECTION
// ============================================================================
bool detectByzantineNode(const TelemetrySample& sample,
                         const std::vector<TelemetrySample>& peerSamples,
                         double threshold = 3.0) {
    if (peerSamples.size() < 3) return false;

    std::vector<double> computeVals;
    for (const auto& peer : peerSamples) computeVals.push_back(peer.compute.cpuUtilization);

    std::sort(computeVals.begin(), computeVals.end());
    double median = computeVals[computeVals.size() / 2];

    std::vector<double> deviations;
    for (double val : computeVals) deviations.push_back(std::abs(val - median));
    std::sort(deviations.begin(), deviations.end());
    double mad = deviations[deviations.size() / 2];

    double modifiedZ = 0.6745 * std::abs(sample.compute.cpuUtilization - median) / (mad + 1e-9);
    return modifiedZ > threshold;
}

// ============================================================================
// TOKEN ECONOMICS & SMART CONTRACT SIMULATION
// ============================================================================
struct TokenReward {
    std::string recipientPubkey;
    double tokenAmount;
    uint64_t timestampMs;
    std::string txHash;
};

TokenReward calculateTokenReward(const TelemetrySample& sample, double baseRewardRate = 0.001) {
    TokenReward reward;
    reward.recipientPubkey = sample.node.pubkey;
    reward.timestampMs = timestampMs();

    double computeContribution = sample.compute.cpuUtilization;
    double efficiencyMultiplier = 1.0 + computeContribution / std::max(0.01, sample.energy.inputPowerW);
    double reputationMultiplier = sample.node.reputationScore;

    reward.tokenAmount = computeContribution * baseRewardRate * efficiencyMultiplier * reputationMultiplier;

    std::ostringstream ss;
    ss << "0x" << std::hash<std::string>{}(sample.node.pubkey + std::to_string(timestampMs()));
    reward.txHash = ss.str();

    return reward;
}

// ============================================================================
// SYSTEM HEALTH & DIAGNOSTICS
// ============================================================================
struct SystemHealth {
    double avgLatency_ms;
    double totalComputePower;
    double networkEfficiency;
    int activeNodes;
    int byzantineNodesDetected;
    double avgPrivacyBudget;
};

SystemHealth analyzeSystemHealth(const std::vector<TelemetrySample>& networkState) {
    SystemHealth health{0,0,0,0,0,0};

    if (networkState.empty()) return health;

    health.activeNodes = networkState.size();
    double totalPower = 0.0;

    for (const auto& sample : networkState) {
        health.avgLatency_ms += sample.compute.latencyMs;
        health.totalComputePower += sample.compute.cpuUtilization;
        health.avgPrivacyBudget += sample.privacy.epsilon;
        totalPower += sample.energy.inputPowerW;
        if (detectByzantineNode(sample, networkState)) health.byzantineNodesDetected++;
    }

    health.avgLatency_ms /= health.activeNodes;
    health.avgPrivacyBudget /= health.activeNodes;
    health.networkEfficiency = totalPower > 0 ? health.totalComputePower / totalPower : 0.0;

    return health;
}

} // namespace ambient
