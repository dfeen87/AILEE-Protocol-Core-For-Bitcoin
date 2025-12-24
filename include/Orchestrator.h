// SPDX-License-Identifier: MIT
// Engine.h — Unified Orchestration Engine for AILEE-Core
// Integrates reputation, latency tracking, node discovery, and task orchestration
// into a cohesive runtime system.

#pragma once

#include "Orchestrator.h"
#include <memory>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <future>

namespace ailee::sched {

// ==================== CONFIGURATION ====================

struct NetworkConfig {
    std::string listenAddress = "0.0.0.0";
    uint16_t listenPort = 8080;
    std::size_t maxConnections = 1000;
    std::chrono::seconds connectionTimeout{30};
    std::chrono::seconds heartbeatInterval{10};
    bool enableTLS = true;
    std::string tlsCertPath;
    std::string tlsKeyPath;
};

struct PerformanceConfig {
    SchedulingStrategy defaultStrategy = SchedulingStrategy::WEIGHTED_SCORE;
    uint32_t maxConcurrentTasks = 100;
    uint32_t workerThreads = 4;
    std::chrono::milliseconds taskTimeout{30000};
    double trustWeight = 0.6;
    double speedWeight = 0.3;
    double powerWeight = 0.1;
    bool enableAdaptiveScheduling = true;
};

struct EconomicConfig {
    uint64_t defaultMaxCostTokens = 1000;
    double minReputationThreshold = 0.5;
    bool enableDynamicPricing = true;
    double priceAdjustmentRate = 0.1;
    uint64_t slashingPenalty = 100;
    double reputationDecayRate = 0.01;
};

struct MonitoringConfig {
    bool enableMetrics = true;
    bool enableLogging = true;
    std::string metricsEndpoint = "localhost:9090";
    std::chrono::seconds metricsInterval{60};
    std::string logLevel = "INFO";
    std::string logPath = "./logs/engine.log";
};

struct Config {
    NetworkConfig network;
    PerformanceConfig performance;
    EconomicConfig economic;
    MonitoringConfig monitoring;
    
    // Feature flags
    bool enableZKProofs = true;
    bool enableFederatedLearning = true;
    bool enableGreenScheduling = false;
    bool enableLoadRebalancing = true;
    
    // Discovery settings
    std::vector<std::string> bootstrapPeers;
    std::chrono::seconds discoveryInterval{30};
    std::size_t minPeerCount = 3;
};

// ==================== CONCRETE IMPLEMENTATIONS ====================

class ReputationLedger final : public IReputationLedger {
public:
    ReputationLedger() = default;
    
    Reputation get(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = reputations_.find(peerId);
        if (it != reputations_.end()) {
            return it->second;
        }
        // Return new neutral reputation
        Reputation rep;
        rep.peerId = peerId;
        rep.trustScore = 0.5;
        return rep;
    }
    
    void update(const std::string& peerId, int deltaSuccess, int deltaFailure) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.peerId = peerId;
        rep.totalTasks += deltaSuccess + deltaFailure;
        rep.successfulTasks += deltaSuccess;
        rep.failedTasks += deltaFailure;
        rep.lastUpdated = std::chrono::system_clock::now();
        
        // Update all-time success rate
        if (rep.totalTasks > 0) {
            rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / rep.totalTasks;
        }
        
        // Update trust score
        rep.trustScore = rep.score();
    }
    
