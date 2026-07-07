#pragma once

#include <cstdint>
#include "l4/ReplayTick.h"
#include "l6/ExternalSchema.h"

namespace ailee {
namespace l6 {

class ReplayExport {
public:
    ExternalReplayTick export_tick(
        uint64_t tick_index,
        const l4::ReplayTick& tick
    );
};

} // namespace l6
} // namespace ailee
