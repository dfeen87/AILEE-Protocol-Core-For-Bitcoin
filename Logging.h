#pragma once
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <atomic>

namespace ailee::log {

// ============================================================================
// Log Levels
// ============================================================================

enum class Level {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
    OFF = 6
};

inline std::string levelToString(Level level) {
    switch (level) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        case Level::FATAL: return "FATAL";
        case Level::OFF:   return "OFF";
        default: return "UNKNOWN";
    }
}

inline std::string levelToShortString(Level level) {
    switch (level) {
        case Level::TRACE: return "TRC";
        case Level::DEBUG: return "DBG";
        case Level::INFO:  return "INF";
        case Level::WARN:  return "WRN";
        case Level::ERROR: return "ERR";
        case Level::FATAL: return "FTL";
        case Level::OFF:   return "OFF";
        default: return "UNK";
    }
}

inline Level levelFromString(const std::string& str) {
    if (str == "TRACE" || str == "trace") return Level::TRACE;
    if (str == "DEBUG" || str == "debug") return Level::DEBUG;
    if (str == "INFO" || str == "info") return Level::INFO;
    if (str == "WARN" || str == "warn") return Level::WARN;
    if (str == "ERROR" || str == "error") return Level::ERROR;
    if (str == "FATAL" || str == "fatal") return Level::FATAL;
    if (str == "OFF" || str == "off") return Level::OFF;
    return Level::INFO;
}

// ============================================================================
// Log Entry
// ============================================================================

struct LogEntry {
    Level level;
    std::string message;
    std::string logger_name;
    std::chrono::system_clock::time_point timestamp;
    std::string thread_id;
    std::string file;
    int line = 0;
    std::string function;
    std::unordered_map<std::string, std::string> context;
    
    LogEntry(Level lvl, std::string msg)
        : level(lvl)
        , message(std::move(msg))
        , timestamp(std::chrono::system_clock::now()) {}
};

// ============================================================================
// Sink Interface
// ============================================================================

using Sink = std::function<void(Level, const std::string&)>;
using StructuredSink = std::function<void(const LogEntry&)>;

class ISink {
public:
    virtual ~ISink() = default;
    virtual void log(const LogEntry& entry) = 0;
    virtual void flush() {}
};

// ============================================================================
// Built-in Sinks
// ============================================================================

class ConsoleSink : public ISink {
public:
    explicit ConsoleSink(bool use_colors = true) 
        : use_colors_(use_colors) {}
    
    void log(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& stream = (entry.level >= Level::ERROR) ? std::cerr : std::cout;
        
        if (use_colors_) {
            stream << getColorCode(entry.level);
        }
        
        stream << formatEntry(entry);
        
        if (use_colors_) {
            stream << "\033[0m"; // Reset color
        }
        
        stream << std::endl;
    }
    
    void flush() override {
        std::cout.flush();
        std::cerr.flush();
    }
    
private:
    std::string getColorCode(Level level) const {
        switch (level) {
            case Level::TRACE: return "\033[37m";   // White
            case Level::DEBUG: return "\033[36m";   // Cyan
            case Level::INFO:  return "\033[32m";   // Green
            case Level::WARN:  return "\033[33m";   // Yellow
            case Level::ERROR: return "\033[31m";   // Red
            case Level::FATAL: return "\033[35m";   // Magenta
            default: return "\033[0m";
        }
    }
    
    std::string formatEntry(const LogEntry& entry) const {
        std::ostringstream oss;
        
        // Timestamp
        auto time = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;
        
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
        
        // Level
        oss << " [" << levelToShortString(entry.level) << "]";
        
        // Logger name
        if (!entry.logger_name.empty()) {
            oss << " [" << entry.logger_name << "]";
        }
        
        // Message
        oss << " " << entry.message;
        
        // Context
        if (!entry.context.empty()) {
            oss << " {";
            bool first = true;
            for (const auto& [key, value] : entry.context) {
                if (!first) oss << ", ";
                oss << key << "=" << value;
                first = false;
            }
            oss << "}";
        }
        
        return oss.str();
    }
    
    bool use_colors_;
    std::mutex mutex_;
};

class FileSink : public ISink {
public:
    explicit FileSink(const std::string& filepath, bool append = true)
        : filepath_(filepath) {
        auto mode = append ? std::ios::app : std::ios::trunc;
        file_.open(filepath, std::ios::out | mode);
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filepath);
        }
    }
    
    ~FileSink() override {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void log(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!file_.is_open()) return;
        
        file_ << formatEntry(entry) << std::endl;
        
        // Auto-flush for important messages
        if (entry.level >= Level::ERROR) {
            file_.flush();
        }
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.flush();
        }
    }
    
