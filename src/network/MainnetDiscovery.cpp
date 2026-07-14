#include "MainnetDiscovery.hpp"
#include <chrono>

MainnetDiscovery::MainnetDiscovery() {}

void MainnetDiscovery::addPeer(const std::string& peerId, const std::string& address)
{
    auto now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::seconds>(now).count();

    peers_[peerId] = MainnetPeer{
        peerId,
        address,
        ts,
        false
    };
}

void MainnetDiscovery::verifyPeer(const std::string& peerId)
{
    if (peers_.count(peerId)) {
        peers_[peerId].verified = true;
    }
}

std::vector<MainnetPeer> MainnetDiscovery::getVerifiedPeers() const
{
    std::vector<MainnetPeer> out;
    for (const auto& [id, peer] : peers_) {
        if (peer.verified) out.push_back(peer);
    }
    return out;
}

std::vector<MainnetPeer> MainnetDiscovery::getAllPeers() const
{
    std::vector<MainnetPeer> out;
    for (const auto& [id, peer] : peers_) {
        out.push_back(peer);
    }
    return out;
}

bool MainnetDiscovery::hasPeer(const std::string& peerId) const
{
    return peers_.count(peerId) > 0;
}
