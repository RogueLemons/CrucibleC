#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Forbids struct fields that are const themselves, e.g. 'const int' or
 * 'int* const', and arrays of those. A pointer to const data, e.g.
 * 'const int*', is allowed: the field itself can still be assigned.
 */
class ConstFieldRule : public MatchFinder::MatchCallback {
private:
    const Config &config;
    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool isThirdParty(const std::string &path) const;
    bool shouldIgnore(const SourceManager &sm, SourceLocation loc) const;

public:
    ConstFieldRule(const Config &cfg,
                   SuppressionManager &sup,
                   Diagnostics &diag);

    void bindFinder(MatchFinder &finder);
    void run(const MatchFinder::MatchResult &result) override;
};
