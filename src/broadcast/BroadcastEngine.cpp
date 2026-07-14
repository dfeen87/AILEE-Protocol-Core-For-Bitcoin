// BroadcastEngine.cpp
#include "BroadcastEngine.hpp"
#include "protocol/ProtocolFrame.hpp"
#include <openssl/sha.h>
#include <chrono>

ailee::l3::NetworkBinding* BroadcastEngine::net = nullptr;
MainnetDiscovery* BroadcastEngine::discovery = nullptr;


// ---------------------------------------------------------
// Bindings
// ---------------------------------------------------------
void BroadcastEngine::bind(ailee::l3::NetworkBinding* binding) {
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