private:
    std::string formatEntry(const LogEntry& entry) const {
        std::ostringstream oss;
        
        auto time = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;
        
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
        oss << " [" << levelToString(entry.level) << "]";
        
        if (!entry.logger_name.empty()) {
            oss << " [" << entry.logger_name << "]";
        }
        
        oss << " " << entry.message;
        
        if (!entry.file.empty()) {
            oss << " (" << entry.file << ":" << entry.line << ")";
        }
        
        return oss.str();
    }
    
    std::string filepath_;
    std::ofstream file_;
    std::mutex mutex_;
};

// ============================================================================
// Logger Class
// ============================================================================

class Logger {
public:
    explicit Logger(Sink sink) 
        : legacy_sink_(std::move(sink))
        , use_legacy_(true)
        , min_level_(Level::INFO) {}
    
    explicit Logger(std::shared_ptr<ISink> sink, const std::string& name = "")
        : structured_sink_(std::move(sink))
        , use_legacy_(false)
        , logger_name_(name)
        , min_level_(Level::INFO) {}
    
    // Multi-sink support
    Logger(std::vector<std::shared_ptr<ISink>> sinks, const std::string& name = "")
        : sinks_(std::move(sinks))
        , use_legacy_(false)
        , logger_name_(name)
        , min_level_(Level::INFO) {}
    
    // Basic logging methods
    void trace(const std::string& msg) { log(Level::TRACE, msg); }
    void debug(const std::string& msg) { log(Level::DEBUG, msg); }
    void info(const std::string& msg)  { log(Level::INFO, msg);  }
    void warn(const std::string& msg)  { log(Level::WARN, msg);  }
    void error(const std::string& msg) { log(Level::ERROR, msg); }
    void fatal(const std::string& msg) { log(Level::FATAL, msg); }
    
