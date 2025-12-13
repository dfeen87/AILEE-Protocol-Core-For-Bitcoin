#pragma once
#include <string>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <memory>
#include <functional>

namespace ailee::core {

// ============================================================================
// Configuration Exceptions
// ============================================================================

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& msg) 
        : std::runtime_error("Configuration error: " + msg) {}
};

class ValidationException : public ConfigException {
public:
    explicit ValidationException(const std::string& msg)
        : ConfigException("Validation failed: " + msg) {}
};

// ============================================================================
// Topic Configuration
// ============================================================================

struct Topics {
    std::string discovery = "ambient-ai-discovery-v1";
    std::string task = "ambient-ai-v1-task";
    std::string result = "ambient-ai-v1-result";
    std::string ledger = "ambient-ai-v1-ledger";
    std::string heartbeat = "ambient-ai-v1-heartbeat";
    std::string payment = "ambient-ai-v1-payment";
    std::string error = "ambient-ai-v1-error";
    
    // Custom topics for extensibility
    std::unordered_map<std::string, std::string> custom;
    
    bool validate() const {
        return !discovery.empty() && !task.empty() && 
               !result.empty() && !ledger.empty();
    }
    
    void validateOrThrow() const {
        if (!validate()) {
            throw ValidationException("Topics must not be empty");
        }
    }
    
    std::string getTopic(const std::string& key) const {
        auto it = custom.find(key);
        return (it != custom.end()) ? it->second : "";
    }
    
    void setTopic(const std::string& key, const std::string& value) {
        if (!value.empty()) {
            custom[key] = value;
        }
    }
};

// ============================================================================
// Interval Configuration
// ============================================================================

struct Intervals {
    std::chrono::milliseconds heartbeat{10000};
    std::chrono::milliseconds taskPost{15000};
    std::chrono::milliseconds ledgerBroadcast{20000};
    std::chrono::milliseconds workerDiscovery{5000};
    std::chrono::milliseconds taskTimeout{30000};
    std::chrono::milliseconds connectionRetry{3000};
    std::chrono::milliseconds healthCheck{60000};
    
    // Limits
    std::chrono::milliseconds minInterval{100};
    std::chrono::milliseconds maxInterval{300000}; // 5 minutes
    
    bool validate() const {
        return heartbeat >= minInterval && heartbeat <= maxInterval &&
               taskPost >= minInterval && taskPost <= maxInterval &&
               ledgerBroadcast >= minInterval && ledgerBroadcast <= maxInterval &&
               workerDiscovery >= minInterval && workerDiscovery <= maxInterval &&
               taskTimeout >= minInterval && taskTimeout <= maxInterval;
    }
    
    void validateOrThrow() const {
        if (!validate()) {
            throw ValidationException(
                "Intervals must be between " + 
                std::to_string(minInterval.count()) + "ms and " + 
                std::to_string(maxInterval.count()) + "ms"
            );
        }
    }
    
    // Ensure reasonable relationships
    bool hasReasonableRelationships() const {
        // Heartbeat should be less frequent than task posting
        if (heartbeat >= taskPost) return false;
        
        // Task timeout should be significantly longer than task post
        if (taskTimeout <= taskPost * 2) return false;
        
        return true;
    }
};

// ============================================================================
// Network Configuration
// ============================================================================

struct NetworkConfig {
    std::string bootstrapAddress;
    std::vector<std::string> bootstrapPeers;
    std::uint16_t listenPort = 4001;
    std::uint32_t maxConnections = 100;
    std::uint32_t maxMessageSize = 10 * 1024 * 1024; // 10MB
    std::chrono::seconds connectionTimeout{30};
    bool enableNatTraversal = true;
    bool enableRelay = true;
    
    bool validate() const {
        return listenPort > 0 && maxConnections > 0 && maxMessageSize > 0;
    }
};

// ============================================================================
// Runtime Configuration
// ============================================================================

enum class ZkVerifierType {
    RISCZERO,
    ZKML,
    SNARK,
    PLONK,
    CUSTOM
};

inline std::string zkVerifierTypeToString(ZkVerifierType type) {
    switch (type) {
        case ZkVerifierType::RISCZERO: return "risczero";
        case ZkVerifierType::ZKML: return "zkml";
        case ZkVerifierType::SNARK: return "snark";
        case ZkVerifierType::PLONK: return "plonk";
        case ZkVerifierType::CUSTOM: return "custom";
        default: return "unknown";
    }
}

inline std::optional<ZkVerifierType> zkVerifierTypeFromString(const std::string& str) {
    if (str == "risczero") return ZkVerifierType::RISCZERO;
    if (str == "zkml") return ZkVerifierType::ZKML;
    if (str == "snark") return ZkVerifierType::SNARK;
    if (str == "plonk") return ZkVerifierType::PLONK;
    if (str == "custom") return ZkVerifierType::CUSTOM;
    return std::nullopt;
}

