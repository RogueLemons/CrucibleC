#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
        const Expr *violation = nullptr;
    };

private:
    bool isThirdParty(const std::string &path) const;

    bool isPointerParam(const ParmVarDecl *p) const;

    bool isParamRef(const Expr *expr,
                    const ParmVarDecl *param) const;

    bool isNullLiteral(const Expr *expr) const;

    /*
     * 'param == NULL', 'NULL != param' and similar.
     */
    bool isNullComparison(const Expr *expr,
                          const ParmVarDecl *param) const;

    /*
     * A call to a function with "null" or "NULL" in its name that is
     * given 'param', e.g. 'is_null(param)'.
     */
    bool isNamedNullCheckCall(const Expr *expr,
                              const ParmVarDecl *param) const;

    /*
     * 'param' or '!param' (any number of '!'), which only counts as a
     * check when used as a condition and allowed by the config.
     */
    bool isBooleanCheck(const Expr *expr,
                        const ParmVarDecl *param) const;

    /*
     * True if 'cond', used as a condition, checks 'param' for null.
     */
    bool isConditionCheck(const Expr *cond,
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
