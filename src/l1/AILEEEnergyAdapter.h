#pragma once

namespace ailee::l1 {

struct EnergySnapshot {
    double joules_per_tx;
    double load_factor;     // [0,1]
    double efficiency_eta;  // AILEE η
};

class AILEEEnergyAdapter {
public:
    AILEEEnergyAdapter(double baseline_joules_per_tx);

    EnergySnapshot snapshot(double current_load) const;

private:
    double baseline_joules_;

    double computeEfficiency(double load) const;
};

} // namespace ailee::l1
