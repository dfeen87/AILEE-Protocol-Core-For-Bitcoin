#pragma once

#include "SwarmConfig.h"

class ProverSwarm; // forward declare, implemented in ProverSwarm.cpp

class SwarmRuntime {
public:
    // Initialize swarm from config and register provers into ProverSwarm
    static void initialize(ProverSwarm& swarm);
};
