#include <gtest/gtest.h>
#include "l6/ZKStubInterface.h"

namespace ailee::l6 {

TEST(ZKStubInterfaceTests, VerifyProofStub_Valid) {
    ZKConstraintSet constraints{"constraint-123", 100};
    ZKTranscript transcript{"transcript-456", 10};
    ZKProofStub proof{"proof-789", "constraint-123", "transcript-456", 1024};

    auto result = verify_proof_stub(proof, constraints, transcript);

    EXPECT_EQ(result.status, ZKVerifyStatus::OK);
    EXPECT_EQ(result.proof_id, "proof-789");
    EXPECT_EQ(result.constraint_set_id, "constraint-123");
    EXPECT_EQ(result.transcript_id, "transcript-456");
}

TEST(ZKStubInterfaceTests, VerifyProofStub_InvalidConstraintId) {
    ZKConstraintSet constraints{"constraint-123", 100};
    ZKTranscript transcript{"transcript-456", 10};
    ZKProofStub proof{"proof-789", "constraint-wrong", "transcript-456", 1024};

    auto result = verify_proof_stub(proof, constraints, transcript);

    EXPECT_EQ(result.status, ZKVerifyStatus::INVALID);
}

TEST(ZKStubInterfaceTests, VerifyProofStub_InvalidTranscriptId) {
    ZKConstraintSet constraints{"constraint-123", 100};
    ZKTranscript transcript{"transcript-456", 10};
    ZKProofStub proof{"proof-789", "constraint-123", "transcript-wrong", 1024};

    auto result = verify_proof_stub(proof, constraints, transcript);

    EXPECT_EQ(result.status, ZKVerifyStatus::INVALID);
}

TEST(ZKStubInterfaceTests, VerifyProofStub_UnsupportedZeroConstraints) {
    ZKConstraintSet constraints{"constraint-123", 0};
    ZKTranscript transcript{"transcript-456", 10};
    ZKProofStub proof{"proof-789", "constraint-123", "transcript-456", 1024};

    auto result = verify_proof_stub(proof, constraints, transcript);

    EXPECT_EQ(result.status, ZKVerifyStatus::UNSUPPORTED);
}

TEST(ZKStubInterfaceTests, ConstraintSetSerialization) {
    ZKConstraintSet constraints{"CS-1", 0x1122334455667788ULL};
    auto bytes = constraints.to_bytes();

    // "CS-1" is 4 bytes
    // 0x1122334455667788 is 8 bytes
    // Total 12 bytes
    EXPECT_EQ(bytes.size(), 12);

    // Check string part
    EXPECT_EQ(bytes[0], 'C');
    EXPECT_EQ(bytes[1], 'S');
    EXPECT_EQ(bytes[2], '-');
    EXPECT_EQ(bytes[3], '1');

    // Check integer part (little-endian)
    EXPECT_EQ(bytes[4], 0x88);
    EXPECT_EQ(bytes[5], 0x77);
    EXPECT_EQ(bytes[6], 0x66);
    EXPECT_EQ(bytes[7], 0x55);
    EXPECT_EQ(bytes[8], 0x44);
    EXPECT_EQ(bytes[9], 0x33);
    EXPECT_EQ(bytes[10], 0x22);
    EXPECT_EQ(bytes[11], 0x11);
}

TEST(ZKStubInterfaceTests, TranscriptSerialization) {
    ZKTranscript transcript{"TR-1", 0x0102030405060708ULL};
    auto bytes = transcript.to_bytes();

    // "TR-1" is 4 bytes
    // uint64 is 8 bytes
    // Total 12 bytes
    EXPECT_EQ(bytes.size(), 12);

    // Check string part
    EXPECT_EQ(bytes[0], 'T');
    EXPECT_EQ(bytes[1], 'R');
    EXPECT_EQ(bytes[2], '-');
    EXPECT_EQ(bytes[3], '1');

    // Check integer part (little-endian)
    EXPECT_EQ(bytes[4], 0x08);
    EXPECT_EQ(bytes[5], 0x07);
    EXPECT_EQ(bytes[6], 0x06);
    EXPECT_EQ(bytes[7], 0x05);
    EXPECT_EQ(bytes[8], 0x04);
    EXPECT_EQ(bytes[9], 0x03);
    EXPECT_EQ(bytes[10], 0x02);
    EXPECT_EQ(bytes[11], 0x01);
}

TEST(ZKStubInterfaceTests, ProofStubSerialization) {
    ZKProofStub proof{"P-1", "C-1", "T-1", 0x0000000000000010ULL};
    auto bytes = proof.to_bytes();

    // "P-1" is 3 bytes
    // "C-1" is 3 bytes
    // "T-1" is 3 bytes
    // uint64 is 8 bytes
    // Total 17 bytes
    EXPECT_EQ(bytes.size(), 17);

    // check P-1
    EXPECT_EQ(bytes[0], 'P');
    EXPECT_EQ(bytes[1], '-');
    EXPECT_EQ(bytes[2], '1');

    // check C-1
    EXPECT_EQ(bytes[3], 'C');
    EXPECT_EQ(bytes[4], '-');
    EXPECT_EQ(bytes[5], '1');

    // check T-1
    EXPECT_EQ(bytes[6], 'T');
    EXPECT_EQ(bytes[7], '-');
    EXPECT_EQ(bytes[8], '1');

    // Check integer part (little-endian)
    EXPECT_EQ(bytes[9], 0x10);
    EXPECT_EQ(bytes[10], 0x00);
    EXPECT_EQ(bytes[11], 0x00);
    EXPECT_EQ(bytes[12], 0x00);
    EXPECT_EQ(bytes[13], 0x00);
    EXPECT_EQ(bytes[14], 0x00);
    EXPECT_EQ(bytes[15], 0x00);
    EXPECT_EQ(bytes[16], 0x00);
}

} // namespace ailee::l6
