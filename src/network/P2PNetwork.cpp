// SPDX-License-Identifier: MIT
// P2PNetwork.cpp — P2P networking implementation with libp2p C++ bindings

#include "P2PNetwork.h"
#include "LogicalClock.h"
#include "ailee_rust_ffi.h"
#include "ReputationRateLimiter.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>

namespace ailee::network {

// ============================================================================
// NetworkMessage Serialization
// ============================================================================

namespace {
    void writeString(std::vector<uint8_t>& buf, const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.size());
        buf.push_back((len >> 24) & 0xFF);
        buf.push_back((len >> 16) & 0xFF);
        buf.push_back((len >> 8) & 0xFF);
        buf.push_back(len & 0xFF);
        buf.insert(buf.end(), str.begin(), str.end());
    }

    bool readString(const uint8_t*& data, size_t& len, std::string& str) {
        if (len < 4) return false;
        uint32_t strLen = (static_cast<uint32_t>(data[0]) << 24) |
                          (static_cast<uint32_t>(data[1]) << 16) |
                          (static_cast<uint32_t>(data[2]) << 8) |
                           static_cast<uint32_t>(data[3]);
        data += 4;
        len -= 4;

        if (len < strLen) return false;
        str.assign(reinterpret_cast<const char*>(data), strLen);
        data += strLen;
        len -= strLen;
        return true;
    }

    void writeUint64(std::vector<uint8_t>& buf, uint64_t val) {
        for (int i = 7; i >= 0; --i) {
            buf.push_back((val >> (i * 8)) & 0xFF);
        }
    }

    bool readUint64(const uint8_t*& data, size_t& len, uint64_t& val) {
        if (len < 8) return false;
        val = 0;
        for (int i = 0; i < 8; ++i) {
            val = (val << 8) | data[i];
        }
        data += 8;
        len -= 8;
        return true;
    }

    void writeBytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& bytes) {
        uint32_t len = static_cast<uint32_t>(bytes.size());
        buf.push_back((len >> 24) & 0xFF);
        buf.push_back((len >> 16) & 0xFF);
        buf.push_back((len >> 8) & 0xFF);
        buf.push_back(len & 0xFF);
        buf.insert(buf.end(), bytes.begin(), bytes.end());
    }

    bool readBytes(const uint8_t*& data, size_t& len, std::vector<uint8_t>& bytes) {
        if (len < 4) return false;
        uint32_t bytesLen = (static_cast<uint32_t>(data[0]) << 24) |
                            (static_cast<uint32_t>(data[1]) << 16) |
                            (static_cast<uint32_t>(data[2]) << 8) |
                             static_cast<uint32_t>(data[3]);
        data += 4;
        len -= 4;

        if (len < bytesLen) return false;
        bytes.assign(data, data + bytesLen);
        data += bytesLen;
        len -= bytesLen;
        return true;
    }
}

std::vector<uint8_t> NetworkMessage::serialize() const {
    std::vector<uint8_t> buf;
    writeString(buf, senderId);
    writeString(buf, topic);
    writeBytes(buf, payload);
    writeUint64(buf, timestamp);
    writeString(buf, messageId);
    return buf;
}

bool NetworkMessage::deserialize(const uint8_t* data, size_t len) {
    if (!readString(data, len, senderId)) return false;
    if (!readString(data, len, topic)) return false;
    if (!readBytes(data, len, payload)) return false;
    if (!readUint64(data, len, timestamp)) return false;
    if (!readString(data, len, messageId)) return false;
    return true; // We don't care if there's extra data at the end for future compatibility
}

// ============================================================================
// StubNetworkTransport - Concrete Implementation
// ============================================================================

class StubNetworkTransport : public INetworkTransport {
public:
    P2PConfig config;
    bool running = false;
    std::string localPeerId;
    std::vector<PeerInfo> peers;
    std::map<std::string, MessageHandler> subscriptions;
    mutable std::mutex mutex;
    
    // Background thread for peer discovery and message handling
    std::thread backgroundThread;
    
    StubNetworkTransport(const P2PConfig& cfg) : config(cfg) {
        localPeerId = loadOrGeneratePeerId();
    }
    
    ~StubNetworkTransport() override {
        stop();
    }
    
    bool start() override {
        std::lock_guard<std::mutex> lock(mutex);
        if (running) return true;
        std::cout << "[StubNetworkTransport] Starting stub network layer" << std::endl;
        
        int res = init_network_ffi();
        if (res != 0) {
            std::cerr << "[StubNetworkTransport] Failed to initialize rust-libp2p network via FFI" << std::endl;
            return false;
        }
        
        running = true;
        backgroundThread = std::thread([this]() {
            simulateNetworkActivity();
        });
        
        std::cout << "[StubNetworkTransport] Network started successfully" << std::endl;
        return true;
    }
    
