#include "ambient_ai_runtime_state_machine.hpp"

namespace ailee {
namespace runtime {

bool AmbientRuntimePhaseTransition::isValid() const {
    if (toPhase == AmbientRuntimePhase::RECOVERY) return true;
    if (fromPhase == AmbientRuntimePhase::RECOVERY && toPhase == AmbientRuntimePhase::INIT) return true;

    // Deterministic state machine checks matching enum values order
    if (static_cast<uint8_t>(toPhase) == static_cast<uint8_t>(fromPhase) + 1) return true;

    if (fromPhase == AmbientRuntimePhase::FINALIZE && toPhase == AmbientRuntimePhase::INIT) return true;
    return false;
}

AmbientRuntimeStateMachine::AmbientRuntimeStateMachine() {
    state.currentPhase = AmbientRuntimePhase::INIT;
    state.currentLogicalTimestamp = 0;
    state.lastProcessedBitcoinHeight = 0;
    state.isHealthy = true;
    state.consecutiveFailures = 0;
}

AmbientRuntimeState AmbientRuntimeStateMachine::getCurrentState() const {
    return state;
}

bool AmbientRuntimeStateMachine::applyTransition(const AmbientRuntimePhaseTransition& transition) {
    if (!transition.isValid()) {
        triggerRecovery(transition.logicalTimestamp);
        return false;
    }

    state.currentPhase = transition.toPhase;
    state.currentLogicalTimestamp = transition.logicalTimestamp;
    state.lastProcessedBitcoinHeight = transition.targetBitcoinHeight;
    state.isHealthy = true;
    state.consecutiveFailures = 0;
    return true;
}

void AmbientRuntimeStateMachine::triggerRecovery(uint64_t logicalTimestamp) {
    state.currentPhase = AmbientRuntimePhase::RECOVERY;
    state.currentLogicalTimestamp = logicalTimestamp;
    state.isHealthy = false;
    state.consecutiveFailures++;
}

bool AmbientRuntimeStateMachine::checkPhaseInvariants(AmbientRuntimePhase phase) const {
    (void)phase;
    return state.isHealthy;
}

}
}
