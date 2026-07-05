// SPDX-License-Identifier: MIT
#include "LogicalClock.h"

namespace ailee::network {

std::atomic<uint64_t> LogicalClock::clock_{0};

} // namespace ailee::network
