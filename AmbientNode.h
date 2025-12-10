#pragma once
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <cstdint>

namespace ailee::net {

// ============================================================================
// Error Handling
// ============================================================================

enum class NetworkErrorCode {
    SUCCESS = 0,
    INVALID_TOPIC,
    INVALID_DATA,
    NOT_CONNECTED,
    ALREADY_SUBSCRIBED,
    SUBSCRIPTION_FAILED,
    PUBLISH_FAILED,
    TIMEOUT,
    NODE_NOT_RUNNING,
    INVALID_CAPABILITIES,
    TASK_REJECTED,
    SERIALIZATION_ERROR,
    UNKNOWN_ERROR
};

struct NetworkError {
    NetworkErrorCode code;
    std::string message;
    
    NetworkError(NetworkErrorCode c = NetworkErrorCode::SUCCESS, std::string msg = "")
        : code(c), message(std::move(msg)) {}
    
    bool isSuccess() const { return code == NetworkErrorCode::SUCCESS; }
    explicit operator bool() const { return !isSuccess(); }
};

// ============================================================================
// Message Types
// ============================================================================

struct Message {
    std::string topic;
    std::vector<std::uint8_t> data;
    std::uint64_t timestamp = 0;
    std::string senderId;
    std::optional<std::string> correlationId; // For request-response patterns
    
    Message() = default;
    Message(std::string t, std::vector<std::uint8_t> d)
        : topic(std::move(t)), data(std::move(d)) {}
    
    bool isValid() const {
        return !topic.empty() && !data.empty();
    }
    
    std::size_t size() const {
        return data.size();
    }
};

// ============================================================================
// PubSub Interface
// ============================================================================

using MessageHandler = std::function<void(const Message&)>;
using SubscriptionId = std::uint64_t;

struct SubscriptionOptions {
    bool allowRedelivery = false;
    std::uint32_t maxRetries = 3;
    std::chrono::milliseconds timeout = std::chrono::milliseconds(5000);
    std::optional<std::string> filterPattern; // For topic filtering
};

struct PublishOptions {
    bool requireAck = false;
    std::chrono::milliseconds timeout = std::chrono::milliseconds(3000);
    std::uint8_t priority = 0; // 0 = normal, higher = more priority
};

class IPubSub {
public:
    virtual ~IPubSub() = default;
    
    // Basic publish/subscribe
    virtual NetworkError publish(const Message& m) = 0;
    virtual NetworkError publish(const Message& m, const PublishOptions& opts) = 0;
    
    virtual NetworkError subscribe(const std::string& topic, MessageHandler handler) = 0;
    virtual NetworkError subscribe(const std::string& topic, MessageHandler handler,
                                   const SubscriptionOptions& opts, SubscriptionId* outId) = 0;
    
    virtual NetworkError unsubscribe(const std::string& topic) = 0;
    virtual NetworkError unsubscribe(SubscriptionId id) = 0;
    
    // Status queries
    virtual bool isConnected() const = 0;
    virtual std::vector<std::string> getSubscribedTopics() const = 0;
    virtual std::size_t getSubscriptionCount() const = 0;
    
    // Lifecycle
    virtual NetworkError connect() = 0;
    virtual void disconnect() noexcept = 0;
};

// ============================================================================
// Worker Node Types
// ============================================================================

enum class WorkerState {
    UNINITIALIZED,
    IDLE,
    WORKING,
    PAUSED,
    ERROR,
    SHUTDOWN
};

inline std::string workerStateToString(WorkerState state) {
    switch (state) {
        case WorkerState::UNINITIALIZED: return "UNINITIALIZED";
        case WorkerState::IDLE: return "IDLE";
        case WorkerState::WORKING: return "WORKING";
        case WorkerState::PAUSED: return "PAUSED";
        case WorkerState::ERROR: return "ERROR";
        case WorkerState::SHUTDOWN: return "SHUTDOWN";
        default: return "UNKNOWN";
    }
}

struct WorkerCapabilities {
    std::string type;        // e.g., "cpu", "gpu", "hybrid"
    std::string capacity;    // e.g., "low", "medium", "high"
    std::uint32_t maxConcurrentTasks = 1;
    std::vector<std::string> supportedFormats;
    std::unordered_map<std::string, std::string> customAttributes;
    
    bool isValid() const {
        return !type.empty() && !capacity.empty() && maxConcurrentTasks > 0;
    }
    
    std::string toString() const {
        std::string result = "TYPE=" + type + ",CAP=" + capacity;
        if (maxConcurrentTasks > 1) {
            result += ",MAX_TASKS=" + std::to_string(maxConcurrentTasks);
        }
        return result;
    }
    
    static std::optional<WorkerCapabilities> parse(const std::string& str);
};

struct WorkerStatus {
    WorkerState state = WorkerState::UNINITIALIZED;
    WorkerCapabilities capabilities;
    std::uint32_t activeTasks = 0;
    std::uint64_t totalTasksProcessed = 0;
    std::uint64_t totalTasksFailed = 0;
    std::chrono::milliseconds uptime{0};
    std::optional<std::string> errorMessage;
    std::uint64_t lastHeartbeat = 0;
    
