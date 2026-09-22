#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class PrefixNamespaceRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    bool isThirdParty(const std::string &path) const;

    bool isHeaderFile(const std::string &path) const;

    std::string sanitize(const std::string &s) const;

    std::string upper(const std::string &s) const;

    std::vector<std::string> splitPath(
        const std::string &path) const;

    // ------------------------------------------------------------
    // Extract dirs after top_dir and before filename
    // ------------------------------------------------------------
    std::vector<std::string> extractDirs(
        const std::string &path) const;

    // ------------------------------------------------------------
    // build namespace prefix
    //
    // work_from_top = true
    //   a/b/c -> a__b__
    //
    // work_from_top = false
    //   a/b/c -> b__c__
    // ------------------------------------------------------------
    std::string buildPrefix(
        const std::string &path) const;

    // ------------------------------------------------------------
    // build include guard
    //
    // Uses FULL path after top_dir
    //
    // Example:
    //
    // WorkshopC/tests/headers/a/b/c/header.h
    //
    // =>
    //
    // TESTS_HEADERS_A_B_C_HEADER_H
    // ------------------------------------------------------------
    std::string buildIncludeGuardName(
        const std::string &path) const;

    bool shouldCheckPath(
        const std::string &path) const;

    void checkName(SourceManager &sm,
                   SourceLocation loc,
                   const std::string &path,
                   const std::string &kind,
                   const std::string &name);

    void checkIncludeGuard(SourceManager &sm,
                           SourceLocation loc,
                           const std::string &path);

    void checkFileIncludeGuard(SourceManager &sm,
                               SourceLocation loc,
                               const std::string &path);

public:
    PrefixNamespaceRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag
    );

    void bindFinder(MatchFinder &finder);

    void run(
        const MatchFinder::MatchResult &result) override;
};
