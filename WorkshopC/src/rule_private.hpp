#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class PrivateRule : public MatchFinder::MatchCallback {
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

    bool isCFile(const std::string &path) const {
        return path.ends_with(".c");
    }

    bool contains(
        const std::string &text,
        const std::string &value
    ) const {
        return text.find(value) != std::string::npos;
    }

public:
    PrivateRule(const RuleConfig &cfg,
                const Config &gc,
                SuppressionManager &sup,
                Diagnostics &diag)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag) {}

    void run(const MatchFinder::MatchResult &result) override {
        const auto *memberExpr =
            result.Nodes.getNodeAs<MemberExpr>("privateAccess");

        if (!memberExpr)
            return;

        if (config.level == RuleLevel::Off)
            return;

        auto &sm = *result.SourceManager;

        SourceLocation loc =
            memberExpr->getExprLoc();

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
        // Paths
        // -------------------------

        std::string spellingPath =
            sm.getFilename(spellingLoc).str();

        std::string expansionPath =
            sm.getFilename(expansionLoc).str();

        // -------------------------
        // Ignore third-party code
        // -------------------------

        if (!spellingPath.empty() &&
            isThirdParty(spellingPath))
        {
            return;
        }

        if (!expansionPath.empty() &&
            isThirdParty(expansionPath))
        {
            return;
        }

        // -------------------------
        // Check field name
        // -------------------------

        const auto *memberDecl =
            memberExpr->getMemberDecl();

        if (!memberDecl)
            return;

        std::string fieldName =
            memberDecl->getNameAsString();

        std::string privateField =
            "_private";

        auto it =
            config.options.find("private_field");

        if (it != config.options.end()) {
            privateField = it->second;
        }

        if (fieldName != privateField)
            return;

        // -------------------------
        // Must be inside function
        // -------------------------

        const auto *func =
            result.Nodes.getNodeAs<FunctionDecl>(
                "parentFunction"
            );

        if (!func) {
            diagnostics.report(
                config.level,
                sm,
                expansionLoc,
                "direct access to private field '" +
                fieldName +
                "' is not allowed"
            );

            return;
        }

        // -------------------------
        // Function requirements
        // -------------------------

        std::string functionName =
            func->getNameAsString();

        std::string getterContains = "pget";
        std::string setterContains = "pset";

        auto getterIt =
            config.options.find("getter_contains");

        if (getterIt != config.options.end()) {
            getterContains = getterIt->second;
        }

        auto setterIt =
            config.options.find("setter_contains");

        if (setterIt != config.options.end()) {
            setterContains = setterIt->second;
        }

        bool validName =
            contains(functionName, getterContains) ||
            contains(functionName, setterContains);

        bool isStatic =
            func->getStorageClass() == SC_Static;

        bool inCFile =
            isCFile(expansionPath);

        if (validName &&
            isStatic &&
            inCFile)
        {
            return;
        }

        // -------------------------
        // Diagnostic
        // -------------------------

        std::string msg =
            "private field '" +
            fieldName +
            "' may only be accessed from static "
            ".c functions containing '" +
            getterContains +
            "' or '" +
            setterContains +
            "'";

        diagnostics.report(
            config.level,
            sm,
            expansionLoc,
            msg
        );
    }
};