#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Naming and qualifier conventions for global (file scope) variables:
 * a required prefix, capital letters, static and const, and optionally
 * no static globals in headers. A variable that is declared more than
 * once is only checked once, at its definition, or at its first
 * declaration if it is not defined in the file being analyzed.
 *
 * With treat_local_static_as_global, static variables inside functions
 * follow the prefix, capital letter and const conventions too, using
 * local_static_prefix when require_local_static_prefix is set and the
 * global prefix settings otherwise.
 */
class GlobalVariableRule : public MatchFinder::MatchCallback {
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
     * True if the type is const, and for pointers also every level
     * pointed to, e.g. 'const int* const'.
     */
    bool isDeepConst(QualType type, const ASTContext &context) const;

    void report(
        DiagCode code,
        const SourceManager &sm,
        const VarDecl *var,
        const std::string &kind,
        const std::string &message);

public:
    GlobalVariableRule(const Config &cfg,
                       SuppressionManager &sup,
                       Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
