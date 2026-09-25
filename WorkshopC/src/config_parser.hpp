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
        SuppressionReasonRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

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
        PrivateAlternativeRuleConfig &cfg,
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

    static void applySetting(
        RestrictedMallocRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    /*
     * Parses an inline list value such as "[a, b]" or "[]".
     */
    static std::vector<std::string> parseInlineList(const std::string &value);

    static std::string unquote(const std::string &value);

    /*
     * Applies one "- item" line of a list valued rule setting, e.g.
     *
     *   restricted_malloc:
     *     list_of_allowed_malloc_functions:
     *       - memory_alloc
     */
    static void applyRuleListItem(
        Config &config,
        const std::string &currentRule,
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
