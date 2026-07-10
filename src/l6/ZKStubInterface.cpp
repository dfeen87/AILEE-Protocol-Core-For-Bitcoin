#include "l6/ZKStubInterface.h"

namespace ailee::l6 {

namespace {

void write_uint64_le(std::vector<uint8_t>& out, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((val >> (8 * i)) & 0xFF));
    }
}

void write_string(std::vector<uint8_t>& out, const std::string& str) {
    out.insert(out.end(), str.begin(), str.end());
}

} // namespace

std::vector<uint8_t> ZKConstraintSet::to_bytes() const {
    std::vector<uint8_t> result;
    write_string(result, id);
    write_uint64_le(result, num_constraints);
    return result;
}

std::vector<uint8_t> ZKTranscript::to_bytes() const {
    std::vector<uint8_t> result;
    write_string(result, id);
    write_uint64_le(result, num_rounds);
    return result;
}

std::vector<uint8_t> ZKProofStub::to_bytes() const {
    std::vector<uint8_t> result;
    write_string(result, proof_id);
    write_string(result, constraint_set_id);
    write_string(result, transcript_id);
    write_uint64_le(result, size_bytes);
    return result;
}

ZKVerifyResult verify_proof_stub(
    const ZKProofStub& proof,
    const ZKConstraintSet& constraints,
    const ZKTranscript& transcript
) {
    ZKVerifyResult result;
    result.proof_id = proof.proof_id;
    result.constraint_set_id = proof.constraint_set_id;
    result.transcript_id = proof.transcript_id;

    if (proof.constraint_set_id != constraints.id) {
        result.status = ZKVerifyStatus::INVALID;
        return result;
    }

    if (proof.transcript_id != transcript.id) {
        result.status = ZKVerifyStatus::INVALID;
        return result;
    }

    if (constraints.num_constraints == 0) {
        result.status = ZKVerifyStatus::UNSUPPORTED;
        return result;
    }

    result.status = ZKVerifyStatus::OK;
    return result;
}

} // namespace ailee::l6
