#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace ailee::l2 {

/**
 * Transaction structure for L2 blockchain
 */
struct Transaction {
    std::string txHash;
    std::string fromAddress;
    std::string toAddress;
    std::uint64_t amount;
    std::string data;
    std::uint64_t timestampMs;
    std::string status; // "pending", "confirmed", "failed"
    std::uint64_t blockHeight; // 0 if not yet included in a block
};

/**
 * Mempool - Thread-safe transaction queue for L2 blockchain
 * 
 * Stores pending transactions waiting to be included in blocks.
 * BlockProducer pulls transactions from this mempool when creating new blocks.
 */
class Mempool {
public:
    Mempool() = default;

    /**
     * Add a transaction to the mempool
     * @param tx Transaction to add
     */
    void addTransaction(const Transaction& tx);

    /**
     * Get up to maxCount pending transactions from the mempool
     * @param maxCount Maximum number of transactions to retrieve
     * @return Vector of transactions (may be fewer than maxCount)
     */
    std::vector<Transaction> getPendingTransactions(std::size_t maxCount);

    /**
     * Mark transactions as confirmed in a block
     * @param txHashes Hashes of transactions that were confirmed
     * @param blockHeight Block height where transactions were confirmed
     */
    void confirmTransactions(const std::vector<std::string>& txHashes, std::uint64_t blockHeight);

    /**
     * Get the number of pending transactions
     * @return Count of pending transactions
     */
    std::size_t getPendingCount() const;

    /**
     * Get all transactions (for debugging/admin purposes)
     * @return All transactions in the mempool
     */
    std::vector<Transaction> getAllTransactions() const;

private:
    mutable std::mutex mutex_;
    std::deque<Transaction> pendingTransactions_;
    std::deque<Transaction> confirmedTransactions_; // Keep recent confirmed txs for queries
};

} // namespace ailee::l2
