#include "rule_strict_switch.hpp"

#include <clang/AST/Expr.h>

bool StrictSwitchRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool StrictSwitchRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    const SourceLocation expansion = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansion))
        return true;

    if (sm.isInSystemHeader(expansion))
        return true;

    const std::string path = sm.getFilename(expansion).str();

    return !path.empty() && isThirdParty(path);
}

bool StrictSwitchRule::endsCase(const Stmt *stmt) const {
    if (!stmt)
        return false;

    if (isa<BreakStmt>(stmt) ||
        isa<ReturnStmt>(stmt) ||
        isa<ContinueStmt>(stmt) ||
        isa<GotoStmt>(stmt) ||
        isa<IndirectGotoStmt>(stmt))
        return true;

    // { ...; break; }
    if (const auto *block = dyn_cast<CompoundStmt>(stmt))
        return !block->body_empty() && endsCase(block->body_back());

    // if (x) { ...; break; } else { ...; return; }
    if (const auto *ifStmt = dyn_cast<IfStmt>(stmt))
        return ifStmt->getElse() &&
               endsCase(ifStmt->getThen()) &&
               endsCase(ifStmt->getElse());

    if (const auto *label = dyn_cast<LabelStmt>(stmt))
        return endsCase(label->getSubStmt());

    // A call to a function that never returns, e.g. abort() or exit()
    if (const auto *expr = dyn_cast<Expr>(stmt)) {
        if (const auto *call =
                dyn_cast<CallExpr>(expr->IgnoreParenImpCasts()))
        {
            const FunctionDecl *callee = call->getDirectCallee();

            return callee && callee->isNoReturn();
        }
    }

    return false;
}

void StrictSwitchRule::checkSection(
    const SwitchCase *label,
    const Stmt *lastStatement,
    const SourceManager &sm)
{
    if (endsCase(lastStatement))
        return;

    const SourceLocation loc = label->getKeywordLoc();

    if (shouldIgnore(sm, loc))
        return;

    const std::string kind =
        isa<DefaultStmt>(label) ? "default case" : "case";

    diagnostics.report(
        config.strictSwitchRule.level,
        sm,
        sm.getExpansionLoc(loc),
        "switch " + kind + " must end with a break or return "
        "(fallthrough is not allowed)"
    );
}

StrictSwitchRule::StrictSwitchRule(const Config &cfg,
                                   SuppressionManager &sup,
                                   Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void StrictSwitchRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        switchStmt(
            unless(isExpansionInSystemHeader())
        ).bind("switch"),
        this
    );
}

void StrictSwitchRule::run(const MatchFinder::MatchResult &result) {
    if (config.strictSwitchRule.level == RuleLevel::Off)
        return;

    const auto *switchStmt =
        result.Nodes.getNodeAs<SwitchStmt>("switch");

    if (!switchStmt)
        return;

    const SourceManager &sm = *result.SourceManager;

    // -------------------------
    // A default case is required
    // -------------------------
    bool hasDefault = false;

    for (const SwitchCase *sc = switchStmt->getSwitchCaseList();
         sc;
         sc = sc->getNextSwitchCase())
    {
        if (isa<DefaultStmt>(sc))
            hasDefault = true;
    }

    if (!hasDefault && !shouldIgnore(sm, switchStmt->getSwitchLoc())) {
        diagnostics.report(
            config.strictSwitchRule.level,
            sm,
            sm.getExpansionLoc(switchStmt->getSwitchLoc()),
            "switch statement must have a default case"
        );
    }

    // -------------------------
    // Every case must end without falling through
    //
    // A section starts at a label placed directly in the switch
    // body and runs until the next such label. Stacked labels
    // ('case 1: case 2:') are nested in the AST and share the
    // statements of the innermost one.
    // -------------------------
    const Stmt *body = switchStmt->getBody();

    const auto *block = dyn_cast_or_null<CompoundStmt>(body);

    const SwitchCase *currentLabel = nullptr;
    const Stmt *lastStatement = nullptr;

    auto startSection = [&](const SwitchCase *label) {
        if (currentLabel)
            checkSection(currentLabel, lastStatement, sm);

        currentLabel = label;
        lastStatement = label->getSubStmt();

        while (const auto *inner = dyn_cast_or_null<SwitchCase>(lastStatement))
            lastStatement = inner->getSubStmt();
    };

    if (!block) {
        if (const auto *label = dyn_cast_or_null<SwitchCase>(body))
            startSection(label);
    }
    else {
        for (const Stmt *child : block->body()) {
            if (const auto *label = dyn_cast<SwitchCase>(child))
                startSection(label);
            else if (currentLabel)
                lastStatement = child;
        }
    }

    if (currentLabel)
        checkSection(currentLabel, lastStatement, sm);
}
