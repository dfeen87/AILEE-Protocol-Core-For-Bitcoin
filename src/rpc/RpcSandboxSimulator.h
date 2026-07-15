#ifndef AILEE_RPC_SANDBOX_SIMULATOR_H
#define AILEE_RPC_SANDBOX_SIMULATOR_H

#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace ailee {

class RpcSandboxSimulator {
public:
    RpcSandboxSimulator();

    // Configures the simulator to mock a 404 error
    void setSimulate404(bool enable);

    // Configures the simulator to introduce a specific delay
    void setSimulatedLatency(int ms);

    // Simulates an RPC request and returns the HTTP status code
    long execute(const std::string& method, const std::string& params, std::string& responseBuffer);

private:
    bool simulate404_;
    int latencyMs_;
    long currentBlockCount_;
};

} // namespace ailee

#endif // AILEE_RPC_SANDBOX_SIMULATOR_H
