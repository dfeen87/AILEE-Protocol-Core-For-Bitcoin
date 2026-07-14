#pragma once

#include <string>
#include <vector>

struct ProverConfig {
    std::string pubkey;
    double capacity = 1.0;
    double latency_ms = 0.0;
    double reputation = 0.0;
};

struct SwarmConfig {
    std::vector<ProverConfig> provers;
};
