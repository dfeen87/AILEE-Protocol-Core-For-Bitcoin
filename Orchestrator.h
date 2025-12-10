// SPDX-License-Identifier: MIT
// Orchestrator.h — Production-Grade Task Orchestration for AILEE-Core
// Multi-strategy scheduling, load balancing, resource optimization, geographic awareness,
// reputation management, and economic incentive alignment for distributed workloads.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <limits>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <cstdint>

namespace ailee::sched {

// ==================== NODE CAPABILITIES & METRICS ====================

struct NodeCapabilities {
    bool hasGPU = false;
    bool hasFPGA = false;
    bool hasTPU = false;
    std::size_t memoryGB = 0;
    std::size_t storageGB = 0;
    uint32_t cpuCores = 0;
    double gpuMemoryGB = 0.0;
    std::vector<std::string> supportedArchitectures; // e.g., "x86_64", "arm64"
    std::vector<std::string> runtimeVersions;        // e.g., "CUDA-12.0", "WasmEdge-0.13"
};

struct NodeMetrics {
    // Identity
    std::string peerId;
    std::string region;              // e.g., "us-east", "eu-west"
    std::optional<std::string> zone; // e.g., "us-east-1a"
    
    // Performance metrics
    double latencyMs = std::numeric_limits<double>::infinity();
    double bandwidthMbps = 0.0;
    double jitterMs = 0.0;           // Network stability
    
    // Resource availability
    double cpuUtilization = 0.0;     // 0.0-1.0
    double memoryUtilization = 0.0;  // 0.0-1.0
    double diskUtilization = 0.0;    // 0.0-1.0
    double gpuUtilization = 0.0;     // 0.0-1.0 (if available)
    
    // Capacity & power
    double capacityScore = 0.0;      // Composite capacity signal (0.0-1.0)
    double energyEfficiency = 0.0;   // GFLOPS/Watt
    double carbonIntensity = 0.0;    // gCO2/kWh
    
    // Economic factors
    double costPerHour = 0.0;        // Token cost
    uint64_t tokensAvailable = 0;    // For payment
    double rewardMultiplier = 1.0;   // Dynamic pricing
    
    // Availability & reliability
    std::chrono::system_clock::time_point lastSeen;
    std::chrono::seconds uptime{0};
    double availabilityRate = 1.0;   // Historical uptime (0.0-1.0)
    uint32_t activeTaskCount = 0;
    uint32_t maxConcurrentTasks = 1;
    
    // Security & trust
    bool isVerified = false;         // KYC/verification status
    bool hasZKProofCapability = true;
    std::optional<std::string> attestationHash; // Hardware attestation
    
    // Capabilities
    NodeCapabilities capabilities;
};

// ==================== REPUTATION SYSTEM ====================

struct Reputation {
    std::string peerId;
    uint64_t totalTasks = 0;
    uint64_t successfulTasks = 0;
    uint64_t failedTasks = 0;
    uint64_t timeoutTasks = 0;
    uint64_t byzantineBehaviors = 0;
    
    // Time-weighted metrics
    double recentSuccessRate = 0.0;  // Last 100 tasks
    double allTimeSuccessRate = 0.0;
    
    // Quality metrics
    double avgResponseTime = 0.0;    // Seconds
    double avgProofVerificationTime = 0.0;
    double avgQualityScore = 0.0;    // Task-specific quality
    
    // Economic history
    uint64_t totalRewardsEarned = 0;
    uint64_t totalSlashings = 0;     // Penalties for failures
    
    // Trust score (0.0-1.0)
    double trustScore = 0.5;         // Starts neutral
    std::chrono::system_clock::time_point lastUpdated;
    
    // Compute overall reputation score
    double score() const {
        if (totalTasks == 0) return 0.5; // Neutral for new nodes
        
        // Weighted combination of factors
        double successWeight = 0.4;
        double recentWeight = 0.3;
        double qualityWeight = 0.2;
        double slashingPenalty = 0.1;
        
        double successComponent = static_cast<double>(successfulTasks) / totalTasks;
        double recentComponent = recentSuccessRate;
        double qualityComponent = avgQualityScore;
        double slashingComponent = (totalSlashings > 0) ? 
            (1.0 / (1.0 + std::log10(1.0 + totalSlashings))) : 1.0;
        
        return (successComponent * successWeight +
                recentComponent * recentWeight +
                qualityComponent * qualityWeight) * slashingComponent;
    }
    
