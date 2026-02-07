// SPDX-License-Identifier: MIT
// Orchestrator.cpp — Production Implementation of Task Orchestration for AILEE-Core
// Complete implementation with all scheduling strategies, optimization algorithms,
// and integration utilities for distributed workload management.

#include "Orchestrator.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <limits>
#include <chrono>
#include <sstream>
#include <unordered_set>

namespace ailee::sched {

// ==================== WEIGHTED ORCHESTRATOR IMPLEMENTATION ====================

Assignment WeightedOrchestrator::assignBestWorker(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates,
    double trustW,
    double speedW,
    double powerW) const
{
    auto start = std::chrono::steady_clock::now();
    
    Assignment result;
    result.assignedAt = std::chrono::system_clock::now();
    result.assignmentId = task.taskId + "-" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());
    
    // Filter candidates based on task requirements
    auto filtered = filterCandidates(candidates, task);
    
    if (filtered.empty()) {
        result.assigned = false;
        result.reason = "No candidates meet task requirements";
        return result;
    }
    
    // Score all candidates
    double bestScore = -std::numeric_limits<double>::infinity();
    std::string bestPeer;
    NodeMetrics bestNode;
    
    for (const auto& candidate : filtered) {
        double score = scoreNode(candidate, task, trustW, speedW, powerW);
        
        // Store for debugging
        result.candidateScores.emplace_back(candidate.peerId, score);
        
        if (score > bestScore) {
            bestScore = score;
            bestPeer = candidate.peerId;
            bestNode = candidate;
        }
    }
    
    if (bestPeer.empty()) {
        result.assigned = false;
        result.reason = "No suitable worker candidates after scoring";
        return result;
    }
    
    // Populate assignment details
    result.assigned = true;
    result.workerPeerId = bestPeer;
    result.workerRegion = bestNode.region;
    result.finalScore = bestScore;
    
    // Get detailed scores
    auto reputation = rep_.get(bestPeer);
    result.reputationScore = reputation.score();
    
    auto latency = lat_.getLatencyMs(bestPeer);
    result.expectedLatencyMs = latency.value_or(std::numeric_limits<double>::infinity());
    result.latencyScore = speedW * (latency.has_value() ? (1.0 / (1.0 + latency.value() / 1000.0)) : 0.0);
    
    result.capacityScore = powerW * bestNode.capacityScore;
    result.costScore = bestNode.costPerHour > 0 ? (1.0 / bestNode.costPerHour) : 1.0;
    result.expectedCostTokens = estimateCost(task, bestNode);
    
    // Estimate completion time
    result.estimatedCompletionTime = utils::estimateCompletionTime(task, bestNode);
    
    // Find backup worker
    auto backup = findBackupWorker(task, filtered, bestPeer);
    if (backup.has_value() && backup->assigned) {
        result.backupWorkerPeerId = backup->workerPeerId;
    }
    
    // Update metrics
    metrics_.totalAssignments++;
    metrics_.successfulAssignments++;
    metrics_.assignmentsByWorker[bestPeer]++;
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    metrics_.avgAssignmentTime = std::chrono::milliseconds(
        (metrics_.avgAssignmentTime.count() * (metrics_.totalAssignments - 1) + 
         duration.count()) / metrics_.totalAssignments);
    
    return result;
}

double WeightedOrchestrator::scoreNode(
    const NodeMetrics& node,
    const TaskPayload& task,
    double trustW,
    double speedW,
    double powerW) const
{
    // Custom scoring function override
    if (customScorer_.has_value()) {
        return customScorer_.value()(node, task);
    }
    
    // Get reputation score
    auto reputation = rep_.get(node.peerId);
    double repScore = reputation.score();
    
    // Apply minimum reputation threshold
    if (repScore < task.minReputationScore) {
        return -std::numeric_limits<double>::infinity();
    }
    
    // Get latency (convert ms to seconds for normalization)
    auto latency = lat_.getLatencyMs(node.peerId);
    double normLat = latency.has_value() && std::isfinite(latency.value()) 
        ? (latency.value() / 1000.0) 
        : 1e6;
    
    // Capacity score (already normalized 0-1)
    double capacity = node.capacityScore;
    
    // Additional factors
    double costFactor = 1.0;
    if (task.maxCostTokens > 0 && node.costPerHour > 0) {
        uint64_t estimatedCost = static_cast<uint64_t>(node.costPerHour);
        if (estimatedCost > task.maxCostTokens) {
            return -std::numeric_limits<double>::infinity(); // Too expensive
        }
        costFactor = 1.0 - (static_cast<double>(estimatedCost) / task.maxCostTokens);
    }
    
    // Geographic preference
    double geoBonus = 0.0;
    if (task.preferredRegion.has_value() && node.region == task.preferredRegion.value()) {
        geoBonus = 0.1; // 10% bonus for preferred region
    }
    
    // Green energy preference
    double greenBonus = 0.0;
    if (task.preferGreenEnergy && node.carbonIntensity < 100.0) {
        greenBonus = 0.05; // 5% bonus for low carbon
    }
    
    // Load factor (penalize overloaded nodes)
    double loadPenalty = 0.0;
    if (node.activeTaskCount >= node.maxConcurrentTasks) {
        return -std::numeric_limits<double>::infinity(); // At capacity
    }
    if (node.maxConcurrentTasks > 0) {
        double loadRatio = static_cast<double>(node.activeTaskCount) / node.maxConcurrentTasks;
        loadPenalty = loadRatio * 0.2; // Up to 20% penalty for high load
    }
    
    // Compute final score
    double score = (repScore * trustW) - (normLat * speedW) + (capacity * powerW) 
                   + geoBonus + greenBonus - loadPenalty;
    
    // Apply cost factor as multiplier
    score *= costFactor;
    
    return score;
}

