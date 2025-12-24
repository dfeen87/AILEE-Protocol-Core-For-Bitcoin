/**
 * ailee_circuit_breaker.h
 *
 * Declarations for the AILEE Autonomous Circuit Breaker (canonical v1.4)
 *
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_CIRCUIT_BREAKER_H
#define AILEE_CIRCUIT_BREAKER_H

#include <string>

#include "ailee_energy_telemetry.h"

namespace ailee {

enum class SystemState {
    OPTIMIZED,
    SOFT_TRIP,
    SAFE_MODE,
    CRITICAL
};

struct BreakerReport {
    SystemState state;
    std::string reason;
    double eis;
    double entropyDelta;
    double driftScore;
};

constexpr double MAX_SAFE_BLOCK_SIZE_MB = 4.0;
constexpr double MAX_LATENCY_TOLERANCE_MS = 2000.0;
constexpr int MIN_PEER_COUNT = 8;
constexpr double MAX_ENTROPY_SURGE_DELTA = 0.35;
constexpr double MIN_EIS_FOR_OPTIMIZATION = 0.2;
constexpr double MAX_AI_DRIFT_SCORE = 0.6;

class CircuitBreaker {
public:
    static double computeAIDrift(double targetBlockSize, double proposedBlockSize);
    static BreakerReport monitor(
        double proposedBlockSize,
        double currentLatency,
        int peerCount,
        double targetBlockSize,
        const EnergyAnalysis& energy,
        double previousEIS
    );
};

} // namespace ailee

#endif // AILEE_CIRCUIT_BREAKER_H
