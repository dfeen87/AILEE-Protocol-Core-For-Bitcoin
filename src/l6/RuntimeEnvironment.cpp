#include "l6/RuntimeEnvironment.h"

namespace ailee::l6 {

ZKBackendType select_backend_type(
    const RuntimeEnvironment& env,
    const ZKBackendConfig& config
) {
    if (env.is_ci) {
        return ZKBackendType::MOCK;
    }
    return config.type;
}

} // namespace ailee::l6
