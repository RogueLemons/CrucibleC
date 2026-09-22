#include "rule_enum.hpp"

bool EnumRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

EnumRule::EnumRule(const Config &cfg,
         SuppressionManager &sup,
         Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void EnumRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        enumDecl().bind("enum"),
        this
    );
}

void EnumRule::run(const MatchFinder::MatchResult &result) {
    const auto *e =
        result.Nodes.getNodeAs<EnumDecl>("enum");

    if (!e)
        return;

    if (config.enumRule.level == RuleLevel::Off)
        return;

    auto &sm = *result.SourceManager;

    SourceLocation loc = e->getLocation();

    bool fromMacro = loc.isMacroID();

    // -------------------------
    // Resolve locations
    // -------------------------

    SourceLocation spellingLoc =
        sm.getSpellingLoc(loc);

    SourceLocation expansionLoc =
        sm.getExpansionLoc(loc);

    // -------------------------
    // Suppression handling
    // -------------------------

    if (suppressions.isSuppressed(sm, expansionLoc))
        return;

    // -------------------------
    // File paths
    // -------------------------

    std::string spellingPath =
        sm.getFilename(spellingLoc).str();

    std::string expansionPath =
        sm.getFilename(expansionLoc).str();

    // -------------------------
    // Ignore third-party macro definitions
    // -------------------------

    if (!spellingPath.empty() &&
        isThirdParty(spellingPath))
    {
        return;
    }

    // -------------------------
    // Ignore third-party expansion sites
    // -------------------------

    if (!expansionPath.empty() &&
        isThirdParty(expansionPath))
    {
        return;
    }

    // -------------------------
    // Build message
    // -------------------------

    std::string name = e->getNameAsString();

    if (name.empty())
        name = "<anonymous>";

    std::string msg =
        "enum '" + name + "' is not allowed";

    if (fromMacro)
        msg += " (macro expansion)";

    // -------------------------
    // Report diagnostic
    // -------------------------

    diagnostics.report(
        config.enumRule.level,
        sm,
        expansionLoc,
        msg
    );
}
