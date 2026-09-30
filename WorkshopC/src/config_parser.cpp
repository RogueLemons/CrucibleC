#include "config_parser.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>

std::string ConfigParser::trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    size_t last = str.find_last_not_of(" \t\r\n");

    return str.substr(first, last - first + 1);
}

RuleLevel ConfigParser::parseLevel(const std::string &str) {
    std::string lower = str;

    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        ::tolower
    );

    if (lower == "off") {
        return RuleLevel::Off;
    }

    if (lower == "warning") {
        return RuleLevel::Warning;
    }

    if (lower == "error") {
        return RuleLevel::Error;
    }

    return RuleLevel::Warning;
}

bool ConfigParser::parseBool(const std::string &str) {
    std::string lower = str;

    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        ::tolower
    );

    return lower == "true" || lower == "1" || lower == "yes";
}

int ConfigParser::parseInt(const std::string &str, int fallback) {
    try {
        return std::stoi(str);
    }
    catch (...) {
        return fallback;
    }
}

void ConfigParser::applySetting(
    SuppressionReasonRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    EnumRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "allow_enum_typedef") {
        cfg.allowEnumTypedef = parseBool(value);
    }
}

void ConfigParser::applySetting(
    PrivateRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "private_field") {
        cfg.privateField = value;
    }
    else if (key == "getter_contains") {
        cfg.getterContains = value;
    }
    else if (key == "setter_contains") {
        cfg.setterContains = value;
    }
}

void ConfigParser::applySetting(
    PrivateAlternativeRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    FunctionPointerRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    TypedefStructRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    AssignmentRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "forbid_zero_init_for_objects_with_pointers") {
        cfg.forbidZeroInitForObjectsWithPointers = parseBool(value);
    }
    else if (key == "forbid_null_assign") {
        cfg.forbidNullAssign = parseBool(value);
    }
    else if (key == "forbid_mut_arg_pointer") {
        cfg.forbidMutArgPointer = parseBool(value);
    }
    else if (key == "forbid_arg_reassign") {
        cfg.forbidArgReassign = parseBool(value);
    }
    else if (key == "forbid_null_as_arg") {
        cfg.forbidNullAsArg = parseBool(value);
    }
}

void ConfigParser::applySetting(
    PrefixNamespaceRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "top_dir") {
        cfg.topDir = value;
    }
    else if (key == "work_from_top") {
        cfg.workFromTop = parseBool(value);
    }
    else if (key == "stop_at_count") {
        cfg.stopAtCount = parseInt(value, cfg.stopAtCount);
    }
    else if (key == "use_seperator") {
        cfg.useSeparator = parseBool(value);
    }
    else if (key == "seperator") {
        cfg.separator = value;
    }
    else if (key == "require_ifndef_for_filepath") {
        cfg.requireIfndefForFilepath = parseBool(value);
    }
    else if (key == "apply_to_functions") {
        cfg.applyToFunctions = parseBool(value);
    }
    else if (key == "apply_to_structs") {
        cfg.applyToStructs = parseBool(value);
    }
    else if (key == "apply_to_typedefs") {
        cfg.applyToTypedefs = parseBool(value);
    }
    else if (key == "case_insensitive") {
        cfg.caseInsensitive = parseBool(value);
    }
}

void ConfigParser::applySetting(
    NullCheckRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "allow_direct_ptr_in_if_statement") {
        cfg.allowDirectPtrInIfStatement = parseBool(value);
    }
}

void ConfigParser::applySetting(
    ArgumentPointerMovementRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "require_operator_for_move_callsite") {
        cfg.requireOperatorForMoveCallsite = parseBool(value);
    }
    else if (key == "require_operator_for_out_callsite") {
        cfg.requireOperatorForOutCallsite = parseBool(value);
    }
    else if (key == "require_operator_for_modify_callsite") {
        cfg.requireOperatorForModifyCallsite = parseBool(value);
    }
}

