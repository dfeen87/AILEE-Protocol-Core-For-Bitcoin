#pragma once

#include <string>
#include "l4/DeterministicTelemetry.h"

namespace ailee {
namespace l4 {

struct DashboardSnapshot {
    std::string json;
};

class DashboardBuilder {
public:
    DashboardSnapshot build_snapshot(const TelemetryBuffer& buffer) const;
};

} // namespace l4
} // namespace ailee