    // Template methods for formatting
    template<typename... Args>
    void trace(const std::string& fmt, Args&&... args) {
        log(Level::TRACE, format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void debug(const std::string& fmt, Args&&... args) {
        log(Level::DEBUG, format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        log(Level::INFO, format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void warn(const std::string& fmt, Args&&... args) {
        log(Level::WARN, format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        log(Level::ERROR, format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void fatal(const std::string& fmt, Args&&... args) {
        log(Level::FATAL, format(fmt, std::forward<Args>(args)...));
    }
    
    // Structured logging with context
    void log(Level level, const std::string& msg, 
             const std::unordered_map<std::string, std::string>& context = {}) {
        if (level < min_level_.load()) return;
        
        LogEntry entry(level, msg);
        entry.logger_name = logger_name_;
        entry.context = context;
        
        logEntry(entry);
        
        total_messages_++;
        if (level >= Level::ERROR) {
            error_count_++;
        }
    }
    
    // Configuration
    void setMinLevel(Level level) {
        min_level_.store(level);
    }
    
    Level getMinLevel() const {
        return min_level_.load();
    }
    
    void setName(const std::string& name) {
        logger_name_ = name;
    }
    
    std::string getName() const {
        return logger_name_;
    }
    
    // Add context that persists across log calls
    void addContext(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(context_mutex_);
        persistent_context_[key] = value;
    }
    
    void removeContext(const std::string& key) {
        std::lock_guard<std::mutex> lock(context_mutex_);
        persistent_context_.erase(key);
    }
    
    void clearContext() {
        std::lock_guard<std::mutex> lock(context_mutex_);
        persistent_context_.clear();
    }
    
    // Add a new sink
    void addSink(std::shared_ptr<ISink> sink) {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        sinks_.push_back(std::move(sink));
        use_legacy_ = false;
    }
    
    // Statistics
    std::uint64_t getTotalMessages() const { return total_messages_.load(); }
    std::uint64_t getErrorCount() const { return error_count_.load(); }
    
    // Flush all sinks
    void flush() {
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        
        if (structured_sink_) {
            structured_sink_->flush();
        }
        
        for (auto& sink : sinks_) {
            if (sink) sink->flush();
        }
    }
    
private:
    void logEntry(LogEntry& entry) {
        // Add persistent context
        {
            std::lock_guard<std::mutex> lock(context_mutex_);
            for (const auto& [key, value] : persistent_context_) {
                if (entry.context.find(key) == entry.context.end()) {
                    entry.context[key] = value;
                }
            }
        }
        
        if (use_legacy_ && legacy_sink_) {
            legacy_sink_(entry.level, entry.message);
            return;
        }
        
        std::lock_guard<std::mutex> lock(sinks_mutex_);
        
        if (structured_sink_) {
            structured_sink_->log(entry);
        }
        
        for (auto& sink : sinks_) {
            if (sink) {
                sink->log(entry);
            }
        }
    }
    
    template<typename T>
    std::string toString(T&& value) {
        std::ostringstream oss;
        oss << std::forward<T>(value);
        return oss.str();
    }
    
    template<typename... Args>
    std::string format(const std::string& fmt, Args&&... args) {
        std::vector<std::string> values = {toString(std::forward<Args>(args))...};
        std::string result = fmt;
        
        size_t index = 0;
        size_t pos = 0;
        while ((pos = result.find("{}", pos)) != std::string::npos && index < values.size()) {
            result.replace(pos, 2, values[index]);
            pos += values[index].length();
            ++index;
        }
        
        return result;
    }
    
    Sink legacy_sink_;
    std::shared_ptr<ISink> structured_sink_;
    std::vector<std::shared_ptr<ISink>> sinks_;
    
    bool use_legacy_;
    std::string logger_name_;
    std::atomic<Level> min_level_;
    
    std::mutex context_mutex_;
    std::unordered_map<std::string, std::string> persistent_context_;
    
    std::mutex sinks_mutex_;
    
    std::atomic<std::uint64_t> total_messages_{0};
    std::atomic<std::uint64_t> error_count_{0};
};

// ============================================================================
// Global Logger Registry
// ============================================================================

class LoggerRegistry {
public:
    static LoggerRegistry& instance() {
        static LoggerRegistry registry;
        return registry;
    }
    
    std::shared_ptr<Logger> getLogger(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            return it->second;
        }
        
        // Create new logger with default console sink
        auto logger = std::make_shared<Logger>(
            std::make_shared<ConsoleSink>(), name
        );
        logger->setMinLevel(default_level_);
        loggers_[name] = logger;
        
        return logger;
    }
    
    void registerLogger(const std::string& name, std::shared_ptr<Logger> logger) {
        std::lock_guard<std::mutex> lock(mutex_);
        loggers_[name] = std::move(logger);
    }
    
    void setDefaultLevel(Level level) {
        default_level_ = level;
        
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, logger] : loggers_) {
            logger->setMinLevel(level);
        }
    }
    
    void flushAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, logger] : loggers_) {
            logger->flush();
        }
    }
    
private:
    LoggerRegistry() = default;
    
    std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
    Level default_level_ = Level::INFO;
};

// ============================================================================
// Convenience Functions
// ============================================================================

inline std::shared_ptr<Logger> getLogger(const std::string& name) {
    return LoggerRegistry::instance().getLogger(name);
}

inline void setGlobalLogLevel(Level level) {
    LoggerRegistry::instance().setDefaultLevel(level);
}

inline void flushAllLoggers() {
    LoggerRegistry::instance().flushAll();
}

// ============================================================================
// Logging Macros (optional, for file/line/function info)
// ============================================================================

#define LOG_TRACE(logger, msg) do { \
    if ((logger) && (logger)->getMinLevel() <= ailee::log::Level::TRACE) { \
        (logger)->trace(msg); \
    } \
} while(0)

#define LOG_DEBUG(logger, msg) do { \
    if ((logger) && (logger)->getMinLevel() <= ailee::log::Level::DEBUG) { \
        (logger)->debug(msg); \
    } \
} while(0)

#define LOG_INFO(logger, msg) do { \
    if ((logger) && (logger)->getMinLevel() <= ailee::log::Level::INFO) { \
        (logger)->info(msg); \
    } \
} while(0)

#define LOG_WARN(logger, msg) do { \
    if ((logger) && (logger)->getMinLevel() <= ailee::log::Level::WARN) { \
        (logger)->warn(msg); \
    } \
} while(0)

#define LOG_ERROR(logger, msg) do { \
    if ((logger) && (logger)->getMinLevel() <= ailee::log::Level::ERROR) { \
        (logger)->error(msg); \
    } \
} while(0)

#define LOG_FATAL(logger, msg) do { \
    if ((logger) && (logger)->getMinLevel() <= ailee::log::Level::FATAL) { \
        (logger)->fatal(msg); \
    } \
} while(0)

} // namespace ailee::log
