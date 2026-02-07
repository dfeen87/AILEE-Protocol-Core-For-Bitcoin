// CardanoAdapter.cpp
// Minimal Cardano adapter scaffold for AILEE-Core Global_Seven.
// TODO: replace with real Cardano RPC integration.

#include "Global_Seven.h"
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>

namespace ailee {
namespace global_seven {

struct CardanoState {
    AdapterConfig cfg;
    ErrorCallback onError;
    TxCallback onTx;
    BlockCallback onBlock;
    EnergyCallback onEnergy;
    std::atomic<bool> running{false};
    std::thread eventThread;
};

static inline void logEvt(Severity s, const std::string& msg, ErrorCallback cb) {
    if (cb) cb(AdapterError{s, msg, "CardanoAdapter", 0});
}

static std::shared_ptr<CardanoState> g_state;

bool CardanoAdapter::init(const AdapterConfig& cfg, ErrorCallback onError) {
    g_state = std::make_shared<CardanoState>();
    g_state->cfg = cfg;
    g_state->onError = onError;
    logEvt(Severity::Warn, "Cardano adapter initialized in scaffold mode", onError);
    if (cfg.nodeEndpoint.empty()) {
        logEvt(Severity::Warn, "Cardano endpoint not configured", onError);
    }
    return true;
}

bool CardanoAdapter::start(TxCallback onTx, BlockCallback onBlock, EnergyCallback onEnergy) {
    if (!g_state) return false;
    g_state->onTx = onTx;
    g_state->onBlock = onBlock;
    g_state->onEnergy = onEnergy;
    g_state->running.store(true);

    g_state->eventThread = std::thread([s = g_state]() {
        using namespace std::chrono_literals;
        while (s->running.load()) {
            if (s->onBlock) {
                BlockHeader bh;
                bh.hash = "cardano_stub_block";
                bh.height = 0;
                bh.chain = Chain::Cardano;
                bh.timestamp = std::chrono::system_clock::now();
                s->onBlock(bh);
            }
            std::this_thread::sleep_for(5s);
        }
    });

    return true;
}

void CardanoAdapter::stop() {
    if (!g_state) return;
    g_state->running.store(false);
    if (g_state->eventThread.joinable()) g_state->eventThread.join();
}

bool CardanoAdapter::broadcastTransaction(const std::vector<TxOut>&,
                                          const std::unordered_map<std::string, std::string>&,
    std::string& outTxHash) {
    outTxHash.clear();
    if (g_state && g_state->onError) {
        g_state->onError(AdapterError{Severity::Warn,
                                      "Cardano broadcast not implemented",
                                      "Broadcast",
                                      -1});
    }
    return false;
}

std::optional<NormalizedTx> CardanoAdapter::getTransaction(const std::string&) {
    return std::nullopt;
}

std::optional<BlockHeader> CardanoAdapter::getBlockHeader(const std::string&) {
    return std::nullopt;
}

std::optional<uint64_t> CardanoAdapter::getBlockHeight() {
    return std::nullopt;
}

} // namespace global_seven
} // namespace ailee
