#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_map>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class NullCheckRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    struct ParamState {
        bool seenGuard = false;
        const Expr *violation = nullptr;
    };

private:
    bool isThirdParty(const std::string &path) const;

    bool isPointerParam(const ParmVarDecl *p) const;

    bool isParamRef(const Expr *expr,
                    const ParmVarDecl *param) const;

    bool isNullLiteral(const Expr *expr) const;

    bool isNullComparison(const Expr *expr,
                          const ParmVarDecl *param) const;

    bool isMacroNullCheck(const Expr *expr) const;

    bool isAllowedBooleanUse(const Expr *expr,
                             const ParmVarDecl *param,
                             bool allowBool) const;

    bool isNullGuard(const Expr *expr,
                     const ParmVarDecl *param) const;

    bool isDerefOfParam(
        const Expr *expr,
        const ParmVarDecl *param
    ) const;

public:
    NullCheckRule(const Config &cfg,
                  SuppressionManager &sup,
                  Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
