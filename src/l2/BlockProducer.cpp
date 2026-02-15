#include "BlockProducer.h"

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
    return state_;
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

    // Increment block height
    state_.blockHeight++;

    // Update timestamp
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    state_.lastBlockTimestampMs = static_cast<std::uint64_t>(nowMs);

    // Log block production (every 10 blocks to avoid spam)
    if (state_.blockHeight % 10 == 0 || state_.blockHeight <= 5) {
        std::ostringstream oss;
        oss << "Block #" << state_.blockHeight << " produced"
            << " (txs: " << state_.totalTransactions << ")";
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
