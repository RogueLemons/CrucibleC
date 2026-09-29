#pragma once

#include <clang/AST/Stmt.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Forbids goto statements, both 'goto label;' and the computed
 * 'goto *address;' of GNU C.
 */
class NoGotoRule : public MatchFinder::MatchCallback {
private:
    const Config &config;
    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool isThirdParty(const std::string &path) const;
    bool shouldIgnore(const SourceManager &sm, SourceLocation loc) const;

public:
    NoGotoRule(const Config &cfg,
               SuppressionManager &sup,
               Diagnostics &diag);

    void bindFinder(MatchFinder &finder);
    void run(const MatchFinder::MatchResult &result) override;
};