Assignment WeightedOrchestrator::assignWithStrategy(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates,
    SchedulingStrategy strategy) const
{
    auto filtered = filterCandidates(candidates, task);
    
    if (filtered.empty()) {
        Assignment result;
        result.assigned = false;
        result.reason = "No candidates meet requirements";
        return result;
    }
    
    switch (strategy) {
        case SchedulingStrategy::WEIGHTED_SCORE:
            return assignBestWorker(task, filtered, 0.6, 0.3, 0.1);
        
        case SchedulingStrategy::ROUND_ROBIN:
            return assignRoundRobin(filtered);
        
        case SchedulingStrategy::LEAST_LOADED:
            return assignLeastLoaded(filtered);
        
        case SchedulingStrategy::LOWEST_LATENCY:
            return assignLowestLatency(filtered);
        
        case SchedulingStrategy::HIGHEST_REPUTATION:
            return assignHighestReputation(filtered);
        
        case SchedulingStrategy::LOWEST_COST:
            return assignLowestCost(filtered);
        
        case SchedulingStrategy::GENETIC_ALGORITHM:
            return assignGeneticAlgorithm(task, filtered);
        
        case SchedulingStrategy::GEOGRAPHIC_AFFINITY:
            if (task.preferredRegion.has_value()) {
                // Filter by preferred region first
                std::vector<NodeMetrics> regionalNodes;
                std::copy_if(filtered.begin(), filtered.end(), 
                    std::back_inserter(regionalNodes),
                    [&](const NodeMetrics& n) { return n.region == task.preferredRegion.value(); });
                
                if (!regionalNodes.empty()) {
                    return assignBestWorker(task, regionalNodes, 0.6, 0.3, 0.1);
                }
            }
            return assignLowestLatency(filtered);
        
        case SchedulingStrategy::LOAD_BALANCING:
            return assignLeastLoaded(filtered);
        
        default:
            return assignBestWorker(task, filtered, 0.6, 0.3, 0.1);
    }
}

std::vector<NodeMetrics> WeightedOrchestrator::filterCandidates(
    const std::vector<NodeMetrics>& candidates,
    const TaskPayload& task) const
{
    std::vector<NodeMetrics> filtered;
    
    for (const auto& node : candidates) {
        // Check blacklist
        if (std::find(task.blacklistedNodes.begin(), task.blacklistedNodes.end(), 
                      node.peerId) != task.blacklistedNodes.end()) {
            continue;
        }
        
        // Check if node is online and available
        auto timeSinceLastSeen = std::chrono::system_clock::now() - node.lastSeen;
        if (timeSinceLastSeen > std::chrono::minutes(5)) {
            continue; // Node appears offline
        }
        
        // Check resource requirements
        if (!utils::meetsRequirements(node, task.requirements)) {
            continue;
        }
        
        // Check reputation threshold
        auto reputation = rep_.get(node.peerId);
        if (reputation.score() < task.minReputationScore) {
            continue;
        }
        
        // Check ZK proof capability if required
        if (task.requireZKProof && !node.hasZKProofCapability) {
            continue;
        }
        
        // Check concurrent task limit
        if (node.activeTaskCount >= node.maxConcurrentTasks) {
            continue;
        }
        
        // Check cost constraint
        if (task.maxCostTokens > 0 && node.costPerHour > task.maxCostTokens) {
            continue;
        }
        
        filtered.push_back(node);
    }
    
    return filtered;
}

