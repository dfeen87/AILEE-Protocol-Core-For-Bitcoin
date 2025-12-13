#include "Ledger.h"
#include <algorithm>
#include <chrono>
#include <limits>

namespace ailee::econ {

namespace {
    // Helper to get current timestamp
    std::uint64_t getCurrentTimestamp() {
        return static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }
}

// ============================================================================
// InMemoryLedger Implementation
// ============================================================================

std::uint64_t InMemoryLedger::balanceOf(const std::string& peerId) const {
    if (!validatePeerId(peerId)) {
        return 0;
    }
    
    std::shared_lock lock(balances_mutex_);
    return getBalance_unsafe(peerId);
}

void InMemoryLedger::credit(const std::string& peerId, std::uint64_t amount) {
    if (!validatePeerId(peerId)) {
        throw LedgerException("Invalid peer ID: " + peerId);
    }
    
    if (!validateAmount(amount)) {
        throw LedgerException("Invalid credit amount: " + std::to_string(amount));
    }
    
    {
        std::unique_lock lock(balances_mutex_);
        std::uint64_t currentBalance = getBalance_unsafe(peerId);
        
        // Check for overflow
        if (currentBalance > std::numeric_limits<std::uint64_t>::max() - amount) {
            throw LedgerException("Credit would cause balance overflow for peer: " + peerId);
        }
        
        setBalance_unsafe(peerId, currentBalance + amount);
    }
    
    emitEvent(LedgerEventType::CREDIT, peerId, amount);
}

bool InMemoryLedger::debit(const std::string& peerId, std::uint64_t amount) {
    if (!validatePeerId(peerId)) {
        return false;
    }
    
    if (!validateAmount(amount)) {
        return false;
    }
    
    {
        std::unique_lock lock(balances_mutex_);
        std::uint64_t currentBalance = getBalance_unsafe(peerId);
        
        if (currentBalance < amount) {
            return false;
        }
        
        setBalance_unsafe(peerId, currentBalance - amount);
    }
    
    emitEvent(LedgerEventType::DEBIT, peerId, amount);
    return true;
}

bool InMemoryLedger::putInEscrow(const Escrow& e) {
    if (!e.isValid()) {
        return false;
    }
    
    if (!validatePeerId(e.clientPeerId)) {
        return false;
    }
    
    // Check if escrow already exists
    {
        std::shared_lock lock(escrows_mutex_);
        if (escrows_.find(e.taskId) != escrows_.end()) {
            return false;
        }
    }
    
    // Debit from client's account
    if (!debit(e.clientPeerId, e.amount)) {
        return false;
    }
    
    // Create escrow with timestamp
    Escrow escrowWithTimestamp = e;
    escrowWithTimestamp.createdAt = getCurrentTimestamp();
    
    {
        std::unique_lock lock(escrows_mutex_);
        escrows_[e.taskId] = escrowWithTimestamp;
    }
    
    emitEvent(LedgerEventType::ESCROW_CREATED, e.clientPeerId, e.amount, e.taskId);
    return true;
}

bool InMemoryLedger::releaseEscrow(const std::string& taskId, const std::string& workerPeerId) {
    if (taskId.empty() || !validatePeerId(workerPeerId)) {
        return false;
    }
    
    Escrow escrow;
    
    // Retrieve and remove escrow
    {
        std::unique_lock lock(escrows_mutex_);
        auto it = escrows_.find(taskId);
        
        if (it == escrows_.end()) {
            return false;
        }
        
        escrow = it->second;
        
        // Check if escrow is locked
        if (escrow.locked) {
            return false;
        }
        
        escrows_.erase(it);
    }
    
    // Credit worker with escrowed amount
    try {
        credit(workerPeerId, escrow.amount);
    } catch (const LedgerException&) {
        // Rollback: put escrow back
        std::unique_lock lock(escrows_mutex_);
        escrows_[taskId] = escrow;
        return false;
    }
    
    emitEvent(LedgerEventType::ESCROW_RELEASED, workerPeerId, escrow.amount, taskId);
    return true;
}

bool InMemoryLedger::refundEscrow(const std::string& taskId) {
    if (taskId.empty()) {
        return false;
    }
    
    Escrow escrow;
    
    // Retrieve and remove escrow
    {
        std::unique_lock lock(escrows_mutex_);
        auto it = escrows_.find(taskId);
        
        if (it == escrows_.end()) {
            return false;
        }
        
        escrow = it->second;
        
        // Check if escrow is locked
        if (escrow.locked) {
            return false;
        }
        
        escrows_.erase(it);
    }
    
    // Refund client
    try {
        credit(escrow.clientPeerId, escrow.amount);
    } catch (const LedgerException&) {
        // Rollback: put escrow back
        std::unique_lock lock(escrows_mutex_);
        escrows_[taskId] = escrow;
        return false;
    }
    
    emitEvent(LedgerEventType::ESCROW_REFUNDED, escrow.clientPeerId, escrow.amount, taskId);
    return true;
}

std::optional<Escrow> InMemoryLedger::getEscrow(const std::string& taskId) const {
    if (taskId.empty()) {
        return std::nullopt;
    }
    
    std::shared_lock lock(escrows_mutex_);
    auto it = escrows_.find(taskId);
    
    if (it == escrows_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

bool InMemoryLedger::hasEscrow(const std::string& taskId) const {
    if (taskId.empty()) {
        return false;
    }
    
    std::shared_lock lock(escrows_mutex_);
    return escrows_.find(taskId) != escrows_.end();
}

std::vector<std::string> InMemoryLedger::getEscrowTaskIds() const {
    std::shared_lock lock(escrows_mutex_);
    
    std::vector<std::string> taskIds;
    taskIds.reserve(escrows_.size());
    
    for (const auto& [taskId, _] : escrows_) {
        taskIds.push_back(taskId);
    }
    
    return taskIds;
}

bool InMemoryLedger::transfer(const std::string& fromPeerId, const std::string& toPeerId, 
                               std::uint64_t amount) {
    if (!validatePeerId(fromPeerId) || !validatePeerId(toPeerId)) {
        return false;
    }
    
    if (fromPeerId == toPeerId) {
        return false;
    }
    
    if (!validateAmount(amount)) {
        return false;
    }
    
    // Atomic transfer using single lock
    std::unique_lock lock(balances_mutex_);
    
    std::uint64_t fromBalance = getBalance_unsafe(fromPeerId);
    
    if (fromBalance < amount) {
        return false;
    }
    
    std::uint64_t toBalance = getBalance_unsafe(toPeerId);
    
    // Check for overflow on recipient
    if (toBalance > std::numeric_limits<std::uint64_t>::max() - amount) {
        return false;
    }
    
    setBalance_unsafe(fromPeerId, fromBalance - amount);
    setBalance_unsafe(toPeerId, toBalance + amount);
    
    lock.unlock();
    
    // Emit events for both sides
    emitEvent(LedgerEventType::DEBIT, fromPeerId, amount);
    emitEvent(LedgerEventType::CREDIT, toPeerId, amount);
    
    return true;
}

void InMemoryLedger::registerEventCallback(LedgerEventCallback callback) {
    std::unique_lock lock(callback_mutex_);
    event_callback_ = std::move(callback);
}

void InMemoryLedger::unregisterEventCallback() {
    std::unique_lock lock(callback_mutex_);
    event_callback_ = nullptr;
}

std::size_t InMemoryLedger::getAccountCount() const {
    std::shared_lock lock(balances_mutex_);
    return balances_.size();
}

std::size_t InMemoryLedger::getEscrowCount() const {
    std::shared_lock lock(escrows_mutex_);
    return escrows_.size();
}

std::uint64_t InMemoryLedger::getTotalBalance() const {
    std::shared_lock lock(balances_mutex_);
    
    std::uint64_t total = 0;
    for (const auto& [_, balance] : balances_) {
        // Saturate on overflow rather than wrap
        if (total > std::numeric_limits<std::uint64_t>::max() - balance) {
            return std::numeric_limits<std::uint64_t>::max();
        }
        total += balance;
    }
    
    return total;
}

std::uint64_t InMemoryLedger::getTotalEscrow() const {
    std::shared_lock lock(escrows_mutex_);
    
    std::uint64_t total = 0;
    for (const auto& [_, escrow] : escrows_) {
        // Saturate on overflow rather than wrap
        if (total > std::numeric_limits<std::uint64_t>::max() - escrow.amount) {
            return std::numeric_limits<std::uint64_t>::max();
        }
        total += escrow.amount;
    }
    
    return total;
}

void InMemoryLedger::clear() {
    std::unique_lock balancesLock(balances_mutex_);
    std::unique_lock escrowsLock(escrows_mutex_);
    
    balances_.clear();
    escrows_.clear();
}

bool InMemoryLedger::removeAccount(const std::string& peerId) {
    if (!validatePeerId(peerId)) {
        return false;
    }
    
    std::unique_lock lock(balances_mutex_);
    
    auto it = balances_.find(peerId);
    if (it == balances_.end()) {
        return false;
    }
    
    // Only allow removal of zero-balance accounts
    if (it->second != 0) {
        return false;
    }
    
    balances_.erase(it);
    return true;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void InMemoryLedger::emitEvent(LedgerEventType type, const std::string& peerId, 
                                std::uint64_t amount, const std::optional<std::string>& taskId) {
    std::unique_lock lock(callback_mutex_);
    
    if (!event_callback_) {
        return;
    }
    
    LedgerEvent event{
        type,
        peerId,
        amount,
        taskId,
        getCurrentTimestamp()
    };
    
    // Call callback without holding lock to prevent deadlocks
    auto callback = event_callback_;
    lock.unlock();
    
    try {
        callback(event);
    } catch (...) {
        // Swallow exceptions from callbacks to maintain ledger consistency
    }
}

bool InMemoryLedger::validatePeerId(const std::string& peerId) const {
    return !peerId.empty() && peerId.length() <= 256;
}

bool InMemoryLedger::validateAmount(std::uint64_t amount) const {
    return amount > 0;
}

std::uint64_t InMemoryLedger::getBalance_unsafe(const std::string& peerId) const {
    auto it = balances_.find(peerId);
    return (it != balances_.end()) ? it->second : 0;
}

void InMemoryLedger::setBalance_unsafe(const std::string& peerId, std::uint64_t balance) {
    if (balance == 0) {
        balances_.erase(peerId);
    } else {
        balances_[peerId] = balance;
    }
}

} // namespace ailee::econ
