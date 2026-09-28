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

/*
 * Every pointer parameter must be checked for null before it is
 * dereferenced. The check follows the control flow of the function: a
 * dereference is only safe where the parameter is known to be non-null on
 * every path that reaches it, e.g. in the branch of 'if (p)', or after
 * 'if (!p) return;'.
 */
class NullCheckRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    struct ParamState {
        const Expr *violation = nullptr;
    };

    // Follows the control flow of one function body
    class FlowAnalyzer;

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
