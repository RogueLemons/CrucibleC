#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Requires every switch statement to have a default case, and every
 * case (including default) to end in a way that can not fall through
 * into the next one: break, return, continue, goto, or a call to a
 * function that never returns (e.g. abort). Stacked labels such as
 * 'case 1: case 2:' share one body and are not fallthrough.
 */
class StrictSwitchRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    bool isThirdParty(const std::string &path) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    /*
     * True if execution can never continue past 'stmt'.
     */
    bool endsCase(const Stmt *stmt) const;

    void checkSection(
        const SwitchCase *label,
        const Stmt *lastStatement,
        const SourceManager &sm);

public:
    StrictSwitchRule(const Config &cfg,
                     SuppressionManager &sup,
                     Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
