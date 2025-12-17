#include "AILEEEnergyAdapter.h"

namespace ailee::l1 {

AILEEEnergyAdapter::AILEEEnergyAdapter(
    double baseline_joules_per_tx
) : baseline_joules_(baseline_joules_per_tx) {}

EnergySnapshot AILEEEnergyAdapter::snapshot(
    double current_load
) const {
    double eta = computeEfficiency(current_load);

    return {
        baseline_joules_ / (eta > 0.0 ? eta : 1.0),
        current_load,
        eta
    };
}

double AILEEEnergyAdapter::computeEfficiency(
    double load
) const {
    // AILEE η is bounded, monotonic, and load-aware
    if (load <= 0.0) return 1.0;
    return 1.0 / (1.0 + load);
}

} // namespace ailee::l1
