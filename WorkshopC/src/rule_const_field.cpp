#include "rule_const_field.hpp"

bool ConstFieldRule::isThirdParty(const std::string &path) const
{
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool ConstFieldRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    const SourceLocation spelling = sm.getSpellingLoc(loc);
    const SourceLocation expansion = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansion))
        return true;

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string spellingPath = sm.getFilename(spelling).str();
    const std::string expansionPath = sm.getFilename(expansion).str();

    return spellingPath.empty() ||
           isThirdParty(spellingPath) ||
           (!expansionPath.empty() && isThirdParty(expansionPath));
}

ConstFieldRule::ConstFieldRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void ConstFieldRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        fieldDecl(
            unless(isExpansionInSystemHeader())
        ).bind("constField"),
        this);
}

void ConstFieldRule::run(const MatchFinder::MatchResult &result)
{
    if (config.constFieldRule.level == RuleLevel::Off ||
        !result.SourceManager ||
        !result.Context)
    {
        return;
    }

    const SourceManager &sm = *result.SourceManager;
    const ASTContext &context = *result.Context;

    const auto *field = result.Nodes.getNodeAs<FieldDecl>("constField");

    // A const member makes a union just as unassignable as a struct
    if (!field || !field->getParent() ||
        !(field->getParent()->isStruct() || field->getParent()->isUnion()))
        return;

    // The elements of an array are what is const, e.g. 'const int values[3]'
    const QualType type = field->getType();
    const QualType element = context.getBaseElementType(type);

    if (!element.isConstQualified())
        return;

    const SourceLocation loc = field->getLocation();

    if (shouldIgnore(sm, loc))
        return;

    std::string what = "is const";

    if (element->isPointerType())
        what = "is a const pointer, only the data it points to may be const";

    if (context.getAsArrayType(type))
        what = element->isPointerType()
                   ? "is an array of const pointers, only the data they point to may be const"
                   : "is an array of const elements";

    diagnostics.report(
        config.constFieldRule.level,
        DiagCode::ConstField,
        sm,
        sm.getExpansionLoc(loc),
        "field '" + field->getNameAsString() + "' of type '" +
            type.getAsString() + "' " + what);
}
