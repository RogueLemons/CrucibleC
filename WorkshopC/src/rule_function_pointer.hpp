#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/AST/TypeLoc.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <set>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class FunctionPointerRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    // Stable dedup key
    mutable std::set<std::string> reported;

private:
    bool isThirdParty(const std::string &path) const;

    // ------------------------------------------------------------
    // Detect: pointer to function type
    // ------------------------------------------------------------
    bool isFunctionPointer(QualType qt) const;

    // ------------------------------------------------------------
    // THIS is the correct fix:
    // Detect whether type was written as typedef or raw syntax
    // ------------------------------------------------------------
    bool isTypedefSpelled(const Decl *decl) const;

    // ------------------------------------------------------------
    // Stable dedup key
    // ------------------------------------------------------------
    std::string makeKey(const Decl *D, const SourceManager &sm) const;

public:
    FunctionPointerRule(const Config &cfg,
                        SuppressionManager &sup,
                        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
