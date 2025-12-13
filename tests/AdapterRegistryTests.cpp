// AdapterRegistryTests.cpp
// Unit tests for AILEE‑Core Global_Seven AdapterRegistry
// Requires GoogleTest (gtest) framework

#include "Global_Seven.h"
#include <gtest/gtest.h>

using namespace ailee::global_seven;

// Dummy adapter for testing
class DummyAdapter : public IChainAdapter {
public:
    bool init(const AdapterConfig&, ErrorCallback) override { return true; }
    bool start(TxCallback, BlockCallback, EnergyCallback) override { return true; }
    void stop() override {}
    bool broadcastTransaction(const std::vector<TxOut>&,
                              const std::unordered_map<std::string, std::string>&,
                              std::string& outChainTxId) override {
        outChainTxId = "dummy_txid";
        return true;
    }
    std::optional<NormalizedTx> getTransaction(const std::string& chainTxId) override {
        NormalizedTx nt;
        nt.chainTxId = chainTxId;
        nt.chain = Chain::Custom1;
        return nt;
    }
    std::optional<BlockHeader> getBlockHeader(const std::string& blockHash) override {
        BlockHeader bh;
        bh.hash = blockHash;
        bh.chain = Chain::Custom1;
        return bh;
    }
    std::optional<uint64_t> getBlockHeight() override { return 42ULL; }
    Chain chain() const override { return Chain::Custom1; }
    AdapterTraits traits() const override {
        return AdapterTraits{true,true,false,false,false,
                             UnitSpec{8,"sats","DUM"}, "DummyAdapter","0.1.0",true};
    }
};

// ---- Tests ----

TEST(AdapterRegistryTest, RegisterAndRetrieveAdapter) {
    auto& registry = AdapterRegistry::instance();

    // Register dummy adapter
    registry.registerAdapter(Chain::Custom1, std::unique_ptr<IChainAdapter>(new DummyAdapter()));

    // Retrieve adapter
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    // Verify traits
    AdapterTraits traits = adapter->traits();
    EXPECT_EQ(traits.adapterName, "DummyAdapter");
    EXPECT_TRUE(traits.audited);
}

TEST(AdapterRegistryTest, BroadcastTransactionWorks) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    std::string txid;
    bool ok = adapter->broadcastTransaction({}, {}, txid);
    EXPECT_TRUE(ok);
    EXPECT_EQ(txid, "dummy_txid");
}

TEST(AdapterRegistryTest, GetTransactionReturnsNormalizedTx) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    auto tx = adapter->getTransaction("abc123");
    ASSERT_TRUE(tx.has_value());
    EXPECT_EQ(tx->chainTxId, "abc123");
    EXPECT_EQ(tx->chain, Chain::Custom1);
}

TEST(AdapterRegistryTest, GetBlockHeaderReturnsHeader) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    auto bh = adapter->getBlockHeader("blockhash");
    ASSERT_TRUE(bh.has_value());
    EXPECT_EQ(bh->hash, "blockhash");
    EXPECT_EQ(bh->chain, Chain::Custom1);
}

TEST(AdapterRegistryTest, GetBlockHeightReturnsValue) {
    auto& registry = AdapterRegistry::instance();
    IChainAdapter* adapter = registry.get(Chain::Custom1);
    ASSERT_NE(adapter, nullptr);

    auto h = adapter->getBlockHeight();
    ASSERT_TRUE(h.has_value());
    EXPECT_EQ(h.value(), 42ULL);
}