    void updateBatch(const std::vector<std::pair<std::string, std::pair<int, int>>>& updates) override {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [peerId, deltas] : updates) {
            auto& rep = reputations_[peerId];
            rep.peerId = peerId;
            rep.totalTasks += deltas.first + deltas.second;
            rep.successfulTasks += deltas.first;
            rep.failedTasks += deltas.second;
            rep.lastUpdated = std::chrono::system_clock::now();
            
            if (rep.totalTasks > 0) {
                rep.allTimeSuccessRate = static_cast<double>(rep.successfulTasks) / rep.totalTasks;
            }
            rep.trustScore = rep.score();
        }
    }
    
    void recordTaskCompletion(const std::string& peerId, bool success, 
                            double qualityScore, std::chrono::milliseconds responseTime) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.peerId = peerId;
        
        if (success) {
            rep.successfulTasks++;
        } else {
            rep.failedTasks++;
        }
        rep.totalTasks++;
        
        // Update quality metrics
        double alpha = 0.1; // Exponential moving average factor
        rep.avgQualityScore = rep.avgQualityScore * (1.0 - alpha) + qualityScore * alpha;
        rep.avgResponseTime = rep.avgResponseTime * (1.0 - alpha) + 
                             responseTime.count() / 1000.0 * alpha;
        
        rep.lastUpdated = std::chrono::system_clock::now();
        rep.trustScore = rep.score();
    }
    
    void recordByzantineBehavior(const std::string& peerId, const std::string& reason) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.byzantineBehaviors++;
        rep.trustScore *= 0.5; // Severe penalty
        rep.lastUpdated = std::chrono::system_clock::now();
    }
    
    void rewardNode(const std::string& peerId, double reputationBoost) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.trustScore = std::min(1.0, rep.trustScore + reputationBoost);
        rep.lastUpdated = std::chrono::system_clock::now();
    }
    
    void slashNode(const std::string& peerId, double reputationPenalty, uint64_t tokenSlash) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rep = reputations_[peerId];
        rep.trustScore = std::max(0.0, rep.trustScore - reputationPenalty);
        rep.totalSlashings += tokenSlash;
        rep.lastUpdated = std::chrono::system_clock::now();
    }
    
    std::vector<std::string> getTopNodes(std::size_t n) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<std::string, double>> scored;
        for (const auto& [peerId, rep] : reputations_) {
            scored.emplace_back(peerId, rep.score());
        }
        std::sort(scored.begin(), scored.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<std::string> result;
        for (std::size_t i = 0; i < std::min(n, scored.size()); ++i) {
            result.push_back(scored[i].first);
        }
        return result;
    }
    
    std::vector<std::string> getNodesAboveThreshold(double threshold) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> result;
        for (const auto& [peerId, rep] : reputations_) {
            if (rep.score() >= threshold) {
                result.push_back(peerId);
            }
        }
        return result;
    }
    
    std::unordered_map<std::string, Reputation> getAllReputations() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return reputations_;
    }
    
    void decayInactiveNodes(std::chrono::seconds inactivityThreshold) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        for (auto& [peerId, rep] : reputations_) {
            auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::seconds>(
                now - rep.lastUpdated);
            if (timeSinceUpdate > inactivityThreshold) {
                rep.decay(timeSinceUpdate);
            }
        }
    }
    
    bool resetReputation(const std::string& peerId, const std::string& reason) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = reputations_.find(peerId);
        if (it != reputations_.end()) {
            it->second = Reputation{};
            it->second.peerId = peerId;
            it->second.trustScore = 0.5;
            return true;
        }
        return false;
    }
    
    std::vector<std::string> exportReputationLog() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> log;
        for (const auto& [peerId, rep] : reputations_) {
            log.push_back("PeerID: " + peerId + ", Score: " + std::to_string(rep.score()) +
                         ", Tasks: " + std::to_string(rep.totalTasks) +
                         ", Success: " + std::to_string(rep.successfulTasks));
        }
        return log;
    }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Reputation> reputations_;
};

class LatencyMap final : public ILatencyMap {
public:
    LatencyMap() = default;
    
    std::optional<double> getLatencyMs(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = latencies_.find(peerId);
        if (it != latencies_.end()) {
            return it->second.latencyMs;
        }
        return std::nullopt;
    }
    
