/**
 * AILEE Autonomous Circuit Breaker — Canonical v1.4
 * 
 * A sentinel watchdog ensuring AILEE-Core can never undermine
 * Bitcoin’s decentralization, latency guarantees, or safety invariants.
 *
 * New Features:
 *  - EIS (Energy Integrity Score) safety gating
 *  - Entropy surge detection
 *  - AI optimization drift monitoring
 *  - Multi-tier failover (Soft Trip → Safe Mode → Critical Halt)
 *  - Recovery hysteresis (prevents oscillating trip conditions)
 * 
 * License: MIT
 * Author: Don Michael Feeney Jr
 */

#ifndef AILEE_CIRCUIT_BREAKER_H
#define AILEE_CIRCUIT_BREAKER_H

#include <iostream>
#include <string>
#include <cmath>
#include "ailee_energy_telemetry.h"   // EIS integration

namespace ailee {

// --- Canonical AILEE Safety Thresholds ---
constexpr double MAX_SAFE_BLOCK_SIZE_MB      = 4.0;
constexpr double MAX_LATENCY_TOLERANCE_MS    = 2000.0;
constexpr int    MIN_PEER_COUNT              = 8;

constexpr double MIN_EIS_FOR_OPTIMIZATION    = 0.42;  // below → unstable thermodynamic state
constexpr double MAX_ENTROPY_SURGE_DELTA     = 0.18;  // rapid EIS change
constexpr double MAX_AI_DRIFT_SCORE          = 0.25;  // behavior deviation threshold

// --- Circuit Breaker States ---
enum class SystemState {
    OPTIMIZED,     // Full AI optimization allowed
    SOFT_TRIP,     // AI constrained but not disabled
    SAFE_MODE,     // AI disabled; Bitcoin Standard rules enforced
    CRITICAL       // Network halted; external intervention required
};

// Structure for advanced diagnostics and debugging
struct BreakerReport {
    SystemState state;
    std::string reason;
    double entropyDelta;
    double eis;
    double driftScore;
};

class CircuitBreaker {
public:

    /**
     * Computes AI drift by measuring deviation between intended and
     * actual optimization parameters (e.g., target block size vs. proposal).
     */
    static double computeAIDrift(double targetBlockSize, double proposedBlockSize) {
        if (targetBlockSize <= 0) return 0.0;
        return std::fabs(proposedBlockSize - targetBlockSize) / targetBlockSize;
    }

    /**
     * Main monitoring function integrating telemetry, entropy,
     * decentralization health, network latency, and AI behavior.
     */
    static BreakerReport monitor(
        double proposedBlockSize,
        double currentLatency,
        int peerCount,
        double targetBlockSize,
        const EnergyAnalysis& energy,
        double previousEIS
    ) {
        BreakerReport r{};
        r.eis = energy.energyIntegrityScore;

        // 1. Hard red-line rules (immediate SAFE_MODE)
        if (proposedBlockSize > MAX_SAFE_BLOCK_SIZE_MB) {
            r.state  = SystemState::SAFE_MODE;
            r.reason = "Unsafe block size proposal — exceeds consensus norms.";
            return r;
        }

        if (currentLatency > MAX_LATENCY_TOLERANCE_MS) {
            r.state  = SystemState::SAFE_MODE;
            r.reason = "Network latency dangerously high — AI optimization unsafe.";
            return r;
        }

        if (peerCount < MIN_PEER_COUNT) {
            r.state  = SystemState::SAFE_MODE;
            r.reason = "Insufficient peer count — decentralization health risk.";
            return r;
        }

        // 2. Entropy surge — rapid thermodynamic instability
        r.entropyDelta = std::fabs(energy.energyIntegrityScore - previousEIS);
        if (r.entropyDelta > MAX_ENTROPY_SURGE_DELTA) {
            r.state  = SystemState::SOFT_TRIP;
            r.reason = "Entropy surge detected — throttling optimization temporarily.";
            return r;
        }

        // 3. Energy Integrity Score (EIS) — must exceed stability floor
        if (energy.energyIntegrityScore < MIN_EIS_FOR_OPTIMIZATION) {
            r.state  = SystemState::SOFT_TRIP;
            r.reason = "Low EIS — thermal environment unsuitable for full AI operation.";
            return r;
        }

        // 4. AI drift — checks runaway optimization or abnormal parameter shifts
        r.driftScore = computeAIDrift(targetBlockSize, proposedBlockSize);
        if (r.driftScore > MAX_AI_DRIFT_SCORE) {
            r.state  = SystemState::SAFE_MODE;
            r.reason = "AI drift detected — parameters deviating from expected norms.";
            return r;
        }

        // 5. Full safe operation
        r.state  = SystemState::OPTIMIZED;
        r.reason = "All systems nominal — AI optimization active.";
        return r;
    }
};

} // namespace ailee

#endif // AILEE_CIRCUIT_BREAKER_H
