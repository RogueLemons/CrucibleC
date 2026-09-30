#pragma once

#include <ostream>
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

    // A string setting, named as in the config, e.g. "rules.private.private_field"
    struct NamedSetting {
        std::string name;
        std::string value;
    };

    /*
     * Adds the setting to 'settings' when the rule is not Off and the
     * value is set, since an empty value is already reported.
     */
    static void addSetting(
        std::vector<NamedSetting> &settings,
        const std::string &rule,
        RuleLevel level,
        const std::string &key,
        const std::string &value
    );

    /*
     * Adds an error for every two settings that have the same value.
     */
    static void requireDistinct(
        std::vector<std::string> &errors,
        const std::vector<NamedSetting> &settings
    );

    /*
     * Adds an error if a boolean setting is true in a rule that is not Off,
     * while the other rule it works together with is Off.
     */
    static void requireRuleOn(
        std::vector<std::string> &errors,
        RuleLevel settingRuleLevel,
        bool setting,
        const std::string &settingName,
        const std::string &requiredRule,
        RuleLevel requiredRuleLevel
    );

    // A path without './' in front or '/' behind, for comparing folders
    static std::string normalizeFolder(const std::string &path);

public:
    static bool loadFromFile(
        const std::string &filepath,
        Config &config
    );

    /*
     * Checks that the loaded config is usable, adding one message per
     * problem to errors. Every string setting of a rule that is not Off
     * must be given a value, since no string setting has a default, and
     * settings that name different things must not have the same value.
     * Rules that are Off are not checked.
     */
    static bool validateConfig(
        const Config &config,
        std::vector<std::string> &errors
    );

    /*
     * Writes the config in the format loadFromFile reads, with every
     * setting, so it can be saved and used as a config file. Empty strings
     * are left out, since an empty value can not be written in the format
     * and a missing string setting is empty.
     */
    static void writeConfig(
        const Config &config,
        std::ostream &out
    );
};