void ConfigParser::applySetting(
    StructResourceManagementRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "pod_struct_creator_suffix") {
        cfg.podStructCreatorSuffix = value;
    }
    else if (key == "raii_struct_creator_suffix") {
        cfg.raiiStructCreatorSuffix = value;
    }
    else if (key == "raii_struct_destroyer_suffix") {
        cfg.raiiStructDestroyerSuffix = value;
    }
    else if (key == "raii_struct_copy_suffix") {
        cfg.raiiStructCopySuffix = value;
    }
    else if (key == "raii_struct_move_suffix") {
        cfg.raiiStructMoveSuffix = value;
    }
    else if (key == "raii_struct_return_suffix") {
        cfg.raiiStructReturnSuffix = value;
    }
    else if (key == "raii_struct_valid_suffix") {
        cfg.raiiStructValidSuffix = value;
    }
    else if (key == "free_struct_creator_suffix") {
        cfg.freeStructCreatorSuffix = value;
    }
    else if (key == "raii_struct_array_destroyer_suffix") {
        cfg.raiiStructArrayDestroyerSuffix = value;
    }
    else if (key == "allow_raii_struct_arrays") {
        cfg.allowRaiiStructArrays = parseBool(value);
    }
    else if (key == "raii_use_after_destroy") {
        cfg.raiiUseAfterDestroy = parseBool(value);
    }
    else if (key == "raii_may_only_move_value_ref") {
        cfg.raiiMayOnlyMoveValueRef = parseBool(value);
    }
    else if (key == "raii_may_only_destroy_value_ref") {
        cfg.raiiMayOnlyDestroyValueRef = parseBool(value);
    }
    else if (key == "raii_destroy_in_reverse_order") {
        cfg.raiiDestroyInReverseOrder = parseBool(value);
    }
    else if (key == "raii_standardized_destroy_definitions") {
        cfg.raiiStandardizedDestroyDefinitions = parseBool(value);
    }
}

std::string ConfigParser::unquote(const std::string &value) {
    if (value.size() >= 2 &&
        (value.front() == '"' || value.front() == '\'') &&
        value.back() == value.front())
    {
        return value.substr(1, value.size() - 2);
    }

    return value;
}

std::vector<std::string> ConfigParser::parseInlineList(const std::string &value) {
    std::vector<std::string> items;

    if (value.size() < 2 || value.front() != '[' || value.back() != ']')
        return items;

    std::string item;

    for (char c : value.substr(1, value.size() - 2)) {
        if (c == ',') {
            item = unquote(trim(item));

            if (!item.empty())
                items.push_back(item);

            item.clear();
        }
        else {
            item += c;
        }
    }

    item = unquote(trim(item));

    if (!item.empty())
        items.push_back(item);

    return items;
}

void ConfigParser::applySetting(
    ReferencePointerRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "disable_null_check_rule_for_reference_pointers") {
        cfg.disableNullCheckRuleForReferencePointers = parseBool(value);
    }
}

void ConfigParser::applySetting(
    FunctionDiscardRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    ArrayStructRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "only_allow_array_passing_to_library_functions") {
        cfg.onlyAllowArrayPassingToLibraryFunctions = parseBool(value);
    }
    else if (key == "enforce_size_suffix_for_array_structs") {
        cfg.enforceSizeSuffixForArrayStructs = parseBool(value);
    }
    else if (key == "size_suffix_with_underscore") {
        cfg.sizeSuffixWithUnderscore = parseBool(value);
    }
    else if (key == "flexible_size_name") {
        cfg.flexibleSizeName = value;
    }
    else if (key == "enforce_suffix_for_array_structs") {
        cfg.enforceSuffixForArrayStructs = parseBool(value);
    }
    else if (key == "array_struct_suffix") {
        cfg.arrayStructSuffix = value;
    }
    else if (key == "struct_name_as_prefix") {
        cfg.structNameAsPrefix = parseBool(value);
    }
    else if (key == "use_naming_rules_on_one_array_field_structs_without_forbidding_public_arrays") {
        cfg.useNamingRulesOnOneArrayFieldStructsWithoutForbiddingPublicArrays = parseBool(value);
    }
}

void ConfigParser::applySetting(
    SpanStructRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "only_allow_array_passing_to_library_functions_and_spans") {
        cfg.onlyAllowArrayPassingToLibraryFunctionsAndSpans = parseBool(value);
    }
    else if (key == "span_struct_suffix") {
        cfg.spanStructSuffix = value;
    }
    else if (key == "const_span_struct_suffix") {
        cfg.constSpanStructSuffix = value;
    }
    else if (key == "allow_spans_to_be_given_fewer_elements_than_their_size") {
        cfg.allowSpansToBeGivenFewerElementsThanTheirSize = parseBool(value);
    }
    else if (key == "require_span_immediately_after_array") {
        cfg.requireSpanImmediatelyAfterArray = parseBool(value);
    }
    else if (key == "only_allow_span_data_passing_in_one_line_static_functions") {
        cfg.onlyAllowSpanDataPassingInOneLineStaticFunctions = parseBool(value);
    }
    else if (key == "allow_pod_span_to_be_initialized_manually_if_static") {
        cfg.allowPodSpanToBeInitializedManuallyIfStatic = parseBool(value);
    }
}

