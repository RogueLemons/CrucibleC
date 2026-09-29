#include "rule_no_goto.hpp"

namespace {

// Clang has no matcher for a computed goto, 'goto *address;'
const internal::VariadicDynCastAllOfMatcher<Stmt, IndirectGotoStmt> indirectGotoStmt;

} // namespace

bool NoGotoRule::isThirdParty(const std::string &path) const
{
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool NoGotoRule::shouldIgnore(
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

NoGotoRule::NoGotoRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void NoGotoRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        gotoStmt(
            unless(isExpansionInSystemHeader())
        ).bind("goto"),
        this);

    finder.addMatcher(
        indirectGotoStmt(
            unless(isExpansionInSystemHeader())
        ).bind("computedGoto"),
        this);
}

void NoGotoRule::run(const MatchFinder::MatchResult &result)
{
    if (config.noGotoRule.level == RuleLevel::Off || !result.SourceManager)
        return;

    const SourceManager &sm = *result.SourceManager;

    SourceLocation loc;
    std::string message;

    if (const auto *jump = result.Nodes.getNodeAs<GotoStmt>("goto")) {
        loc = jump->getGotoLoc();
        message = "goto '" + jump->getLabel()->getNameAsString() + "' is not allowed";
    }
    else if (const auto *jump = result.Nodes.getNodeAs<IndirectGotoStmt>("computedGoto")) {
        loc = jump->getGotoLoc();
        message = "computed goto is not allowed";
    }
    else {
        return;
    }

    if (shouldIgnore(sm, loc))
        return;

    diagnostics.report(
        config.noGotoRule.level,
        DiagCode::GotoNotAllowed,
        sm,
        sm.getExpansionLoc(loc),
        message);
}
