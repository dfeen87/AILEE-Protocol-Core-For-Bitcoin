#include "TransferValidator.h"
#include <iostream>

namespace ailee {

TransferValidator::TransferValidator(const PolicyRulesManager& policyManager)
    : policyManager_(policyManager) {}

bool TransferValidator::validateTransfer(const TransferRequest& request, std::string& outReason) const {
    if (request.amount <= 0) {
        outReason = "Amount must be strictly positive.";
        return false;
    }

    if (request.sourceChain.empty() || request.destinationChain.empty()) {
        outReason = "Source or destination chain cannot be empty.";
        return false;
    }

    bool passedPolicy = policyManager_.evaluateTransfer(request.assetType, request.amount, request.requesterReputation);

    if (!passedPolicy) {
        outReason = "Transfer rejected by Governor policy engine (reputation, limit, or asset restriction).";
        return false;
    }

    outReason = "Transfer validated successfully.";
    return true;
}

} // namespace ailee