    void stop() override {
        std::unique_lock<std::mutex> lock(mutex);
        if (!running) return;

        std::cout << "[StubNetworkTransport] Stopping stub network" << std::endl;
        running = false;

        // Unlock to join thread to avoid deadlock
        lock.unlock();
        if (backgroundThread.joinable()) {
            backgroundThread.join();
        }
        lock.lock();
        std::cout << "[StubNetworkTransport] Network stopped" << std::endl;
    }

    bool isRunning() const override {
        std::lock_guard<std::mutex> lock(mutex);
        return running;
    }

    std::string getLocalPeerId() const override {
        return localPeerId;
    }

    std::vector<PeerInfo> getPeers() const override {
        std::lock_guard<std::mutex> lock(mutex);
        return peers;
    }

    bool subscribe(const std::string& topic, MessageHandler handler) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return false;

        int res = subscribe_topic_ffi(topic.c_str());
        if (res == 0) {
            subscriptions[topic] = handler;
            std::cout << "[StubNetworkTransport] Subscribed to topic: " << topic << " (stub mode)" << std::endl;
            return true;
        }
        return false;
    }
    
    bool unsubscribe(const std::string& topic) override {
        std::lock_guard<std::mutex> lock(mutex);
        subscriptions.erase(topic);
        std::cout << "[StubNetworkTransport] Unsubscribed from topic: " << topic << std::endl;
        return true;
    }
    
    bool publish(const std::string& topic, const std::vector<uint8_t>& payload) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return false;

        std::cout << "[StubNetworkTransport] Publishing to topic: " << topic
                  << " (size: " << payload.size() << " bytes, stub mode)" << std::endl;

        int res = broadcast_message_ffi(topic.c_str(), payload.data(), payload.size());
        
        // Simulate local delivery if subscribed
        auto it = subscriptions.find(topic);
        if (it != subscriptions.end()) {
            NetworkMessage msg;
            msg.senderId = localPeerId;
            msg.topic = topic;
            msg.payload = payload;
            msg.timestamp = LogicalClock::next();
            msg.messageId = generateMessageId();
            
            // Deliver synchronously
            try {
                it->second(msg);
            } catch (const std::exception& e) {
                std::cerr << "[StubNetworkTransport] Error in message handler: " << e.what() << std::endl;
            }
        }
        return res == 0;
    }

    std::optional<std::vector<uint8_t>> sendToPeer(
        const std::string& peerId,
        const std::string& protocol,
        const std::vector<uint8_t>& payload) override {
        
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return std::nullopt;

        std::cout << "[StubNetworkTransport] Sending to peer: " << peerId
                  << " (protocol: " << protocol << ", size: " << payload.size() << " bytes)" << std::endl;

        return std::nullopt;
    }
    
    bool connectToPeer(const std::string& multiaddr) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running) return false;

        std::cout << "[StubNetworkTransport] Connecting to peer: " << multiaddr << " (stub mode)" << std::endl;
        
        PeerInfo peer;
        peer.peerId = generatePeerId();
        peer.multiaddr = multiaddr;
        peer.connected = true;
        peer.lastSeen = LogicalClock::next();
        peer.latencyMs = 50;
        
        peers.push_back(peer);
        return true;
    }
    
    bool disconnectPeer(const std::string& peerId) override {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "[StubNetworkTransport] Disconnecting peer: " << peerId << std::endl;

        peers.erase(
            std::remove_if(peers.begin(), peers.end(),
                [&peerId](const PeerInfo& p) { return p.peerId == peerId; }),
            peers.end()
        );
        return true;
    }

private:
    std::string loadOrGeneratePeerId() {
        if (!config.privateKeyPath.empty()) {
            std::ifstream keyFile(config.privateKeyPath);
            if (keyFile.is_open()) {
                std::string peerId;
                std::getline(keyFile, peerId);
                if (!peerId.empty()) {
                    return peerId;
                }
            }
        }
        
        auto peerId = generatePeerId();
        
        if (!config.privateKeyPath.empty()) {
            std::ofstream keyFile(config.privateKeyPath);
            if (keyFile.is_open()) {
                keyFile << peerId;
            }
        }
        return peerId;
    }
    
    static std::string generatePeerId() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        std::stringstream ss;
        ss << "Qm";
        for (int i = 0; i < 44; i++) {
            static const char charset[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
            ss << charset[dis(gen) % (sizeof(charset) - 1)];
        }
        return ss.str();
    }
    
    static std::string generateMessageId() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        
        std::stringstream ss;
        for (int i = 0; i < 32; i++) {
            ss << std::hex << dis(gen);
        }
        return ss.str();
    }
    
    void simulateNetworkActivity() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            std::lock_guard<std::mutex> lock(mutex);
            if (!running) break;

            if (peers.size() < config.maxPeers / 2) {
                PeerInfo peer;
                peer.peerId = generatePeerId();
                peer.multiaddr = "/ip4/192.168.1." + std::to_string(100 + peers.size()) + "/tcp/4001";
                peer.connected = true;
                peer.lastSeen = LogicalClock::next();
                peer.latencyMs = 20 + (peers.size() * 5);
                
                peers.push_back(peer);
                std::cout << "[StubNetworkTransport] Discovered peer (simulated): " << peer.peerId << std::endl;
            }
        }
    }
};

