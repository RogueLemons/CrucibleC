#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class ArrayStructRule : public MatchFinder::MatchCallback {
private:
    const Config &config;
    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool isThirdParty(const std::string &path) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    bool isLibraryFunction(
        const FunctionDecl *function,
        const SourceManager &sm) const;

    void report(
        DiagCode code,
        SourceLocation loc,
        const std::string &message,
        const SourceManager &sm) const;

public:
    ArrayStructRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
