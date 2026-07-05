#include "BuildInfo.hpp"

// Fallbacks are provided via macros injected by CMake.
// If CMake doesn't define them, we provide compile-time deterministic fallbacks here.

#ifndef AILEE_COMMIT_HASH
#define AILEE_COMMIT_HASH "unknown"
#endif

#ifndef AILEE_BUILD_NUMBER
#define AILEE_BUILD_NUMBER "0"
#endif

#ifndef AILEE_PROTOCOL_VERSION
#define AILEE_PROTOCOL_VERSION "1.0.0"
#endif

namespace ailee {
namespace build {

std::string BuildInfo::getCommitHash() {
    return AILEE_COMMIT_HASH;
}

std::string BuildInfo::getBuildNumber() {
    return AILEE_BUILD_NUMBER;
}

std::string BuildInfo::getProtocolVersion() {
    return AILEE_PROTOCOL_VERSION;
}

} // namespace build
} // namespace ailee