// ============================================================================
// P2PNetwork - Public Interface Implementation
// ============================================================================

P2PNetwork::P2PNetwork(const P2PConfig& config)
    : transport_(std::make_unique<StubNetworkTransport>(config)) {
}

P2PNetwork::P2PNetwork(std::unique_ptr<INetworkTransport> transport)
    : transport_(std::move(transport)) {
}

P2PNetwork::~P2PNetwork() {
    if (transport_) {
        transport_->stop();
    }
}

bool P2PNetwork::start() {
    return transport_->start();
}

void P2PNetwork::stop() {
    transport_->stop();
}

bool P2PNetwork::isRunning() const {
    return transport_->isRunning();
}

std::string P2PNetwork::getLocalPeerId() const {
    return transport_->getLocalPeerId();
}

std::vector<PeerInfo> P2PNetwork::getPeers() const {
    return transport_->getPeers();
}

bool P2PNetwork::subscribe(const std::string& topic, MessageHandler handler) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_[topic] = handler;
    }
    
    // Create an interception handler to enforce rate limiting
    auto interceptor = [this, topic](const NetworkMessage& msg) {
        MessageHandler currentHandler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscriptions_.find(topic);
            if (it != subscriptions_.end()) {
                currentHandler = it->second;
            }
        }
        // We'll pass a default reputation for now; in a full node, reputation is fetched via other means
        this->internalMessageHandler(msg, currentHandler, 0.5);
    };

    return transport_->subscribe(topic, interceptor);
}

bool P2PNetwork::unsubscribe(const std::string& topic) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_.erase(topic);
    }
    return transport_->unsubscribe(topic);
}

void P2PNetwork::handleIncomingMessage(const NetworkMessage& msg, double peerReputation) {
    // Advance logical clock on received messages so rate limits actually work
    LogicalClock::next();

    MessageHandler handler;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscriptions_.find(msg.topic);
        if (it != subscriptions_.end()) {
            handler = it->second;
        }
    }
    if (handler) {
        internalMessageHandler(msg, handler, peerReputation);
    }
}

void P2PNetwork::internalMessageHandler(const NetworkMessage& msg, MessageHandler userHandler, double peerReputation) {
    if (!rateLimiter_.allowMessage(msg.senderId, peerReputation, msg.topic, msg.payload)) {
        std::cout << "[P2PNetwork] Rate limiter dropped message from peer " << msg.senderId
                  << " on topic " << msg.topic << std::endl;
        return;
    }

    if (userHandler) {
        try {
            userHandler(msg);
            std::lock_guard<std::mutex> lock(mutex_);
            internalStats_.totalMessagesReceived++;
            internalStats_.bytesDownloaded += msg.payload.size();
        } catch (const std::exception& e) {
            std::cerr << "[P2PNetwork] Error in message handler: " << e.what() << std::endl;
        }
    }
}

bool P2PNetwork::publish(const std::string& topic, const std::vector<uint8_t>& payload) {
    bool success = transport_->publish(topic, payload);
    if (success) {
        std::lock_guard<std::mutex> lock(mutex_);
        internalStats_.totalMessagesSent++;
        internalStats_.bytesUploaded += payload.size();
    }
    return success;
}

std::optional<std::vector<uint8_t>> P2PNetwork::sendToPeer(
    const std::string& peerId,
    const std::string& protocol,
    const std::vector<uint8_t>& payload) {
    
    auto result = transport_->sendToPeer(peerId, protocol, payload);
    if (result) {
        std::lock_guard<std::mutex> lock(mutex_);
        internalStats_.totalMessagesSent++;
        internalStats_.bytesUploaded += payload.size();
        internalStats_.totalMessagesReceived++;
        internalStats_.bytesDownloaded += result->size();
    }
    return result;
}

bool P2PNetwork::connectToPeer(const std::string& multiaddr) {
    return transport_->connectToPeer(multiaddr);
}

bool P2PNetwork::disconnectPeer(const std::string& peerId) {
    return transport_->disconnectPeer(peerId);
}

P2PNetwork::NetworkStats P2PNetwork::getStats() const {
    NetworkStats stats;
    stats.connectedPeers = transport_->getPeers().size();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats.totalMessagesSent = internalStats_.totalMessagesSent;
        stats.totalMessagesReceived = internalStats_.totalMessagesReceived;
        stats.bytesUploaded = internalStats_.bytesUploaded;
        stats.bytesDownloaded = internalStats_.bytesDownloaded;
    }
    return stats;
}

} // namespace ailee::network
