#include <gtest/gtest.h>
#include "l2/DeterministicEngine.h"
#include <cstring>
#include <vector>

using namespace ailee::l2;
using namespace ailee::reflection;
using namespace ailee::l1;
using namespace ailee::mesh;
using namespace ailee::identity;

TEST(DeterministicEngineTest, InitializationIsZeroed) {
    DeterministicEngine engine;
    // Step on an empty reflection to ensure step counter works
    ReflectionSnapshot rs;
    std::memset(&rs, 0, sizeof(rs));
    SettlementIngestion si;
    std::memset(&si, 0, sizeof(si));
    MeshCoherenceResult mc;
    std::memset(&mc, 0, sizeof(mc));
    NodeId nid;
    std::memset(&nid, 0, sizeof(nid));

    auto res = engine.step(rs, si, mc, nid, 1);

    EXPECT_EQ(res.status_code, 0);
    EXPECT_EQ(res.new_state.step_counter, 1);
}

TEST(DeterministicEngineTest, RunOfflineSync) {
    ReflectionSnapshot rs;
    std::memset(&rs, 0, sizeof(rs));
    SettlementIngestion si;
    std::memset(&si, 0, sizeof(si));
    MeshCoherenceResult mc;
    std::memset(&mc, 0, sizeof(mc));
    NodeId nid;
    std::memset(&nid, 0, sizeof(nid));

    std::vector<ReflectionSnapshot> rss = {rs, rs, rs};
    std::vector<SettlementIngestion> sis = {si, si, si};
    std::vector<MeshCoherenceResult> mcs = {mc, mc, mc};

    auto summary = DeterministicEngine::run_offline(rss, sis, mcs, nid, 1);

    EXPECT_EQ(summary.total_steps, 3);
    EXPECT_FALSE(summary.vector_mismatch);
}

TEST(DeterministicEngineTest, RunOfflineMismatch) {
    ReflectionSnapshot rs;
    std::memset(&rs, 0, sizeof(rs));
    SettlementIngestion si;
    std::memset(&si, 0, sizeof(si));
    MeshCoherenceResult mc;
    std::memset(&mc, 0, sizeof(mc));
    NodeId nid;
    std::memset(&nid, 0, sizeof(nid));

    std::vector<ReflectionSnapshot> rss = {rs, rs};
    std::vector<SettlementIngestion> sis = {si, si, si};
    std::vector<MeshCoherenceResult> mcs = {mc};

    auto summary = DeterministicEngine::run_offline(rss, sis, mcs, nid, 1);

    EXPECT_EQ(summary.total_steps, 1);
    EXPECT_TRUE(summary.vector_mismatch);
}
