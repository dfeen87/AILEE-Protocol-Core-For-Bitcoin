// SPDX-License-Identifier: MIT
// DistributedTaskProtocol.cpp — Task distribution protocol implementation

#include "DistributedTaskProtocol.h"
#include <iostream>
#include <map>
#include <queue>
#include <mutex>
#include <thread>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

namespace ailee::orchestration {

// Task queue comparator (priority-based)
struct TaskComparator {
    bool operator()(const DistributedTask& a, const DistributedTask& b) const {
        if (a.priority != b.priority) {
            return static_cast<int>(a.priority) < static_cast<int>(b.priority);
        }
        return a.createdAt > b.createdAt; // Earlier tasks first
    }
};

class DistributedTaskProtocol::Impl {
public:
    Config config;
    std::shared_ptr<network::P2PNetwork> network;
    bool running = false;
    
    // Task storage
    std::map<std::string, DistributedTask> pendingTasks;
    std::map<std::string, DistributedTask> runningTasks;
    std::map<std::string, TaskResult> completedTasks;
    std::priority_queue<DistributedTask, std::vector<DistributedTask>, TaskComparator> taskQueue;
    
    // Executors
    std::map<TaskType, TaskExecutor> executors;
    TaskEventCallback eventCallback;
    
    // Statistics
    ProtocolStats stats{};
    
    std::mutex mutex;
    std::thread workerThread;
    bool stopWorker = false;
    
    Impl(std::shared_ptr<network::P2PNetwork> net, const Config& cfg)
        : config(cfg), network(net) {}
    
    ~Impl() {
        if (running) {
            stopWorker = true;
            if (workerThread.joinable()) {
                workerThread.join();
            }
        }
    }
    
    void handleTaskMessage(const network::NetworkMessage& msg) {
        // Deserialize task from payload
        // For now, simple stub
        std::lock_guard<std::mutex> lock(mutex);
        
        std::cout << "[TaskProtocol] Received task message from: " << msg.senderId 
                  << " (size: " << msg.payload.size() << " bytes)" << std::endl;
        
        stats.tasksReceived++;
        
        if (eventCallback) {
            eventCallback("task_" + msg.messageId, TaskEvent::RECEIVED, "Task received from network");
        }
    }
    
    void handleResultMessage(const network::NetworkMessage& msg) {
        std::lock_guard<std::mutex> lock(mutex);
        
        std::cout << "[TaskProtocol] Received result message from: " << msg.senderId 
                  << " (size: " << msg.payload.size() << " bytes)" << std::endl;
    }
    
    void workerLoop() {
        while (!stopWorker) {
            std::unique_lock<std::mutex> lock(mutex);
            
            // Process task queue
            if (!taskQueue.empty() && runningTasks.size() < config.maxConcurrentTasks) {
                auto task = taskQueue.top();
                taskQueue.pop();
                
                runningTasks[task.taskId] = task;
                pendingTasks.erase(task.taskId);
                lock.unlock();
                
                // Execute task asynchronously
                executeTaskAsync(task);
            } else {
                lock.unlock();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    void executeTaskAsync(const DistributedTask& task) {
        std::thread([this, task]() {
            auto startTime = std::chrono::steady_clock::now();
            
            if (eventCallback) {
                eventCallback(task.taskId, TaskEvent::STARTED, "Task execution started");
            }
            
            // Find executor for task type
            TaskExecutor executor;
            {
                std::lock_guard<std::mutex> lock(mutex);
                auto it = executors.find(task.type);
                if (it != executors.end()) {
                    executor = it->second;
                }
            }
            
            std::optional<TaskResult> result;
            if (executor) {
                result = executor(task);
            } else {
                // No executor registered
                result = TaskResult{
                    task.taskId,
                    config.nodeId,
                    false,
                    {},
                    "No executor registered for task type",
                    static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()),
                    0
                };
            }
            
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            std::lock_guard<std::mutex> lock(mutex);
            runningTasks.erase(task.taskId);
            
            if (result) {
                completedTasks[task.taskId] = *result;
                stats.tasksExecuted++;
                
                if (eventCallback) {
                    eventCallback(task.taskId, TaskEvent::COMPLETED, "Task execution completed");
                }
            } else {
                stats.tasksFailed++;
                
                if (eventCallback) {
                    eventCallback(task.taskId, TaskEvent::FAILED, "Task execution failed");
                }
            }
        }).detach();
    }
};

DistributedTaskProtocol::DistributedTaskProtocol(
    std::shared_ptr<network::P2PNetwork> network,
    const Config& config)
    : impl_(std::make_unique<Impl>(network, config)) {
}

DistributedTaskProtocol::~DistributedTaskProtocol() = default;

bool DistributedTaskProtocol::start() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->running) {
        return true;
    }
    
