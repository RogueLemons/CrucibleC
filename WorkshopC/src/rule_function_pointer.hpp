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
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    // Stable dedup key
    mutable std::set<std::string> reported;

private:
    bool isThirdParty(const std::string &path) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (path.find(p) != std::string::npos)
                return true;
        }
        return false;
    }

    // ------------------------------------------------------------
    // Detect: pointer to function type
    // ------------------------------------------------------------
    bool isFunctionPointer(QualType qt) const {
        if (qt.isNull())
            return false;

        const Type *t = qt.getTypePtrOrNull();
        if (!t)
            return false;

        t = t->getUnqualifiedDesugaredType();

        if (const auto *ptr = dyn_cast<PointerType>(t)) {
            const Type *pt = ptr->getPointeeType().getTypePtrOrNull();
            if (!pt)
                return false;

            return pt->isFunctionType();
        }

        return false;
    }

    // ------------------------------------------------------------
    // THIS is the correct fix:
    // Detect whether type was written as typedef or raw syntax
    // ------------------------------------------------------------
    bool isTypedefSpelled(const Decl *decl) const {
        const TypeSourceInfo *TSI = nullptr;

        if (const auto *vd = dyn_cast<VarDecl>(decl))
            TSI = vd->getTypeSourceInfo();
        else if (const auto *pd = dyn_cast<ParmVarDecl>(decl))
            TSI = pd->getTypeSourceInfo();

        if (!TSI)
            return false;

        TypeLoc TL = TSI->getTypeLoc();

        // Walk through type locations to find typedef spelling
        while (!TL.isNull()) {
            if (TL.getTypeLocClass() == TypeLoc::Typedef)
                return true;

            TL = TL.getNextTypeLoc();
        }

        return false;
    }

    // ------------------------------------------------------------
    // Stable dedup key
    // ------------------------------------------------------------
    std::string makeKey(const Decl *D, const SourceManager &sm) const {
        D = D->getCanonicalDecl();

        SourceLocation loc = D->getLocation();
        PresumedLoc ploc = sm.getPresumedLoc(loc);

        if (!ploc.isValid())
            return "";

        return std::string(ploc.getFilename()) + ":" +
               std::to_string(ploc.getLine()) + ":" +
               D->getDeclKindName();
    }

public:
    FunctionPointerRule(const RuleConfig &cfg,
                        const Config &gc,
                        SuppressionManager &sup,
                        Diagnostics &diag)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag) {}

    void run(const MatchFinder::MatchResult &result) override {
        const auto *vd =
            result.Nodes.getNodeAs<VarDecl>("funcptr");

        const auto *pd =
            result.Nodes.getNodeAs<ParmVarDecl>("funcptr");

        if (!vd && !pd)
            return;

        if (config.level == RuleLevel::Off)
            return;

        auto &sm = *result.SourceManager;

        const Decl *decl =
            vd ? static_cast<const Decl*>(vd)
               : static_cast<const Decl*>(pd);

        QualType qt =
            vd ? vd->getType()
               : pd->getType();

        // -------------------------
        // Location
        // -------------------------
        SourceLocation loc = decl->getLocation();
        bool fromMacro = loc.isMacroID();

        SourceLocation spellingLoc = sm.getSpellingLoc(loc);
        SourceLocation expansionLoc = sm.getExpansionLoc(loc);

        // -------------------------
        // Suppression
        // -------------------------
        if (suppressions.isSuppressed(sm, expansionLoc))
            return;

        std::string spellingPath = sm.getFilename(spellingLoc).str();
        std::string expansionPath = sm.getFilename(expansionLoc).str();

        if (!spellingPath.empty() && isThirdParty(spellingPath))
            return;

        if (!expansionPath.empty() && isThirdParty(expansionPath))
            return;

        // -------------------------
        // Must be function pointer
        // -------------------------
        if (!isFunctionPointer(qt))
            return;

        // -------------------------
        // TYPODEF EXCEPTION (FIXED PROPERLY)
        // -------------------------
        if (isTypedefSpelled(decl))
            return;

        // -------------------------
        // Dedup
        // -------------------------
        std::string key = makeKey(decl, sm);

        if (!key.empty()) {
            if (reported.count(key))
                return;

            reported.insert(key);
        }

        // -------------------------
        // Message
        // -------------------------
        std::string name;

        if (vd)
            name = vd->getNameAsString();
        else if (pd)
            name = pd->getNameAsString();

        std::string msg = "function pointer usage is not allowed";

        if (!name.empty())
            msg += " (" + name + ")";

        if (fromMacro)
            msg += " (macro expansion)";

        diagnostics.report(
            config.level,
            sm,
            expansionLoc,
            msg
        );
    }
};