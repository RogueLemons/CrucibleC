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
    EnumRuleConfig &cfg,
    const std::string &key,
    const std::string &value
) {
    if (key == "level") {
        cfg.level = parseLevel(value);
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
}

void ConfigParser::applyRuleSetting(
    Config &config,
    const std::string &currentRule,
    const std::string &key,
    const std::string &value
) {
    if (currentRule == "enum") {
        applySetting(config.enumRule, key, value);
    }
    else if (currentRule == "private") {
        applySetting(config.privateRule, key, value);
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

    bool inProjectIncludes = false;
    bool inThirdPartyIncludes = false;

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        // sections
        if (line == "paths:") {
            currentSection = "paths";
            continue;
        }

        if (line == "rules:") {
            currentSection = "rules";
            continue;
        }

        // include lists
        if (line == "project_includes:") {
            inProjectIncludes = true;
            inThirdPartyIncludes = false;
            continue;
        }

        if (line == "third_party_includes:") {
            inProjectIncludes = false;
            inThirdPartyIncludes = true;
            continue;
        }

        // list item
        if (line.starts_with("-")) {
            std::string value = trim(line.substr(1));

            if (inProjectIncludes) {
                config.projectIncludes.push_back(value);
            }
            else if (inThirdPartyIncludes) {
                config.thirdPartyIncludes.push_back(value);
            }

            continue;
        }

        // rule
        if (currentSection == "rules" && line.back() == ':') {
            currentRule = trim(
                line.substr(0, line.size() - 1)
            );

            continue;
        }

        // rule settings
        size_t colon = line.find(':');

        if (colon != std::string::npos && !currentRule.empty()) {
            std::string key =
                trim(line.substr(0, colon));

            std::string value =
                trim(line.substr(colon + 1));

            applyRuleSetting(config, currentRule, key, value);
        }
    }

    return true;
}
