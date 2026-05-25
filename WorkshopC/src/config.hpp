#pragma once

#include <map>
#include <string>
#include <vector>

enum class RuleLevel {
    Off,
    Warning,
    Error
};

struct RuleConfig {
    RuleLevel level = RuleLevel::Warning;
    std::map<std::string, std::string> options;
};

class Config {
public:
    std::map<std::string, RuleConfig> rules;

    std::vector<std::string> projectIncludes;
    std::vector<std::string> thirdPartyIncludes;

    RuleConfig getRuleConfig(const std::string &ruleName) const {
        auto it = rules.find(ruleName);

        if (it != rules.end()) {
            return it->second;
        }

        return RuleConfig();
    }

    bool hasRule(const std::string &ruleName) const {
        auto it = rules.find(ruleName);

        if (it == rules.end()) {
            return false;
        }

        return it->second.level != RuleLevel::Off;
    }

    const std::vector<std::string>& getProjectIncludes() const {
        return projectIncludes;
    }

    const std::vector<std::string>& getThirdPartyIncludes() const {
        return thirdPartyIncludes;
    }
};