    bool isAvailable() const {
        return state == WorkerState::IDLE && 
               activeTasks < capabilities.maxConcurrentTasks;
    }
    
    double getSuccessRate() const {
        auto total = totalTasksProcessed + totalTasksFailed;
        return total > 0 ? static_cast<double>(totalTasksProcessed) / total : 1.0;
    }
};

struct TaskInfo {
    std::string taskId;
    std::string requesterPeerId;
    std::vector<std::uint8_t> payload;
    std::uint64_t submittedAt = 0;
    std::uint64_t startedAt = 0;
    std::optional<std::uint64_t> completedAt;
    std::uint32_t retryCount = 0;
};

// ============================================================================
// Worker Node Callbacks
// ============================================================================

using TaskHandler = std::function<bool(const TaskInfo&, std::vector<std::uint8_t>* result)>;
using StatusChangeCallback = std::function<void(WorkerState oldState, WorkerState newState)>;
using ErrorCallback = std::function<void(const NetworkError&)>;

// ============================================================================
// Worker Node Interface
// ============================================================================

class IWorkerNode {
public:
    virtual ~IWorkerNode() = default;
    
    // Lifecycle management
    virtual NetworkError start() = 0;
    virtual void stop() noexcept = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    
    // Status and monitoring
    virtual WorkerStatus status() const = 0;
    virtual WorkerState getState() const = 0;
    virtual bool isRunning() const = 0;
    
    // Configuration
    virtual void setCapabilities(const WorkerCapabilities& caps) = 0;
    virtual WorkerCapabilities getCapabilities() const = 0;
    
    // Task handling
    virtual void setTaskHandler(TaskHandler handler) = 0;
    virtual std::vector<TaskInfo> getActiveTasks() const = 0;
    virtual bool cancelTask(const std::string& taskId) = 0;
    
    // Callbacks
    virtual void setStatusChangeCallback(StatusChangeCallback callback) = 0;
    virtual void setErrorCallback(ErrorCallback callback) = 0;
    
    // Heartbeat and health
    virtual void sendHeartbeat() = 0;
    virtual std::chrono::milliseconds getUptime() const = 0;
};

// ============================================================================
// Concrete Implementation: AmbientWorkerNode
// ============================================================================

class AmbientWorkerNode : public IWorkerNode {
public:
    explicit AmbientWorkerNode(std::shared_ptr<IPubSub> pubsub, 
                              const WorkerCapabilities& caps);
    
    AmbientWorkerNode(const AmbientWorkerNode&) = delete;
    AmbientWorkerNode& operator=(const AmbientWorkerNode&) = delete;
    AmbientWorkerNode(AmbientWorkerNode&&) noexcept = default;
    AmbientWorkerNode& operator=(AmbientWorkerNode&&) noexcept = default;
    
    ~AmbientWorkerNode() override;
    
    // Lifecycle
    NetworkError start() override;
    void stop() noexcept override;
    void pause() override;
    void resume() override;
    
    // Status
    WorkerStatus status() const override;
    WorkerState getState() const override;
    bool isRunning() const override;
    
    // Configuration
    void setCapabilities(const WorkerCapabilities& caps) override;
    WorkerCapabilities getCapabilities() const override;
    
    // Task handling
    void setTaskHandler(TaskHandler handler) override;
    std::vector<TaskInfo> getActiveTasks() const override;
    bool cancelTask(const std::string& taskId) override;
    
    // Callbacks
    void setStatusChangeCallback(StatusChangeCallback callback) override;
    void setErrorCallback(ErrorCallback callback) override;
    
    // Heartbeat
    void sendHeartbeat() override;
    std::chrono::milliseconds getUptime() const override;
    
private:
    void changeState(WorkerState newState);
    void handleIncomingTask(const Message& msg);
    void processTask(const TaskInfo& taskInfo);
    void reportTaskResult(const std::string& taskId, bool success, 
                         const std::vector<std::uint8_t>& result);
    void emitError(NetworkErrorCode code, const std::string& message);
    
    std::shared_ptr<IPubSub> pubsub_;
    WorkerCapabilities capabilities_;
    
    mutable std::mutex state_mutex_;
    std::atomic<WorkerState> state_{WorkerState::UNINITIALIZED};
    
    mutable std::mutex tasks_mutex_;
    std::unordered_map<std::string, TaskInfo> active_tasks_;
    
    std::uint64_t total_tasks_processed_ = 0;
    std::uint64_t total_tasks_failed_ = 0;
    
    std::mutex callback_mutex_;
    TaskHandler task_handler_;
    StatusChangeCallback status_callback_;
    ErrorCallback error_callback_;
    
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<std::uint64_t> last_heartbeat_{0};
    
    SubscriptionId task_subscription_id_ = 0;
};

} // namespace ailee::net
