#pragma once

#include "SwarmConfig.h"

class SwarmConfigLoader {
public:
    // Load from config/ailee.toml (or whatever path you decide)
    static SwarmConfig load(const std::string& path = "config/ailee.toml");
};
