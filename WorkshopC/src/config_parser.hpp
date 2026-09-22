#pragma once

#include <string>

#include "config.hpp"

class ConfigParser {
private:
    static std::string trim(const std::string &str);

    static RuleLevel parseLevel(const std::string &str);

    static bool parseBool(const std::string &str);

    static int parseInt(const std::string &str, int fallback);

    static void applySetting(
        EnumRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        PrivateRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        FunctionPointerRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        TypedefStructRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        AssignmentRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        PrefixNamespaceRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        NullCheckRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        ArgumentPointerMovementRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        StructResourceManagementRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applyRuleSetting(
        Config &config,
        const std::string &currentRule,
        const std::string &key,
        const std::string &value
    );

public:
    static bool loadFromFile(
        const std::string &filepath,
        Config &config
    );
};
