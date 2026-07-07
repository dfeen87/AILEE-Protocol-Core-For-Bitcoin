#include "l4/DeterministicDashboard.h"
#include <sstream>

namespace ailee {
namespace l4 {

DashboardSnapshot DashboardBuilder::build_snapshot(const TelemetryBuffer& buffer) const {
    std::ostringstream oss;
    oss << "{ \"telemetry\": [";

    for (size_t i = 0; i < buffer.samples.size(); ++i) {
        const auto& sample = buffer.samples[i];
        oss << "{";
        oss << "\"tick\":" << sample.tick_count << ",";
        oss << "\"epoch\":" << sample.epoch_height << ",";
        oss << "\"total_nodes\":" << sample.total_nodes << ",";
        oss << "\"in_sync_nodes\":" << sample.in_sync_nodes << ",";
        oss << "\"consistent_state_root_nodes\":" << sample.consistent_state_root_nodes << ",";
        oss << "\"inconsistent_state_root_nodes\":" << sample.inconsistent_state_root_nodes << ",";
        oss << "\"global_coherence_score\":" << sample.global_coherence_score;
        oss << "}";
        if (i < buffer.samples.size() - 1) {
            oss << ",";
        }
    }

    oss << "] }";
    return { oss.str() };
}

} // namespace l4
} // namespace ailee
