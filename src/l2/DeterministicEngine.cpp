#include "l2/DeterministicEngine.h"
#include <cstring>
#include <algorithm>

namespace ailee {
namespace l2 {

DeterministicEngine::DeterministicEngine() {
    std::memset(&m_state, 0, sizeof(EngineState));
}

DeterministicEngine::DeterministicEngine(const EngineState& initial_state) {
    std::memcpy(&m_state, &initial_state, sizeof(EngineState));
}

void DeterministicEngine::set_state(const EngineState& new_state) {
    std::memcpy(&m_state, &new_state, sizeof(EngineState));
}

EngineStepResult DeterministicEngine::step(
    const reflection::ReflectionSnapshot& reflection_snapshot,
    const l1::SettlementIngestion& settlement_ingestion,
    const mesh::MeshCoherenceResult& mesh_coherence_result,
    const identity::NodeId& node_id,
    uint32_t protocol_version
) {
    m_state.epoch = compute_epoch_state(reflection_snapshot, node_id, protocol_version);
    m_state.state_root = compute_state_root(reflection_snapshot, settlement_ingestion, mesh_coherence_result);
    m_state.recovery = build_recovery_payload(m_state.state_root, m_state.epoch, node_id);
    m_state.context = build_execution_context(node_id, m_state.epoch, m_state.state_root, mesh_coherence_result);
    m_state.step_counter++;

    EngineStepResult result;
    std::memset(&result, 0, sizeof(EngineStepResult));
    result.new_state = m_state;
    result.status_code = 0; // Success

    return result;
}

EngineRunSummary DeterministicEngine::run_offline(
    const std::vector<reflection::ReflectionSnapshot>& snapshots,
    const std::vector<l1::SettlementIngestion>& ingestions,
    const std::vector<mesh::MeshCoherenceResult>& coherences,
    const identity::NodeId& node_id,
    uint32_t protocol_version
) {
    DeterministicEngine engine;

    size_t min_len = std::min({snapshots.size(), ingestions.size(), coherences.size()});
    bool mismatch = (snapshots.size() != ingestions.size()) || (ingestions.size() != coherences.size());

    for (size_t i = 0; i < min_len; ++i) {
        engine.step(snapshots[i], ingestions[i], coherences[i], node_id, protocol_version);
    }

    EngineRunSummary summary;
    std::memset(&summary, 0, sizeof(EngineRunSummary));
    summary.final_state = engine.m_state;
    summary.total_steps = engine.m_state.step_counter;
    if (min_len > 0) {
        summary.final_coherence = coherences[min_len - 1].score;
    }
    summary.vector_mismatch = mismatch;

    return summary;
}

} // namespace l2
} // namespace ailee
