#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * Always enabled, with no setting: every 'WorkshopC off' must be turned
 * back on with 'WorkshopC on' in the same file, headers and source files
 * alike, since an unterminated 'off' silently disables every rule for
 * the rest of the file. An 'on' without an 'off', and an 'off' while
 * already off, are reported as well.
 */
class SuppressionBalanceRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool checked = false;

public:
    SuppressionBalanceRule(const Config &cfg,
                           SuppressionManager &sup,
                           Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
