#ifndef AILEE_POLICY_RULES_H
#define AILEE_POLICY_RULES_H

#include <string>
#include <map>
#include <vector>

namespace ailee {

struct PolicyRule {
    std::string ruleId;
    std::string description;
    bool isActive;
    int minReputationScore;
    long maxTransferAmount;
    std::vector<std::string> allowedAssetTypes;
};

class PolicyRulesManager {
public:
    PolicyRulesManager();

    void loadDefaults();
    const PolicyRule* getRule(const std::string& ruleId) const;
    bool evaluateTransfer(const std::string& assetType, long amount, int requesterReputation) const;

private:
    std::map<std::string, PolicyRule> rules_;
};

} // namespace ailee

#endif // AILEE_POLICY_RULES_H
