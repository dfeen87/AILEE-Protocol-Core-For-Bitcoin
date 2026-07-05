#include "ambient_ai_mesh_neighborhood.hpp"

namespace ailee {
namespace ambient_mesh {

void LocalNeighborhoodManager::updateNeighborState(
    const std::string& peerId,
    int64_t energyScore,
    uint64_t currentLogicalTick
) {
    NeighborEntry entry;
    entry.peerId = peerId;
    entry.cachedEnergyScore = energyScore;
    entry.linkState.establishedTick = currentLogicalTick;
    entry.linkState.lastActiveTick = currentLogicalTick;
    entry.linkState.isActive = true;

    auto it = neighborTable.find(entry);
    if (it != neighborTable.end()) {
        neighborTable.erase(it);
    }
    neighborTable.insert(entry);
}

void LocalNeighborhoodManager::pruneInactiveNeighbors(uint64_t currentLogicalTick) {
    auto it = neighborTable.begin();
    while (it != neighborTable.end()) {
        // Prune if inactive for more than 100 ticks
        if (currentLogicalTick > it->linkState.lastActiveTick + 100) {
            it = neighborTable.erase(it);
        } else {
            ++it;
        }
    }
}

std::set<NeighborEntry> LocalNeighborhoodManager::getActiveNeighbors() const {
    return neighborTable;
}

bool LocalNeighborhoodManager::observeAndCacheMessage(const Hash256& messageId, uint64_t currentLogicalTick) {
    std::string key(messageId.begin(), messageId.end());
    if (recentRouteCache.find(key) != recentRouteCache.end()) {
        return false; // Already seen
    }

    // Manage cache size deterministically, prune old entries
    auto it = recentRouteCache.begin();
    while (it != recentRouteCache.end()) {
        if (currentLogicalTick > it->second.receivedTick + 1000) {
            it = recentRouteCache.erase(it);
        } else {
            ++it;
        }
    }

    recentRouteCache[key] = {messageId, currentLogicalTick};
    return true;
}

}
}
