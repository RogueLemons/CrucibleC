#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class EnumRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    bool isThirdParty(const std::string &path) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (path.find(p) != std::string::npos)
                return true;
        }

        return false;
    }

public:
    EnumRule(const RuleConfig &cfg,
             const Config &gc,
             SuppressionManager &sup,
             Diagnostics &diag)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag) {}

    void run(const MatchFinder::MatchResult &result) override {
        const auto *e =
            result.Nodes.getNodeAs<EnumDecl>("enum");

        if (!e)
            return;

        if (config.level == RuleLevel::Off)
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
            config.level,
            sm,
            expansionLoc,
            msg
        );
    }
};