void ConfigParser::applySetting(
    ConstFieldRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    NoGotoRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    InterfacesRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "vtable_suffix") {
        cfg.vtableSuffix = value;
    }
    else if (key == "interface_suffix") {
        cfg.interfaceSuffix = value;
    }
    else if (key == "const_interface_suffix") {
        cfg.constInterfaceSuffix = value;
    }
    else if (key == "interface_must_have_fields_that_are_private_alternative") {
        cfg.interfaceMustHaveFieldsThatArePrivateAlternative = parseBool(value);
    }
}

void ConfigParser::applySetting(
    GlobalVariableRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "require_prefix") {
        cfg.requirePrefix = parseBool(value);
    }
    else if (key == "prefix") {
        cfg.prefix = unquote(value);
    }
    else if (key == "must_be_caps") {
        cfg.mustBeCaps = parseBool(value);
    }
    else if (key == "must_be_static") {
        cfg.mustBeStatic = parseBool(value);
    }
    else if (key == "must_be_const") {
        cfg.mustBeConst = parseBool(value);
    }
    else if (key == "treat_local_static_as_global") {
        cfg.treatLocalStaticAsGlobal = parseBool(value);
    }
    else if (key == "require_local_static_prefix") {
        cfg.requireLocalStaticPrefix = parseBool(value);
    }
    else if (key == "local_static_prefix") {
        cfg.localStaticPrefix = unquote(value);
    }
    else if (key == "forbid_static_in_header") {
        cfg.forbidStaticInHeader = parseBool(value);
    }
}

void ConfigParser::applySetting(
    StrictSwitchRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
}

void ConfigParser::applySetting(
    SingleReturnRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "allow_early_return") {
        cfg.allowEarlyReturn = parseBool(value);
    }
    else if (key == "require_return_for_void") {
        cfg.requireReturnForVoid = parseBool(value);
    }
}

void ConfigParser::applySetting(
    RestrictedMallocRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
    }
    else if (key == "list_of_allowed_malloc_functions") {
        cfg.listOfAllowedMallocFunctions = parseInlineList(value);
    }
}

void ConfigParser::applyRuleListItem(
    Config &config,
    const std::string &currentRule,
    const std::string &key,
    const std::string &value
) {
    const std::string item = unquote(value);

    if (item.empty())
        return;

    if (currentRule == "restricted_malloc" &&
        key == "list_of_allowed_malloc_functions")
    {
        config.restrictedMallocRule.listOfAllowedMallocFunctions.push_back(item);
    }
}

void ConfigParser::applyRuleSetting(
    Config &config,
    const std::string &currentRule,
    const std::string &key,
    const std::string &value
) {
    if (currentRule == "suppression_reason_rule") {
        applySetting(config.suppressionReasonRule, key, value);
    }
    else if (currentRule == "enum") {
        applySetting(config.enumRule, key, value);
    }
    else if (currentRule == "private") {
        applySetting(config.privateRule, key, value);
    }
    else if (currentRule == "private_alternative") {
        applySetting(config.privateAlternativeRule, key, value);
    }
    else if (currentRule == "function_pointer") {
        applySetting(config.functionPointerRule, key, value);
    }
    else if (currentRule == "typedef_struct") {
        applySetting(config.typedefStructRule, key, value);
    }
    else if (currentRule == "assignment") {
        applySetting(config.assignmentRule, key, value);
    }
    else if (currentRule == "prefix_namespace") {
        applySetting(config.prefixNamespaceRule, key, value);
    }
    else if (currentRule == "null_check") {
        applySetting(config.nullCheckRule, key, value);
    }
    else if (currentRule == "argument_pointer_movement") {
        applySetting(config.argumentPointerMovementRule, key, value);
    }
    else if (currentRule == "struct_resource_management") {
        applySetting(config.structResourceManagementRule, key, value);
    }
    else if (currentRule == "restricted_malloc") {
        applySetting(config.restrictedMallocRule, key, value);
    }
    else if (currentRule == "single_return") {
        applySetting(config.singleReturnRule, key, value);
    }
    else if (currentRule == "strict_switch") {
        applySetting(config.strictSwitchRule, key, value);
    }
    else if (currentRule == "global_variable") {
        applySetting(config.globalVariableRule, key, value);
    }
    else if (currentRule == "reference_pointer") {
        applySetting(config.referencePointerRule, key, value);
    }
    else if (currentRule == "function_discard") {
        applySetting(config.functionDiscardRule, key, value);
    }
    else if (currentRule == "array_struct") {
        applySetting(config.arrayStructRule, key, value);
    }
    else if (currentRule == "span_struct") {
        applySetting(config.spanStructRule, key, value);
    }
    else if (currentRule == "const_field") {
        applySetting(config.constFieldRule, key, value);
    }
    else if (currentRule == "no_goto") {
        applySetting(config.noGotoRule, key, value);
    }
    else if (currentRule == "interfaces") {
        applySetting(config.interfacesRule, key, value);
    }
}

