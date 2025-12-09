// SPDX-License-Identifier: MIT
// AmbientAI.h — Ambient energy/compute interfaces for AILEE-Core
// Maps core white paper concepts into practical C++ surfaces.

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <cstdint>
#include <atomic>
#include <functional>
#include <mutex>

namespace ambient {

// --------- Core data models ---------

struct NodeId {
    std::string pubkey;         // Verifiable identity (placeholder)
    std::string region;         // Geo/cluster tag
    std::string deviceClass;    // e.g., "smartphone", "gateway", "miner"
};

struct EnergyProfile {
    double inputPowerW = 0.0;           // Instantaneous input power
    double wasteHeatRecoveredW = 0.0;   // Recovered thermal energy
    double temperatureC = 0.0;          // Device temperature
    double ambientTempC = 0.0;          // Environment temperature
    double carbonIntensity_gCO2_kWh = 0.0; // Optional: grid intensity
};

struct ComputeProfile {
    double cpuUtilization = 0.0;   // %
    double npuUtilization = 0.0;   // %
    double gpuUtilization = 0.0;   // %
    double availableMemMB = 0.0;
    double bandwidthMbps = 0.0;
    double latencyMs = 0.0;
};

struct PrivacyBudget {
    double epsilon = 1.0; // Differential privacy budget (placeholder)
    double delta  = 1e-5;
};

struct TelemetrySample {
    NodeId node;
    EnergyProfile energy;
    ComputeProfile compute;
    std::chrono::system_clock::time_point timestamp;
    PrivacyBudget privacy;
};

// --------- Federated learning & verifiability stubs ---------

struct FederatedUpdate {
    std::string modelId;
    std::vector<float> gradient;     // Quantized/aggregated gradients (placeholder)
    PrivacyBudget privacy;
    // Optional: encrypted payloads via HE in future
};

struct ZKProofStub {
    std::string proofHash;           // Placeholder for zk-SNARK/STARK proof artifact
    std::string circuitId;           // Which circuit validated the computation
    bool verified = false;
};

// --------- Token incentives & reputation ---------

struct IncentiveRecord {
    std::string taskId;
    NodeId node;
    double rewardTokens = 0.0;
    bool distributed = false;
};

struct Reputation {
    NodeId node;
    double score = 0.0;           // Composite score: reliability, timeliness, correctness
    uint64_t completedTasks = 0;
    uint64_t disputes = 0;
};

// --------- Safety/circuit-breaker policy ---------

struct SafetyPolicy {
    double maxTemperatureC = 80.0;
    double maxLatencyMs    = 300.0;
    double maxBlockMB      = 8.0;
    int    maxErrorCount   = 25;
};

// --------- Ambient energy/compute node ---------

class AmbientNode {
public:
    explicit AmbientNode(NodeId id, SafetyPolicy policy)
        : id_(std::move(id)), policy_(policy) {}

    // Ingest telemetry (local, privacy-preserving)
    void ingestTelemetry(const TelemetrySample& sample) {
        std::lock_guard<std::mutex> lock(mu_);
        lastSample_ = sample;
        // Simple safety checks; advanced logic can call AILEE CircuitBreaker
        if (sample.energy.temperatureC > policy_.maxTemperatureC ||
            sample.compute.latencyMs > policy_.maxLatencyMs) {
            safeMode_.store(true);
        } else {
            safeMode_.store(false);
        }
    }

    // Local federated step (placeholder)
    FederatedUpdate runLocalTraining(const std::string& modelId,
                                     const std::vector<float>& miniBatch) {
        std::lock_guard<std::mutex> lock(mu_);
        FederatedUpdate up;
        up.modelId = modelId;
        up.privacy = lastSample_.privacy;
        // Toy transform: sum minibatch for gradient magnitude
        float sum = 0.0f;
        for (auto v : miniBatch) sum += v;
        up.gradient = {sum}; // Replace with real local train step
        return up;
    }

    // Verifiable compute stub
    ZKProofStub verifyComputation(const std::string& taskId,
                                  const std::string& circuitId,
                                  const std::string& resultHash) {
        ZKProofStub p;
        p.circuitId = circuitId;
        p.proofHash = "zk_" + taskId + "_" + resultHash.substr(0, 16);
        p.verified  = true; // Replace with real ZK verifier integration
        return p;
    }

    // Incentive accounting
    IncentiveRecord accrueReward(const std::string& taskId, double tokens) const {
        IncentiveRecord rec;
        rec.taskId = taskId;
        rec.node = id_;
        rec.rewardTokens = tokens;
        rec.distributed = false;
        return rec;
    }

    // Reputation updates
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

    // Safe-mode toggle
    bool isSafeMode() const { return safeMode_.load(); }

    // Accessors
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

// --------- Mesh coordination (cluster-level orchestration) ---------

class MeshCoordinator {
public:
    using TaskFn = std::function<double(const AmbientNode&)>; // returns reward

    explicit MeshCoordinator(std::string clusterId)
        : clusterId_(std::move(clusterId)) {}

    void registerNode(AmbientNode* node) {
        std::lock_guard<std::mutex> lock(mu_);
        nodes_.push_back(node);
    }

    // Simple selection: prefer low latency, high bandwidth, not in safe mode
    AmbientNode* selectNodeForTask() {
        std::lock_guard<std::mutex> lock(mu_);
        AmbientNode* best = nullptr;
        double bestScore = -1.0;
        for (auto* n : nodes_) {
            auto last = n->last();
            if (!last.has_value()) continue;
            if (n->isSafeMode()) continue;
            double score = (last->compute.bandwidthMbps) - (last->compute.latencyMs * 0.1);
            if (score > bestScore) { bestScore = score; best = n; }
        }
        return best;
    }

    // Execute a task on selected node and accrue reward
    IncentiveRecord dispatchAndReward(const std::string& taskId, TaskFn fn, double baseRewardTokens) {
        AmbientNode* n = selectNodeForTask();
        if (!n) {
            return IncentiveRecord{taskId, NodeId{"", "", ""}, 0.0, false};
        }
        double multiplier = fn(*n); // e.g., performance-based multiplier
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
