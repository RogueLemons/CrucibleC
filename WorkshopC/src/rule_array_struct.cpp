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
}

void ArrayStructRule::run(const MatchFinder::MatchResult &result)
{
    if (config.arrayStructRule.level == RuleLevel::Off ||
        !result.SourceManager)
    {
        return;
    }

    const SourceManager &sm = *result.SourceManager;

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