std::vector<Assignment> WeightedOrchestrator::assignParallel(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates,
    uint32_t numWorkers) const
{
    std::vector<Assignment> assignments;
    auto filtered = filterCandidates(candidates, task);
    
    if (filtered.size() < numWorkers) {
        // Not enough workers available
        Assignment failure;
        failure.assigned = false;
        failure.reason = "Insufficient workers for parallel execution (need " + 
                        std::to_string(numWorkers) + ", have " + 
                        std::to_string(filtered.size()) + ")";
        assignments.push_back(failure);
        return assignments;
    }
    
    // Score all candidates
    auto ranked = rankCandidates(filtered, task);
    
    // Select top N workers, ensuring diversity
    std::unordered_set<std::string> selectedRegions;
    std::vector<std::string> selectedWorkers;
    
    for (const auto& [peerId, score] : ranked) {
        if (selectedWorkers.size() >= numWorkers) break;
        
        // Find node metrics
        auto it = std::find_if(filtered.begin(), filtered.end(),
            [&](const NodeMetrics& n) { return n.peerId == peerId; });
        
        if (it != filtered.end()) {
            // Prefer geographic diversity (avoid single region failure)
            bool regionOverused = false;
            if (selectedRegions.count(it->region) > 0) {
                size_t regionCount = std::count_if(
                    selectedWorkers.begin(), selectedWorkers.end(),
                    [&](const std::string& id) {
                        auto node = std::find_if(filtered.begin(), filtered.end(),
                            [&](const NodeMetrics& n) { return n.peerId == id; });
                        return node != filtered.end() && node->region == it->region;
                    });
                
                if (regionCount > numWorkers / 4) { // Max 25% per region
                    regionOverused = true;
                }
            }
            
            if (!regionOverused) {
                selectedWorkers.push_back(peerId);
                selectedRegions.insert(it->region);
            }
        }
    }
    
    // Create assignments
    for (const auto& workerId : selectedWorkers) {
        auto it = std::find_if(filtered.begin(), filtered.end(),
            [&](const NodeMetrics& n) { return n.peerId == workerId; });
        
        if (it != filtered.end()) {
            Assignment assignment;
            assignment.assigned = true;
            assignment.workerPeerId = workerId;
            assignment.workerRegion = it->region;
            assignment.assignedAt = std::chrono::system_clock::now();
            assignment.assignmentId = task.taskId + "-parallel-" + 
                                     std::to_string(assignments.size());
            
            auto reputation = rep_.get(workerId);
            assignment.reputationScore = reputation.score();
            assignment.expectedLatencyMs = lat_.getLatencyMs(workerId).value_or(0.0);
            assignment.expectedCostTokens = estimateCost(task, *it);
            
            assignments.push_back(assignment);
        }
    }
    
    return assignments;
}

