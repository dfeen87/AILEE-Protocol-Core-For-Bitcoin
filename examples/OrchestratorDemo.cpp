// OrchestratorDemo.cpp
#include "Global_Seven.h"
#include <iostream>

using namespace ailee::global_seven;

static void logError(const AdapterError& e) {
    std::cerr << "[ERROR] (" << e.component << ") code=" << e.code << " msg=" << e.message << std::endl;
}

int main() {
    // 1) Register adapters
    register_default_adapters();

    // 2) Configure Bitcoin (target settlement chain)
    AdapterConfig btcCfg;
    btcCfg.chain = Chain::Bitcoin;
    btcCfg.nodeEndpoint = "http://127.0.0.1:8332";
    btcCfg.authUsername = "user";
    btcCfg.authPassword = "pass";
    btcCfg.network = "testnet";
    btcCfg.extra["zmq"] = "tcp://127.0.0.1:28332";

    // 3) Configure Ethereum (source chain)
    AdapterConfig ethCfg;
    ethCfg.chain = Chain::Ethereum;
    ethCfg.nodeEndpoint = "http://127.0.0.1:8545";
    ethCfg.network = "sepolia";
    ethCfg.extra["ws"] = "ws://127.0.0.1:8546";

    // 4) Init and start adapters
    auto* btc = AdapterRegistry::instance().get(Chain::Bitcoin);
    auto* eth = AdapterRegistry::instance().get(Chain::Ethereum);

    if (!btc || !eth) {
        std::cerr << "Adapters not found. Exiting." << std::endl;
        return 1;
    }

    if (!btc->init(btcCfg, logError)) { std::cerr << "BTC init failed\n"; return 1; }
    if (!eth->init(ethCfg, logError)) { std::cerr << "ETH init failed\n"; return 1; }

    btc->start(
        /*onTx*/[](const NormalizedTx& tx){ std::cout << "[BTC] tx " << tx.chainTxId << "\n"; },
        /*onBlock*/[](const BlockHeader& bh){ std::cout << "[BTC] block " << bh.height << "\n"; },
        /*onEnergy*/[](const EnergyTelemetry& et){ std::cout << "[BTC] energy score " << et.energyEfficiencyScore << "\n"; }
    );

    eth->start(
        [](const NormalizedTx& tx){ std::cout << "[ETH] tx " << tx.chainTxId << "\n"; },
        [](const BlockHeader& bh){ std::cout << "[ETH] block " << bh.height << "\n"; },
        [](const EnergyTelemetry& et){ std::cout << "[ETH] energy score " << et.energyEfficiencyScore << "\n"; }
    );

    // 5) Create orchestrator
    SettlementOrchestrator orchestrator(logError);
    orchestrator.setOracleConfidenceFloor(0.7, true);

    // 6) Prepare a settlement intent (SwapCrossChain ETH->BTC anchored to BTC)
    UnitSpec wei{18, "wei", "ETH"};
    UnitSpec sats{8, "sats", "BTC"};

    Amount srcAmt{Chain::Ethereum, wei, 1000000000000000000ULL}; // 1 ETH
    Amount minRecv{Chain::Bitcoin, sats, 300000000ULL};          // 3,000,000,00 sats = 3 BTC (demo)

    SettlementIntent intent{
        SettlementKind::SwapCrossChain,
        Chain::Ethereum,
        Chain::Bitcoin,
        srcAmt,
        minRecv,
        FeePolicy{ /*baseFee*/1000, /*percentFee*/0.0025, /*maxFeeCap*/50000 },
        SlippagePolicy{ /*maxSlippagePct*/0.01, /*enforceHard*/true },
        OracleSignal{ "Internal", /*price*/2000.0, /*confidence*/0.9, std::chrono::system_clock::now() },
        { {"vaultId","btc_vault_01"}, {"pegTag","ETH2BTC"}, {"targetAddress","tb1qexample..."} }
    };

    std::string targetTxId;
    RiskFlags risk;
    bool ok = orchestrator.execute(intent, targetTxId, risk);
    if (!ok) {
        std::cerr << "Settlement failed: " << risk.reason << std::endl;
    } else {
        std::cout << "Settlement broadcast to BTC: txid=" << targetTxId << std::endl;
    }

    // 7) Cleanup
    eth->stop();
    btc->stop();

    return 0;
}
