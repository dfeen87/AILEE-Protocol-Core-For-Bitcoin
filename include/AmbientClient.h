#pragma once
#include "AmbientNode.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <future>
#include <atomic>

namespace ailee::net {

// ============================================================================
// Task Types
// ============================================================================

enum class TaskStatus {
    PENDING,
    SUBMITTED,
    ASSIGNED,
    PROCESSING,
    COMPLETED,
    FAILED,
    CANCELLED,
    TIMEOUT
};

inline std::string taskStatusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::PENDING: return "PENDING";
        case TaskStatus::SUBMITTED: return "SUBMITTED";
        case TaskStatus::ASSIGNED: return "ASSIGNED";
        case TaskStatus::PROCESSING: return "PROCESSING";
        case TaskStatus::COMPLETED: return "COMPLETED";
        case TaskStatus::FAILED: return "FAILED";
        case TaskStatus::CANCELLED: return "CANCELLED";
        case TaskStatus::TIMEOUT: return "TIMEOUT";
        default: return "UNKNOWN";
    }
}

struct TaskRequest {
    std::string taskId;
    std::vector<std::uint8_t> payload;
    WorkerCapabilities requiredCapabilities;
    std::chrono::milliseconds timeout = std::chrono::milliseconds(30000);
    std::uint32_t maxRetries = 3;
    std::uint8_t priority = 0;
    std::unordered_map<std::string, std::string> metadata;
    
    bool isValid() const {
        return !taskId.empty() && !payload.empty();
    }
    
    std::size_t size() const {
        return payload.size();
    }
};

struct TaskResult {
    std::string taskId;
    TaskStatus status;
    std::vector<std::uint8_t> result;
    std::optional<std::string> workerPeerId;
    std::uint64_t submittedAt = 0;
    std::uint64_t completedAt = 0;
    std::optional<NetworkError> error;
    std::uint32_t retryCount = 0;
    
    bool isSuccess() const {
        return status == TaskStatus::COMPLETED;
    }
    
    std::chrono::milliseconds processingTime() const {
        if (completedAt > submittedAt) {
            return std::chrono::milliseconds(completedAt - submittedAt);
        }
        return std::chrono::milliseconds(0);
    }
};

// ============================================================================
// Callbacks
// ============================================================================

using TaskCompletionCallback = std::function<void(const TaskResult&)>;
using TaskProgressCallback = std::function<void(const std::string& taskId, TaskStatus status)>;
using WorkerDiscoveryCallback = std::function<void(const std::string& peerId, const WorkerCapabilities&)>;

// ============================================================================
// Requester Client Options
// ============================================================================

struct RequesterOptions {
    std::chrono::milliseconds defaultTimeout = std::chrono::milliseconds(30000);
    std::uint32_t maxConcurrentTasks = 10;
    std::uint32_t maxRetries = 3;
    bool enableTaskPriority = true;
    bool autoRetryOnFailure = true;
    std::chrono::milliseconds workerDiscoveryInterval = std::chrono::milliseconds(5000);
};

// ============================================================================
// Requester Client Interface
// ============================================================================

class IRequesterClient {
public:
    virtual ~IRequesterClient() = default;
    
    // Task submission
    virtual NetworkError postTask(const TaskRequest& request) = 0;
    virtual NetworkError postTask(const std::vector<std::uint8_t>& payload, 
                                 std::string* outTaskId) = 0;
    
    // Async task submission with futures
    virtual std::future<TaskResult> postTaskAsync(const TaskRequest& request) = 0;
    
    // Task management
    virtual bool cancelTask(const std::string& taskId) = 0;
    virtual std::optional<TaskResult> getTaskResult(const std::string& taskId) const = 0;
    virtual TaskStatus getTaskStatus(const std::string& taskId) const = 0;
    virtual std::vector<std::string> getActiveTasks() const = 0;
    
    // Blocking wait for result
    virtual std::optional<TaskResult> waitForResult(const std::string& taskId, 
                                                    std::chrono::milliseconds timeout) = 0;
    
    // Worker discovery
    virtual std::vector<std::string> getAvailableWorkers() const = 0;
    virtual std::optional<WorkerCapabilities> getWorkerCapabilities(const std::string& peerId) const = 0;
    virtual void requestWorkerDiscovery() = 0;
    
    // Callbacks
    virtual void setCompletionCallback(TaskCompletionCallback callback) = 0;
    virtual void setProgressCallback(TaskProgressCallback callback) = 0;
    virtual void setWorkerDiscoveryCallback(WorkerDiscoveryCallback callback) = 0;
    
    // Lifecycle
    virtual NetworkError start() = 0;
    virtual void stop() noexcept = 0;
    virtual bool isRunning() const = 0;
    
    // Statistics
    virtual std::size_t getPendingTaskCount() const = 0;
    virtual std::size_t getCompletedTaskCount() const = 0;
    virtual std::size_t getFailedTaskCount() const = 0;
};

// ============================================================================
// Concrete Implementation: AmbientRequesterClient
// ============================================================================

