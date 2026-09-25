#include "rule_suppression_reason.hpp"

#include <set>

bool SuppressionReasonRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

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

    const SourceManager &sm = *result.SourceManager;

    // Every file that took part in this translation unit, so that
    // suppressions in project headers are checked as well.
    std::set<std::string> paths;

    for (auto it = sm.fileinfo_begin(); it != sm.fileinfo_end(); ++it) {
        const FileID fid = sm.translateFile(it->first);

        if (fid.isInvalid())
            continue;

        const SourceLocation start = sm.getLocForStartOfFile(fid);

        if (sm.isInSystemHeader(start))
            continue;

        const std::string path = sm.getFilename(start).str();

        if (path.empty() || isThirdParty(path))
            continue;

        paths.insert(path);
    }

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