    // Decay reputation over time (penalize inactive nodes)
    void decay(std::chrono::seconds timeSinceLastTask, double decayRate = 0.01) {
        double days = timeSinceLastTask.count() / 86400.0;
        trustScore *= std::exp(-decayRate * days);
    }
};

// ==================== TASK SPECIFICATION ====================

enum class TaskPriority {
    LOW,
    NORMAL,
    HIGH,
    CRITICAL
};

enum class TaskType {
    AI_INFERENCE,
    AI_TRAINING,
    FEDERATED_LEARNING,
    WASM_EXECUTION,
    ZK_PROOF_GENERATION,
    DATA_PROCESSING,
    BANDWIDTH_RELAY,
    CUSTOM
};

struct ResourceRequirements {
    uint32_t minCpuCores = 1;
    std::size_t minMemoryGB = 1;
    std::size_t minStorageGB = 1;
    double minBandwidthMbps = 1.0;
    bool requiresGPU = false;
    bool requiresTPU = false;
    double minGpuMemoryGB = 0.0;
    std::vector<std::string> requiredCapabilities;
};

struct TaskPayload {
    // Identity
    std::string taskId;
    TaskType taskType = TaskType::CUSTOM;
    TaskPriority priority = TaskPriority::NORMAL;
    
    // Payload
    std::vector<uint8_t> payloadBytes;   // Opaque job specification
    std::optional<std::string> payloadHash; // Integrity check
    
    // Requirements
    ResourceRequirements requirements;
    
    // Constraints
    std::chrono::milliseconds timeout{30000}; // 30s default
    std::chrono::system_clock::time_point deadline; // Hard deadline
    std::optional<std::string> preferredRegion;     // Geographic preference
    std::vector<std::string> blacklistedNodes;      // Exclude specific nodes
    std::vector<std::string> whitelistedNodes;      // Prefer specific nodes
    
    // Economic parameters
    uint64_t maxCostTokens = 0;          // Budget limit
    bool requireZKProof = true;
    double minReputationScore = 0.5;
    
    // Scheduling hints
    bool allowParallelExecution = false; // Can split across nodes
    uint32_t numParallelWorkers = 1;
    bool requiresLowLatency = false;
    bool requiresHighBandwidth = false;
    bool preferGreenEnergy = false;      // Prefer low carbon intensity
    
    // Retry policy
    uint32_t maxRetries = 3;
    std::chrono::seconds retryBackoff{5};
    
    // Metadata
    std::string submitterId;             // Who submitted the task
    std::chrono::system_clock::time_point submittedAt;
    std::optional<std::string> parentTaskId; // For task dependencies
};

// ==================== ASSIGNMENT RESULT ====================

struct Assignment {
    bool assigned = false;
    std::string reason;                  // Explanation (especially on failure)
    
    // Worker selection
    std::string workerPeerId;
    std::optional<std::string> workerRegion;
    std::optional<std::string> backupWorkerPeerId; // Failover option
    
    // Scoring details
    double finalScore = 0.0;
    double reputationScore = 0.0;
    double latencyScore = 0.0;
    double capacityScore = 0.0;
    double costScore = 0.0;
    
    // Expected performance
    double expectedLatencyMs = 0.0;
    double expectedCostTokens = 0.0;
    std::chrono::milliseconds estimatedCompletionTime{0};
    
    // Assignment metadata
    std::chrono::system_clock::time_point assignedAt;
    std::string assignmentId;            // Unique assignment identifier
    
    // All candidates considered (for debugging)
    std::vector<std::pair<std::string, double>> candidateScores;
};

// ==================== SCHEDULING STRATEGIES ====================

enum class SchedulingStrategy {
    WEIGHTED_SCORE,      // Weighted combination of factors
    ROUND_ROBIN,         // Simple rotation
    LEAST_LOADED,        // Lowest current utilization
    LOWEST_LATENCY,      // Fastest response time
    HIGHEST_REPUTATION,  // Most trusted node
    LOWEST_COST,         // Cheapest option
    GREEDY_BEST,         // Single best metric (configurable)
    GENETIC_ALGORITHM,   // Evolutionary optimization
    REINFORCEMENT_LEARNING, // ML-based scheduling
    LOAD_BALANCING,      // Even distribution
    GEOGRAPHIC_AFFINITY, // Prefer nearby nodes
    CUSTOM               // User-defined strategy
};

// ==================== REPUTATION LEDGER INTERFACE ====================

class IReputationLedger {
public:
    virtual ~IReputationLedger() = default;
    
