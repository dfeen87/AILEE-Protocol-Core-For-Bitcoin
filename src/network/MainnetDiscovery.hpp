#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct MainnetPeer {
    std::string peerId;
    std::string address;
    uint64_t lastSeen;
    bool verified;
};

class MainnetDiscovery {
public:
    MainnetDiscovery();

    // Add peer from bootstrap list or gossip
    void addPeer(const std::string& peerId, const std::string& address);

    // Mark peer as verified after handshake
    void verifyPeer(const std::string& peerId);

    // Get list of verified peers
    std::vector<MainnetPeer> getVerifiedPeers() const;

    // Get all peers
    std::vector<MainnetPeer> getAllPeers() const;

    // Check if peer exists
    bool hasPeer(const std::string& peerId) const;

private:
    std::unordered_map<std::string, MainnetPeer> peers_;
};