std::vector<std::pair<std::string, double>> WeightedOrchestrator::rankCandidates(
    const std::vector<NodeMetrics>& candidates,
    const TaskPayload& task) const
{
    std::vector<std::pair<std::string, double>> ranked;
    
    for (const auto& node : candidates) {
        double score = scoreNode(node, task, 0.6, 0.3, 0.1);
        ranked.emplace_back(node.peerId, score);
    }
    
    // Sort descending by score
    std::sort(ranked.begin(), ranked.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return ranked;
}

std::optional<Assignment> WeightedOrchestrator::findBackupWorker(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates,
    const std::string& excludePeerId) const
{
    std::vector<NodeMetrics> backupCandidates;
    std::copy_if(candidates.begin(), candidates.end(),
        std::back_inserter(backupCandidates),
        [&](const NodeMetrics& n) { return n.peerId != excludePeerId; });
    
    if (backupCandidates.empty()) {
        return std::nullopt;
    }
    
    return assignBestWorker(task, backupCandidates, 0.6, 0.3, 0.1);
}

namespace {

std::vector<int> hungarianMinimize(const std::vector<std::vector<double>>& cost) {
    if (cost.empty() || cost.front().empty()) {
        return {};
    }

    const size_t n = cost.size();
    const size_t m = cost.front().size();
    const double inf = std::numeric_limits<double>::infinity();

    std::vector<double> u(n + 1), v(m + 1);
    std::vector<int> p(m + 1), way(m + 1);

    for (size_t i = 1; i <= n; ++i) {
        p[0] = static_cast<int>(i);
        size_t j0 = 0;
        std::vector<double> minv(m + 1, inf);
        std::vector<bool> used(m + 1, false);

        do {
            used[j0] = true;
            int i0 = p[j0];
            double delta = inf;
            size_t j1 = 0;

            for (size_t j = 1; j <= m; ++j) {
                if (used[j]) continue;
                double cur = cost[i0 - 1][j - 1] - u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = static_cast<int>(j0);
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }

            for (size_t j = 0; j <= m; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            size_t j1 = static_cast<size_t>(way[j0]);
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    std::vector<int> assignment(n, -1);
    for (size_t j = 1; j <= m; ++j) {
        if (p[j] != 0) {
            assignment[p[j] - 1] = static_cast<int>(j - 1);
        }
    }

    return assignment;
}

} // namespace

std::vector<Assignment> WeightedOrchestrator::scheduleBatch(
    const std::vector<TaskPayload>& tasks,
    const std::vector<NodeMetrics>& candidates) const
{
    std::vector<Assignment> assignments;
    
    if (tasks.empty()) {
        return assignments;
    }

    auto start = std::chrono::steady_clock::now();
    constexpr double kUnassignedCost = 1e12;

    struct Slot {
        const NodeMetrics* node;
    };
    std::vector<Slot> slots;

    for (const auto& node : candidates) {
        uint32_t capacity = 0;
        if (node.maxConcurrentTasks > node.activeTaskCount) {
            capacity = node.maxConcurrentTasks - node.activeTaskCount;
        }
        for (uint32_t i = 0; i < capacity; ++i) {
            slots.push_back(Slot{&node});
        }
    }

    std::vector<std::unordered_set<std::string>> allowed(tasks.size());
    std::vector<std::vector<NodeMetrics>> filteredPerTask;
    filteredPerTask.reserve(tasks.size());

    for (size_t i = 0; i < tasks.size(); ++i) {
        auto filtered = filterCandidates(candidates, tasks[i]);
        for (const auto& node : filtered) {
            allowed[i].insert(node.peerId);
        }
        filteredPerTask.push_back(std::move(filtered));
    }

    size_t columns = slots.size() + tasks.size();
    std::vector<std::vector<double>> cost(tasks.size(),
                                          std::vector<double>(columns, kUnassignedCost));

    for (size_t i = 0; i < tasks.size(); ++i) {
        for (size_t j = 0; j < slots.size(); ++j) {
            const auto& node = *slots[j].node;
            if (allowed[i].count(node.peerId) == 0) {
                cost[i][j] = kUnassignedCost;
                continue;
            }
            double score = scoreNode(node, tasks[i], 0.6, 0.3, 0.1);
            if (!std::isfinite(score)) {
                cost[i][j] = kUnassignedCost;
            } else {
                cost[i][j] = -score;
            }
        }
    }

    auto assignmentIdx = hungarianMinimize(cost);
    
    assignments.reserve(tasks.size());

    for (size_t i = 0; i < tasks.size(); ++i) {
        Assignment assignment;
        assignment.assignedAt = std::chrono::system_clock::now();
        assignment.assignmentId = tasks[i].taskId + "-batch-" + std::to_string(i);

        int col = (i < assignmentIdx.size()) ? assignmentIdx[i] : -1;
        if (col < 0 || static_cast<size_t>(col) >= slots.size() ||
            cost[i][col] >= kUnassignedCost / 2) {
            assignment.assigned = false;
            assignment.reason = "No feasible assignment after global optimization";
            assignments.push_back(assignment);
            continue;
        }

        const auto& node = *slots[static_cast<size_t>(col)].node;
        assignment.assigned = true;
        assignment.workerPeerId = node.peerId;
        assignment.workerRegion = node.region;
        assignment.finalScore = -cost[i][col];

        auto reputation = rep_.get(node.peerId);
        assignment.reputationScore = reputation.score();
        auto latency = lat_.getLatencyMs(node.peerId);
        assignment.expectedLatencyMs = latency.value_or(std::numeric_limits<double>::infinity());
        assignment.latencyScore = 0.3 * (latency.has_value() ? (1.0 / (1.0 + latency.value() / 1000.0)) : 0.0);
        assignment.capacityScore = 0.1 * node.capacityScore;
        assignment.costScore = node.costPerHour > 0 ? (1.0 / node.costPerHour) : 1.0;
        assignment.expectedCostTokens = estimateCost(tasks[i], node);
        assignment.estimatedCompletionTime = utils::estimateCompletionTime(tasks[i], node);

        auto backup = findBackupWorker(tasks[i], filteredPerTask[i], node.peerId);
        if (backup.has_value() && backup->assigned) {
            assignment.backupWorkerPeerId = backup->workerPeerId;
        }

        assignments.push_back(assignment);
    }

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    auto perAssignment = std::chrono::milliseconds(
        duration.count() / std::max<size_t>(1, tasks.size()));

    for (const auto& assignment : assignments) {
        if (!assignment.assigned) {
            continue;
        }
        metrics_.totalAssignments++;
        metrics_.successfulAssignments++;
        metrics_.assignmentsByWorker[assignment.workerPeerId]++;
        metrics_.avgAssignmentTime = std::chrono::milliseconds(
            (metrics_.avgAssignmentTime.count() * (metrics_.totalAssignments - 1) +
             perAssignment.count()) / metrics_.totalAssignments);
    }
    
    return assignments;
}

std::vector<std::pair<std::string, std::string>> WeightedOrchestrator::rebalanceTasks(
    const std::vector<std::pair<std::string, std::string>>& currentAssignments,
    const std::vector<NodeMetrics>& candidates) const
{
    std::vector<std::pair<std::string, std::string>> migrations;
    
    // Count tasks per node
    std::unordered_map<std::string, uint32_t> taskCounts;
    for (const auto& [taskId, nodeId] : currentAssignments) {
        taskCounts[nodeId]++;
    }
    
    // Find overloaded and underloaded nodes
    std::vector<std::string> overloaded;
    std::vector<std::string> underloaded;
    
    for (const auto& node : candidates) {
        uint32_t currentTasks = taskCounts[node.peerId];
        uint32_t maxTasks = node.maxConcurrentTasks;
        
        if (maxTasks == 0) continue;
        
        double loadRatio = static_cast<double>(currentTasks) / maxTasks;
        
        if (loadRatio > 0.8) { // >80% loaded
            overloaded.push_back(node.peerId);
        } else if (loadRatio < 0.3) { // <30% loaded
            underloaded.push_back(node.peerId);
        }
    }
    
    // Migrate tasks from overloaded to underloaded
    for (const auto& overloadedNode : overloaded) {
        for (const auto& [taskId, nodeId] : currentAssignments) {
            if (nodeId == overloadedNode && !underloaded.empty()) {
                std::string targetNode = underloaded.back();
                migrations.emplace_back(taskId, overloadedNode + "→" + targetNode);
                
                // Update counts
                taskCounts[overloadedNode]--;
                taskCounts[targetNode]++;
                
                // Check if target is now loaded
                auto targetMetrics = std::find_if(candidates.begin(), candidates.end(),
                    [&](const NodeMetrics& n) { return n.peerId == targetNode; });
                
                if (targetMetrics != candidates.end()) {
                    double newLoad = static_cast<double>(taskCounts[targetNode]) / 
                                    targetMetrics->maxConcurrentTasks;
                    if (newLoad > 0.7) {
                        underloaded.pop_back();
                    }
                }
                
                break; // Move one task at a time
            }
        }
    }
    
    return migrations;
}

std::optional<Assignment> WeightedOrchestrator::findCheapestWorker(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates) const
{
    auto filtered = filterCandidates(candidates, task);
    
    if (filtered.empty()) {
        return std::nullopt;
    }
    
    // Find node with lowest cost
    auto cheapest = std::min_element(filtered.begin(), filtered.end(),
        [](const NodeMetrics& a, const NodeMetrics& b) {
            return a.costPerHour < b.costPerHour;
        });
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = cheapest->peerId;
    result.workerRegion = cheapest->region;
    result.expectedCostTokens = estimateCost(task, *cheapest);
    result.assignedAt = std::chrono::system_clock::now();
    
    return result;
}

uint64_t WeightedOrchestrator::estimateCost(
    const TaskPayload& task,
    const NodeMetrics& worker) const
{
    // Simple cost estimation based on expected completion time
    auto completionTime = utils::estimateCompletionTime(task, worker);
    double hours = completionTime.count() / (1000.0 * 3600.0);
    
    return static_cast<uint64_t>(hours * worker.costPerHour * worker.rewardMultiplier);
}

Assignment WeightedOrchestrator::optimizeCostPerformance(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates,
    double costWeight) const
{
    auto filtered = filterCandidates(candidates, task);
    
    if (filtered.empty()) {
        Assignment result;
        result.assigned = false;
        result.reason = "No candidates available";
        return result;
    }
    
    double performanceWeight = 1.0 - costWeight;
    
    double bestScore = -std::numeric_limits<double>::infinity();
    NodeMetrics bestNode;
    
    for (const auto& node : filtered) {
        // Performance score (inverse latency + reputation)
        auto latency = lat_.getLatencyMs(node.peerId).value_or(1000.0);
        auto reputation = rep_.get(node.peerId).score();
        double perfScore = reputation - (latency / 1000.0);
        
        // Cost score (inverse cost)
        double costScore = (node.costPerHour > 0) ? (1.0 / node.costPerHour) : 1.0;
        
        // Combined score
        double score = (perfScore * performanceWeight) + (costScore * costWeight);
        
        if (score > bestScore) {
            bestScore = score;
            bestNode = node;
        }
    }
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = bestNode.peerId;
    result.workerRegion = bestNode.region;
    result.finalScore = bestScore;
    result.expectedCostTokens = estimateCost(task, bestNode);
    result.assignedAt = std::chrono::system_clock::now();
    
    return result;
}

// ==================== STRATEGY-SPECIFIC IMPLEMENTATIONS ====================

Assignment WeightedOrchestrator::assignRoundRobin(
    const std::vector<NodeMetrics>& candidates) const
{
    static std::atomic<size_t> roundRobinIndex{0};
    
    size_t index = roundRobinIndex.fetch_add(1) % candidates.size();
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = candidates[index].peerId;
    result.workerRegion = candidates[index].region;
    result.assignedAt = std::chrono::system_clock::now();
    result.reason = "Round-robin selection";
    
    return result;
}

Assignment WeightedOrchestrator::assignLeastLoaded(
    const std::vector<NodeMetrics>& candidates) const
{
    auto leastLoaded = std::min_element(candidates.begin(), candidates.end(),
        [](const NodeMetrics& a, const NodeMetrics& b) {
            double loadA = (a.maxConcurrentTasks > 0) ? 
                static_cast<double>(a.activeTaskCount) / a.maxConcurrentTasks : 1.0;
            double loadB = (b.maxConcurrentTasks > 0) ?
                static_cast<double>(b.activeTaskCount) / b.maxConcurrentTasks : 1.0;
            return loadA < loadB;
        });
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = leastLoaded->peerId;
    result.workerRegion = leastLoaded->region;
    result.assignedAt = std::chrono::system_clock::now();
    result.reason = "Least loaded node";
    
    return result;
}

Assignment WeightedOrchestrator::assignLowestLatency(
    const std::vector<NodeMetrics>& candidates) const
{
    double minLatency = std::numeric_limits<double>::infinity();
    std::string bestPeer;
    std::string bestRegion;
    
    for (const auto& node : candidates) {
        auto latency = lat_.getLatencyMs(node.peerId);
        if (latency.has_value() && latency.value() < minLatency) {
            minLatency = latency.value();
            bestPeer = node.peerId;
            bestRegion = node.region;
        }
    }
    
    Assignment result;
    if (!bestPeer.empty()) {
        result.assigned = true;
        result.workerPeerId = bestPeer;
        result.workerRegion = bestRegion;
        result.expectedLatencyMs = minLatency;
        result.reason = "Lowest latency";
    } else {
        result.assigned = false;
        result.reason = "No latency data available";
    }
    result.assignedAt = std::chrono::system_clock::now();
    
    return result;
}

Assignment WeightedOrchestrator::assignHighestReputation(
    const std::vector<NodeMetrics>& candidates) const
{
    double maxRep = -1.0;
    std::string bestPeer;
    std::string bestRegion;
    
    for (const auto& node : candidates) {
        auto reputation = rep_.get(node.peerId);
        double score = reputation.score();
        if (score > maxRep) {
            maxRep = score;
            bestPeer = node.peerId;
            bestRegion = node.region;
        }
    }
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = bestPeer;
    result.workerRegion = bestRegion;
    result.reputationScore = maxRep;
    result.assignedAt = std::chrono::system_clock::now();
    result.reason = "Highest reputation";
    
    return result;
}

Assignment WeightedOrchestrator::assignLowestCost(
    const std::vector<NodeMetrics>& candidates) const
{
    auto cheapest = std::min_element(candidates.begin(), candidates.end(),
        [](const NodeMetrics& a, const NodeMetrics& b) {
            return a.costPerHour < b.costPerHour;
        });
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = cheapest->peerId;
    result.workerRegion = cheapest->region;
    result.expectedCostTokens = static_cast<uint64_t>(cheapest->costPerHour);
    result.assignedAt = std::chrono::system_clock::now();
    result.reason = "Lowest cost";
    
    return result;
}

Assignment WeightedOrchestrator::assignGeneticAlgorithm(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates) const
{
    // Simplified genetic algorithm for demonstration
    // In production, use more sophisticated GA with crossover/mutation
    
    constexpr int populationSize = 20;
    constexpr int generations = 10;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, candidates.size() - 1);
    
    // Initial population (random assignments)
    std::vector<size_t> population;
    for (int i = 0; i < populationSize; ++i) {
        population.push_back(dist(gen));
    }
    
    // Evolution
    for (int g = 0; g < generations; ++g) {
        // Evaluate fitness
        std::vector<std::pair<size_t, double>> fitness;
        for (size_t idx : population) {
            double score = scoreNode(candidates[idx], task, 0.6, 0.3, 0.1);
            fitness.emplace_back(idx, score);
        }
        
        // Sort by fitness
        std::sort(fitness.begin(), fitness.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Selection + mutation (keep top 50%, mutate rest)
        population.clear();
        for (int i = 0; i < populationSize / 2; ++i) {
            population.push_back(fitness[i].first);
        }
        for (int i = populationSize / 2; i < populationSize; ++i) {
            population.push_back(dist(gen));
        }
    }
    
    // Return best individual
    double bestScore = -std::numeric_limits<double>::infinity();
    size_t bestIdx = 0;
    for (size_t idx : population) {
        double score = scoreNode(candidates[idx], task, 0.6, 0.3, 0.1);
        if (score > bestScore) {
            bestScore = score;
            bestIdx = idx;
        }
    }
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = candidates[bestIdx].peerId;
    result.workerRegion = candidates[bestIdx].region;
    result.finalScore = bestScore;
    result.assignedAt = std::chrono::system_clock::now();
    result.reason = "Genetic algorithm optimization";
    
    return result;
}

// ==================== UTILITY IMPLEMENTATIONS ====================

namespace utils {

double normalizeScore(double value, double min, double max) {
    if (max <= min) return 0.0;
    return std::clamp((value - min) / (max - min), 0.0, 1.0);
}

double computeDistance(const std::string& region1, const std::string& region2) {
    // Simplified: use hardcoded distance matrix
    // In production, use actual lat/lon coordinates with Haversine formula
    static const std::unordered_map<std::string, std::unordered_map<std::string, double>> distances = {
        {"us-east", {{"us-west", 4000}, {"eu-west", 6000}, {"ap-south", 12000}, {"sa-east", 7000}}},
        {"us-west", {{"us-east", 4000}, {"eu-west", 8000}, {"ap-south", 10000}, {"sa-east", 8500}}},
        {"eu-west", {{"us-east", 6000}, {"us-west", 8000}, {"ap-south", 7000}, {"sa-east", 9000}}},
        {"ap-south", {{"us-east", 12000}, {"us-west", 10000}, {"eu-west", 7000}, {"sa-east", 15000}}},
        {"sa-east", {{"us-east", 7000}, {"us-west", 8500}, {"eu-west", 9000}, {"ap-south", 15000}}}
    };
    
    if (region1 == region2) return 0.0;
    
    auto it1 = distances.find(region1);
    if (it1 != distances.end()) {
        auto it2 = it1->second.find(region2);
        if (it2 != it1->second.end()) {
            return it2->second;
        }
    }
    
    // Default distance if not in matrix (assume far apart)
    return 10000.0;
}

std::chrono::milliseconds estimateCompletionTime(
    const TaskPayload& task,
    const NodeMetrics& node)
{
    // Base time estimate based on task type
    std::chrono::milliseconds baseTime{5000}; // 5 seconds default
    
    switch (task.taskType) {
        case TaskType::AI_INFERENCE:
            baseTime = std::chrono::milliseconds(node.capabilities.hasGPU ? 100 : 1000);
            break;
        case TaskType::AI_TRAINING:
            baseTime = std::chrono::milliseconds(node.capabilities.hasGPU ? 10000 : 60000);
            break;
        case TaskType::FEDERATED_LEARNING:
            baseTime = std::chrono::milliseconds(5000);
            break;
        case TaskType::WASM_EXECUTION:
            baseTime = std::chrono::milliseconds(1000);
            break;
        case TaskType::ZK_PROOF_GENERATION:
            baseTime = std::chrono::milliseconds(3000);
            break;
        case TaskType::DATA_PROCESSING:
            baseTime = std::chrono::milliseconds(2000);
            break;
        case TaskType::BANDWIDTH_RELAY:
            baseTime = std::chrono::milliseconds(500);
            break;
        default:
            baseTime = std::chrono::milliseconds(5000);
    }
    
    // Adjust for node utilization
    double utilizationFactor = 1.0 + (node.cpuUtilization * 0.5);
    
    // Adjust for network latency
    double latencyMs = node.latencyMs;
    if (std::isfinite(latencyMs)) {
        baseTime += std::chrono::milliseconds(static_cast<long long>(latencyMs));
    }
    
    // Apply utilization factor
    baseTime = std::chrono::milliseconds(
        static_cast<long long>(baseTime.count() * utilizationFactor));
    
    return baseTime;
}

bool meetsRequirements(const NodeMetrics& node, const ResourceRequirements& req) {
    // Check CPU cores
    if (node.capabilities.cpuCores < req.minCpuCores) {
        return false;
    }
    
    // Check memory
    if (node.capabilities.memoryGB < req.minMemoryGB) {
        return false;
    }
    
    // Check storage
    if (node.capabilities.storageGB < req.minStorageGB) {
        return false;
    }
    
    // Check bandwidth
    if (node.bandwidthMbps < req.minBandwidthMbps) {
        return false;
    }
    
    // Check GPU requirement
    if (req.requiresGPU && !node.capabilities.hasGPU) {
        return false;
    }
    
    // Check GPU memory if GPU required
    if (req.requiresGPU && node.capabilities.gpuMemoryGB < req.minGpuMemoryGB) {
        return false;
    }
    
    // Check TPU requirement
    if (req.requiresTPU && !node.capabilities.hasTPU) {
        return false;
    }
    
    // Check specific capabilities
    for (const auto& reqCap : req.requiredCapabilities) {
        bool found = false;
        for (const auto& nodeCap : node.capabilities.supportedArchitectures) {
            if (nodeCap == reqCap) {
                found = true;
                break;
            }
        }
        if (!found) {
            // Also check runtime versions
            for (const auto& runtime : node.capabilities.runtimeVersions) {
                if (runtime.find(reqCap) != std::string::npos) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            return false;
        }
    }
    
    return true;
}

double computeLoadBalance(const std::vector<NodeMetrics>& nodes) {
    if (nodes.empty()) return 0.0;
    
    // Compute load for each node
    std::vector<double> loads;
    loads.reserve(nodes.size());
    
    for (const auto& node : nodes) {
        if (node.maxConcurrentTasks > 0) {
            double load = static_cast<double>(node.activeTaskCount) / node.maxConcurrentTasks;
            loads.push_back(load);
        }
    }
    
    if (loads.empty()) return 0.0;
    
    // Compute standard deviation (measure of imbalance)
    double mean = std::accumulate(loads.begin(), loads.end(), 0.0) / loads.size();
    
    double variance = 0.0;
    for (double load : loads) {
        variance += (load - mean) * (load - mean);
    }
    variance /= loads.size();
    
    double stddev = std::sqrt(variance);
    
    // Return normalized score (0 = perfect balance, 1 = maximum imbalance)
    return std::min(stddev, 1.0);
}

std::vector<std::string> selectDiverseNodes(
    const std::vector<NodeMetrics>& candidates,
    std::size_t count)
{
    if (candidates.size() <= count) {
        // Return all candidates if not enough
        std::vector<std::string> all;
        all.reserve(candidates.size());
        for (const auto& node : candidates) {
            all.push_back(node.peerId);
        }
        return all;
    }
    
    std::vector<std::string> selected;
    selected.reserve(count);
    std::unordered_set<std::string> selectedIds;
    selectedIds.reserve(count);
    
    // Track regions to ensure diversity
    std::unordered_map<std::string, size_t> regionCounts;
    
    // First pass: one node per region
    std::unordered_set<std::string> seenRegions;
    for (const auto& node : candidates) {
        if (selected.size() >= count) break;
        
        if (seenRegions.find(node.region) == seenRegions.end()) {
            selected.push_back(node.peerId);
            selectedIds.insert(node.peerId);
            seenRegions.insert(node.region);
            regionCounts[node.region]++;
        }
    }
    
    // Second pass: fill remaining slots with best nodes
    std::vector<NodeMetrics> remaining;
    for (const auto& node : candidates) {
        if (selectedIds.find(node.peerId) == selectedIds.end()) {
            remaining.push_back(node);
        }
    }
    
    // Sort remaining by capacity score
    std::sort(remaining.begin(), remaining.end(),
        [](const NodeMetrics& a, const NodeMetrics& b) {
            return a.capacityScore > b.capacityScore;
        });
    
    // Add remaining nodes, preferring under-represented regions
    for (const auto& node : remaining) {
        if (selected.size() >= count) break;
        
        // Prefer regions with fewer nodes
        size_t currentCount = regionCounts[node.region];
        size_t regionBucketCount = std::max<size_t>(1, seenRegions.size());
        size_t maxPerRegion = (count / regionBucketCount) + 1;
        
        if (currentCount < maxPerRegion) {
            selected.push_back(node.peerId);
            selectedIds.insert(node.peerId);
            regionCounts[node.region]++;
        }
    }
    
    // Fill any remaining slots (if region limits prevented filling)
    for (const auto& node : remaining) {
        if (selected.size() >= count) break;
        
        if (selectedIds.find(node.peerId) == selectedIds.end()) {
            selected.push_back(node.peerId);
            selectedIds.insert(node.peerId);
        }
    }
    
    return selected;
}

} // namespace utils

// ==================== INTEGRATION IMPLEMENTATIONS ====================

namespace integration {

std::unique_ptr<IOrchestrator> createAmbientAIOrchestrator(
    const IReputationLedger& rep,
    const ILatencyMap& lat)
{
    auto orchestrator = std::make_unique<WeightedOrchestrator>(rep, lat);
    
    // Configure for AmbientAI workloads
    // Prioritize reputation (Byzantine tolerance) and capacity
    orchestrator->setStrategy(SchedulingStrategy::WEIGHTED_SCORE);
    
    return orchestrator;
}

std::vector<Assignment> scheduleFederatedLearning(
    const std::vector<TaskPayload>& tasks,
    const std::vector<NodeMetrics>& candidates,
    std::size_t minParticipants)
{
    std::vector<Assignment> assignments;
    
    if (candidates.size() < minParticipants) {
        Assignment failure;
        failure.assigned = false;
        failure.reason = "Insufficient participants for federated learning (need " +
                        std::to_string(minParticipants) + ", have " +
                        std::to_string(candidates.size()) + ")";
        assignments.push_back(failure);
        return assignments;
    }
    
    // Select diverse set of high-reputation nodes
    std::vector<NodeMetrics> qualified;
    std::copy_if(candidates.begin(), candidates.end(),
        std::back_inserter(qualified),
        [](const NodeMetrics& n) {
            return n.capabilities.memoryGB >= 8 &&  // FL needs memory
                   n.availabilityRate > 0.95;        // High availability
        });
    
    if (qualified.size() < minParticipants) {
        Assignment failure;
        failure.assigned = false;
        failure.reason = "Insufficient qualified participants";
        assignments.push_back(failure);
        return assignments;
    }
    
    // Select diverse nodes
    auto selectedIds = utils::selectDiverseNodes(qualified, minParticipants);
    
    // Create assignments
    for (const auto& nodeId : selectedIds) {
        auto it = std::find_if(qualified.begin(), qualified.end(),
            [&](const NodeMetrics& n) { return n.peerId == nodeId; });
        
        if (it != qualified.end()) {
            Assignment assignment;
            assignment.assigned = true;
            assignment.workerPeerId = nodeId;
            assignment.workerRegion = it->region;
            assignment.assignedAt = std::chrono::system_clock::now();
            assignment.reason = "Federated learning participant";
            assignments.push_back(assignment);
        }
    }
    
    return assignments;
}

Assignment scheduleGreenTask(
    const TaskPayload& task,
    const std::vector<NodeMetrics>& candidates)
{
    // Filter for green energy nodes (low carbon intensity)
    std::vector<NodeMetrics> greenNodes;
    std::copy_if(candidates.begin(), candidates.end(),
        std::back_inserter(greenNodes),
        [](const NodeMetrics& n) {
            return n.carbonIntensity < 100.0 &&  // < 100 gCO2/kWh
                   n.energyEfficiency > 100.0;    // > 100 GFLOPS/W
        });
    
    if (greenNodes.empty()) {
        // Fall back to most efficient node
        auto mostEfficient = std::max_element(candidates.begin(), candidates.end(),
            [](const NodeMetrics& a, const NodeMetrics& b) {
                return a.energyEfficiency < b.energyEfficiency;
            });
        
        Assignment result;
        if (mostEfficient != candidates.end()) {
            result.assigned = true;
            result.workerPeerId = mostEfficient->peerId;
            result.workerRegion = mostEfficient->region;
            result.reason = "Most energy-efficient node (no fully green nodes available)";
        } else {
            result.assigned = false;
            result.reason = "No suitable nodes available";
        }
        return result;
    }
    
    // Among green nodes, select best overall
    auto bestGreen = std::max_element(greenNodes.begin(), greenNodes.end(),
        [](const NodeMetrics& a, const NodeMetrics& b) {
            // Score = efficiency - carbon intensity
            double scoreA = a.energyEfficiency - (a.carbonIntensity / 10.0);
            double scoreB = b.energyEfficiency - (b.carbonIntensity / 10.0);
            return scoreA < scoreB;
        });
    
    Assignment result;
    result.assigned = true;
    result.workerPeerId = bestGreen->peerId;
    result.workerRegion = bestGreen->region;
    result.assignedAt = std::chrono::system_clock::now();
    
    std::ostringstream reasonStream;
    reasonStream << "Green energy node (carbon: " << bestGreen->carbonIntensity
                 << " gCO2/kWh, efficiency: " << bestGreen->energyEfficiency
                 << " GFLOPS/W)";
    result.reason = reasonStream.str();
    
    return result;
}

} // namespace integration

} // namespace ailee::sched
