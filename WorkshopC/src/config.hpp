#pragma once

#include <string>
#include <vector>

enum class RuleLevel {
    Off,
    Warning,
    Error
};

struct SuppressionReasonRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct EnumRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool allowEnumTypedef = false;
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

    bool raiiUseAfterDestroy = true;
    bool raiiMayOnlyMoveValueRef = false;
};

struct RestrictedMallocRuleConfig {
    RuleLevel level = RuleLevel::Off;

    std::vector<std::string> listOfAllowedMallocFunctions;
};

struct SingleReturnRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool allowEarlyReturn = true;
    bool requireReturnForVoid = true;
};

struct StrictSwitchRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct GlobalVariableRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool requirePrefix = false;
    std::string prefix = "g_";
    bool mustBeCaps = false;
    bool mustBeStatic = false;
    bool mustBeConst = false;

    bool treatLocalStaticAsGlobal = false;
    bool requireLocalStaticPrefix = false;
    std::string localStaticPrefix = "s_";
    bool forbidStaticInHeader = false;
};

struct ReferencePointerRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool disableNullCheckRuleForReferencePointers = true;
};

struct Config {
    SuppressionReasonRuleConfig suppressionReasonRule;
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
    RestrictedMallocRuleConfig restrictedMallocRule;
    SingleReturnRuleConfig singleReturnRule;
    StrictSwitchRuleConfig strictSwitchRule;
    GlobalVariableRuleConfig globalVariableRule;
    ReferencePointerRuleConfig referencePointerRule;

    std::vector<std::string> thirdPartyIncludes;
};