    std::cout << "[TaskProtocol] Starting distributed task protocol" << std::endl;
    std::cout << "[TaskProtocol] Node ID: " << impl_->config.nodeId << std::endl;
    std::cout << "[TaskProtocol] Max concurrent tasks: " << impl_->config.maxConcurrentTasks << std::endl;
    
    // Subscribe to task topics
    std::string taskTopic = impl_->config.tasksTopicPrefix;
    std::string resultTopic = impl_->config.resultsTopicPrefix;
    
    impl_->network->subscribe(taskTopic, [this](const network::NetworkMessage& msg) {
        impl_->handleTaskMessage(msg);
    });
    
    impl_->network->subscribe(resultTopic, [this](const network::NetworkMessage& msg) {
        impl_->handleResultMessage(msg);
    });
    
    // Start worker thread
    impl_->stopWorker = false;
    impl_->workerThread = std::thread([this]() { impl_->workerLoop(); });
    
    impl_->running = true;
    return true;
}

void DistributedTaskProtocol::stop() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        return;
    }
    
    std::cout << "[TaskProtocol] Stopping distributed task protocol" << std::endl;
    
    impl_->stopWorker = true;
    if (impl_->workerThread.joinable()) {
        impl_->workerThread.join();
    }
    
    impl_->running = false;
}

bool DistributedTaskProtocol::isRunning() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->running;
}

bool DistributedTaskProtocol::distributeTask(const DistributedTask& task) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->running) {
        std::cerr << "[TaskProtocol] Cannot distribute task: protocol not running" << std::endl;
        return false;
    }
    
    // Add to pending tasks
    impl_->pendingTasks[task.taskId] = task;
    impl_->taskQueue.push(task);
    impl_->stats.tasksSent++;
    
    // Serialize and publish to network
    // TODO: Proper serialization
    std::vector<uint8_t> payload; // Empty for now
    
    impl_->network->publish(impl_->config.tasksTopicPrefix, payload);
    
    std::cout << "[TaskProtocol] Distributed task: " << task.taskId << std::endl;
    return true;
}

std::optional<TaskResult> DistributedTaskProtocol::executeTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->pendingTasks.find(taskId);
    if (it == impl_->pendingTasks.end()) {
        return std::nullopt;
    }
    
    // Execute immediately
    auto task = it->second;
    impl_->pendingTasks.erase(it);
    
    // Find executor
    auto executorIt = impl_->executors.find(task.type);
    if (executorIt == impl_->executors.end()) {
        return std::nullopt;
    }
    
    return executorIt->second(task);
}

bool DistributedTaskProtocol::cancelTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    impl_->pendingTasks.erase(taskId);
    impl_->runningTasks.erase(taskId);
    
    if (impl_->eventCallback) {
        impl_->eventCallback(taskId, TaskEvent::CANCELLED, "Task cancelled");
    }
    
    return true;
}

DistributedTaskProtocol::TaskStatus DistributedTaskProtocol::getTaskStatus(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->completedTasks.count(taskId)) {
        auto& result = impl_->completedTasks.at(taskId);
        return result.success ? TaskStatus::COMPLETED : TaskStatus::FAILED;
    }
    
    if (impl_->runningTasks.count(taskId)) {
        return TaskStatus::RUNNING;
    }
    
    if (impl_->pendingTasks.count(taskId)) {
        return TaskStatus::PENDING;
    }
    
    return TaskStatus::UNKNOWN;
}

std::optional<TaskResult> DistributedTaskProtocol::getTaskResult(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto it = impl_->completedTasks.find(taskId);
    if (it != impl_->completedTasks.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

void DistributedTaskProtocol::registerExecutor(TaskType type, TaskExecutor executor) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->executors[type] = executor;
    std::cout << "[TaskProtocol] Registered executor for task type: " << static_cast<int>(type) << std::endl;
}

void DistributedTaskProtocol::setEventCallback(TaskEventCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->eventCallback = callback;
}

DistributedTaskProtocol::ProtocolStats DistributedTaskProtocol::getStats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto stats = impl_->stats;
    stats.currentPendingTasks = impl_->pendingTasks.size();
    stats.currentRunningTasks = impl_->runningTasks.size();
    
    return stats;
}

} // namespace ailee::orchestration
