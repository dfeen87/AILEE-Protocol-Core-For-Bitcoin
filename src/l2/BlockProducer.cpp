#include "BlockProducer.h"
#include "Mempool.h"
#include "ReorgDetector.h"

#include <chrono>
#include <sstream>
#include <iostream>

// Simple logging function - logs to stdout
static void log(const std::string& level, const std::string& msg) {
    std::cout << "[" << level << "] " << msg << std::endl;
}

namespace ailee::l2 {

BlockProducer::BlockProducer(const Config& config)
    : config_(config) {
}

BlockProducer::~BlockProducer() {
    stop();
}

void BlockProducer::start() {
    if (running_.load()) {
        log("WARN", "BlockProducer::start() - already running");
        return;
    }

    running_.store(true);
    producerThread_ = std::make_unique<std::thread>([this]() {
        blockProductionLoop();
    });

    log("INFO", "BlockProducer started - producing blocks every " + 
        std::to_string(config_.blockIntervalMs) + "ms");
    log("INFO", "Anchor commitment interval: " + 
        std::to_string(config_.commitmentInterval) + " blocks");
}

void BlockProducer::stop() {
    if (!running_.load()) {
        return;
    }

    running_.store(false);
    if (producerThread_ && producerThread_->joinable()) {
        producerThread_->join();
    }
    producerThread_.reset();

    log("INFO", "BlockProducer stopped");
}

BlockProducer::State BlockProducer::getState() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    State stateCopy = state_;
    
    // Update pending transaction count from mempool if available
    if (mempool_) {
        stateCopy.pendingTransactions = mempool_->getPendingCount();
    }
    
    return stateCopy;
}

void BlockProducer::setMempool(Mempool* mempool) {
    mempool_ = mempool;
    log("INFO", "BlockProducer mempool reference set");
}

void BlockProducer::setReorgDetector(ailee::l1::ReorgDetector* detector) {
    reorgDetector_ = detector;
    log("INFO", "BlockProducer reorg detector set");
}

void BlockProducer::recordTransaction() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    state_.totalTransactions++;
}

void BlockProducer::blockProductionLoop() {
    log("INFO", "Block production loop started");

    while (running_.load()) {
        auto startTime = std::chrono::steady_clock::now();

        // Produce a new block
        produceBlock();

        // Check if we need to create an anchor commitment
        checkAnchorCommitment();

        // Sleep for the configured interval
        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto sleepDuration = std::chrono::milliseconds(config_.blockIntervalMs) - elapsed;

        if (sleepDuration.count() > 0) {
            std::this_thread::sleep_for(sleepDuration);
        }
    }

    log("INFO", "Block production loop exited");
}

void BlockProducer::produceBlock() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    // Security check: If we have a reorg detector, check for deep reorgs
    if (reorgDetector_) {
        // In a real implementation, we would get the current L1 height.
        // For now, we check if any deep reorgs have occurred recently.
        auto reorgs = reorgDetector_->getRecentReorgHistory(1);
        if (!reorgs.empty()) {
            const auto& lastReorg = reorgs.front();
            // In a production environment, we halt immediately upon detecting a deep reorg
            // to ensure no invalid state transitions are committed to L2.
            log("CRITICAL", "Deep reorg detected at height " + std::to_string(lastReorg.reorgHeight) +
                ". HALTING block production to prevent state corruption.");

            // Stop the producer loop immediately
            running_ = false;
            return;
        }
    }

    // Increment block height
    state_.blockHeight++;

    // Update timestamp
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    state_.lastBlockTimestampMs = static_cast<std::uint64_t>(nowMs);

    // Pull transactions from mempool if available
    std::size_t txsInBlock = 0;
    if (mempool_) {
        auto transactions = mempool_->getPendingTransactions(config_.maxTransactionsPerBlock);
        txsInBlock = transactions.size();
        
        if (txsInBlock > 0) {
            // Validate and confirm transactions
            std::vector<std::string> validTxHashes;
            validTxHashes.reserve(txsInBlock);

            for (const auto& tx : transactions) {
                // Audit Fix: Basic Validation
                // 1. Check if hash is valid length
                if (tx.txHash.length() != 64) {
                    log("WARN", "Invalid transaction hash detected: " + tx.txHash + ". Skipping.");
                    continue;
                }
                // 2. Check if sender/receiver present (basic sanity)
                if (tx.fromAddress.empty() || tx.toAddress.empty()) {
                    log("WARN", "Malformed transaction detected (missing sender/receiver). Skipping.");
                    continue;
                }
                // 3. Enforce signature presence
                if (tx.signature.empty()) {
                    log("WARN", "Transaction missing signature. Rejecting: " + tx.txHash);
                    continue;
                }

                validTxHashes.push_back(tx.txHash);
            }

            if (!validTxHashes.empty()) {
                mempool_->confirmTransactions(validTxHashes, state_.blockHeight);
                // Update total transaction count
                state_.totalTransactions += validTxHashes.size();
            }
        }
    }

    // Log block production (every 10 blocks to avoid spam, or if block contains transactions)
    if (state_.blockHeight % 10 == 0 || state_.blockHeight <= 5 || txsInBlock > 0) {
        std::ostringstream oss;
        oss << "Block #" << state_.blockHeight << " produced"
            << " (txs in block: " << txsInBlock
            << ", total txs: " << state_.totalTransactions << ")";
        log("INFO", oss.str());
    }
}

void BlockProducer::checkAnchorCommitment() {
    std::lock_guard<std::mutex> lock(stateMutex_);

    // Check if we've reached the commitment interval
    std::uint64_t blocksSinceAnchor = state_.blockHeight - state_.lastAnchorHeight;

    if (blocksSinceAnchor >= config_.commitmentInterval) {
        // Time to create an anchor commitment
        state_.lastAnchorHeight = state_.blockHeight;

        std::ostringstream oss;
        oss << "Anchor commitment created at block #" << state_.blockHeight
            << " (interval: " << config_.commitmentInterval << " blocks)";
        log("INFO", oss.str());
    }
}

} // namespace ailee::l2