    void updateLatency(const std::string& peerId, double latencyMs) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& entry = latencies_[peerId];
        entry.latencyMs = latencyMs;
        entry.lastUpdated = std::chrono::system_clock::now();
    }
    
    std::optional<double> getBandwidthMbps(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = latencies_.find(peerId);
        if (it != latencies_.end()) {
            return it->second.bandwidthMbps;
        }
        return std::nullopt;
    }
    
    std::optional<double> getJitterMs(const std::string& peerId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = latencies_.find(peerId);
        if (it != latencies_.end()) {
            return it->second.jitterMs;
        }
        return std::nullopt;
    }
    
    std::optional<double> probeLatency(const std::string& peerId) override {
        // In production, this would send actual network probes
        // For now, return cached value or simulate
        auto cached = getLatencyMs(peerId);
        if (cached) return cached;
        
        // Simulate probe (would be real network ping in production)
        double simulatedLatency = 50.0 + (rand() % 100);
        updateLatency(peerId, simulatedLatency);
        return simulatedLatency;
    }
    
    std::optional<double> getDistanceKm(const std::string& peerId) const override {
        // Would use geographic coordinates in production
        return std::nullopt;
    }
    
    std::unordered_map<std::string, double> getAllLatencies() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_map<std::string, double> result;
        for (const auto& [peerId, entry] : latencies_) {
            result[peerId] = entry.latencyMs;
        }
        return result;
    }
    
    void cleanupStale(std::chrono::seconds maxAge) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::system_clock::now();
        for (auto it = latencies_.begin(); it != latencies_.end();) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.lastUpdated);
            if (age > maxAge) {
                it = latencies_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
private:
    struct LatencyEntry {
        double latencyMs = 0.0;
        double bandwidthMbps = 0.0;
        double jitterMs = 0.0;
        std::chrono::system_clock::time_point lastUpdated;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, LatencyEntry> latencies_;
};

// ==================== TASK QUEUE ====================

class TaskQueue {
public:
    void enqueue(const TaskPayload& task, TaskPriority priority = TaskPriority::NORMAL) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (priority == TaskPriority::CRITICAL) {
            criticalQueue_.push(task);
        } else if (priority == TaskPriority::HIGH) {
            highQueue_.push(task);
        } else if (priority == TaskPriority::NORMAL) {
            normalQueue_.push(task);
        } else {
            lowQueue_.push(task);
        }
        cv_.notify_one();
    }
    
    std::optional<TaskPayload> dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return !criticalQueue_.empty() || !highQueue_.empty() ||
                   !normalQueue_.empty() || !lowQueue_.empty() || shutdown_;
        });
        
        if (shutdown_) return std::nullopt;
        
        if (!criticalQueue_.empty()) {
            auto task = criticalQueue_.front();
            criticalQueue_.pop();
            return task;
        }
        if (!highQueue_.empty()) {
            auto task = highQueue_.front();
            highQueue_.pop();
            return task;
        }
        if (!normalQueue_.empty()) {
            auto task = normalQueue_.front();
            normalQueue_.pop();
            return task;
        }
        if (!lowQueue_.empty()) {
            auto task = lowQueue_.front();
            lowQueue_.pop();
            return task;
        }
        
        return std::nullopt;
    }
    
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return criticalQueue_.size() + highQueue_.size() + 
               normalQueue_.size() + lowQueue_.size();
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        cv_.notify_all();
    }
    
private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<TaskPayload> criticalQueue_;
    std::queue<TaskPayload> highQueue_;
    std::queue<TaskPayload> normalQueue_;
    std::queue<TaskPayload> lowQueue_;
    bool shutdown_ = false;
};

// ==================== ENGINE ====================

class Engine {
public:
    explicit Engine(const Config& config)
        : config_(config),
          repLedger_(),
          latencyMap_(),
          orchestrator_(repLedger_, latencyMap_),
          running_(false)
    {
        orchestrator_.setStrategy(config_.performance.defaultStrategy);
    }
    
    ~Engine() {
        stop();
    }
    
    // ========== Lifecycle ==========
    
    void start() {
        if (running_.exchange(true)) {
            return; // Already running
        }
        
        // Start worker threads
        for (uint32_t i = 0; i < config_.performance.workerThreads; ++i) {
            workerThreads_.emplace_back(&Engine::workerLoop, this);
        }
        
        // Start discovery thread
        discoveryThread_ = std::thread(&Engine::discoveryLoop, this);
        
        // Start monitoring thread
        if (config_.monitoring.enableMetrics) {
            monitoringThread_ = std::thread(&Engine::monitoringLoop, this);
        }
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }
        
        taskQueue_.shutdown();
        
        // Join worker threads
        for (auto& thread : workerThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        workerThreads_.clear();
        
        // Join discovery thread
        if (discoveryThread_.joinable()) {
            discoveryThread_.join();
        }
        
        // Join monitoring thread
        if (monitoringThread_.joinable()) {
            monitoringThread_.join();
        }

        failAllPending("Engine stopped before assignment");
    }
    
    bool isRunning() const {
        return running_.load();
    }
    
    // ========== Task Submission ==========
    
