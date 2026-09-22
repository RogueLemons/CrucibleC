#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class AssignmentRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    const Expr* norm(const Expr *e) const;

    bool isThirdParty(const std::string &path) const;

    bool shouldSkip(SourceManager &sm, SourceLocation loc) const;

    std::string nameOf(const Decl *d) const;

    bool isNullExpr(const Expr *e) const;

    bool containsPointer(QualType qt) const;

    bool isLiteralZeroInit(const Expr *e, SourceManager &sm) const;

public:
    AssignmentRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag
    );

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
