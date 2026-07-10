#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace ailee::l6 {

struct ZKConstraintSet {
    std::string id;              // deterministic identifier for the constraint set
    uint64_t num_constraints;    // number of constraints (logical, not enforced yet)

    std::vector<uint8_t> to_bytes() const;
};

struct ZKTranscript {
    std::string id;              // deterministic identifier for the transcript
    uint64_t num_rounds;         // number of interaction rounds (logical)

    std::vector<uint8_t> to_bytes() const;
};

struct ZKProofStub {
    std::string proof_id;        // deterministic identifier for the proof
    std::string constraint_set_id;
    std::string transcript_id;
    uint64_t size_bytes;         // logical size of the proof

    std::vector<uint8_t> to_bytes() const;
};

enum class ZKVerifyStatus {
    OK,
    INVALID,
    UNSUPPORTED
};

struct ZKVerifyResult {
    ZKVerifyStatus status;
    std::string proof_id;
    std::string constraint_set_id;
    std::string transcript_id;
};

ZKVerifyResult verify_proof_stub(
    const ZKProofStub& proof,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
);

} // namespace ailee::l6
