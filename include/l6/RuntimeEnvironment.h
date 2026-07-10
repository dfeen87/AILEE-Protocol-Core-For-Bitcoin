#pragma once

#include "l6/ZKProvingBackend.h"

namespace ailee::l6 {

struct RuntimeEnvironment {
    bool is_ci = false;
    bool is_dev = false;
    bool is_staging = false;
    bool is_mainnet_sim = false;
};

ZKBackendType select_backend_type(
    const RuntimeEnvironment& env,
    const ZKBackendConfig& config
);

} // namespace ailee::l6
