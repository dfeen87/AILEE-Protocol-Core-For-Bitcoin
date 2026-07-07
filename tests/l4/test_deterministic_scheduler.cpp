#include <gtest/gtest.h>
#include "l4/DeterministicScheduler.h"
#include "l4/ClusterSim.h"
#include <cstring>
#include <iostream>

using namespace ailee::l4;
using namespace ailee::l2;
using namespace ailee::l3;

class DeterministicSchedulerTest : public ::testing::Test {
public:
    void SetUp() override {
    }
};

TEST_F(DeterministicSchedulerTest, PhaseOrderingAndEffects) {
    ClusterView view = {};
    view.total_nodes = 3;
    view.nodes.resize(3);
    for (size_t i = 0; i < 3; i++) {
        view.nodes[i] = ClusterNodeState{};
        view.nodes[i].node_id_hash = i + 1;
    }

    std::vector<DeterministicEngine> engines;
    for (size_t i = 0; i < 3; i++) {
        engines.emplace_back(view.nodes[i].engine_state);
    }

    std::vector<std::pair<size_t, size_t>> schedule = { {0, 1}, {1, 2} };

    DeterministicScheduler scheduler;

    // 0: ENGINE_STEP
    EXPECT_EQ(scheduler.state.tick_count, 0);
    uint64_t initial_step_counter = view.nodes[0].step_counter;
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 1);
    EXPECT_GT(view.nodes[0].step_counter, initial_step_counter);

    // 1: GOSSIP_PEER_SYNC
    size_t initial_sync_states = view.nodes[1].peer_sync_states.size();
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 2);
    EXPECT_GT(view.nodes[1].peer_sync_states.size(), initial_sync_states);

    // 2: RECOVERY
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 3);

    // 3: MESH_EPOCH_BUILD
    uint64_t initial_epoch = scheduler.state.epoch_height;
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 4);
    EXPECT_GT(scheduler.state.epoch_height, initial_epoch);
    EXPECT_NE(scheduler.state.last_mesh_state_root_hash, 0);

    // 4: MESH_ANCHOR_BUILD
    uint64_t initial_anchor_id = scheduler.state.last_anchor_id;
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 5);
    EXPECT_NE(scheduler.state.last_anchor_id, initial_anchor_id);
    EXPECT_GT(view.mesh_envelopes.size(), 0);

    // 5: STATE_ROOT_ANNOUNCEMENTS
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 6);
    EXPECT_EQ(scheduler.current_announcements.size(), 3);

    // 6: STATE_ROOT_VALIDATION
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 7);
    EXPECT_EQ(view.nodes[0].state_root_status, StateRootStatus::CONSISTENT);

    // 7: TRANSPORT_DELIVERY
    // Pre-condition: queue is empty before tick? Actually the tick enqueues AND drains.
    EXPECT_EQ(view.transport_queue.messages.size(), 0);
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 8);
    // After draining, it should be 0.
    EXPECT_EQ(view.transport_queue.messages.size(), 0);

    // 8: COHERENCE_UPDATE
    uint64_t initial_total_steps = view.total_steps;
    scheduler.run_tick(view, schedule, engines);
    EXPECT_EQ(scheduler.state.tick_count, 9);
    EXPECT_EQ(view.total_steps, initial_total_steps + 1);
}

TEST_F(DeterministicSchedulerTest, Reproducibility) {
    std::vector<ClusterNodeState> initial_nodes(3);
    for (size_t i = 0; i < 3; i++) {
        initial_nodes[i] = ClusterNodeState{};
        initial_nodes[i].node_id_hash = i * 999;
    }
    std::vector<std::pair<size_t, size_t>> schedule = { {0, 1}, {1, 2}, {2, 0} };

    ClusterView run1 = run_cluster_simulation(initial_nodes, schedule, 5);
    ClusterView run2 = run_cluster_simulation(initial_nodes, schedule, 5);

    EXPECT_EQ(run1.total_nodes, run2.total_nodes);
    EXPECT_EQ(run1.total_steps, run2.total_steps);

    for (size_t i = 0; i < run1.nodes.size(); ++i) {
        EXPECT_EQ(run1.nodes[i].step_counter, run2.nodes[i].step_counter);
        EXPECT_EQ(run1.nodes[i].node_id_hash, run2.nodes[i].node_id_hash);
        EXPECT_EQ(std::memcmp(&run1.nodes[i].engine_state, &run2.nodes[i].engine_state, sizeof(EngineState)), 0);
        EXPECT_EQ(std::memcmp(&run1.nodes[i].last_envelope, &run2.nodes[i].last_envelope, sizeof(ExecutionEnvelope)), 0);
    }
}
