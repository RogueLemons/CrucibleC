#include "rule_restricted_malloc.hpp"

#include <algorithm>

bool RestrictedMallocRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool RestrictedMallocRule::isAllowedFunction(const FunctionDecl *function) const {
    if (!function)
        return false;

    const auto &allowed =
        config.restrictedMallocRule.listOfAllowedMallocFunctions;

    return std::find(
        allowed.begin(),
        allowed.end(),
        function->getNameAsString()) != allowed.end();
}

std::string RestrictedMallocRule::allowedFunctionsText() const {
    const auto &allowed =
        config.restrictedMallocRule.listOfAllowedMallocFunctions;

    std::string text;

    for (const auto &name : allowed) {
        if (!text.empty())
            text += ", ";

        text += "'" + name + "'";
    }

    return text;
}

RestrictedMallocRule::RestrictedMallocRule(const Config &cfg,
                                           SuppressionManager &sup,
                                           Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void RestrictedMallocRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        declRefExpr(
            to(functionDecl(hasAnyName(
                "malloc",
                "calloc",
                "realloc",
                "free",
                "strdup",
                "strndup",
                "asprintf",
                "getline",
                "realpath"
            )).bind("memoryFunction")),
            unless(isExpansionInSystemHeader()),
            optionally(hasAncestor(functionDecl().bind("enclosingFunction")))
        ).bind("use"),
        this
    );
}

void RestrictedMallocRule::run(const MatchFinder::MatchResult &result) {
    if (config.restrictedMallocRule.level == RuleLevel::Off)
        return;

    const auto *use =
        result.Nodes.getNodeAs<DeclRefExpr>("use");

    const auto *memoryFunction =
        result.Nodes.getNodeAs<FunctionDecl>("memoryFunction");

    if (!use || !memoryFunction)
        return;

    const SourceManager &sm = *result.SourceManager;

    const SourceLocation loc = use->getLocation();
    const SourceLocation spellingLoc = sm.getSpellingLoc(loc);
    const SourceLocation expansionLoc = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansionLoc))
        return;

    if (sm.isInSystemHeader(spellingLoc))
        return;

    const std::string spellingPath = sm.getFilename(spellingLoc).str();
    const std::string expansionPath = sm.getFilename(expansionLoc).str();

    if (!spellingPath.empty() && isThirdParty(spellingPath))
        return;

    if (!expansionPath.empty() && isThirdParty(expansionPath))
        return;

    const auto *enclosingFunction =
        result.Nodes.getNodeAs<FunctionDecl>("enclosingFunction");

    if (isAllowedFunction(enclosingFunction))
        return;

    const std::string name = memoryFunction->getNameAsString();
    const std::string allowed = allowedFunctionsText();

    std::string message;

    if (allowed.empty()) {
        message =
            "'" + name + "' may not be used, no functions are allowed "
            "to use it (list_of_allowed_malloc_functions is empty)";
    }
    else {
        message =
            "'" + name + "' may only be used inside the functions " +
            allowed;
    }

    diagnostics.report(
        config.restrictedMallocRule.level,
        sm,
        expansionLoc,
        message
    );
}
