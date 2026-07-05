#include "ambient_ai_consensus_state_machine.hpp"

namespace ailee {
namespace consensus {

bool AmbientConsensusTransition::isValidTransition() const {
    if (fromPhase == ConsensusPhase::RECOVERY) return true; // Can recover to anywhere
    if (toPhase == ConsensusPhase::RECOVERY) return true;

    // Strict deterministic sequence
    if (fromPhase == ConsensusPhase::INIT && toPhase == ConsensusPhase::COLLECT) return true;
    if (fromPhase == ConsensusPhase::COLLECT && toPhase == ConsensusPhase::ORDER) return true;
    if (fromPhase == ConsensusPhase::ORDER && toPhase == ConsensusPhase::MERKLEIZE) return true;
    if (fromPhase == ConsensusPhase::MERKLEIZE && toPhase == ConsensusPhase::ANCHOR) return true;
    if (fromPhase == ConsensusPhase::ANCHOR && toPhase == ConsensusPhase::FINALIZE) return true;
    if (fromPhase == ConsensusPhase::FINALIZE && toPhase == ConsensusPhase::INIT) return true;
    return false;
}

AmbientConsensusStateMachine::AmbientConsensusStateMachine() : currentPhase(ConsensusPhase::INIT) {}

ConsensusPhase AmbientConsensusStateMachine::getCurrentPhase() const {
    return currentPhase;
}

bool AmbientConsensusStateMachine::advancePhase(const AmbientConsensusTransition& transition) {
    if (transition.fromPhase != currentPhase) return false;
    if (!transition.isValidTransition()) return false;

    if (transition.toPhase == ConsensusPhase::COLLECT) {
        if (!validateCollectPhaseInvariants()) return false;
    } else if (transition.toPhase == ConsensusPhase::ORDER) {
        if (!validateOrderPhaseInvariants()) return false;
    } else if (transition.toPhase == ConsensusPhase::MERKLEIZE) {
        if (!validateMerkleizePhaseInvariants()) return false;
    } else if (transition.toPhase == ConsensusPhase::ANCHOR) {
        if (!validateAnchorPhaseInvariants()) return false;
    }

    currentPhase = transition.toPhase;
    return true;
}

bool AmbientConsensusStateMachine::initiateRecovery() {
    currentPhase = ConsensusPhase::RECOVERY;
    return true;
}

bool AmbientConsensusStateMachine::validateCollectPhaseInvariants() const { return true; }
bool AmbientConsensusStateMachine::validateOrderPhaseInvariants() const { return true; }
bool AmbientConsensusStateMachine::validateMerkleizePhaseInvariants() const { return true; }
bool AmbientConsensusStateMachine::validateAnchorPhaseInvariants() const { return true; }

}
}
