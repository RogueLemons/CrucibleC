#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/TypeLoc.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * A function, or another function pointer, that is assigned or passed to
 * a function pointer must have the same movement tags (argument pointer
 * movement rule) and reference tags (reference pointer rule) as the
 * function pointer type, parameter by parameter. Calls through the
 * function pointer are then checked against the tags of its type.
 *
 * Checked wherever a function pointer gets its value: initializations
 * (also in arrays and struct fields), assignments, arguments and return
 * statements.
 */
class FunctionPointerTagRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    const SourceManager *sourceManager = nullptr;

    // What is assigned or passed: a function or a function pointer
    struct BoundFunction {
        bool known = false;
        std::string description;
        std::vector<const ParmVarDecl *> parameters;
    };

private:
    bool isThirdParty(const std::string &path) const;

    bool shouldIgnore(SourceLocation loc) const;

    BoundFunction boundFunctionOf(const Expr *source) const;

    /*
     * Checks 'source' against the function pointer type written at
     * 'target', going into initializer lists of arrays and structs.
     */
    void checkInitializer(TypeLoc target, const Expr *source);

    void checkBinding(
        FunctionProtoTypeLoc target,
        const std::string &targetName,
        const Expr *source);

    /*
     * A call through a conditional expression, (flag ? first : second)(),
     * can only follow one set of tags, so all branches must agree.
     */
    void checkConditionalCallee(const CallExpr *call);

public:
    FunctionPointerTagRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
