#include "rule_array_struct.hpp"

bool ArrayStructRule::isThirdParty(const std::string &path) const
{
    for (const auto &include : config.thirdPartyIncludes) {
        if (!include.empty() && path.find(include) != std::string::npos)
            return true;
    }

    return false;
}

bool ArrayStructRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    const SourceLocation spelling = sm.getSpellingLoc(loc);
    const SourceLocation expansion = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansion))
        return true;

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string spellingPath = sm.getFilename(spelling).str();
    const std::string expansionPath = sm.getFilename(expansion).str();

    return spellingPath.empty() ||
           isThirdParty(spellingPath) ||
           (!expansionPath.empty() && isThirdParty(expansionPath));
}

bool ArrayStructRule::isLibraryFunction(
    const FunctionDecl *function,
    const SourceManager &sm) const
{
    if (!function)
        return false;

    const SourceLocation location = function->getLocation();
    const SourceLocation spelling = sm.getSpellingLoc(location);

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string path = sm.getFilename(spelling).str();
    return !path.empty() && isThirdParty(path);
}

void ArrayStructRule::report(
    DiagCode code,
    SourceLocation loc,
    const std::string &message,
    const SourceManager &sm) const
{
    if (shouldIgnore(sm, loc))
        return;

    diagnostics.report(
        config.arrayStructRule.level,
        code,
        sm,
        sm.getExpansionLoc(loc),
        message);
}

bool ArrayStructRule::isLibraryDeclaration(
    const Decl *declaration,
    const SourceManager &sm) const
{
    if (!declaration)
        return false;

    const SourceLocation spelling = sm.getSpellingLoc(declaration->getLocation());

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string path = sm.getFilename(spelling).str();
    return !path.empty() && isThirdParty(path);
}

bool ArrayStructRule::getSizeSuffix(
    QualType type,
    const ASTContext &context,
    std::string &suffix) const
{
    const ArrayStructRuleConfig &rule = config.arrayStructRule;
    const std::string separator = rule.sizeSuffixWithUnderscore ? "_" : "";

    suffix.clear();

    while (const ArrayType *array = context.getAsArrayType(type)) {
        if (const auto *constant = dyn_cast<ConstantArrayType>(array))
            suffix += separator + std::to_string(constant->getZExtSize());
        else if (isa<IncompleteArrayType>(array))
            suffix += separator + rule.flexibleSizeName;
        else
            return false;

        type = array->getElementType();
    }

    return true;
}

void ArrayStructRule::checkArrayStructName(
    const RecordDecl *record,
    const SourceManager &sm,
    const ASTContext &context) const
{
    const ArrayStructRuleConfig &rule = config.arrayStructRule;

    if (!rule.enforceSizeSuffixForArrayStructs &&
        !rule.enforceSuffixForArrayStructs &&
        !rule.structNameAsPrefix)
    {
        return;
    }

    // Only structs whose single field is an array
    const FieldDecl *field = nullptr;

    for (const FieldDecl *candidate : record->fields()) {
        if (field)
            return;

        field = candidate;
    }

    if (!field || !context.getAsArrayType(field->getType()))
        return;

    // An anonymous struct is named by its typedef
    std::string name = record->getNameAsString();

    if (name.empty()) {
        if (const TypedefNameDecl *typedefName = record->getTypedefNameForAnonDecl())
            name = typedefName->getNameAsString();
    }

    if (name.empty())
        return;

    const std::string description =
        "struct '" + name + "' holds only the array '" +
        field->getNameAsString() + "'";

    std::string ending;

    if (rule.enforceSuffixForArrayStructs)
        ending += rule.arrayStructSuffix;

    if (rule.enforceSizeSuffixForArrayStructs) {
        std::string sizeSuffix;

        if (getSizeSuffix(field->getType(), context, sizeSuffix))
            ending += sizeSuffix;
    }

    if (!ending.empty() &&
        (name.size() < ending.size() ||
         name.compare(name.size() - ending.size(), ending.size(), ending) != 0))
    {
        report(
            DiagCode::ArrayStructNameEnding,
            record->getLocation(),
            description + " and must end with '" + ending + "'",
            sm);
    }

    if (!rule.structNameAsPrefix)
        return;

    // The element struct, below all dimensions of a matrix
    QualType element = field->getType();

    while (const ArrayType *array = context.getAsArrayType(element))
        element = array->getElementType();

    const RecordDecl *elementRecord = element->getAsRecordDecl();

    if (!elementRecord || isLibraryDeclaration(elementRecord, sm))
        return;

    std::string prefix = elementRecord->getNameAsString();

    if (prefix.empty()) {
        if (const TypedefNameDecl *typedefName = elementRecord->getTypedefNameForAnonDecl())
            prefix = typedefName->getNameAsString();
    }

    if (prefix.empty() || name.compare(0, prefix.size(), prefix) == 0)
        return;

    report(
        DiagCode::ArrayStructNamePrefix,
        record->getLocation(),
        description + " of '" + prefix + "' and must start with '" +
            prefix + "'",
        sm);
}

ArrayStructRule::ArrayStructRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void ArrayStructRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        varDecl(
            hasType(arrayType()),
            unless(isExpansionInSystemHeader())
        ).bind("arrayDeclaration"),
        this);

    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("arrayCall"),
        this);

    finder.addMatcher(
        recordDecl(
            isStruct(),
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("arrayStruct"),
        this);
}

void ArrayStructRule::run(const MatchFinder::MatchResult &result)
{
    if (config.arrayStructRule.level == RuleLevel::Off ||
        !result.SourceManager)
    {
        return;
    }

    const SourceManager &sm = *result.SourceManager;

    if (const auto *record = result.Nodes.getNodeAs<RecordDecl>("arrayStruct")) {
        if (result.Context)
            checkArrayStructName(record, sm, *result.Context);

        return;
    }

    if (const auto *array =
            result.Nodes.getNodeAs<VarDecl>("arrayDeclaration"))
    {
        const SourceLocation loc = array->getLocation();

        report(
            DiagCode::ArrayOutsideStruct,
            loc,
            "array variable '" + array->getNameAsString() +
                "' may only be declared as a field inside a struct",
            sm);
    }

    if (!config.arrayStructRule.onlyAllowArrayPassingToLibraryFunctions)
        return;

    const auto *call =
        result.Nodes.getNodeAs<CallExpr>("arrayCall");

    if (!call)
        return;

    const FunctionDecl *callee = call->getDirectCallee();

    if (isLibraryFunction(callee, sm))
        return;

    for (const Expr *argument : call->arguments()) {
        if (!argument)
            continue;

        const Expr *stripped = argument->IgnoreParenImpCasts();
        const auto *member = dyn_cast<MemberExpr>(stripped);

        if (!member || !member->getType()->isArrayType())
            continue;

        report(
            DiagCode::ArrayPassedToNonLibraryFunction,
            argument->getExprLoc(),
            "array field '" +
                member->getMemberDecl()->getNameAsString() +
                "' may only be passed directly to a standard-library or third-party function",
            sm);
    }
}
