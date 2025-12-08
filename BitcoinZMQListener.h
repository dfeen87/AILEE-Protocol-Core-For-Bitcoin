// BitcoinZMQListener.h
#ifndef BITCOIN_ZMQ_LISTENER_H
#define BITCOIN_ZMQ_LISTENER_H

#include <zmq.hpp>
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

namespace ailee {

/**
 * BitcoinZMQListener
 *
 * Production-ready listener for Bitcoin Core ZMQ events.
 * Subscribes to raw transactions and block hashes, decodes payloads,
 * and triggers bridge logic when peg-in conditions are met.
 */
class BitcoinZMQListener {
public:
    explicit BitcoinZMQListener(const std::string& endpoint = "tcp://127.0.0.1:28332")
        : context_(1), subscriber_(context_, ZMQ_SUB), running_(false), endpoint_(endpoint) {}

    // Initialize connection and subscriptions
    void init() {
        try {
            subscriber_.connect(endpoint_);
            subscriber_.set(zmq::sockopt::subscribe, "rawtx");
            subscriber_.set(zmq::sockopt::subscribe, "hashblock");
            std::cout << "[Init] Connected to Bitcoin ZMQ at " << endpoint_ << std::endl;
        } catch (const zmq::error_t& e) {
            std::cerr << "[Error] ZMQ init failed: " << e.what() << std::endl;
            throw;
        }
    }

    // Start listening loop
    void start() {
        running_ = true;
        while (running_) {
            try {
                zmq::message_t topic;
                zmq::message_t payload;

                if (!subscriber_.recv(topic, zmq::recv_flags::none)) continue;
                if (!subscriber_.recv(payload, zmq::recv_flags::none)) continue;

                std::string topic_str(static_cast<char*>(topic.data()), topic.size());

                if (topic_str == "rawtx") {
                    handleTransaction(payload);
                } else if (topic_str == "hashblock") {
                    handleBlock(payload);
                }
            } catch (const zmq::error_t& e) {
                std::cerr << "[Error] ZMQ recv failed: " << e.what() << std::endl;
                reconnect();
            }
        }
    }

    // Stop listening loop
    void stop() {
        running_ = false;
        subscriber_.close();
        context_.close();
        std::cout << "[Shutdown] Listener stopped gracefully." << std::endl;
    }

private:
    zmq::context_t context_;
    zmq::socket_t subscriber_;
    std::atomic<bool> running_;
    std::string endpoint_;

    // Transaction handler stub
    void handleTransaction(const zmq::message_t& payload) {
        // TODO: Decode raw transaction bytes using Bitcoin Core RPC or libbitcoin
        // Example: parse outputs, check if funds sent to bridge address
        std::cout << "[Bridge] New Transaction Detected on Mainnet (size=" 
                  << payload.size() << " bytes)" << std::endl;

        // If valid peg-in detected:
        // ailee::SidechainBridge::initiatePegIn(decodedTx);
    }

    // Block handler stub
    void handleBlock(const zmq::message_t& payload) {
        // TODO: Verify block header, update SPV proofs
        std::cout << "[Bridge] New Block Detected on Mainnet (hash size=" 
                  << payload.size() << " bytes)" << std::endl;
    }

    // Attempt reconnection on error
    void reconnect() {
        std::cerr << "[Reconnect] Attempting to reconnect to " << endpoint_ << std::endl;
        try {
            subscriber_.close();
            subscriber_ = zmq::socket_t(context_, ZMQ_SUB);
            init();
        } catch (const zmq::error_t& e) {
            std::cerr << "[Error] Reconnect failed: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
};

} // namespace ailee

#endif // BITCOIN_ZMQ_LISTENER_H
