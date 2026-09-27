#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Reference pointers: a pointer parameter tagged with the reference tag
 * (e.g. 'void foo(REF int* arg)') always points to a real object, so it
 * can never be null.
 *
 *   - The argument given to a reference parameter must be the address of
 *     an object ('&variable', '&variable.field', '&array[i]', or a field
 *     reached through another reference), an array or string literal, or
 *     another reference parameter passed on.
 *   - A reference parameter may not be reassigned.
 *   - The reference tag may only be used on pointer parameters.
 */
class ReferencePointerRule : public MatchFinder::MatchCallback {
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
     * Strips parentheses, casts and the argument pointer operator
     * wrappers (e.g. mut(...)).
     */
    const Expr *stripWrappers(const Expr *expr) const;

    /*
     * True if 'expr' names a reference parameter.
     */
    bool isReference(const Expr *expr) const;

    /*
     * True if 'expr' is an object whose address can never be null.
     */
    bool isObject(const Expr *expr) const;

    bool isValidArgument(const Expr *arg) const;

    void report(
        const SourceManager &sm,
        SourceLocation loc,
        const std::string &message);

    void checkCall(const CallExpr *call, const SourceManager &sm);

    void checkReassignment(
        const Expr *target,
        SourceLocation loc,
        const SourceManager &sm);

    void checkFunction(const FunctionDecl *function, const SourceManager &sm);

public:
    ReferencePointerRule(const Config &cfg,
                         SuppressionManager &sup,
                         Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
