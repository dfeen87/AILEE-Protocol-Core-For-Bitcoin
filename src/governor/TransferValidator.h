#ifndef AILEE_TRANSFER_VALIDATOR_H
#define AILEE_TRANSFER_VALIDATOR_H

#include <string>
#include "PolicyRules.h"

namespace ailee {

struct TransferRequest {
    std::string transactionId;
    std::string sourceChain;
    std::string destinationChain;
    std::string assetType;
    long amount;
    int requesterReputation;
    std::string contractAddress;
};

class TransferValidator {
public:
    TransferValidator(const PolicyRulesManager& policyManager);

    // Evaluates a transfer request against governor policies
    bool validateTransfer(const TransferRequest& request, std::string& outReason) const;

private:
    const PolicyRulesManager& policyManager_;
};

} // namespace ailee

#endif // AILEE_TRANSFER_VALIDATOR_H
