#pragma once

#include <cstdint>
#include <vector>
#include "NodeIdentity.h"
#include "ReflectionLayer.h"
#include "SettlementIngestion.h"
#include "MeshCoherence.h"
#include "l2/EpochScheduler.h"
#include "l2/StateRootPipeline.h"
#include "l2/RecoveryPayload.h"
#include "l2/ExecutionContext.h"

namespace ailee {
namespace l2 {

struct alignas(64) EngineState {
    EpochState epoch;               // 128 bytes
    StateRoot state_root;           // 64 bytes
    RecoveryPayload recovery;       // 192 bytes
    ExecutionContext context;       // 192 bytes
    uint64_t step_counter;          // 8 bytes
    uint8_t padding[56];            // 128+64+192+192+8 = 584. 640 - 584 = 56.
};
static_assert(sizeof(EngineState) == 640, "EngineState must be a multiple of 64 bytes");

struct alignas(64) EngineStepResult {
    EngineState new_state;          // 640 bytes
    uint32_t status_code;           // 4 bytes
    uint8_t padding[60];            // 644. 704 - 644 = 60.
};
static_assert(sizeof(EngineStepResult) == 704, "EngineStepResult must be a multiple of 64 bytes");

struct alignas(64) EngineRunSummary {
    EngineState final_state;        // 640 bytes
    uint64_t total_steps;           // 8 bytes
    uint32_t final_coherence;       // 4 bytes
    bool vector_mismatch;           // 1 byte
    uint8_t padding[51];            // 640+8+4+1 = 653. 704 - 653 = 51.
};
static_assert(sizeof(EngineRunSummary) == 704, "EngineRunSummary must be a multiple of 64 bytes");

class DeterministicEngine {
public:
    DeterministicEngine();

    EngineStepResult step(
        const reflection::ReflectionSnapshot& reflection_snapshot,
        const l1::SettlementIngestion& settlement_ingestion,
        const mesh::MeshCoherenceResult& mesh_coherence_result,
        const identity::NodeId& node_id,
        uint32_t protocol_version
    );

    static EngineRunSummary run_offline(
        const std::vector<reflection::ReflectionSnapshot>& snapshots,
        const std::vector<l1::SettlementIngestion>& ingestions,
        const std::vector<mesh::MeshCoherenceResult>& coherences,
        const identity::NodeId& node_id,
        uint32_t protocol_version
    );

private:
    EngineState m_state;
};

} // namespace l2
} // namespace ailee
