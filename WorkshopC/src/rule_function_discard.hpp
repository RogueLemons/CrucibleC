#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Requires callers to use the result of every non-void function call.
 * An explicit (void) cast is the documented escape hatch for an
 * intentionally discarded result.
 */
class FunctionDiscardRule : public MatchFinder::MatchCallback {
private:
    const Config &config;
    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool isThirdParty(const std::string &path) const;
    bool shouldIgnore(const SourceManager &sm, SourceLocation loc) const;
    bool isDiscarded(const CallExpr *call, ASTContext &context) const;
    std::string getCalleeName(const CallExpr *call) const;

public:
    FunctionDiscardRule(const Config &cfg,
                        SuppressionManager &sup,
                        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);
    void run(const MatchFinder::MatchResult &result) override;
};
