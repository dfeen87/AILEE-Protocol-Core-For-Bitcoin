// BroadcastEngine.cpp
#include "BroadcastEngine.hpp"
#include "protocol/ProtocolFrame.hpp"
#include <openssl/sha.h>
#include <chrono>

NetworkBinding* BroadcastEngine::net = nullptr;
MainnetDiscovery* BroadcastEngine::discovery = nullptr;

// ---------------------------------------------------------
// Deterministic frame signing (matches PeerSync + Orchestrator)
// ---------------------------------------------------------
static std::string sign_frame(const ProtocolFrame& pf)
{
    std::string data = pf.frame_id + pf.type + pf.version +
                       pf.node_id + std::to_string(pf.timestamp) +
                       pf.payload;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()),
           data.size(), hash);

    std::string hex;
    hex.reserve(SHA256_DIGEST_LENGTH * 2);
    static const char* digits = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        hex.push_back(digits[(hash[i] >> 4) & 0xF]);
        hex.push_back(digits[hash[i] & 0xF]);
    }

    return hex;
}

// ---------------------------------------------------------
// Bindings
// ---------------------------------------------------------
void BroadcastEngine::bind(NetworkBinding* binding) {
    net = binding;
}

void BroadcastEngine::bindDiscovery(MainnetDiscovery* d) {
    discovery = d;
}

// ---------------------------------------------------------
// Emit signed frame to local network + verified mainnet peers
// ---------------------------------------------------------
void BroadcastEngine::emit(const std::string& type,
                           const std::string& version,
                           const Json::Value& payload)
{
    if (!net) return;

    ProtocolFrame pf;
    pf.frame_id  = std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    pf.type      = type;
    pf.version   = version;
    pf.node_id   = net->localNodeId();
    pf.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    pf.payload   = Json::writeString(Json::StreamWriterBuilder{}, payload);
    pf.signature = sign_frame(pf);

    std::string serialized = net->serializeFrame(pf);

    // Local broadcast
    net->broadcast(serialized);

    // Mainnet propagation: send to all verified peers
    if (discovery) {
        for (const auto& peer : discovery->getVerifiedPeers()) {
            net->broadcastTo(peer.address, serialized);
        }
    }
}
