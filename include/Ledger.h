#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <vector>
#include <stdexcept>

namespace ailee::econ {

// Custom exception types for better error handling
class LedgerException : public std::runtime_error {
public:
    explicit LedgerException(const std::string& msg) : std::runtime_error(msg) {}
};

class InsufficientFundsException : public LedgerException {
public:
    explicit InsufficientFundsException(const std::string& peerId, std::uint64_t requested, std::uint64_t available)
        : LedgerException("Insufficient funds for peer " + peerId + 
                         ": requested " + std::to_string(requested) + 
                         ", available " + std::to_string(available)) {}
};

class EscrowException : public LedgerException {
public:
    explicit EscrowException(const std::string& msg) : LedgerException(msg) {}
};

// Enhanced structs with validation
struct Payment {
    std::string fromPeerId;
    std::string toPeerId;
    std::uint64_t amount = 0;
    
    bool isValid() const {
        return !fromPeerId.empty() && !toPeerId.empty() && 
               fromPeerId != toPeerId && amount > 0;
    }
};

struct Escrow {
    std::string taskId;
    std::string clientPeerId;
    std::uint64_t amount = 0;
    bool locked = false;
    std::uint64_t createdAt = 0;  // Timestamp for tracking
    
    bool isValid() const {
        return !taskId.empty() && !clientPeerId.empty() && amount > 0;
    }
};

// Event types for observability
enum class LedgerEventType {
    CREDIT,
    DEBIT,
    ESCROW_CREATED,
    ESCROW_RELEASED,
    ESCROW_REFUNDED
};

struct LedgerEvent {
    LedgerEventType type;
    std::string peerId;
    std::uint64_t amount;
    std::optional<std::string> taskId;
    std::uint64_t timestamp;
};

using LedgerEventCallback = std::function<void(const LedgerEvent&)>;

class ILedger {
public:
    virtual ~ILedger() = default;
    
    // Core balance operations
    virtual std::uint64_t balanceOf(const std::string& peerId) const = 0;
    virtual void credit(const std::string& peerId, std::uint64_t amount) = 0;
    virtual bool debit(const std::string& peerId, std::uint64_t amount) = 0;
    
    // Escrow operations
    virtual bool putInEscrow(const Escrow& e) = 0;
    virtual bool releaseEscrow(const std::string& taskId, const std::string& workerPeerId) = 0;
    virtual bool refundEscrow(const std::string& taskId) = 0;
    
    // Enhanced query methods
    virtual std::optional<Escrow> getEscrow(const std::string& taskId) const = 0;
    virtual bool hasEscrow(const std::string& taskId) const = 0;
    virtual std::vector<std::string> getEscrowTaskIds() const = 0;
    
    // Atomic transfer operation
    virtual bool transfer(const std::string& fromPeerId, const std::string& toPeerId, 
                         std::uint64_t amount) = 0;
    
    // Observability
    virtual void registerEventCallback(LedgerEventCallback callback) = 0;
    virtual void unregisterEventCallback() = 0;
};

class InMemoryLedger final : public ILedger {
public:
    InMemoryLedger() = default;
    
    // Disable copy, allow move
    InMemoryLedger(const InMemoryLedger&) = delete;
    InMemoryLedger& operator=(const InMemoryLedger&) = delete;
    InMemoryLedger(InMemoryLedger&&) noexcept = default;
    InMemoryLedger& operator=(InMemoryLedger&&) noexcept = default;
    
    // Core operations with thread safety
    std::uint64_t balanceOf(const std::string& peerId) const override;
    void credit(const std::string& peerId, std::uint64_t amount) override;
    bool debit(const std::string& peerId, std::uint64_t amount) override;
    
    // Escrow operations
    bool putInEscrow(const Escrow& e) override;
    bool releaseEscrow(const std::string& taskId, const std::string& workerPeerId) override;
    bool refundEscrow(const std::string& taskId) override;
    
    // Enhanced query methods
    std::optional<Escrow> getEscrow(const std::string& taskId) const override;
    bool hasEscrow(const std::string& taskId) const override;
    std::vector<std::string> getEscrowTaskIds() const override;
    
    // Atomic transfer
    bool transfer(const std::string& fromPeerId, const std::string& toPeerId, 
                 std::uint64_t amount) override;
    
    // Observability
    void registerEventCallback(LedgerEventCallback callback) override;
    void unregisterEventCallback() override;
    
    // Utility methods
    std::size_t getAccountCount() const;
    std::size_t getEscrowCount() const;
    std::uint64_t getTotalBalance() const;
    std::uint64_t getTotalEscrow() const;
    
    // Administrative operations
    void clear();
    bool removeAccount(const std::string& peerId);
    
private:
    // Thread-safe data access
    mutable std::shared_mutex balances_mutex_;
    mutable std::shared_mutex escrows_mutex_;
    mutable std::mutex callback_mutex_;
    
    std::unordered_map<std::string, std::uint64_t> balances_;
    std::unordered_map<std::string, Escrow> escrows_;
    
    LedgerEventCallback event_callback_;
    
    // Helper methods
    void emitEvent(LedgerEventType type, const std::string& peerId, 
                   std::uint64_t amount, const std::optional<std::string>& taskId = std::nullopt);
    
    bool validatePeerId(const std::string& peerId) const;
    bool validateAmount(std::uint64_t amount) const;
    
    // Internal helpers that assume locks are held
    std::uint64_t getBalance_unsafe(const std::string& peerId) const;
    void setBalance_unsafe(const std::string& peerId, std::uint64_t balance);
};

} // namespace ailee::econ