bool ConfigParser::loadFromFile(
    const std::string &filepath,
    Config &config
) {
    std::ifstream file(filepath);

    if (!file.is_open()) {
        return false;
    }

    std::string line;

    std::string currentSection;
    std::string currentRule;

    // Indentation of the current rule's name, so that a nested
    // "key:" line (a list valued setting) can be told apart from
    // the name of the next rule.
    size_t currentRuleIndent = 0;

    // The list valued rule setting that "- item" lines belong to.
    std::string currentListKey;

    bool inThirdPartyIncludes = false;

    while (std::getline(file, line)) {
        const size_t indent = line.find_first_not_of(" \t");

        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        // sections
        if (line == "paths:" || line == "rules:") {
            currentSection = line.substr(0, line.size() - 1);
            currentRule.clear();
            currentListKey.clear();
            inThirdPartyIncludes = false;
            continue;
        }

        if (line == "third_party_includes:") {
            inThirdPartyIncludes = true;
            continue;
        }

        // Any other key outside the rules section ends the list above, so
        // the items of an unknown list are ignored instead of being added
        // to third_party_includes
        if (currentSection != "rules" && line.back() == ':') {
            inThirdPartyIncludes = false;
            continue;
        }

        // list item
        if (line.starts_with("-")) {
            std::string value = trim(line.substr(1));

            if (!currentListKey.empty()) {
                applyRuleListItem(config, currentRule, currentListKey, value);
            }
            else if (inThirdPartyIncludes) {
                config.thirdPartyIncludes.push_back(value);
            }

            continue;
        }

        // rule, or a list valued setting nested inside the current rule
        if (currentSection == "rules" && line.back() == ':') {
            const std::string key = trim(
                line.substr(0, line.size() - 1)
            );

            if (!currentRule.empty() && indent > currentRuleIndent) {
                currentListKey = key;
                continue;
            }

            currentRule = key;
            currentRuleIndent = indent;
            currentListKey.clear();
            continue;
        }

        // top level settings, outside of the rules section
        size_t colon = line.find(':');

        if (currentSection != "rules" && colon != std::string::npos) {
            const std::string key = trim(line.substr(0, colon));
            const std::string value = unquote(trim(line.substr(colon + 1)));

            inThirdPartyIncludes = false;

            if (key == "compile_commands_dir")
                config.compileCommandsDir = value;

            continue;
        }

        // rule settings

        if (colon != std::string::npos && !currentRule.empty()) {
            currentListKey.clear();

            std::string key =
                trim(line.substr(0, colon));

            std::string value =
                trim(line.substr(colon + 1));

            applyRuleSetting(config, currentRule, key, value);
        }
    }

    return true;
}

void ConfigParser::requireString(
    std::vector<std::string> &errors,
    const std::string &rule,
    RuleLevel level,
    const std::string &key,
    const std::string &value
) {
    if (level != RuleLevel::Off && value.empty()) {
        errors.push_back(
            "rules." + rule + "." + key + " must be set when the rule is not Off"
        );
    }
}

void ConfigParser::requireStrings(
    std::vector<std::string> &errors,
    const std::string &rule,
    RuleLevel level,
    const std::string &key,
    const std::vector<std::string> &values
) {
    for (const std::string &value : values) {
        if (level != RuleLevel::Off && value.empty()) {
            errors.push_back(
                "rules." + rule + "." + key + " may not contain an empty item when the rule is not Off"
            );
            return;
        }
    }
}

void ConfigParser::addSetting(
    std::vector<NamedSetting> &settings,
    const std::string &rule,
    RuleLevel level,
    const std::string &key,
    const std::string &value
) {
    if (level != RuleLevel::Off && !value.empty())
        settings.push_back({ "rules." + rule + "." + key, value });
}

void ConfigParser::requireDistinct(
    std::vector<std::string> &errors,
    const std::vector<NamedSetting> &settings
) {
    for (size_t i = 0; i < settings.size(); ++i) {
        for (size_t j = i + 1; j < settings.size(); ++j) {
            if (settings[i].value == settings[j].value) {
                errors.push_back(
                    settings[i].name + " and " + settings[j].name +
                    " must be different, both are '" + settings[i].value + "'"
                );
            }
        }
    }
}

