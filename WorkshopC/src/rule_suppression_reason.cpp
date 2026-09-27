#include "rule_suppression_reason.hpp"

SuppressionReasonRule::SuppressionReasonRule(const Config &cfg,
                                             SuppressionManager &sup,
                                             Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void SuppressionReasonRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        translationUnitDecl().bind("tu"),
        this
    );
}

void SuppressionReasonRule::run(const MatchFinder::MatchResult &result) {
    if (checked)
        return;

    checked = true;

    if (config.suppressionReasonRule.level == RuleLevel::Off)
        return;

    // Every project file that took part in this translation unit, so
    // that suppressions in project headers are checked as well.
    const auto paths = SuppressionManager::projectFiles(
        *result.SourceManager,
        config.thirdPartyIncludes);

    for (const auto &path : paths) {
        for (const auto &missing : suppressions.findMissingReasons(path)) {
            diagnostics.report(
                config.suppressionReasonRule.level,
                path,
                missing.line,
                missing.column,
                "'WorkshopC off' must be followed on the next line "
                "by a comment starting with 'Reason: '"
            );
        }
    }
}