    // ========== Core Operations ==========
    
    // Get reputation for peer
    virtual Reputation get(const std::string& peerId) const = 0;
    
    // Update reputation (success/failure)
    virtual void update(const std::string& peerId, int deltaSuccess, int deltaFailure) = 0;
    
    // Batch update (for efficiency)
    virtual void updateBatch(const std::vector<std::pair<std::string, std::pair<int, int>>>& updates) = 0;
    
    // ========== Advanced Operations ==========
    
    // Record task completion with quality score
    virtual void recordTaskCompletion(
        const std::string& peerId,
        bool success,
        double qualityScore,
        std::chrono::milliseconds responseTime) = 0;
    
    // Record Byzantine behavior (slash reputation)
    virtual void recordByzantineBehavior(const std::string& peerId, const std::string& reason) = 0;
    
    // Reward node (increase reputation)
    virtual void rewardNode(const std::string& peerId, double reputationBoost) = 0;
    
    // Slash node (decrease reputation + economic penalty)
    virtual void slashNode(const std::string& peerId, double reputationPenalty, uint64_t tokenSlash) = 0;
    
    // ========== Query Operations ==========
    
    // Get top N nodes by reputation
    virtual std::vector<std::string> getTopNodes(std::size_t n) const = 0;
    
    // Get nodes with reputation above threshold
    virtual std::vector<std::string> getNodesAboveThreshold(double threshold) const = 0;
    
    // Get all reputations (for analysis)
    virtual std::unordered_map<std::string, Reputation> getAllReputations() const = 0;
    
    // ========== Maintenance ==========
    
    // Decay inactive node reputations
    virtual void decayInactiveNodes(std::chrono::seconds inactivityThreshold) = 0;
    
    // Reset reputation (for testing or disputes)
    virtual bool resetReputation(const std::string& peerId, const std::string& reason) = 0;
    
    // Export reputation data (for auditing)
    virtual std::vector<std::string> exportReputationLog() const = 0;
};

// ==================== LATENCY MAP INTERFACE ====================

class ILatencyMap {
public:
    virtual ~ILatencyMap() = default;
    
    // Get latency to peer
    virtual std::optional<double> getLatencyMs(const std::string& peerId) const = 0;
    
    // Update latency measurement
    virtual void updateLatency(const std::string& peerId, double latencyMs) = 0;
    
    // Get bandwidth to peer
    virtual std::optional<double> getBandwidthMbps(const std::string& peerId) const = 0;
    
    // Get network jitter (stability metric)
    virtual std::optional<double> getJitterMs(const std::string& peerId) const = 0;
    
    // Probe latency actively (ping)
    virtual std::optional<double> probeLatency(const std::string& peerId) = 0;
    
    // Get geographic distance estimate
    virtual std::optional<double> getDistanceKm(const std::string& peerId) const = 0;
    
    // Get all latency measurements
    virtual std::unordered_map<std::string, double> getAllLatencies() const = 0;
    
    // Cleanup stale measurements
    virtual void cleanupStale(std::chrono::seconds maxAge) = 0;
};

// ==================== ORCHESTRATOR INTERFACE ====================

class IOrchestrator {
public:
    virtual ~IOrchestrator() = default;
    
    // ========== Core Scheduling ==========
    
    // Assign best worker for task using weighted scoring
    virtual Assignment assignBestWorker(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        double trustWeight = 0.6,
        double speedWeight = 0.3,
        double powerWeight = 0.1) const = 0;
    
    // Assign using specific strategy
    virtual Assignment assignWithStrategy(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        SchedulingStrategy strategy) const = 0;
    
    // Assign to multiple workers (parallel execution)
    virtual std::vector<Assignment> assignParallel(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        uint32_t numWorkers) const = 0;
    
    // ========== Advanced Scheduling ==========
    
