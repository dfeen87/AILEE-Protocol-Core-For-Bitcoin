#ifndef AILEE_GOVERNOR_ENGINE_H
#define AILEE_GOVERNOR_ENGINE_H

#include "../rpc/BitcoinRPCClient.h"
#include "PolicyRules.h"
#include "TransferValidator.h"
#include <string>

namespace ailee {

class GovernorEngine {
public:
    GovernorEngine(BitcoinRPCClient& rpcClient);

    // Checks the L1 connection health and enters safe mode if critical errors occurred
    void heartbeatCheck();

    // Indicates whether the system is in safe mode
    bool isSafeMode() const;

    // Evaluates a cross-chain/L2 transfer. Blocks if in safe mode.
    bool processTransfer(const TransferRequest& request, std::string& outReason);

private:
    BitcoinRPCClient& rpcClient_;
    PolicyRulesManager policyManager_;
    TransferValidator transferValidator_;
    bool safeModeActive_;
};

} // namespace ailee

#endif // AILEE_GOVERNOR_ENGINE_H
