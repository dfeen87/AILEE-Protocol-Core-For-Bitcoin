#pragma once

#include <string>

namespace ailee {
namespace build {

class BuildInfo {
public:
    static std::string getCommitHash();
    static std::string getBuildNumber();
    static std::string getProtocolVersion();
};

} // namespace build
} // namespace ailee