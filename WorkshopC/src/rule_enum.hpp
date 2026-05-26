#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class EnumRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

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
             Diagnostics &diag)
        : config(cfg),
          globalConfig(gc),
          diagnostics(diag) {}

    void run(const MatchFinder::MatchResult &result) override {
        const auto *e = result.Nodes.getNodeAs<EnumDecl>("enum");

        if (!e)
            return;

        if (config.level == RuleLevel::Off)
            return;

        auto &sm = *result.SourceManager;

        SourceLocation loc = e->getLocation();

        bool fromMacro = loc.isMacroID();

        // -------------------------
        // Resolve REAL source location
        // -------------------------

        SourceLocation spellingLoc =
            sm.getSpellingLoc(loc);

        SourceLocation expansionLoc =
            sm.getExpansionLoc(loc);

        std::string spellingPath =
            sm.getFilename(spellingLoc).str();

        std::string expansionPath =
            sm.getFilename(expansionLoc).str();

        // -------------------------
        // Ignore third-party macros
        // based on WHERE MACRO WAS DEFINED
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

        std::string name = e->getNameAsString();

        if (name.empty())
            name = "<anonymous>";

        std::string msg =
            "enum '" + name + "' is not allowed";

        if (fromMacro)
            msg += " (macro expansion)";

        // -------------------------
        // Use expansion location
        // so diagnostics point to actual usage site
        // -------------------------

        diagnostics.report(
            config.level,
            sm,
            expansionLoc,
            msg
        );
    }
};