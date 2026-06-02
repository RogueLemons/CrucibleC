#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Attr.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class ArgumentPointerMovementRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    static constexpr const char* kMoveTag = "workshopc_move";
    static constexpr const char* kOutTag  = "workshopc_out";
    static constexpr const char* kModTag  = "workshopc_modify";

private:

    struct FunctionState {
        std::vector<std::string> declTags;
        bool hasDecl = false;
    };

    std::unordered_map<std::string, FunctionState> functions;

private:

    bool isNonConstPointer(QualType qt) const {
        const auto *ptr = qt->getAs<PointerType>();
        if (!ptr) return false;
        return !ptr->getPointeeType().isConstQualified();
    }

    bool isThirdParty(const std::string &path) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (path.find(p) != std::string::npos)
                return true;
        }
        return false;
    }

    std::string getTag(const ParmVarDecl *param) const {
        for (const auto *attr : param->attrs()) {
            if (const auto *A = dyn_cast<AnnotateAttr>(attr)) {
                StringRef t = A->getAnnotation();
                if (t == kMoveTag || t == kOutTag || t == kModTag)
                    return t.str();
            }
        }
        return "";
    }

    std::string makeKey(const FunctionDecl *FD) const {
        std::ostringstream oss;

        oss << FD->getNameAsString() << "(";

        bool first = true;
        for (const ParmVarDecl *P : FD->parameters()) {
            if (!first) oss << ",";
            first = false;
            oss << P->getType().getAsString();
        }

        oss << ")";
        return oss.str();
    }

    void report(const std::string &msg,
                const ParmVarDecl *P,
                const SourceManager &sm,
                SourceLocation loc)
    {
        diagnostics.report(
            config.level,
            sm,
            loc,
            msg
        );
    }

    // =====================================================
    // NEW: EXCLUDE INTERNAL WORKSHOPC HELPERS
    // =====================================================
    bool isWorkshopCInternal(const FunctionDecl *FD) const {
        if (!FD) return false;

        std::string name = FD->getNameAsString();

        return name == "workshopc_move"
            || name == "workshopc_out"
            || name == "workshopc_modify";
    }

public:
    ArgumentPointerMovementRule(
        const RuleConfig &cfg,
        const Config &gc,
        SuppressionManager &sup,
        Diagnostics &diag)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag)
    {}

    void run(const MatchFinder::MatchResult &result) override {

        const auto *FD =
            result.Nodes.getNodeAs<FunctionDecl>("func");

        if (!FD || FD->isImplicit())
            return;

        // =====================================================
        // 🔥 IMPORTANT: SKIP INTERNAL HELPER FUNCTIONS
        // =====================================================
        if (isWorkshopCInternal(FD))
            return;

        auto &sm = *result.SourceManager;

        SourceLocation loc = FD->getLocation();
        SourceLocation expansionLoc = sm.getExpansionLoc(loc);

        // -------------------------
        // suppression
        // -------------------------
        if (suppressions.isSuppressed(sm, expansionLoc))
            return;

        // -------------------------
        // system header filter
        // -------------------------
        if (sm.isInSystemHeader(expansionLoc))
            return;

        // -------------------------
        // third-party filter
        // -------------------------
        std::string expansionPath =
            sm.getFilename(expansionLoc).str();

        if (!expansionPath.empty() &&
            isThirdParty(expansionPath))
        {
            return;
        }

        // -------------------------
        // process parameters
        // -------------------------
        std::vector<std::string> tags;

        for (const ParmVarDecl *P : FD->parameters()) {

            QualType qt = P->getType();
            std::string tag;

            if (qt->isPointerType() && isNonConstPointer(qt)) {

                for (const auto *attr : P->attrs()) {
                    if (const auto *A = dyn_cast<AnnotateAttr>(attr)) {
                        StringRef t = A->getAnnotation();
                        if (t == kMoveTag || t == kOutTag || t == kModTag)
                            tag = t.str();
                    }
                }

                if (tag.empty()) {
                    report(
                        "function '" + FD->getNameAsString() +
                        "' parameter '" + P->getNameAsString() +
                        "' uses non-const pointer; movement attribute required "
                        "(workshopc_move, workshopc_out, workshopc_modify)",
                        P,
                        sm,
                        expansionLoc
                    );
                }
            }

            tags.push_back(tag);
        }

        // -------------------------
        // store declaration state
        // -------------------------
        std::string key = makeKey(FD);
        auto &state = functions[key];

        if (!FD->isThisDeclarationADefinition()) {
            state.declTags = tags;
            state.hasDecl = true;
            return;
        }

        // -------------------------
        // definition check
        // -------------------------
        if (!state.hasDecl)
            return;

        if (state.declTags.size() != tags.size())
            return;

        for (size_t i = 0; i < tags.size(); i++) {

            if (state.declTags[i] != tags[i]) {

                const ParmVarDecl *P = FD->parameters()[i];

                report(
                    "function '" + FD->getNameAsString() +
                    "' parameter '" + P->getNameAsString() +
                    "' movement attribute mismatch between declaration and definition",
                    P,
                    sm,
                    expansionLoc
                );
            }
        }
    }
};