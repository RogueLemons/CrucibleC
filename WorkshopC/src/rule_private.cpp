#include "rule_private.hpp"

bool PrivateRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool PrivateRule::isCFile(const std::string &path) const {
    return path.ends_with(".c");
}

bool PrivateRule::contains(
    const std::string &text,
    const std::string &value
) const {
    return text.find(value) != std::string::npos;
}

PrivateRule::PrivateRule(const Config &cfg,
            SuppressionManager &sup,
            Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void PrivateRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        ast_matchers::memberExpr(
            hasAncestor(functionDecl().bind("parentFunction"))
        ).bind("privateAccess"),
        this
    );
}

void PrivateRule::run(const MatchFinder::MatchResult &result) {
    const auto *memberExpr =
        result.Nodes.getNodeAs<MemberExpr>("privateAccess");

    if (!memberExpr)
        return;

    if (config.privateRule.level == RuleLevel::Off)
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

    const std::string &privateField =
        config.privateRule.privateField;

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
            config.privateRule.level,
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

    const std::string &getterContains = config.privateRule.getterContains;
    const std::string &setterContains = config.privateRule.setterContains;

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
        config.privateRule.level,
        sm,
        expansionLoc,
        msg
    );
}
