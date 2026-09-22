#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_set>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class TypedefStructRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    std::unordered_set<const TagDecl*> seen;

private:
    bool isThirdParty(const std::string &file) const;

    bool isExternalMacroExpansion(const SourceManager &sm,
                                  const RecordDecl *RD) const;

    bool shouldIgnore(const SourceManager &sm, SourceLocation loc) const;

    void report(const RecordDecl *RD, const SourceManager &sm);

    bool hasTypedefFor(const TagDecl *TD,
                       const MatchFinder::MatchResult &result) const;

public:
    TypedefStructRule(const Config &cfg,
                      SuppressionManager &sup,
                      Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
