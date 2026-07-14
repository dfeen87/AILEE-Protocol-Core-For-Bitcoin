#pragma once

#include "SwarmConfig.h"
#include <vector>
#include <mutex>

class ProverRegistry {
public:
    static ProverRegistry& instance() {
        static ProverRegistry reg;
        return reg;
    }

    void loadFromConfig(const SwarmConfig& cfg) {
        std::lock_guard<std::mutex> lock(mutex_);
        provers_ = cfg.provers;
    }

    const std::vector<ProverConfig>& getProvers() const {
        return provers_;
    }

    bool empty() const {
        return provers_.empty();
    }

private:
    ProverRegistry() = default;

    std::vector<ProverConfig> provers_;
    mutable std::mutex mutex_;
};
