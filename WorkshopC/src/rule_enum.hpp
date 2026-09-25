#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_set>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class EnumRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool typedefsCollected = false;

    /*
     * Canonical declarations of every enum that has a typedef.
     */
    std::unordered_set<const EnumDecl *> enumsWithTypedef;

private:
    bool isThirdParty(const std::string &path) const;

    /*
     * True if the location is in a system header or a third party
     * folder, either where it is spelled or where it is expanded.
     */
    bool isIgnoredLocation(
        const SourceManager &sm,
        SourceLocation loc) const;

    void collectTypedefs(ASTContext &ctx);

    /*
     * The canonical enum declaration behind a type, or null if the
     * type is not an enum type.
     */
    const EnumDecl *getEnumDecl(QualType type) const;

    /*
     * An enum is governed by the typedef rules if it has a typedef
     * and is not defined in a system header or third party folder.
     */
    bool isGovernedEnum(
        const EnumDecl *enumDecl,
        const SourceManager &sm) const;

    /*
     * True if 'expr' is a member of 'target', a value that already
     * has the type of 'target' (which includes explicit casts), or
     * a conditional expression made only from such values.
     */
    bool isEnumMember(
        const Expr *expr,
        const EnumDecl *target) const;

    std::string describe(const Expr *expr) const;

    void checkEnumDeclaration(
        const EnumDecl *enumDecl,
        const MatchFinder::MatchResult &result);

    void checkVariableInit(
        const VarDecl *var,
        const MatchFinder::MatchResult &result);

    void checkAssignment(
        const BinaryOperator *op,
        const MatchFinder::MatchResult &result);

    void checkIncrementDecrement(
        const UnaryOperator *op,
        const MatchFinder::MatchResult &result);

    void checkCall(
        const CallExpr *call,
        const MatchFinder::MatchResult &result);

    void reportAt(
        const SourceManager &sm,
        SourceLocation loc,
        const std::string &message);

public:
    EnumRule(const Config &cfg,
             SuppressionManager &sup,
             Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
