#pragma once

#include <string>
#include <vector>

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
        ReferencePointerRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        FunctionDiscardRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        ArrayStructRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        SpanStructRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        ConstFieldRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        NoGotoRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        InterfacesRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        GlobalVariableRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        StrictSwitchRuleConfig &cfg,
        const std::string &key,
        const std::string &value
    );

    static void applySetting(
        SingleReturnRuleConfig &cfg,
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

    /*
     * Adds an error if the rule is not Off and the setting is empty.
     */
    static void requireString(
        std::vector<std::string> &errors,
        const std::string &rule,
        RuleLevel level,
        const std::string &key,
        const std::string &value
    );

    /*
     * Adds an error if the rule is not Off and any item of the list is
     * empty. An empty list is allowed.
     */
    static void requireStrings(
        std::vector<std::string> &errors,
        const std::string &rule,
        RuleLevel level,
        const std::string &key,
        const std::vector<std::string> &values
    );

public:
    static bool loadFromFile(
        const std::string &filepath,
        Config &config
    );

    /*
     * Checks that the loaded config is usable, adding one message per
     * problem to errors. Every string setting of a rule that is not Off
     * must be given a value, since no string setting has a default.
     */
    static bool validateConfig(
        const Config &config,
        std::vector<std::string> &errors
    );
};
