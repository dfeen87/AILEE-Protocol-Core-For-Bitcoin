#include "PolicyRules.h"

namespace ailee {

PolicyRulesManager::PolicyRulesManager() {
    loadDefaults();
}

void PolicyRulesManager::loadDefaults() {
    rules_["DEFAULT_TRANSFER"] = {
        "DEFAULT_TRANSFER",
        "Base rule for standard Web3 asset transfers",
        true,
        50,       // Minimum reputation score (out of 100)
        1000000,  // Max transfer amount (in smallest units)
        {"USDC", "WBTC", "ETH"}
    };

    rules_["HIGH_RISK_TRANSFER"] = {
        "HIGH_RISK_TRANSFER",
        "Strict rule for volatile or unknown assets",
        true,
        90,       // Requires very high reputation
        10000,    // Very low max transfer amount
        {"MEME_TOKEN"}
    };
}

const PolicyRule* PolicyRulesManager::getRule(const std::string& ruleId) const {
    auto it = rules_.find(ruleId);
    if (it != rules_.end()) {
        return &(it->second);
    }
    return nullptr;
}

bool PolicyRulesManager::evaluateTransfer(const std::string& assetType, long amount, int requesterReputation) const {
    const PolicyRule* rule = getRule("DEFAULT_TRANSFER");

    // Check if asset is high risk (for simplicity, anything not in default is checked against high risk)
    bool isDefaultAsset = false;
    if (rule) {
        for (const auto& allowed : rule->allowedAssetTypes) {
            if (allowed == assetType) {
                isDefaultAsset = true;
                break;
            }
        }
    }

    if (!isDefaultAsset) {
        rule = getRule("HIGH_RISK_TRANSFER");
    }

    if (!rule || !rule->isActive) {
        return false; // Fail safe
    }

    if (requesterReputation < rule->minReputationScore) {
        return false;
    }

    if (amount > rule->maxTransferAmount) {
        return false;
    }

    return true;
}

} // namespace ailee
