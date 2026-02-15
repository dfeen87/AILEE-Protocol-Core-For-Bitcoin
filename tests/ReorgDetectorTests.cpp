#include "ReorgDetector.h"
#include "gtest/gtest.h"

#include <filesystem>
#include <chrono>

namespace {

std::string getTestDbPath() {
    return "/tmp/ailee_reorg_test_" + std::to_string(
        std::chrono::system_clock::now().time_since_epoch().count());
}

void cleanupTestDb(const std::string& path) {
    if (std::filesystem::exists(path)) {
        std::filesystem::remove_all(path);
    }
}

TEST(ReorgDetector, InitializeAndClose) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    EXPECT_TRUE(detector.initialize(&err));
    if (!err.empty()) {
        std::cerr << "Failed to initialize: " << err << "\n";
    }
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, TrackAndRetrieveBlocks) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Track some blocks
    EXPECT_TRUE(detector.trackBlock(100, "hash100", 1000));
    EXPECT_TRUE(detector.trackBlock(101, "hash101", 1001));
    EXPECT_TRUE(detector.trackBlock(102, "hash102", 1002));
    
    // Retrieve blocks
    auto hash100 = detector.getBlockHashAtHeight(100);
    ASSERT_TRUE(hash100.has_value());
    EXPECT_EQ(*hash100, "hash100");
    
    auto hash101 = detector.getBlockHashAtHeight(101);
    ASSERT_TRUE(hash101.has_value());
    EXPECT_EQ(*hash101, "hash101");
    
    auto nonexistent = detector.getBlockHashAtHeight(999);
    EXPECT_FALSE(nonexistent.has_value());
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, DetectSimpleReorg) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Track initial blocks
    detector.trackBlock(100, "hash100a", 1000);
    detector.trackBlock(101, "hash101a", 1001);
    
    // No reorg when same hash
    auto reorg1 = detector.detectReorg(100, "hash100a", 1002);
    EXPECT_FALSE(reorg1.has_value());
    
    // Reorg when different hash
    auto reorg2 = detector.detectReorg(100, "hash100b", 1003);
    ASSERT_TRUE(reorg2.has_value());
    EXPECT_EQ(reorg2->reorgHeight, 100);
    EXPECT_EQ(reorg2->oldBlockHash, "hash100a");
    EXPECT_EQ(reorg2->newBlockHash, "hash100b");
    
    // Verify new hash is stored
    auto currentHash = detector.getBlockHashAtHeight(100);
    ASSERT_TRUE(currentHash.has_value());
    EXPECT_EQ(*currentHash, "hash100b");
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, RegisterAndRetrieveAnchor) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Register an anchor
    ailee::l1::AnchorCommitmentRecord anchor;
    anchor.anchorHash = "anchor123";
    anchor.bitcoinTxId = "tx456";
    anchor.bitcoinHeight = 200;
    anchor.confirmations = 0;
    anchor.broadcastTime = 2000;
    anchor.retryCount = 0;
    anchor.status = ailee::l1::AnchorStatus::PENDING;
    anchor.l2StateRoot = "stateroot789";
    
    EXPECT_TRUE(detector.registerAnchor(anchor, &err));
    if (!err.empty()) {
        std::cerr << "Register anchor error: " << err << "\n";
    }
    
    // Retrieve the anchor
    auto retrieved = detector.getAnchorStatus("anchor123");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->anchorHash, "anchor123");
    EXPECT_EQ(retrieved->bitcoinTxId, "tx456");
    EXPECT_EQ(retrieved->bitcoinHeight, 200);
    EXPECT_EQ(retrieved->l2StateRoot, "stateroot789");
    EXPECT_EQ(retrieved->status, ailee::l1::AnchorStatus::PENDING);
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, UpdateAnchorConfirmations) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath, 6); // 6 confirmations threshold
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Register an anchor
    ailee::l1::AnchorCommitmentRecord anchor;
    anchor.anchorHash = "anchor123";
    anchor.bitcoinTxId = "tx456";
    anchor.bitcoinHeight = 200;
    anchor.confirmations = 0;
    anchor.broadcastTime = 2000;
    anchor.status = ailee::l1::AnchorStatus::PENDING;
    anchor.l2StateRoot = "stateroot789";
    
    ASSERT_TRUE(detector.registerAnchor(anchor, &err));
    
    // Update confirmations (not enough yet)
    EXPECT_TRUE(detector.updateAnchorConfirmations("anchor123", 3, &err));
    auto retrieved1 = detector.getAnchorStatus("anchor123");
    ASSERT_TRUE(retrieved1.has_value());
    EXPECT_EQ(retrieved1->confirmations, 3);
    EXPECT_EQ(retrieved1->status, ailee::l1::AnchorStatus::PENDING);
    
    // Update confirmations (now confirmed)
    EXPECT_TRUE(detector.updateAnchorConfirmations("anchor123", 6, &err));
    auto retrieved2 = detector.getAnchorStatus("anchor123");
    ASSERT_TRUE(retrieved2.has_value());
    EXPECT_EQ(retrieved2->confirmations, 6);
    EXPECT_EQ(retrieved2->status, ailee::l1::AnchorStatus::CONFIRMED);
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, InvalidateAnchorsOnReorg) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Register anchors at different heights
    ailee::l1::AnchorCommitmentRecord anchor1;
    anchor1.anchorHash = "anchor100";
    anchor1.bitcoinHeight = 100;
    anchor1.status = ailee::l1::AnchorStatus::CONFIRMED;
    anchor1.l2StateRoot = "state100";
    ASSERT_TRUE(detector.registerAnchor(anchor1, &err));
    
    ailee::l1::AnchorCommitmentRecord anchor2;
    anchor2.anchorHash = "anchor101";
    anchor2.bitcoinHeight = 101;
    anchor2.status = ailee::l1::AnchorStatus::CONFIRMED;
    anchor2.l2StateRoot = "state101";
    ASSERT_TRUE(detector.registerAnchor(anchor2, &err));
    
    ailee::l1::AnchorCommitmentRecord anchor3;
    anchor3.anchorHash = "anchor102";
    anchor3.bitcoinHeight = 102;
    anchor3.status = ailee::l1::AnchorStatus::CONFIRMED;
    anchor3.l2StateRoot = "state102";
    ASSERT_TRUE(detector.registerAnchor(anchor3, &err));
    
    // Track blocks
    detector.trackBlock(100, "hash100", 1000);
    detector.trackBlock(101, "hash101", 1001);
    
    // Trigger reorg at height 101
    auto reorg = detector.detectReorg(101, "hash101_new", 1002);
    ASSERT_TRUE(reorg.has_value());
    ASSERT_EQ(reorg->invalidatedAnchors.size(), 2); // anchor101 and anchor102
    
    // Verify anchors are invalidated
    auto status101 = detector.getAnchorStatus("anchor101");
    ASSERT_TRUE(status101.has_value());
    EXPECT_EQ(status101->status, ailee::l1::AnchorStatus::INVALIDATED_REORG);
    EXPECT_EQ(status101->confirmations, 0);
    
    auto status102 = detector.getAnchorStatus("anchor102");
    ASSERT_TRUE(status102.has_value());
    EXPECT_EQ(status102->status, ailee::l1::AnchorStatus::INVALIDATED_REORG);
    
    // Anchor at height 100 should still be confirmed
    auto status100 = detector.getAnchorStatus("anchor100");
    ASSERT_TRUE(status100.has_value());
    EXPECT_EQ(status100->status, ailee::l1::AnchorStatus::CONFIRMED);
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, DetectOrphanedAnchors) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath, 6, 1000); // 1000ms max pending
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Register anchors with different broadcast times
    ailee::l1::AnchorCommitmentRecord anchor1;
    anchor1.anchorHash = "anchor_old";
    anchor1.broadcastTime = 500;
    anchor1.confirmations = 0;
    anchor1.status = ailee::l1::AnchorStatus::PENDING;
    anchor1.l2StateRoot = "state1";
    ASSERT_TRUE(detector.registerAnchor(anchor1, &err));
    
    ailee::l1::AnchorCommitmentRecord anchor2;
    anchor2.anchorHash = "anchor_recent";
    anchor2.broadcastTime = 2000;
    anchor2.confirmations = 0;
    anchor2.status = ailee::l1::AnchorStatus::PENDING;
    anchor2.l2StateRoot = "state2";
    ASSERT_TRUE(detector.registerAnchor(anchor2, &err));
    
    // Check at time 2500 (old anchor is >1000ms old, recent is not)
    auto orphaned = detector.getOrphanedAnchors(2500);
    EXPECT_EQ(orphaned.size(), 1);
    EXPECT_EQ(orphaned[0].anchorHash, "anchor_old");
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, PersistReorgEvents) {
    std::string dbPath = getTestDbPath();
    
    {
        ailee::l1::ReorgDetector detector(dbPath);
        std::string err;
        ASSERT_TRUE(detector.initialize(&err));
        
        // Trigger a reorg
        detector.trackBlock(100, "hash100a", 1000);
        auto reorg = detector.detectReorg(100, "hash100b", 1001);
        ASSERT_TRUE(reorg.has_value());
        
        detector.close();
    }
    
    // Reopen database and check history
    {
        ailee::l1::ReorgDetector detector(dbPath);
        std::string err;
        ASSERT_TRUE(detector.initialize(&err));
        
        auto history = detector.getReorgHistory();
        EXPECT_EQ(history.size(), 1);
        EXPECT_EQ(history[0].reorgHeight, 100);
        EXPECT_EQ(history[0].oldBlockHash, "hash100a");
        EXPECT_EQ(history[0].newBlockHash, "hash100b");
        
        detector.close();
    }
    
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, PruneOldBlocks) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Track many blocks
    for (int i = 100; i < 200; ++i) {
        detector.trackBlock(i, "hash" + std::to_string(i), 1000 + i);
    }
    
    // Prune keeping only last 10
    EXPECT_TRUE(detector.pruneOldBlocks(10, &err));
    if (!err.empty()) {
        std::cerr << "Prune error: " << err << "\n";
    }
    
    // Check that old blocks are gone
    EXPECT_FALSE(detector.getBlockHashAtHeight(100).has_value());
    EXPECT_FALSE(detector.getBlockHashAtHeight(150).has_value());
    
    // Check that recent blocks remain
    EXPECT_TRUE(detector.getBlockHashAtHeight(190).has_value());
    EXPECT_TRUE(detector.getBlockHashAtHeight(199).has_value());
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, GetAnchorsByStatus) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    // Register anchors with different statuses
    ailee::l1::AnchorCommitmentRecord anchor1;
    anchor1.anchorHash = "pending1";
    anchor1.status = ailee::l1::AnchorStatus::PENDING;
    anchor1.l2StateRoot = "state1";
    ASSERT_TRUE(detector.registerAnchor(anchor1, &err));
    
    ailee::l1::AnchorCommitmentRecord anchor2;
    anchor2.anchorHash = "confirmed1";
    anchor2.status = ailee::l1::AnchorStatus::CONFIRMED;
    anchor2.l2StateRoot = "state2";
    ASSERT_TRUE(detector.registerAnchor(anchor2, &err));
    
    ailee::l1::AnchorCommitmentRecord anchor3;
    anchor3.anchorHash = "pending2";
    anchor3.status = ailee::l1::AnchorStatus::PENDING;
    anchor3.l2StateRoot = "state3";
    ASSERT_TRUE(detector.registerAnchor(anchor3, &err));
    
    // Get by status
    auto pending = detector.getAnchorsByStatus(ailee::l1::AnchorStatus::PENDING);
    EXPECT_EQ(pending.size(), 2);
    
    auto confirmed = detector.getAnchorsByStatus(ailee::l1::AnchorStatus::CONFIRMED);
    EXPECT_EQ(confirmed.size(), 1);
    EXPECT_EQ(confirmed[0].anchorHash, "confirmed1");
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, ReorgCallback) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath);
    
    std::string err;
    ASSERT_TRUE(detector.initialize(&err));
    
    bool callbackTriggered = false;
    std::uint64_t reorgHeight = 0;
    
    detector.setReorgCallback([&](const ailee::l1::ReorgEvent& event) {
        callbackTriggered = true;
        reorgHeight = event.reorgHeight;
    });
    
    // Trigger reorg
    detector.trackBlock(100, "hash100a", 1000);
    detector.detectReorg(100, "hash100b", 1001);
    
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(reorgHeight, 100);
    
    detector.close();
    cleanupTestDb(dbPath);
}

TEST(ReorgDetector, DeepReorgCheck) {
    std::string dbPath = getTestDbPath();
    ailee::l1::ReorgDetector detector(dbPath, 6); // 6 confirmations threshold
    
    EXPECT_FALSE(detector.shouldHaltForDeepReorg(3));
    EXPECT_FALSE(detector.shouldHaltForDeepReorg(6));
    EXPECT_TRUE(detector.shouldHaltForDeepReorg(7));
    EXPECT_TRUE(detector.shouldHaltForDeepReorg(100));
    
    cleanupTestDb(dbPath);
}

} // namespace
