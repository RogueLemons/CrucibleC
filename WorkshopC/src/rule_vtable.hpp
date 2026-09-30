#pragma once

#include <clang/AST/ASTContext.h>
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

/*
 * The vtable part of the interfaces rule. A vtable struct, named with
 * vtable_suffix, holds only function pointers that take a void* or
 * const void* first, the object the function works on.
 *
 * A vtable variable is static const and initialized at declaration with
 * braces, with a function for every function pointer, never NULL or 0.
 */
class VtableRule : public MatchFinder::MatchCallback {
private:
    const Config &config;
    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool isThirdParty(const std::string &path) const;
    bool shouldIgnore(const SourceManager &sm, SourceLocation loc) const;

    void report(
        DiagCode code,
        SourceLocation loc,
        const std::string &message,
        const SourceManager &sm) const;

    // The vtable struct of 'type', or null
    const RecordDecl *getVtable(QualType type) const;

    // Every field is a function pointer taking a void* or const void* first
    void checkVtableStruct(
        const RecordDecl *record,
        const SourceManager &sm) const;

    // Static const, and fully initialized with functions
    void checkVtableVariable(
        const VarDecl *variable,
        const RecordDecl *vtable,
        const SourceManager &sm) const;

    void checkInitializer(
        const VarDecl *variable,
        const RecordDecl *vtable,
        const SourceManager &sm) const;

public:
    VtableRule(const Config &cfg,
               SuppressionManager &sup,
               Diagnostics &diag);

    void bindFinder(MatchFinder &finder);
    void run(const MatchFinder::MatchResult &result) override;
};