void ConfigParser::requireRuleOn(
    std::vector<std::string> &errors,
    RuleLevel settingRuleLevel,
    bool setting,
    const std::string &settingName,
    const std::string &requiredRule,
    RuleLevel requiredRuleLevel
) {
    if (settingRuleLevel != RuleLevel::Off &&
        setting &&
        requiredRuleLevel == RuleLevel::Off)
    {
        errors.push_back(
            "rules." + settingName + " is true, which needs the " +
            requiredRule + " rule to be Warning or Error, not Off"
        );
    }
}

std::string ConfigParser::normalizeFolder(const std::string &path) {
    std::string folder = path;

    std::replace(folder.begin(), folder.end(), '\\', '/');

    while (folder.starts_with("./"))
        folder.erase(0, 2);

    while (!folder.empty() && folder.back() == '/')
        folder.pop_back();

    return folder;
}

bool ConfigParser::validateConfig(
    const Config &config,
    std::vector<std::string> &errors
) {
    const size_t errorsBefore = errors.size();

    const PrivateRuleConfig &priv = config.privateRule;
    requireString(errors, "private", priv.level, "private_field", priv.privateField);
    requireString(errors, "private", priv.level, "getter_contains", priv.getterContains);
    requireString(errors, "private", priv.level, "setter_contains", priv.setterContains);

    const PrefixNamespaceRuleConfig &prefix = config.prefixNamespaceRule;
    requireString(errors, "prefix_namespace", prefix.level, "top_dir", prefix.topDir);
    requireString(errors, "prefix_namespace", prefix.level, "seperator", prefix.separator);

    const StructResourceManagementRuleConfig &srm = config.structResourceManagementRule;
    requireString(errors, "struct_resource_management", srm.level, "pod_struct_creator_suffix", srm.podStructCreatorSuffix);
    requireString(errors, "struct_resource_management", srm.level, "raii_struct_creator_suffix", srm.raiiStructCreatorSuffix);
    requireString(errors, "struct_resource_management", srm.level, "raii_struct_destroyer_suffix", srm.raiiStructDestroyerSuffix);
    requireString(errors, "struct_resource_management", srm.level, "raii_struct_copy_suffix", srm.raiiStructCopySuffix);
    requireString(errors, "struct_resource_management", srm.level, "raii_struct_move_suffix", srm.raiiStructMoveSuffix);
    requireString(errors, "struct_resource_management", srm.level, "raii_struct_return_suffix", srm.raiiStructReturnSuffix);
    requireString(errors, "struct_resource_management", srm.level, "raii_struct_valid_suffix", srm.raiiStructValidSuffix);
    requireString(errors, "struct_resource_management", srm.level, "free_struct_creator_suffix", srm.freeStructCreatorSuffix);

    if (srm.level != RuleLevel::Off &&
        srm.allowRaiiStructArrays &&
        srm.raiiStructArrayDestroyerSuffix.empty())
    {
        errors.push_back(
            "rules.struct_resource_management.raii_struct_array_destroyer_suffix must be set when allow_raii_struct_arrays is true"
        );
    }

    const InterfacesRuleConfig &interfaces = config.interfacesRule;
    requireString(errors, "interfaces", interfaces.level, "vtable_suffix", interfaces.vtableSuffix);
    requireString(errors, "interfaces", interfaces.level, "interface_suffix", interfaces.interfaceSuffix);
    requireString(errors, "interfaces", interfaces.level, "const_interface_suffix", interfaces.constInterfaceSuffix);

    const RestrictedMallocRuleConfig &restrictedMalloc = config.restrictedMallocRule;
    requireStrings(errors, "restricted_malloc", restrictedMalloc.level, "list_of_allowed_malloc_functions", restrictedMalloc.listOfAllowedMallocFunctions);

    const GlobalVariableRuleConfig &global = config.globalVariableRule;
    requireString(errors, "global_variable", global.level, "prefix", global.prefix);
    requireString(errors, "global_variable", global.level, "local_static_prefix", global.localStaticPrefix);

    const ArrayStructRuleConfig &array = config.arrayStructRule;
    requireString(errors, "array_struct", array.level, "flexible_size_name", array.flexibleSizeName);
    requireString(errors, "array_struct", array.level, "array_struct_suffix", array.arrayStructSuffix);

    const SpanStructRuleConfig &span = config.spanStructRule;
    requireString(errors, "span_struct", span.level, "span_struct_suffix", span.spanStructSuffix);
    requireString(errors, "span_struct", span.level, "const_span_struct_suffix", span.constSpanStructSuffix);

    // Settings that name different things may not have the same value

    std::vector<NamedSetting> accessors;
    addSetting(accessors, "private", priv.level, "getter_contains", priv.getterContains);
    addSetting(accessors, "private", priv.level, "setter_contains", priv.setterContains);
    requireDistinct(errors, accessors);

    if (prefix.level != RuleLevel::Off) {
        if (prefix.stopAtCount < 0) {
            errors.push_back(
                "rules.prefix_namespace.stop_at_count may not be negative, it is " +
                std::to_string(prefix.stopAtCount)
            );
        }

        const std::string topDir = normalizeFolder(prefix.topDir);

        for (const std::string &include : config.thirdPartyIncludes) {
            if (!topDir.empty() && normalizeFolder(include) == topDir) {
                errors.push_back(
                    "rules.prefix_namespace.top_dir '" + prefix.topDir +
                    "' may not be one of the third_party_includes"
                );
                break;
            }
        }
    }

    std::vector<NamedSetting> structFunctions;
    addSetting(structFunctions, "struct_resource_management", srm.level, "pod_struct_creator_suffix", srm.podStructCreatorSuffix);
    addSetting(structFunctions, "struct_resource_management", srm.level, "raii_struct_creator_suffix", srm.raiiStructCreatorSuffix);
    addSetting(structFunctions, "struct_resource_management", srm.level, "raii_struct_destroyer_suffix", srm.raiiStructDestroyerSuffix);
    addSetting(structFunctions, "struct_resource_management", srm.level, "raii_struct_copy_suffix", srm.raiiStructCopySuffix);
    addSetting(structFunctions, "struct_resource_management", srm.level, "raii_struct_move_suffix", srm.raiiStructMoveSuffix);
    addSetting(structFunctions, "struct_resource_management", srm.level, "raii_struct_return_suffix", srm.raiiStructReturnSuffix);
    addSetting(structFunctions, "struct_resource_management", srm.level, "raii_struct_valid_suffix", srm.raiiStructValidSuffix);
    addSetting(structFunctions, "struct_resource_management", srm.level, "free_struct_creator_suffix", srm.freeStructCreatorSuffix);

    if (srm.allowRaiiStructArrays)
        addSetting(structFunctions, "struct_resource_management", srm.level, "raii_struct_array_destroyer_suffix", srm.raiiStructArrayDestroyerSuffix);

    requireDistinct(errors, structFunctions);

    std::vector<NamedSetting> structNames;
    addSetting(structNames, "interfaces", interfaces.level, "vtable_suffix", interfaces.vtableSuffix);
    addSetting(structNames, "interfaces", interfaces.level, "interface_suffix", interfaces.interfaceSuffix);
    addSetting(structNames, "interfaces", interfaces.level, "const_interface_suffix", interfaces.constInterfaceSuffix);
    addSetting(structNames, "array_struct", array.level, "flexible_size_name", array.flexibleSizeName);
    addSetting(structNames, "array_struct", array.level, "array_struct_suffix", array.arrayStructSuffix);
    addSetting(structNames, "span_struct", span.level, "span_struct_suffix", span.spanStructSuffix);
    addSetting(structNames, "span_struct", span.level, "const_span_struct_suffix", span.constSpanStructSuffix);
    requireDistinct(errors, structNames);

    // Settings that only work together with another setting or rule

    if (array.level != RuleLevel::Off &&
        array.useNamingRulesOnOneArrayFieldStructsWithoutForbiddingPublicArrays &&
        !array.enforceSizeSuffixForArrayStructs &&
        !array.enforceSuffixForArrayStructs &&
        !array.structNameAsPrefix)
    {
        errors.push_back(
            "rules.array_struct.use_naming_rules_on_one_array_field_structs_without_forbidding_public_arrays "
            "needs at least one of enforce_size_suffix_for_array_structs, "
            "enforce_suffix_for_array_structs or struct_name_as_prefix to be true"
        );
    }

    requireRuleOn(errors, span.level, span.allowPodSpanToBeInitializedManuallyIfStatic,
        "span_struct.allow_pod_span_to_be_initialized_manually_if_static",
        "struct_resource_management", srm.level);

    requireRuleOn(errors, interfaces.level, interfaces.interfaceMustHaveFieldsThatArePrivateAlternative,
        "interfaces.interface_must_have_fields_that_are_private_alternative",
        "private_alternative", config.privateAlternativeRule.level);

    requireRuleOn(errors, config.referencePointerRule.level,
        config.referencePointerRule.disableNullCheckRuleForReferencePointers,
        "reference_pointer.disable_null_check_rule_for_reference_pointers",
        "null_check", config.nullCheckRule.level);

    return errors.size() == errorsBefore;
}

