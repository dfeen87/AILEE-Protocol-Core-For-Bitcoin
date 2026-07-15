#include "GovernorEngine.h"
#include <iostream>

namespace ailee {

GovernorEngine::GovernorEngine(BitcoinRPCClient& rpcClient)
    : rpcClient_(rpcClient),
      policyManager_(),
      transferValidator_(policyManager_),
      safeModeActive_(false) {}

void GovernorEngine::heartbeatCheck() {
    // Check RPC status. If endpoint is completely lost (404), enter safe mode.
    RpcErrorState rpcError = rpcClient_.getLastError();

    if (rpcError == RpcErrorState::ENDPOINT_NOT_FOUND) {
        if (!safeModeActive_) {
            std::cerr << "[Governor] CRITICAL: RPC Endpoint Not Found (404). Activating Safe Mode." << std::endl;
            safeModeActive_ = true;
        }
    } else if (rpcError == RpcErrorState::CRITICAL_NULL_FIELD) {
        if (!safeModeActive_) {
            std::cerr << "[Governor] CRITICAL: Null field detected in critical RPC output. Activating Safe Mode." << std::endl;
            safeModeActive_ = true;
        }
    } else if (rpcError == RpcErrorState::INVALID_JSON_RESPONSE) {
         if (!safeModeActive_) {
            std::cerr << "[Governor] WARNING: Invalid JSON from RPC. Activating Safe Mode until resolved." << std::endl;
            safeModeActive_ = true;
        }
    } else {
        // We could implement recovery logic here if the connection is restored,
        // but Bitcoin's conservative nature implies human intervention might be required to lift safe mode.
        // For simulation purposes, we won't auto-lift safe mode once a semantic failure like 404 occurs.
    }
}

bool GovernorEngine::isSafeMode() const {
    return safeModeActive_;
}

bool GovernorEngine::processTransfer(const TransferRequest& request, std::string& outReason) {
    // Heartbeat check before processing any critical operations
    heartbeatCheck();

    if (safeModeActive_) {
        outReason = "Governor is in SAFE MODE due to L1 RPC failure. All transfers blocked.";
        std::cerr << "[Governor] Transfer Blocked: " << outReason << std::endl;
        return false;
    }

    bool isValid = transferValidator_.validateTransfer(request, outReason);
    if (isValid) {
        std::cout << "[Governor] Transfer Approved: " << request.transactionId << std::endl;
    } else {
        std::cerr << "[Governor] Transfer Denied: " << outReason << std::endl;
    }

    return isValid;
}

} // namespace ailee
