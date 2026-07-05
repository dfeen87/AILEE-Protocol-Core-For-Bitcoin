#include <openssl/sha.h>
#include "ambient_ai_participation_measurement.hpp"

namespace ailee {
namespace participation {

std::vector<uint8_t> AmbientContributionSummary::serialize() const {
    std::vector<uint8_t> out;
    auto push32 = [&out](uint32_t val) {
        for (int i = 3; i >= 0; --i) {
            out.push_back((val >> (i * 8)) & 0xFF);
        }
    };
    push32(totalRoutingSuccesses);
    push32(totalSensingSnapshots);
    push32(totalUptimeSegments);
    return out;
}

Hash256 AmbientContributionSummary::hash() const {
    auto bytes = serialize();
    Hash256 result;
    SHA256(bytes.data(), bytes.size(), result.data());
    return result;
}

bool AmbientContributionLog::logEvent(const AmbientContributionEvent& event) {
    if (validEvents.size() >= ParticipationLimits::MAX_EVENTS_PER_EPOCH) {
        return false;
    }
    if (seenEventIds.find(event.eventId) != seenEventIds.end()) {
        return false;
    }
    seenEventIds.insert(event.eventId);
    validEvents.push_back(event);
    return true;
}

AmbientContributionSummary AmbientContributionLog::finalizeEpochSummary() {
    AmbientContributionSummary summary = {0, 0, 0};
    for (const auto& ev : validEvents) {
        if (ev.type == ContributionType::ROUTING_SUCCESS) summary.totalRoutingSuccesses++;
        if (ev.type == ContributionType::SENSING_SNAPSHOT) summary.totalSensingSnapshots++;
        if (ev.type == ContributionType::UPTIME_SEGMENT) summary.totalUptimeSegments++;
    }
    validEvents.clear();
    seenEventIds.clear();
    return summary;
}

bool AmbientContributionLog::HashCompare::operator()(const Hash256& a, const Hash256& b) const {
    for (size_t i = 0; i < 32; ++i) {
        if (a[i] != b[i]) return a[i] < b[i];
    }
    return false;
}

}
}