class AmbientRequesterClient : public IRequesterClient {
public:
    explicit AmbientRequesterClient(std::shared_ptr<IPubSub> pubsub,
                                   const RequesterOptions& opts = RequesterOptions{});
    
    AmbientRequesterClient(const AmbientRequesterClient&) = delete;
    AmbientRequesterClient& operator=(const AmbientRequesterClient&) = delete;
    AmbientRequesterClient(AmbientRequesterClient&&) noexcept = default;
    AmbientRequesterClient& operator=(AmbientRequesterClient&&) noexcept = default;
    
    ~AmbientRequesterClient() override;
    
    // Task submission
    NetworkError postTask(const TaskRequest& request) override;
    NetworkError postTask(const std::vector<std::uint8_t>& payload, 
                         std::string* outTaskId) override;
    
    std::future<TaskResult> postTaskAsync(const TaskRequest& request) override;
    
    // Task management
    bool cancelTask(const std::string& taskId) override;
    std::optional<TaskResult> getTaskResult(const std::string& taskId) const override;
    TaskStatus getTaskStatus(const std::string& taskId) const override;
    std::vector<std::string> getActiveTasks() const override;
    
    std::optional<TaskResult> waitForResult(const std::string& taskId, 
                                           std::chrono::milliseconds timeout) override;
    
    // Worker discovery
    std::vector<std::string> getAvailableWorkers() const override;
    std::optional<WorkerCapabilities> getWorkerCapabilities(const std::string& peerId) const override;
    void requestWorkerDiscovery() override;
    
    // Callbacks
    void setCompletionCallback(TaskCompletionCallback callback) override;
    void setProgressCallback(TaskProgressCallback callback) override;
    void setWorkerDiscoveryCallback(WorkerDiscoveryCallback callback) override;
    
    // Lifecycle
    NetworkError start() override;
    void stop() noexcept override;
    bool isRunning() const override;
    
    // Statistics
    std::size_t getPendingTaskCount() const override;
    std::size_t getCompletedTaskCount() const override;
    std::size_t getFailedTaskCount() const override;
    
private:
    struct TaskState {
        TaskRequest request;
        TaskStatus status;
        std::optional<TaskResult> result;
        std::uint64_t submittedAt;
        std::uint32_t retryCount;
        std::promise<TaskResult> promise;
        std::shared_ptr<std::condition_variable> cv;
        
        TaskState(TaskRequest req)
            : request(std::move(req))
            , status(TaskStatus::PENDING)
            , submittedAt(0)
            , retryCount(0)
            , cv(std::make_shared<std::condition_variable>()) {}
    };
    
    struct WorkerInfo {
        std::string peerId;
        WorkerCapabilities capabilities;
        WorkerState state;
        std::uint64_t lastSeen;
        
        bool isAvailable() const {
            return state == WorkerState::IDLE;
        }
    };
    
    void handleTaskResult(const Message& msg);
    void handleWorkerAnnouncement(const Message& msg);
    void updateTaskStatus(const std::string& taskId, TaskStatus newStatus);
    void completeTask(const std::string& taskId, const TaskResult& result);
    void retryTask(const std::string& taskId);
    std::string generateTaskId();
    std::optional<std::string> selectWorker(const WorkerCapabilities& required);
    void publishTask(const TaskRequest& request);
    void cleanupExpiredTasks();
    void emitProgress(const std::string& taskId, TaskStatus status);
    
    std::shared_ptr<IPubSub> pubsub_;
    RequesterOptions options_;
    
    std::atomic<bool> running_{false};
    
    mutable std::mutex tasks_mutex_;
    std::unordered_map<std::string, std::shared_ptr<TaskState>> tasks_;
    
    mutable std::mutex workers_mutex_;
    std::unordered_map<std::string, WorkerInfo> workers_;
    
    std::mutex callback_mutex_;
    TaskCompletionCallback completion_callback_;
    TaskProgressCallback progress_callback_;
    WorkerDiscoveryCallback worker_discovery_callback_;
    
    std::atomic<std::uint64_t> task_counter_{0};
    std::atomic<std::size_t> completed_count_{0};
    std::atomic<std::size_t> failed_count_{0};
    
    SubscriptionId result_subscription_id_ = 0;
    SubscriptionId worker_announcement_subscription_id_ = 0;
};

// ============================================================================
// Utility Functions
// ============================================================================

// Helper to create a simple task request
inline TaskRequest createSimpleTask(const std::vector<std::uint8_t>& payload) {
    TaskRequest request;
    request.taskId = ""; // Will be generated
    request.payload = payload;
    return request;
}

// Helper to check if capabilities match requirements
inline bool capabilitiesMatch(const WorkerCapabilities& available, 
                              const WorkerCapabilities& required) {
    if (!required.type.empty() && available.type != required.type) {
        return false;
    }
    
    if (!required.capacity.empty() && available.capacity != required.capacity) {
        return false;
    }
    
    // Check if worker can handle the required formats
    for (const auto& format : required.supportedFormats) {
        bool found = false;
        for (const auto& availFormat : available.supportedFormats) {
            if (availFormat == format) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    return true;
}

} // namespace ailee::net
