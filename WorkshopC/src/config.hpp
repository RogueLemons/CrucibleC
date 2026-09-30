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

    // Names only need the same letters as the prefix, in any case
    bool caseInsensitive = false;
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
    std::string raiiStructArrayDestroyerSuffix;

    bool raiiUseAfterDestroy = true;
    bool raiiMayOnlyMoveValueRef = false;
    bool raiiMayOnlyDestroyValueRef = false;
    bool raiiDestroyInReverseOrder = false;
    bool raiiStandardizedDestroyDefinitions = false;
};

struct InterfacesRuleConfig {
    RuleLevel level = RuleLevel::Off;

    // A vtable struct holds only function pointers taking a void* or
    // const void* first, and its variables are static const, fully
    // initialized with functions
    std::string vtableSuffix;

    // Not checked yet: the interface part of the rule
    std::string interfaceSuffix;
    std::string constInterfaceSuffix;
    bool interfaceMustHaveFieldsThatArePrivateAlternative = false;

    bool isVtableName(const std::string &name) const
    {
        return !vtableSuffix.empty() &&
               name.size() >= vtableSuffix.size() &&
               name.compare(name.size() - vtableSuffix.size(), vtableSuffix.size(), vtableSuffix) == 0;
    }
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

struct FunctionDiscardRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct ArrayStructRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool onlyAllowArrayPassingToLibraryFunctions = false;

    // The naming options below only apply to structs whose single field
    // is an array.

    // The name ends with the element count of each dimension, e.g.
    // '_5_10' for 'int values[5][10]'
    bool enforceSizeSuffixForArrayStructs = false;
    bool sizeSuffixWithUnderscore = true;

    // Used instead of a count for a flexible array, e.g. 'int values[]'
    std::string flexibleSizeName = "flexible";

    // The name ends with arrayStructSuffix, before any size suffix
    bool enforceSuffixForArrayStructs = false;
    std::string arrayStructSuffix = "_array";

    // The name starts with the name of the element struct, unless that
    // struct comes from the standard library or a third party
    bool structNameAsPrefix = false;
};

struct SpanStructRuleConfig {
    RuleLevel level = RuleLevel::Off;

    bool onlyAllowArrayPassingToLibraryFunctionsAndSpans = false;
    std::string spanStructSuffix = "_span";

    // A span of const data, e.g. 'const int* data'. Checked before
    // spanStructSuffix, which it usually ends with too.
    std::string constSpanStructSuffix = "_const_span";

    // A span may cover only the start of an array: its count may be less
    // than the array's element count, but never more
    bool allowSpansToBeGivenFewerElementsThanTheirSize = false;

    // Every array outside of a struct must be followed right away by a
    // span variable holding the whole array
    bool requireSpanImmediatelyAfterArray = false;

    // The data of a span may only be passed on inside a static function
    // with a single statement, a wrapper that takes the span
    bool onlyAllowSpanDataPassingInOneLineStaticFunctions = false;

    // With the struct resource management rule, a span with static storage
    // duration may be initialized with braces, e.g. '{ values, 4 }', since
    // a static initializer can not call the span's pod function
    bool allowPodSpanToBeInitializedManuallyIfStatic = false;

    bool isConstSpanStructName(const std::string &name) const
    {
        return endsWithSuffix(name, constSpanStructSuffix);
    }

    // A span or const span struct name
    bool isSpanStructName(const std::string &name) const
    {
        return isConstSpanStructName(name) ||
               endsWithSuffix(name, spanStructSuffix);
    }

private:
    static bool endsWithSuffix(const std::string &name, const std::string &suffix)
    {
        return !suffix.empty() &&
               name.size() >= suffix.size() &&
               name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
};

struct ConstFieldRuleConfig {
    RuleLevel level = RuleLevel::Off;
};

struct NoGotoRuleConfig {
    RuleLevel level = RuleLevel::Off;
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
    FunctionDiscardRuleConfig functionDiscardRule;
    ArrayStructRuleConfig arrayStructRule;
    SpanStructRuleConfig spanStructRule;
    ConstFieldRuleConfig constFieldRule;
    NoGotoRuleConfig noGotoRule;
    InterfacesRuleConfig interfacesRule;

    // List of paths to third-party include directories, relative to the config file
    std::vector<std::string> thirdPartyIncludes;

    // Folder containing compile_commands.json, relative to the config file
    std::string compileCommandsDir;
};
