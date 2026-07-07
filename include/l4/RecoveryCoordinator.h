#pragma once

#include <cstdint>
#include <vector>
#include "l4/ClusterTypes.h"
#include "l2/RecoveryPayload.h"

namespace ailee {
namespace l4 {

enum class RecoveryReason : uint8_t {
    STATE_MISMATCH = 0,
    BEHIND = 1,
    STALE = 2,
    UNRECOVERABLE = 3
};

struct alignas(64) RecoveryRequest {
    uint64_t requester_node_id_hash;
    uint64_t target_epoch;
    uint8_t target_state_root[32];
    RecoveryReason reason;
    uint8_t padding[15]; // 8 + 8 + 32 + 1 + 15 = 64
};
static_assert(sizeof(RecoveryRequest) == 64, "RecoveryRequest must be 64 bytes");

struct alignas(64) RecoveryResponse {
    uint64_t requester_node_id_hash; // 8 bytes
    uint64_t provider_node_id_hash; // 8 bytes
    uint64_t epoch; // 8 bytes
    uint8_t state_root[32]; // 32 bytes
    l2::RecoveryPayload payload; // 192 bytes
    bool full_recovery; // 1 byte
    uint8_t padding[63]; // full_recovery offset is 256. 256 + 1 + 63 = 320 bytes.
};
static_assert(sizeof(RecoveryResponse) == 320, "RecoveryResponse must be 320 bytes");

enum class RecoverySessionStatus : uint8_t {
    PENDING = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2
};

struct alignas(64) RecoverySession {
    std::vector<RecoveryRequest> requests; // 24 bytes
    std::vector<RecoveryResponse> responses; // 24 bytes
    RecoverySessionStatus status; // 1 byte
    uint8_t padding[15]; // 24 + 24 + 1 + 15 = 64 bytes
};
static_assert(sizeof(RecoverySession) == 64, "RecoverySession must be 64 bytes");

// Pure functions for recovery
std::vector<RecoveryRequest> build_recovery_requests(const ClusterView& view);

std::vector<RecoveryResponse> match_recovery_responses(
    const ClusterView& view,
    const std::vector<RecoveryRequest>& requests
);

void apply_recovery(ClusterNodeState& node, const RecoveryResponse& response);

} // namespace l4
} // namespace ailee
