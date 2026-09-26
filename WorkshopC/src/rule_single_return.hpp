#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Requires every function to have a single return statement, which
 * must be the last statement of the function body.
 *
 * allow_early_return: an 'if' without 'else' placed directly in the
 * function's top-level block may return from its branch (a guard
 * clause), in addition to the final return. A top-level statement
 * coming from a macro, e.g. RETURN_IF_NULL(p), counts as such too.
 *
 * require_return_for_void: void functions must end with 'return;'
 * as well, otherwise the final return is optional for them.
 */
class SingleReturnRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    bool isThirdParty(const std::string &path) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    void collectReturns(
        const Stmt *stmt,
        std::vector<const ReturnStmt *> &returns) const;

    /*
     * The return statements of a top-level statement that are
     * allowed as early returns.
     */
    void collectEarlyReturns(
        const Stmt *topLevel,
        std::unordered_set<const ReturnStmt *> &allowed) const;

public:
    SingleReturnRule(const Config &cfg,
                     SuppressionManager &sup,
                     Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
