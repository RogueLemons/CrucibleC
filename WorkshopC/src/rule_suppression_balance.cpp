#include "rule_suppression_balance.hpp"

SuppressionBalanceRule::SuppressionBalanceRule(const Config &cfg,
                                               SuppressionManager &sup,
                                               Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void SuppressionBalanceRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        translationUnitDecl().bind("tu"),
        this
    );
}

void SuppressionBalanceRule::run(const MatchFinder::MatchResult &result) {
    if (checked)
        return;

    checked = true;

    const auto paths = SuppressionManager::projectFiles(
        *result.SourceManager,
        config.thirdPartyIncludes);

    for (const auto &path : paths) {
        for (const auto &problem : suppressions.findUnbalanced(path)) {
            diagnostics.report(
                RuleLevel::Error,
                problem.code,
                path,
                problem.line,
                problem.column,
                problem.message
            );
        }
    }
}