    std::future<Assignment> submitTask(const TaskPayload& task) {
        auto promise = std::make_shared<std::promise<Assignment>>();
        auto future = promise->get_future();
        
        taskQueue_.enqueue(task, task.priority);
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pendingPromises_[task.taskId] = promise;
        }
        
        return future;
    }
    
    void runTask(const TaskPayload& task) {
        auto candidates = discoverNodes();
        
        if (candidates.empty()) {
            // No nodes available
            totalTasksFailed_++;
            resolvePromise(task.taskId,
                           makeFailureAssignment(task, "No available nodes for task"));
            return;
        }
        
        auto assignment = orchestrator_.assignBestWorker(
            task, candidates,
            config_.performance.trustWeight,
            config_.performance.speedWeight,
            config_.performance.powerWeight
        );
        
        if (assignment.assigned) {
            dispatch(assignment, task);
        } else {
            totalTasksFailed_++;
            resolvePromise(task.taskId, assignment);
        }
    }
    
    // ========== Node Management ==========
    
    void registerNode(const NodeMetrics& node) {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        knownNodes_[node.peerId] = node;
    }
    
    void unregisterNode(const std::string& peerId) {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        knownNodes_.erase(peerId);
    }
    
    std::vector<NodeMetrics> getNodes() const {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        std::vector<NodeMetrics> nodes;
        nodes.reserve(knownNodes_.size());
        for (const auto& [id, node] : knownNodes_) {
            nodes.push_back(node);
        }
        return nodes;
    }
    
    // ========== Configuration ==========
    
    void updateConfig(const Config& config) {
        config_ = config;
        orchestrator_.setStrategy(config_.performance.defaultStrategy);
    }
    
    const Config& getConfig() const {
        return config_;
    }
    
    // ========== Telemetry ==========
    
    struct EngineMetrics {
        uint64_t totalTasksSubmitted = 0;
        uint64_t totalTasksCompleted = 0;
        uint64_t totalTasksFailed = 0;
        uint64_t activeNodes = 0;
        std::size_t queuedTasks = 0;
        std::chrono::milliseconds avgTaskLatency{0};
        IOrchestrator::OrchestratorMetrics orchestratorMetrics;
    };
    
    EngineMetrics getMetrics() const {
        EngineMetrics metrics;
        metrics.totalTasksSubmitted = totalTasksSubmitted_.load();
        metrics.totalTasksCompleted = totalTasksCompleted_.load();
        metrics.totalTasksFailed = totalTasksFailed_.load();
        metrics.queuedTasks = taskQueue_.size();
        metrics.orchestratorMetrics = orchestrator_.getMetrics();
        
        std::lock_guard<std::mutex> lock(nodesMutex_);
        metrics.activeNodes = knownNodes_.size();
        
        return metrics;
    }
    
    // ========== Access to Components ==========
    
    IReputationLedger& getReputationLedger() { return repLedger_; }
    ILatencyMap& getLatencyMap() { return latencyMap_; }
    IOrchestrator& getOrchestrator() { return orchestrator_; }
    
