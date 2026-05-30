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
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    std::unordered_set<const TagDecl*> seen;

private:
    bool isThirdParty(const std::string &file) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (!p.empty() && file.find(p) != std::string::npos)
                return true;
        }
        return false;
    }

    bool isExternalMacroExpansion(const SourceManager &sm,
                                  const RecordDecl *RD) const
    {
        SourceLocation loc = RD->getLocation();

        if (!loc.isMacroID())
            return false;

        SourceLocation expansion = sm.getExpansionLoc(loc);
        std::string file = sm.getFilename(expansion).str();

        return isThirdParty(file);
    }

    bool shouldIgnore(const SourceManager &sm, SourceLocation loc) const {
        if (loc.isInvalid())
            return true;

        if (suppressions.isSuppressed(sm, loc))
            return true;

        // IMPORTANT: use spelling location for correctness
        SourceLocation spell = sm.getSpellingLoc(loc);

        if (sm.isInSystemHeader(spell))
            return true;

        std::string file = sm.getFilename(spell).str();
        if (file.empty())
            return true;

        return isThirdParty(file);
    }

    void report(const RecordDecl *RD, const SourceManager &sm) {
        diagnostics.report(
            config.level,
            sm,
            RD->getLocation(),
            "struct '" + RD->getNameAsString() + "' must have a typedef"
        );
    }

    bool hasTypedefFor(const TagDecl *TD,
                       const MatchFinder::MatchResult &result) const
    {
        const auto &ctx = *result.Context;

        for (const auto *D : ctx.getTranslationUnitDecl()->decls()) {
            const auto *TDN = dyn_cast<TypedefNameDecl>(D);
            if (!TDN)
                continue;

            QualType QT = TDN->getUnderlyingType();
            const auto *RT = QT->getAs<RecordType>();
            if (!RT)
                continue;

            const TagDecl *target = RT->getDecl();
            if (!target)
                continue;

            if (target->getCanonicalDecl() == TD->getCanonicalDecl())
                return true;
        }

        return false;
    }

public:
    TypedefStructRule(const RuleConfig &cfg,
                      const Config &gc,
                      SuppressionManager &sup,
                      Diagnostics &diag)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag) {}

    void run(const MatchFinder::MatchResult &result) override {
        const auto &sm = *result.SourceManager;

        const auto *RD =
            result.Nodes.getNodeAs<RecordDecl>("struct");

        if (!RD)
            return;

        if (!RD->isStruct())
            return;

        if (!RD->isThisDeclarationADefinition())
            return;

        if (!RD->getIdentifier())
            return; // ignore anonymous structs

        if (shouldIgnore(sm, RD->getLocation()))
            return;

        const TagDecl *canon = RD->getCanonicalDecl();
        if (!canon)
            return;

        if (seen.count(canon))
            return;

        seen.insert(canon);

        // NEW: ignore structs originating from external macro expansions
        if (isExternalMacroExpansion(sm, RD))
            return;

        if (!hasTypedefFor(canon, result)) {
            report(RD, sm);
        }
    }
};