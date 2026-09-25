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
 * Restricts every use of the memory allocating library functions
 * (malloc, calloc, realloc, free, strdup, strndup, asprintf, getline
 * and realpath) to functions whose name is in the configured list.
 * A use is any reference to one of them, so a call as well as passing
 * one as a function pointer.
 */
class RestrictedMallocRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    bool isThirdParty(const std::string &path) const;

    bool isAllowedFunction(const FunctionDecl *function) const;

    std::string allowedFunctionsText() const;

public:
    RestrictedMallocRule(const Config &cfg,
                         SuppressionManager &sup,
                         Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
