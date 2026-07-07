#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "l3/NetworkReflection.h"
#include "l3/NetworkBinding.h"
#include "l2/DeterministicEngine.h"

namespace ailee {
namespace l3 {

struct NetworkConfig {
    std::string rpc_url;
    std::string rpc_user;
    std::string rpc_password;
    std::string network_type;
    uint32_t max_steps;
};

// Wraps a std::vector but keeps 64-byte alignment
// Note: std::vector dynamic allocation breaks pure POD nature inside the struct,
// but alignas(64) guarantees the struct's base pointer is aligned.
struct alignas(64) ExecutionEnvelopeSequence {
    std::vector<l2::ExecutionEnvelope> envelopes;
    uint64_t count;
    // padding removed because std::vector is dynamically sized and struct is not truly POD anyway. // Padding to hit 64-byte boundary (vector usually 24 bytes + 8 = 32. 32 + 32 = 64).
};

struct alignas(64) NetworkRunSummary {
    uint64_t total_steps_executed;
    uint32_t final_network_height;
    uint32_t envelopes_produced;
    ExecutionEnvelopeSequence sequence;
};

class NetworkDriver {
public:
    static NetworkRunSummary run_offline(
        const NetworkConfig& config,
        const identity::NodeId& node_id,
        uint32_t protocol_version
    );

private:
    static NetworkSnapshot fetch_network_snapshot(const NetworkConfig& config, uint32_t step);
};

} // namespace l3
} // namespace ailee