    // Schedule batch of tasks (optimize globally)
    virtual std::vector<Assignment> scheduleBatch(
        const std::vector<TaskPayload>& tasks,
        const std::vector<NodeMetrics>& candidates) const = 0;
    
    // Find backup worker (failover)
    virtual std::optional<Assignment> findBackupWorker(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        const std::string& excludePeerId) const = 0;
    
    // Rebalance load across nodes
    virtual std::vector<std::pair<std::string, std::string>> rebalanceTasks(
        const std::vector<std::pair<std::string, std::string>>& currentAssignments,
        const std::vector<NodeMetrics>& candidates) const = 0;
    
    // ========== Worker Pool Management ==========
    
    // Filter candidates by requirements
    virtual std::vector<NodeMetrics> filterCandidates(
        const std::vector<NodeMetrics>& candidates,
        const TaskPayload& task) const = 0;
    
    // Score single node for task
    virtual double scoreNode(
        const NodeMetrics& node,
        const TaskPayload& task,
        double trustWeight,
        double speedWeight,
        double powerWeight) const = 0;
    
    // Rank all candidates
    virtual std::vector<std::pair<std::string, double>> rankCandidates(
        const std::vector<NodeMetrics>& candidates,
        const TaskPayload& task) const = 0;
    
    // ========== Cost Optimization ==========
    
    // Find cheapest worker meeting requirements
    virtual std::optional<Assignment> findCheapestWorker(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates) const = 0;
    
    // Estimate task cost
    virtual uint64_t estimateCost(
        const TaskPayload& task,
        const NodeMetrics& worker) const = 0;
    
    // Optimize for cost-performance tradeoff
    virtual Assignment optimizeCostPerformance(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        double costWeight = 0.5) const = 0;
    
    // ========== Configuration ==========
    
    // Set scheduling strategy
    virtual void setStrategy(SchedulingStrategy strategy) = 0;
    
    // Get current strategy
    virtual SchedulingStrategy getStrategy() const = 0;
    
    // Set custom scoring function
    using ScoringFunction = std::function<double(const NodeMetrics&, const TaskPayload&)>;
    virtual void setCustomScoring(ScoringFunction scorer) = 0;
    
    // ========== Telemetry ==========
    
    struct OrchestratorMetrics {
        uint64_t totalAssignments = 0;
        uint64_t successfulAssignments = 0;
        uint64_t failedAssignments = 0;
        std::chrono::milliseconds avgAssignmentTime{0};
        std::unordered_map<std::string, uint64_t> assignmentsByWorker;
        std::unordered_map<std::string, uint64_t> assignmentsByStrategy;
    };
    virtual OrchestratorMetrics getMetrics() const = 0;
    virtual void resetMetrics() = 0;
};

// ==================== DEFAULT WEIGHTED ORCHESTRATOR ====================

class WeightedOrchestrator final : public IOrchestrator {
public:
    explicit WeightedOrchestrator(
        const IReputationLedger& rep,
        const ILatencyMap& lat)
        : rep_(rep), lat_(lat), strategy_(SchedulingStrategy::WEIGHTED_SCORE) {}
    
    // ========== Core Implementation ==========
    
    Assignment assignBestWorker(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        double trustWeight = 0.6,
        double speedWeight = 0.3,
        double powerWeight = 0.1) const override;
    
    Assignment assignWithStrategy(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        SchedulingStrategy strategy) const override;
    
    std::vector<Assignment> assignParallel(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        uint32_t numWorkers) const override;
    
    std::vector<Assignment> scheduleBatch(
        const std::vector<TaskPayload>& tasks,
        const std::vector<NodeMetrics>& candidates) const override;
    
    std::optional<Assignment> findBackupWorker(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        const std::string& excludePeerId) const override;
    
    std::vector<std::pair<std::string, std::string>> rebalanceTasks(
        const std::vector<std::pair<std::string, std::string>>& currentAssignments,
        const std::vector<NodeMetrics>& candidates) const override;
    
    // ========== Worker Pool Management ==========
    
    std::vector<NodeMetrics> filterCandidates(
        const std::vector<NodeMetrics>& candidates,
        const TaskPayload& task) const override;
    