void ConfigParser::writeConfig(
    const Config &config,
    std::ostream &out
) {
    auto level = [&](RuleLevel value) {
        out << "    level: "
            << (value == RuleLevel::Error ? "Error" :
                value == RuleLevel::Warning ? "Warning" : "Off")
            << "\n";
    };

    auto boolean = [&](const char *key, bool value) {
        out << "    " << key << ": " << (value ? "true" : "false") << "\n";
    };

    auto number = [&](const char *key, int value) {
        out << "    " << key << ": " << value << "\n";
    };

    auto text = [&](const char *key, const std::string &value) {
        if (!value.empty())
            out << "    " << key << ": " << value << "\n";
    };

    auto rule = [&](const char *name) {
        out << "\n  " << name << ":\n";
    };

    out << "# WorkshopC Configuration File\n\n";

    if (config.thirdPartyIncludes.empty()) {
        out << "third_party_includes: []\n";
    }
    else {
        out << "third_party_includes:\n";

        for (const std::string &include : config.thirdPartyIncludes)
            out << "  - " << include << "\n";
    }

    if (!config.compileCommandsDir.empty())
        out << "\ncompile_commands_dir: " << config.compileCommandsDir << "\n";

    out << "\nrules:";

    rule("suppression_reason_rule");
    level(config.suppressionReasonRule.level);

    rule("enum");
    level(config.enumRule.level);
    boolean("allow_enum_typedef", config.enumRule.allowEnumTypedef);

    const PrivateRuleConfig &priv = config.privateRule;
    rule("private");
    level(priv.level);
    text("private_field", priv.privateField);
    text("getter_contains", priv.getterContains);
    text("setter_contains", priv.setterContains);

    rule("private_alternative");
    level(config.privateAlternativeRule.level);

    rule("function_pointer");
    level(config.functionPointerRule.level);

    rule("typedef_struct");
    level(config.typedefStructRule.level);

    const AssignmentRuleConfig &assignment = config.assignmentRule;
    rule("assignment");
    level(assignment.level);
    boolean("forbid_zero_init_for_objects_with_pointers", assignment.forbidZeroInitForObjectsWithPointers);
    boolean("forbid_null_assign", assignment.forbidNullAssign);
    boolean("forbid_mut_arg_pointer", assignment.forbidMutArgPointer);
    boolean("forbid_arg_reassign", assignment.forbidArgReassign);
    boolean("forbid_null_as_arg", assignment.forbidNullAsArg);

    const PrefixNamespaceRuleConfig &prefix = config.prefixNamespaceRule;
    rule("prefix_namespace");
    level(prefix.level);
    text("top_dir", prefix.topDir);
    boolean("work_from_top", prefix.workFromTop);
    number("stop_at_count", prefix.stopAtCount);
    boolean("use_seperator", prefix.useSeparator);
    text("seperator", prefix.separator);
    boolean("require_ifndef_for_filepath", prefix.requireIfndefForFilepath);
    boolean("apply_to_functions", prefix.applyToFunctions);
    boolean("apply_to_structs", prefix.applyToStructs);
    boolean("apply_to_typedefs", prefix.applyToTypedefs);
    boolean("case_insensitive", prefix.caseInsensitive);

    rule("null_check");
    level(config.nullCheckRule.level);
    boolean("allow_direct_ptr_in_if_statement", config.nullCheckRule.allowDirectPtrInIfStatement);

    const ArgumentPointerMovementRuleConfig &movement = config.argumentPointerMovementRule;
    rule("argument_pointer_movement");
    level(movement.level);
    boolean("require_operator_for_move_callsite", movement.requireOperatorForMoveCallsite);
    boolean("require_operator_for_out_callsite", movement.requireOperatorForOutCallsite);
    boolean("require_operator_for_modify_callsite", movement.requireOperatorForModifyCallsite);

    const StructResourceManagementRuleConfig &srm = config.structResourceManagementRule;
    rule("struct_resource_management");
    level(srm.level);
    text("pod_struct_creator_suffix", srm.podStructCreatorSuffix);
    text("raii_struct_creator_suffix", srm.raiiStructCreatorSuffix);
    text("raii_struct_destroyer_suffix", srm.raiiStructDestroyerSuffix);
    text("raii_struct_copy_suffix", srm.raiiStructCopySuffix);
    text("raii_struct_move_suffix", srm.raiiStructMoveSuffix);
    text("raii_struct_return_suffix", srm.raiiStructReturnSuffix);
    text("raii_struct_valid_suffix", srm.raiiStructValidSuffix);
    text("free_struct_creator_suffix", srm.freeStructCreatorSuffix);
    boolean("allow_raii_struct_arrays", srm.allowRaiiStructArrays);
    text("raii_struct_array_destroyer_suffix", srm.raiiStructArrayDestroyerSuffix);
    boolean("raii_use_after_destroy", srm.raiiUseAfterDestroy);
    boolean("raii_may_only_move_value_ref", srm.raiiMayOnlyMoveValueRef);
    boolean("raii_may_only_destroy_value_ref", srm.raiiMayOnlyDestroyValueRef);
    boolean("raii_destroy_in_reverse_order", srm.raiiDestroyInReverseOrder);
    boolean("raii_standardized_destroy_definitions", srm.raiiStandardizedDestroyDefinitions);

    const InterfacesRuleConfig &interfaces = config.interfacesRule;
    rule("interfaces");
    level(interfaces.level);
    text("vtable_suffix", interfaces.vtableSuffix);
    text("interface_suffix", interfaces.interfaceSuffix);
    text("const_interface_suffix", interfaces.constInterfaceSuffix);
    boolean("interface_must_have_fields_that_are_private_alternative", interfaces.interfaceMustHaveFieldsThatArePrivateAlternative);

    const RestrictedMallocRuleConfig &restrictedMalloc = config.restrictedMallocRule;
    rule("restricted_malloc");
    level(restrictedMalloc.level);

    if (restrictedMalloc.listOfAllowedMallocFunctions.empty()) {
        out << "    list_of_allowed_malloc_functions: []\n";
    }
    else {
        out << "    list_of_allowed_malloc_functions:\n";

        for (const std::string &function : restrictedMalloc.listOfAllowedMallocFunctions)
            out << "      - " << function << "\n";
    }

    const SingleReturnRuleConfig &singleReturn = config.singleReturnRule;
    rule("single_return");
    level(singleReturn.level);
    boolean("allow_early_return", singleReturn.allowEarlyReturn);
    boolean("require_return_for_void", singleReturn.requireReturnForVoid);

    rule("strict_switch");
    level(config.strictSwitchRule.level);

    const GlobalVariableRuleConfig &global = config.globalVariableRule;
    rule("global_variable");
    level(global.level);
    boolean("require_prefix", global.requirePrefix);
    text("prefix", global.prefix);
    boolean("must_be_caps", global.mustBeCaps);
    boolean("must_be_static", global.mustBeStatic);
    boolean("must_be_const", global.mustBeConst);
    boolean("treat_local_static_as_global", global.treatLocalStaticAsGlobal);
    boolean("require_local_static_prefix", global.requireLocalStaticPrefix);
    text("local_static_prefix", global.localStaticPrefix);
    boolean("forbid_static_in_header", global.forbidStaticInHeader);

    rule("reference_pointer");
    level(config.referencePointerRule.level);
    boolean("disable_null_check_rule_for_reference_pointers", config.referencePointerRule.disableNullCheckRuleForReferencePointers);

    rule("function_discard");
    level(config.functionDiscardRule.level);

    const ArrayStructRuleConfig &array = config.arrayStructRule;
    rule("array_struct");
    level(array.level);
    boolean("only_allow_array_passing_to_library_functions", array.onlyAllowArrayPassingToLibraryFunctions);
    boolean("enforce_size_suffix_for_array_structs", array.enforceSizeSuffixForArrayStructs);
    boolean("size_suffix_with_underscore", array.sizeSuffixWithUnderscore);
    text("flexible_size_name", array.flexibleSizeName);
    boolean("enforce_suffix_for_array_structs", array.enforceSuffixForArrayStructs);
    text("array_struct_suffix", array.arrayStructSuffix);
    boolean("struct_name_as_prefix", array.structNameAsPrefix);
    boolean("use_naming_rules_on_one_array_field_structs_without_forbidding_public_arrays", array.useNamingRulesOnOneArrayFieldStructsWithoutForbiddingPublicArrays);

    const SpanStructRuleConfig &span = config.spanStructRule;
    rule("span_struct");
    level(span.level);
    text("span_struct_suffix", span.spanStructSuffix);
    text("const_span_struct_suffix", span.constSpanStructSuffix);
    boolean("only_allow_array_passing_to_library_functions_and_spans", span.onlyAllowArrayPassingToLibraryFunctionsAndSpans);
    boolean("only_allow_span_data_passing_in_one_line_static_functions", span.onlyAllowSpanDataPassingInOneLineStaticFunctions);
    boolean("allow_spans_to_be_given_fewer_elements_than_their_size", span.allowSpansToBeGivenFewerElementsThanTheirSize);
    boolean("require_span_immediately_after_array", span.requireSpanImmediatelyAfterArray);
    boolean("allow_pod_span_to_be_initialized_manually_if_static", span.allowPodSpanToBeInitializedManuallyIfStatic);

    rule("const_field");
    level(config.constFieldRule.level);

    rule("no_goto");
    level(config.noGotoRule.level);
}
