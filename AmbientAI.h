// SPDX-License-Identifier: MIT
// AmbientAI.h — Ambient energy/compute interfaces for AILEE-Core
// Fully integrates telemetry history, ZK proofs, federated learning, safety policies, token incentives, and cluster orchestration.

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <optional>
#include <cstdint>
#include <atomic>
#include <functional>
#include <mutex>
#include <numeric>

#include "zk_proofs.h"

namespace ambient {

// ================= Core Data Models =================

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
    double delta  = 1e-5;
    double privacyBudgetRemaining = 1.0;
    bool homomorphicEncryptionEnabled = false;
    bool zeroKnowledgeProofEnabled = false;
};

struct TelemetrySample {
    NodeId node;
    EnergyProfile energy;
    ComputeProfile compute;
    std::chrono::system_clock::time_point timestamp;
    PrivacyBudget privacy;
};

// ================= Telemetry History =================

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
        for (const auto& s : history)
            sum += s.compute.cpuUtilization + s.compute.npuUtilization + s.compute.gpuUtilization;
        return sum / history.size();
    }

    double avgEnergyEfficiency() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history)
            sum += s.energy.inputPowerW > 0 ? s.compute.cpuUtilization / s.energy.inputPowerW : 0.0;
        return sum / history.size();
    }

    double avgPrivacyBudget() const {
        if (history.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& s : history) sum += s.privacy.epsilon;
        return sum / history.size();
    }
};

// ================= Federated Learning =================

struct FederatedUpdate {
    std::string modelId;
    std::vector<float> gradient;
    PrivacyBudget privacy;
};

// ================= ZK Proof Integration =================

struct ZKProofStub {
    std::string proofHash;
    std::string circuitId;
    bool verified = false;
    uint64_t timestampMs = 0;
};

// ================= Token Incentives & Reputation =================

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

// ================= Safety / Circuit-Breaker Policy =================

struct SafetyPolicy {
    double maxTemperatureC = 80.0;
    double maxLatencyMs    = 300.0;
    double maxBlockMB      = 8.0;
    int    maxErrorCount   = 25;
};

// ================= AmbientNode Class =================

class AmbientNode {
public:
    explicit AmbientNode(NodeId id, SafetyPolicy policy)
        : id_(std::move(id)), policy_(policy) {}

    void ingestTelemetry(const TelemetrySample& sample) {
        std::lock_guard<std::mutex> lock(mu_);
        lastSample_ = sample;
        history_.addSample(sample);

        safeMode_.store(sample.energy.temperatureC > policy_.maxTemperatureC ||
                        sample.compute.latencyMs > policy_.maxLatencyMs);

        ailee::zk::ZKEngine zkEngine;
        auto proof = zkEngine.generateProof(id_.pubkey + std::to_string(timestampMs()),
                                            std::to_string(sample.compute.cpuUtilization));
        lastProof_ = { "telemetry_circuit", proof.proofData, zkEngine.verifyProof(proof), proof.timestampMs };
    }

    FederatedUpdate runLocalTraining(const std::string& modelId, const std::vector<float>& miniBatch) {
        std::lock_guard<std::mutex> lock(mu_);
        FederatedUpdate up;
        up.modelId = modelId;
        up.privacy = lastSample_.privacy;

        float sum = std::accumulate(miniBatch.begin(), miniBatch.end(), 0.0f);
        up.gradient = {sum};
        return up;
    }

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
        lastProof_ = p;
        return p;
    }

    IncentiveRecord accrueReward(const std::string& taskId, double tokens) const {
        IncentiveRecord rec{taskId, id_, tokens, false};
        return rec;
    }

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
    std::optional<ZKProofStub> lastProof() const {
        std::lock_guard<std::mutex> lock(mu_);
        return lastProof_;
    }
    NodeTelemetryHistory getHistory() const { return history_; }

private:
    NodeId id_;
    SafetyPolicy policy_;
    mutable std::mutex mu_;
    std::optional<TelemetrySample> lastSample_;
    ZKProofStub lastProof_;
    NodeTelemetryHistory history_;
    Reputation rep_{id_, 0.0, 0, 0};
    std::atomic<bool> safeMode_{false};

    static inline uint64_t timestampMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
};

// ================= MeshCoordinator Class =================

class MeshCoordinator {
public:
    using TaskFn = std::function<double(const AmbientNode&)>;

    explicit MeshCoordinator(std::string clusterId)
        : clusterId_(std::move(clusterId)) {}

    void registerNode(AmbientNode* node) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_.push_back(node);
    }

    AmbientNode* selectNodeForTask(bool requireValidProof = true) {
        std::lock_guard<std::mutex> lock(mu_);
        AmbientNode* best = nullptr;
        double bestScore = -1.0;

        for (auto* n : nodes_) {
            auto last = n->last();
            if (!last.has_value()) continue;
            if (n->isSafeMode()) continue;

            if (requireValidProof) {
                auto proof = n->lastProof();
                if (!proof.has_value() || !proof->verified) continue;
            }

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

} // namespace ambient

