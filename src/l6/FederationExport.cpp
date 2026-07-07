#include "l6/FederationExport.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace ailee {
namespace l6 {

namespace {

std::string hash_envelope(const l4::MeshPropagationEnvelope& env) {
    // Generate a stable hash string for the external envelope representation
    // To match Phase 16 deterministic hashing requirements.
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&env);
    uint64_t hash = 5381;
    for (size_t i = 0; i < sizeof(l4::MeshPropagationEnvelope); ++i) {
        hash = ((hash << 5) + hash) + data[i]; // hash * 33 + c
    }

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

} // anonymous namespace

ExternalClusterView FederationExport::export_view(const l4::ClusterView& view) {
    ExternalClusterView ext_view;
    ext_view.node_count = view.total_nodes;

    // Deterministic coherence rounding to double
    // Assuming global_coherence_score is 0-100 integer representing percentage.
    // We convert it to a double. Since it's integer based, it's deterministic.
    ext_view.coherence = static_cast<double>(view.coherence_summary.global_coherence_score);

    ext_view.envelopes.reserve(view.mesh_envelopes.size());
    for (const auto& internal_env : view.mesh_envelopes) {
        ExternalEnvelope ext_env;
        ext_env.id = internal_env.source_node_id_hash;
        ext_env.timestamp = internal_env.anchor.epoch.epoch_height; // Map epoch to timestamp as a stable stand-in
        ext_env.payload_hash = hash_envelope(internal_env);
        ext_view.envelopes.push_back(ext_env);
    }

    // Deterministic envelope ordering by ID, then timestamp, then payload_hash
    std::sort(ext_view.envelopes.begin(), ext_view.envelopes.end(),
        [](const ExternalEnvelope& a, const ExternalEnvelope& b) {
            if (a.id != b.id) return a.id < b.id;
            if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
            return a.payload_hash < b.payload_hash;
        });

    return ext_view;
}

} // namespace l6
} // namespace ailee