    double scoreNode(
        const NodeMetrics& node,
        const TaskPayload& task,
        double trustWeight,
        double speedWeight,
        double powerWeight) const override;
    
    std::vector<std::pair<std::string, double>> rankCandidates(
        const std::vector<NodeMetrics>& candidates,
        const TaskPayload& task) const override;
    
    // ========== Cost Optimization ==========
    
    std::optional<Assignment> findCheapestWorker(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates) const override;
    
    uint64_t estimateCost(
        const TaskPayload& task,
        const NodeMetrics& worker) const override;
    
    Assignment optimizeCostPerformance(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates,
        double costWeight = 0.5) const override;
    
    // ========== Configuration ==========
    
    void setStrategy(SchedulingStrategy strategy) override { strategy_ = strategy; }
    SchedulingStrategy getStrategy() const override { return strategy_; }
    void setCustomScoring(ScoringFunction scorer) override { customScorer_ = scorer; }
    
    // ========== Telemetry ==========
    
    OrchestratorMetrics getMetrics() const override { return metrics_; }
    void resetMetrics() override { metrics_ = OrchestratorMetrics{}; }
    
private:
    const IReputationLedger& rep_;
    const ILatencyMap& lat_;
    SchedulingStrategy strategy_;
    std::optional<ScoringFunction> customScorer_;
    mutable OrchestratorMetrics metrics_;
    
    // Helper methods
    Assignment assignRoundRobin(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignLeastLoaded(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignLowestLatency(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignHighestReputation(
        const std::vector<NodeMetrics>& candidates) const;
    Assignment assignLowestCost(const std::vector<NodeMetrics>& candidates) const;
    Assignment assignGeneticAlgorithm(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates) const;
};

// ==================== ADVANCED SCHEDULERS ====================

// Machine learning-based scheduler (learns from historical data)
class ReinforcementLearningOrchestrator : public IOrchestrator {
public:
    // Train on historical assignments
    void train(const std::vector<std::pair<Assignment, double>>& history);
    
    // Predict best assignment
    Assignment predict(const TaskPayload& task,
                       const std::vector<NodeMetrics>& candidates);
};

// Multi-objective optimizer (Pareto frontier)
class MultiObjectiveOrchestrator : public IOrchestrator {
public:
    struct Objectives {
        double costWeight = 0.25;
        double latencyWeight = 0.25;
        double reputationWeight = 0.25;
        double energyWeight = 0.25;
    };
    
    void setObjectives(const Objectives& obj) { objectives_ = obj; }
    
    // Find Pareto-optimal assignments
    std::vector<Assignment> findParetoFrontier(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates) const;
    
private:
    Objectives objectives_;
};

// ==================== UTILITY FUNCTIONS ====================

namespace utils {
    // Normalize score to [0, 1] range
    double normalizeScore(double value, double min, double max);
    
    // Compute geographic distance (Haversine)
    double computeDistance(const std::string& region1, const std::string& region2);
    
    // Estimate task completion time
    std::chrono::milliseconds estimateCompletionTime(
        const TaskPayload& task,
        const NodeMetrics& node);
    
    // Check if node meets requirements
    bool meetsRequirements(const NodeMetrics& node,
                           const ResourceRequirements& req);
    
    // Compute load balance score (lower = more balanced)
    double computeLoadBalance(const std::vector<NodeMetrics>& nodes);
    
    // Select diverse set of nodes (avoid single point of failure)
    std::vector<std::string> selectDiverseNodes(
        const std::vector<NodeMetrics>& candidates,
        std::size_t count);
}

// ==================== INTEGRATION WITH AILEE-CORE ====================

namespace integration {
    // Create orchestrator for AmbientAI tasks
    std::unique_ptr<IOrchestrator> createAmbientAIOrchestrator(
        const IReputationLedger& rep,
        const ILatencyMap& lat);
    
    // Schedule federated learning tasks
    std::vector<Assignment> scheduleFederatedLearning(
        const std::vector<TaskPayload>& tasks,
        const std::vector<NodeMetrics>& candidates,
        std::size_t minParticipants);
    
    // Optimize for green energy (low carbon)
    Assignment scheduleGreenTask(
        const TaskPayload& task,
        const std::vector<NodeMetrics>& candidates);
}

} // namespace ailee::sched