private:
    Config config_;
    ReputationLedger repLedger_;
    LatencyMap latencyMap_;
    WeightedOrchestrator orchestrator_;
    
    std::atomic<bool> running_;
    TaskQueue taskQueue_;
    
    mutable std::mutex nodesMutex_;
    std::unordered_map<std::string, NodeMetrics> knownNodes_;
    
    std::vector<std::thread> workerThreads_;
    std::thread discoveryThread_;
    std::thread monitoringThread_;
    
    std::unordered_map<std::string, std::shared_ptr<std::promise<Assignment>>> pendingPromises_;
    mutable std::mutex pendingMutex_;
    
    std::atomic<uint64_t> totalTasksSubmitted_{0};
    std::atomic<uint64_t> totalTasksCompleted_{0};
    std::atomic<uint64_t> totalTasksFailed_{0};
    
    // ========== Worker Loop ==========
    
    void workerLoop() {
        while (running_.load()) {
            auto taskOpt = taskQueue_.dequeue();
            if (!taskOpt) break; // Shutdown
            
            totalTasksSubmitted_++;
            
            try {
                runTask(*taskOpt);
            } catch (const std::exception& e) {
                totalTasksFailed_++;
                resolvePromise(taskOpt->taskId,
                               makeFailureAssignment(*taskOpt,
                                                    "Task processing error: " + std::string(e.what())));
            }
        }
    }
    
    // ========== Discovery Loop ==========
    
    void discoveryLoop() {
        while (running_.load()) {
            try {
                // Discover new nodes (would use libp2p or similar in production)
                auto newNodes = performDiscovery();
                
                for (const auto& node : newNodes) {
                    registerNode(node);
                }
                
                // Update latency measurements
                updateLatencies();
                
                // Decay inactive reputations
                repLedger_.decayInactiveNodes(std::chrono::hours(24));
                
            } catch (const std::exception& e) {
                // Log error
            }
            
            std::this_thread::sleep_for(config_.discoveryInterval);
        }
    }
    
    // ========== Monitoring Loop ==========
    
    void monitoringLoop() {
        while (running_.load()) {
            try {
                auto metrics = getMetrics();
                // Export metrics (would send to Prometheus/etc in production)
                
            } catch (const std::exception& e) {
                // Log error
            }
            
            std::this_thread::sleep_for(config_.monitoring.metricsInterval);
        }
    }
    
    // ========== Helper Methods ==========
    
    std::vector<NodeMetrics> discoverNodes() const {
        return getNodes();
    }
    
    std::vector<NodeMetrics> performDiscovery() {
        // In production, this would query DHT, bootstrap peers, etc.
        // For now, return empty vector
        return {};
    }
    
    void updateLatencies() {
        auto nodes = getNodes();
        for (const auto& node : nodes) {
            latencyMap_.probeLatency(node.peerId);
        }
    }
    
    void dispatch(const Assignment& assignment, const TaskPayload& task) {
        // In production, this would send task to worker node
        // For now, simulate success
        
        totalTasksCompleted_++;
        
        // Update reputation
        bool success = assignment.assigned;
        repLedger_.recordTaskCompletion(
            assignment.workerPeerId,
            success,
            1.0, // quality score
            std::chrono::milliseconds(100)
        );
        
        // Resolve promise if exists
        resolvePromise(task.taskId, assignment);
    }

    Assignment makeFailureAssignment(const TaskPayload& task, const std::string& reason) const {
        Assignment assignment;
        assignment.assigned = false;
        assignment.reason = reason;
        assignment.assignmentId = task.taskId + "-failed";
        assignment.assignedAt = std::chrono::system_clock::now();
        return assignment;
    }

    void resolvePromise(const std::string& taskId, const Assignment& assignment) {
        std::shared_ptr<std::promise<Assignment>> promise;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            auto it = pendingPromises_.find(taskId);
            if (it == pendingPromises_.end()) {
                return;
            }
            promise = std::move(it->second);
            pendingPromises_.erase(it);
        }

        if (promise) {
            try {
                promise->set_value(assignment);
            } catch (const std::future_error&) {
                // Promise already satisfied or broken; ignore.
            }
        }
    }

    void failAllPending(const std::string& reason) {
        std::unordered_map<std::string, std::shared_ptr<std::promise<Assignment>>> pending;
        {
            std::lock_guard<std::mutex> lock(pendingMutex_);
            pending.swap(pendingPromises_);
        }

        for (auto& [taskId, promise] : pending) {
            if (!promise) {
                continue;
            }
            Assignment assignment;
            assignment.assigned = false;
            assignment.reason = reason;
            assignment.assignmentId = taskId + "-cancelled";
            assignment.assignedAt = std::chrono::system_clock::now();
            try {
                promise->set_value(assignment);
            } catch (const std::future_error&) {
                // Promise already satisfied or broken; ignore.
            }
        }
    }
};

// ==================== FACTORY FUNCTIONS ====================

inline std::unique_ptr<Engine> createEngine(const Config& config) {
    return std::make_unique<Engine>(config);
}

inline Config createDefaultConfig() {
    Config config;
    config.performance.defaultStrategy = SchedulingStrategy::WEIGHTED_SCORE;
    config.performance.maxConcurrentTasks = 100;
    auto cores = std::thread::hardware_concurrency();
    config.performance.workerThreads = cores == 0 ? 1 : cores;
    config.economic.defaultMaxCostTokens = 1000;
    config.economic.minReputationThreshold = 0.5;
    return config;
}

} // namespace ailee::sched
