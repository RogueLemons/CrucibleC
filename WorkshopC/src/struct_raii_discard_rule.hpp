#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "struct_database.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * The value returned by a function that returns a raii struct by value
 * owns resources, so it must end up somewhere that can later destroy it.
 *
 * Reported:
 *   - discarding the value, also with a (void) cast
 *   - accessing a member directly on the returned value (make().x),
 *     which leaves no way to destroy it
 *
 * Allowed, since ownership moves somewhere:
 *   - initializing a variable, assigning, returning, passing it as a
 *     function argument, and using it inside an initializer list
 */
class StructRaiiDiscardRule : public MatchFinder::MatchCallback {
private:
    enum class Use {
        Owned,
        Discarded,
        VoidCast,
        MemberAccess
    };

    struct PendingCall {
        const CallExpr *call = nullptr;
        Use use = Use::Owned;
        std::string memberName;
    };

    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    StructDatabase &database;

    const SourceManager *sourceManager = nullptr;

    std::vector<PendingCall> pendingCalls;

private:
    bool isThirdParty(const std::string &file) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    /*
     * Name of the struct returned by value, or empty if the call does
     * not return a struct by value.
     */
    std::string getReturnedStructName(const CallExpr *call) const;

    std::string getCalleeName(const CallExpr *call) const;

    /*
     * Walks up from the call through parentheses, casts and other
     * transparent expressions to find out what happens to its value.
     */
    Use classifyUse(
        const CallExpr *call,
        ASTContext &context,
        std::string *memberName) const;

public:
    StructRaiiDiscardRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;

    void finalize();
};
