// AdapterRegistry.cpp
// Production‑ready registry for AILEE‑Core Global_Seven adapters.
// Provides thread‑safe registration, retrieval, and default bootstrap.

#include "Global_Seven.h"
#include <mutex>
#include <iostream>

namespace ailee {
namespace global_seven {

// ---- Internal state ----
static std::mutex g_registryMutex;

// ---- AdapterRegistry methods ----

void AdapterRegistry::registerAdapter(Chain chain, std::unique_ptr<IChainAdapter> adapter) {
    std::lock_guard<std::mutex> lock(g_registryMutex);
    adapters_[chain] = std::move(adapter);

    // Optional: log registration
    std::cout << "[AdapterRegistry] Registered adapter for chain="
              << static_cast<int>(chain) << std::endl;
}

IChainAdapter* AdapterRegistry::get(Chain chain) const {
    auto it = adapters_.find(chain);
    return (it != adapters_.end()) ? it->second.get() : nullptr;
}

// ---- Default bootstrap ----
// Registers the adapters that have concrete implementations available.
// Extend this list as new adapters are implemented.

void register_default_adapters() {
    std::lock_guard<std::mutex> lock(g_registryMutex);

    // Bitcoin adapter (audited, production‑ready)
    AdapterRegistry::instance().registerAdapter(
        Chain::Bitcoin,
        std::unique_ptr<IChainAdapter>(new BitcoinAdapter())
    );

    // Ethereum adapter (work‑in‑progress, audited=false)
    AdapterRegistry::instance().registerAdapter(
        Chain::Ethereum,
        std::unique_ptr<IChainAdapter>(new EthereumAdapter())
    );

    // Litecoin adapter (stub)
    AdapterRegistry::instance().registerAdapter(
        Chain::Litecoin,
        std::unique_ptr<IChainAdapter>(new LitecoinAdapter())
    );

    // Dogecoin adapter (stub)
    AdapterRegistry::instance().registerAdapter(
        Chain::Dogecoin,
        std::unique_ptr<IChainAdapter>(new DogecoinAdapter())
    );

    // Cardano adapter (stub)
    AdapterRegistry::instance().registerAdapter(
        Chain::Cardano,
        std::unique_ptr<IChainAdapter>(new CardanoAdapter())
    );

    // Solana adapter (stub)
    AdapterRegistry::instance().registerAdapter(
        Chain::Solana,
        std::unique_ptr<IChainAdapter>(new SolanaAdapter())
    );

    // Polkadot adapter (stub)
    AdapterRegistry::instance().registerAdapter(
        Chain::Polkadot,
        std::unique_ptr<IChainAdapter>(new PolkadotAdapter())
    );

    std::cout << "[AdapterRegistry] Default adapters registered." << std::endl;
}

} // namespace global_seven
} // namespace ailee