enum class WasmRuntimeType {
    WASMEDGE,
    WASMER,
    WAVM,
    WASMTIME,
    CUSTOM
};

inline std::string wasmRuntimeTypeToString(WasmRuntimeType type) {
    switch (type) {
        case WasmRuntimeType::WASMEDGE: return "wasmedge";
        case WasmRuntimeType::WASMER: return "wasmer";
        case WasmRuntimeType::WAVM: return "wavm";
        case WasmRuntimeType::WASMTIME: return "wasmtime";
        case WasmRuntimeType::CUSTOM: return "custom";
        default: return "unknown";
    }
}

inline std::optional<WasmRuntimeType> wasmRuntimeTypeFromString(const std::string& str) {
    if (str == "wasmedge") return WasmRuntimeType::WASMEDGE;
    if (str == "wasmer") return WasmRuntimeType::WASMER;
    if (str == "wavm") return WasmRuntimeType::WAVM;
    if (str == "wasmtime") return WasmRuntimeType::WASMTIME;
    if (str == "custom") return WasmRuntimeType::CUSTOM;
    return std::nullopt;
}

struct RuntimeConfig {
    std::string zkVerifierId = "risczero";
    std::string wasmRuntimeId = "wasmedge";
    
    ZkVerifierType zkVerifierType = ZkVerifierType::RISCZERO;
    WasmRuntimeType wasmRuntimeType = WasmRuntimeType::WASMEDGE;
    
    std::uint64_t maxMemoryBytes = 512 * 1024 * 1024; // 512MB
    std::uint32_t maxExecutionTimeMs = 60000; // 1 minute
    bool enableSandbox = true;
    
    // Custom runtime options
    std::unordered_map<std::string, std::string> customOptions;
    
    void syncTypes() {
        auto zkType = zkVerifierTypeFromString(zkVerifierId);
        if (zkType) zkVerifierType = *zkType;
        
        auto wasmType = wasmRuntimeTypeFromString(wasmRuntimeId);
        if (wasmType) wasmRuntimeType = *wasmType;
    }
    
    bool validate() const {
        return !zkVerifierId.empty() && !wasmRuntimeId.empty() &&
               maxMemoryBytes > 0 && maxExecutionTimeMs > 0;
    }
};

// ============================================================================
// Security Configuration
// ============================================================================

struct SecurityConfig {
    bool requireProofOfWork = false;
    std::uint32_t proofOfWorkDifficulty = 4;
    bool requireSignedMessages = true;
    bool enableEncryption = true;
    bool validateTaskPayloads = true;
    
    // Allowed/blocked peers
    std::vector<std::string> allowedPeers;
    std::vector<std::string> blockedPeers;
    
    bool isPeerAllowed(const std::string& peerId) const {
        // Check blocked list first
        for (const auto& blocked : blockedPeers) {
            if (blocked == peerId) return false;
        }
        
        // If allow list is empty, allow all (except blocked)
        if (allowedPeers.empty()) return true;
        
        // Check allow list
        for (const auto& allowed : allowedPeers) {
            if (allowed == peerId) return true;
        }
        
        return false;
    }
};

// ============================================================================
// Performance Configuration
// ============================================================================

struct PerformanceConfig {
    std::uint32_t workerThreads = 4;
    std::uint32_t ioThreads = 2;
    std::uint32_t taskQueueSize = 1000;
    std::uint32_t messageQueueSize = 10000;
    bool enableTaskBatching = true;
    std::uint32_t taskBatchSize = 10;
    
    // Memory limits
    std::uint64_t maxTotalMemoryBytes = 2ULL * 1024 * 1024 * 1024; // 2GB
    std::uint64_t maxTaskMemoryBytes = 256 * 1024 * 1024; // 256MB
    
    bool validate() const {
        return workerThreads > 0 && ioThreads > 0 &&
               taskQueueSize > 0 && messageQueueSize > 0 &&
               maxTotalMemoryBytes > 0 && maxTaskMemoryBytes > 0 &&
               maxTaskMemoryBytes <= maxTotalMemoryBytes;
    }
};

// ============================================================================
// Main Configuration
// ============================================================================

struct Config {
    Topics topics;
    Intervals intervals;
    NetworkConfig network;
    RuntimeConfig runtime;
    SecurityConfig security;
    PerformanceConfig performance;
    
    // Deprecated fields for backward compatibility
    std::string zkVerifierId = "risczero";
    std::string wasmRuntimeId = "wasmedge";
    
    // Environment and deployment
    std::string environment = "development"; // "development", "staging", "production"
    std::string nodeId;
    std::string version = "1.0.0";
    
    // Feature flags
    std::unordered_map<std::string, bool> features;
    
    Config() {
        syncDeprecatedFields();
    }
    
    // Validate entire configuration
    bool validate() const {
        return topics.validate() &&
               intervals.validate() &&
               network.validate() &&
               runtime.validate() &&
               performance.validate();
    }
    
