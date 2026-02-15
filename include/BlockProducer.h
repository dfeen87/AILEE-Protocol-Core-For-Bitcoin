#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace ailee::l2 {

/**
 * BlockProducer - Time-based block production for L2 chain
 * 
 * Produces blocks at a configurable interval (default: 1 second)
 * Tracks block height, transaction count, and anchor commitments
 */
class BlockProducer {
public:
    struct Config {
        std::uint64_t blockIntervalMs = 1000;    // 1 block per second
        std::uint64_t commitmentInterval = 100;   // Anchor every 100 blocks
    };

    struct State {
        std::uint64_t blockHeight = 0;
        std::uint64_t totalTransactions = 0;
        std::uint64_t lastAnchorHeight = 0;
        std::uint64_t lastBlockTimestampMs = 0;
    };

    explicit BlockProducer(const Config& config);
    ~BlockProducer();

    // Start/stop block production
    void start();
    void stop();

    // Get current state (thread-safe)
    State getState() const;

    // Called by other systems to report transactions
    void recordTransaction();

private:
    void blockProductionLoop();
    void produceBlock();
    void checkAnchorCommitment();

    Config config_;
    mutable std::mutex stateMutex_;
    State state_;
    
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> producerThread_;
};

} // namespace ailee::l2
