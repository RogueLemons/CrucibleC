#pragma once

#include <string>
#include <vector>

enum class RuleLevel {
    Off,
    Warning,
    Error
};

struct EnumRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct PrivateRuleConfig {
    RuleLevel level = RuleLevel::Off;

    std::string privateField = "_private";
    std::string getterContains = "pget";
    std::string setterContains = "pset";
};

struct PrivateAlternativeRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct FunctionPointerRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct TypedefStructRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct AssignmentRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool forbidZeroInitForObjectsWithPointers = false;
    bool forbidNullAssign = false;
    bool forbidMutArgPointer = false;
    bool forbidArgReassign = false;
    bool forbidNullAsArg = false;
};

struct PrefixNamespaceRuleConfig {
    RuleLevel level = RuleLevel::Off;

    std::string topDir = "src";
    bool workFromTop = false;
    int stopAtCount = 10;
    bool useSeparator = false;
    std::string separator = "_";
    bool requireIfndefForFilepath = false;
    bool applyToFunctions = false;
    bool applyToStructs = false;
    bool applyToTypedefs = false;
};

struct NullCheckRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool allowDirectPtrInIfStatement = false;
};

struct ArgumentPointerMovementRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool requireOperatorForMoveCallsite = false;
    bool requireOperatorForOutCallsite = false;
    bool requireOperatorForModifyCallsite = false;
};

struct StructResourceManagementRuleConfig {
    RuleLevel level = RuleLevel::Off;

    std::string podStructCreatorSuffix;
    std::string raiiStructCreatorSuffix;
    std::string raiiStructDestroyerSuffix;
    std::string raiiStructCopySuffix;
    std::string raiiStructMoveSuffix;
    std::string raiiStructReturnSuffix;
    std::string raiiStructValidSuffix;
    std::string freeStructCreatorSuffix;
};

struct Config {
    EnumRuleConfig enumRule;
    PrivateRuleConfig privateRule;
    PrivateAlternativeRuleConfig privateAlternativeRule;
    FunctionPointerRuleConfig functionPointerRule;
    TypedefStructRuleConfig typedefStructRule;
    AssignmentRuleConfig assignmentRule;
    PrefixNamespaceRuleConfig prefixNamespaceRule;
    NullCheckRuleConfig nullCheckRule;
    ArgumentPointerMovementRuleConfig argumentPointerMovementRule;
    StructResourceManagementRuleConfig structResourceManagementRule;

    std::vector<std::string> projectIncludes;
    std::vector<std::string> thirdPartyIncludes;
};
