#pragma once

#include <cstdint>
#include <string>

namespace ailee::l1 {

struct MempoolSnapshot {
    uint64_t tx_count;
    double   avg_fee_rate_sat_vb;
    double   tx_arrival_rate_tps;
    double   congestion_index;   // normalized [0,1]
};

class AILEEMempoolAdapter {
public:
    explicit AILEEMempoolAdapter(const std::string& node_endpoint);

    // Collects a normalized snapshot of mempool conditions
    MempoolSnapshot snapshot() const;

private:
    std::string node_endpoint_;

    double computeCongestionIndex(uint64_t tx_count,
                                  double arrival_rate) const;
};

} // namespace ailee::l1