    void validateOrThrow() const {
        topics.validateOrThrow();
        intervals.validateOrThrow();
        
        if (!network.validate()) {
            throw ValidationException("Invalid network configuration");
        }
        if (!runtime.validate()) {
            throw ValidationException("Invalid runtime configuration");
        }
        if (!performance.validate()) {
            throw ValidationException("Invalid performance configuration");
        }
        if (!intervals.hasReasonableRelationships()) {
            throw ValidationException("Interval relationships are unreasonable");
        }
    }
    
    // Feature flag helpers
    bool isFeatureEnabled(const std::string& feature) const {
        auto it = features.find(feature);
        return it != features.end() && it->second;
    }
    
    void enableFeature(const std::string& feature) {
        features[feature] = true;
    }
    
    void disableFeature(const std::string& feature) {
        features[feature] = false;
    }
    
    // Environment helpers
    bool isDevelopment() const { return environment == "development"; }
    bool isStaging() const { return environment == "staging"; }
    bool isProduction() const { return environment == "production"; }
    
private:
    void syncDeprecatedFields() {
        runtime.zkVerifierId = zkVerifierId;
        runtime.wasmRuntimeId = wasmRuntimeId;
        runtime.syncTypes();
    }
};

// ============================================================================
// Configuration Builder
// ============================================================================

class ConfigBuilder {
public:
    ConfigBuilder() = default;
    
    ConfigBuilder& setTopics(const Topics& t) {
        config_.topics = t;
        return *this;
    }
    
    ConfigBuilder& setIntervals(const Intervals& i) {
        config_.intervals = i;
        return *this;
    }
    
    ConfigBuilder& setNetwork(const NetworkConfig& n) {
        config_.network = n;
        return *this;
    }
    
    ConfigBuilder& setRuntime(const RuntimeConfig& r) {
        config_.runtime = r;
        return *this;
    }
    
    ConfigBuilder& setSecurity(const SecurityConfig& s) {
        config_.security = s;
        return *this;
    }
    
    ConfigBuilder& setPerformance(const PerformanceConfig& p) {
        config_.performance = p;
        return *this;
    }
    
    ConfigBuilder& setEnvironment(const std::string& env) {
        config_.environment = env;
        return *this;
    }
    
    ConfigBuilder& setNodeId(const std::string& id) {
        config_.nodeId = id;
        return *this;
    }
    
    ConfigBuilder& enableFeature(const std::string& feature) {
        config_.features[feature] = true;
        return *this;
    }
    
    Config build() {
        config_.validateOrThrow();
        return config_;
    }
    
    Config buildUnchecked() {
        return config_;
    }
    
private:
    Config config_;
};

// ============================================================================
// Configuration Presets
// ============================================================================

inline Config createDevelopmentConfig() {
    Config config;
    config.environment = "development";
    config.security.requireProofOfWork = false;
    config.security.requireSignedMessages = false;
    config.performance.workerThreads = 2;
    config.intervals.heartbeat = std::chrono::milliseconds(5000);
    return config;
}

inline Config createProductionConfig() {
    Config config;
    config.environment = "production";
    config.security.requireProofOfWork = true;
    config.security.requireSignedMessages = true;
    config.security.enableEncryption = true;
    config.performance.workerThreads = 8;
    config.intervals.heartbeat = std::chrono::milliseconds(10000);
    return config;
}

inline Config createTestConfig() {
    Config config;
    config.environment = "test";
    config.intervals.heartbeat = std::chrono::milliseconds(100);
    config.intervals.taskPost = std::chrono::milliseconds(200);
    config.intervals.ledgerBroadcast = std::chrono::milliseconds(300);
    config.performance.taskQueueSize = 10;
    return config;
}

// ============================================================================
// Configuration Loading (extensibility point)
// ============================================================================

using ConfigLoader = std::function<Config(const std::string& path)>;
using ConfigSaver = std::function<bool(const Config&, const std::string& path)>;

class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager mgr;
        return mgr;
    }
    
    void registerLoader(const std::string& format, ConfigLoader loader) {
        loaders_[format] = std::move(loader);
    }
    
    void registerSaver(const std::string& format, ConfigSaver saver) {
        savers_[format] = std::move(saver);
    }
    
    Config load(const std::string& path, const std::string& format = "json") {
        auto it = loaders_.find(format);
        if (it == loaders_.end()) {
            throw ConfigException("No loader registered for format: " + format);
        }
        return it->second(path);
    }
    
    bool save(const Config& config, const std::string& path, const std::string& format = "json") {
        auto it = savers_.find(format);
        if (it == savers_.end()) {
            throw ConfigException("No saver registered for format: " + format);
        }
        return it->second(config, path);
    }
    
private:
    ConfigManager() = default;
    std::unordered_map<std::string, ConfigLoader> loaders_;
    std::unordered_map<std::string, ConfigSaver> savers_;
};

} // namespace ailee::core
