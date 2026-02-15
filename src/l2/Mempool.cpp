#include "Mempool.h"

#include <algorithm>

namespace ailee::l2 {

void Mempool::addTransaction(const Transaction& tx) {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingTransactions_.push_back(tx);
}

std::vector<Transaction> Mempool::getPendingTransactions(std::size_t maxCount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Transaction> result;
    std::size_t count = std::min(maxCount, pendingTransactions_.size());
    
    for (std::size_t i = 0; i < count; ++i) {
        result.push_back(pendingTransactions_[i]);
    }
    
    return result;
}

void Mempool::confirmTransactions(const std::vector<std::string>& txHashes, std::uint64_t blockHeight) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Move confirmed transactions from pending to confirmed
    for (const auto& txHash : txHashes) {
        auto it = std::find_if(
            pendingTransactions_.begin(),
            pendingTransactions_.end(),
            [&txHash](const Transaction& tx) { return tx.txHash == txHash; }
        );
        
        if (it != pendingTransactions_.end()) {
            // Update transaction status and block height
            it->status = "confirmed";
            it->blockHeight = blockHeight;
            
            // Move to confirmed transactions
            confirmedTransactions_.push_back(*it);
            
            // Remove from pending
            pendingTransactions_.erase(it);
        }
    }
    
    // Keep only recent confirmed transactions (last 1000)
    if (confirmedTransactions_.size() > 1000) {
        confirmedTransactions_.erase(
            confirmedTransactions_.begin(),
            confirmedTransactions_.begin() + (confirmedTransactions_.size() - 1000)
        );
    }
}

std::size_t Mempool::getPendingCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pendingTransactions_.size();
}

std::vector<Transaction> Mempool::getAllTransactions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Transaction> result;
    result.reserve(pendingTransactions_.size() + confirmedTransactions_.size());
    
    // Add pending transactions
    for (const auto& tx : pendingTransactions_) {
        result.push_back(tx);
    }
    
    // Add confirmed transactions
    for (const auto& tx : confirmedTransactions_) {
        result.push_back(tx);
    }
    
    return result;
}

} // namespace ailee::l2
