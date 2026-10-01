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
 * Naming conventions are selected separately for mutable and deeply
 * const globals, and for mutable and deeply const function-local
 * statics. File-scope storage and const constraints apply only to
 * globals. A declaration is checked once, at its definition or first
 * declaration when no definition is available in the analyzed file.
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
