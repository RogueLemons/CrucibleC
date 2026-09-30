#include "rule_interface.hpp"

#include "rule_private_alternative.hpp"

#include <vector>

bool InterfaceRule::isThirdParty(const std::string &path) const
{
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool InterfaceRule::shouldIgnore(
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

void InterfaceRule::report(
    DiagCode code,
    SourceLocation loc,
    const std::string &message,
    const SourceManager &sm) const
{
    if (shouldIgnore(sm, loc))
        return;

    diagnostics.report(
        config.interfacesRule.level,
        code,
        sm,
        sm.getExpansionLoc(loc),
        message);
}

void InterfaceRule::checkObjectField(
    const FieldDecl *field,
    const std::string &interfaceName,
    bool isConst,
    const SourceManager &sm) const
{
    const auto *pointer = field->getType().getCanonicalType()->getAs<PointerType>();
    const QualType pointee = pointer ? pointer->getPointeeType() : QualType();

    const bool typeOk =
        pointer &&
        pointee->isVoidType() &&
        pointee.isConstQualified() == isConst &&
        !pointee.isVolatileQualified();

    if (typeOk && field->getName() == "object")
        return;

    const std::string expected = isConst ? "const void* object" : "void* object";

    report(
        DiagCode::InterfaceObjectField,
        field->getLocation(),
        "the first field of " + std::string(isConst ? "const interface '" : "interface '") +
            interfaceName + "' must be '" + expected + "', not '" +
            field->getType().getAsString() + " " + field->getNameAsString() + "'",
        sm);
}

void InterfaceRule::checkVtableField(
    const FieldDecl *field,
    const std::string &interfaceName,
    const SourceManager &sm) const
{
    const auto *pointer = field->getType().getCanonicalType()->getAs<PointerType>();
    const QualType pointee = pointer ? pointer->getPointeeType() : QualType();
    const RecordDecl *record = pointer ? pointee->getAsRecordDecl() : nullptr;

    const bool typeOk =
        record &&
        pointee.isConstQualified() &&
        config.interfacesRule.isVtableName(record->getNameAsString());

    if (typeOk && field->getName() == "vtable")
        return;

    report(
        DiagCode::InterfaceVtableField,
        field->getLocation(),
        "the second field of interface '" + interfaceName +
            "' must be a pointer to a const vtable struct named 'vtable', e.g. 'const <name>" +
            config.interfacesRule.vtableSuffix + "* vtable', not '" +
            field->getType().getAsString() + " " + field->getNameAsString() + "'",
        sm);
}

void InterfaceRule::checkInterface(
    const RecordDecl *record,
    const SourceManager &sm) const
{
    const InterfacesRuleConfig &rule = config.interfacesRule;
    const std::string name = record->getNameAsString();
    const bool isConst = rule.isConstInterfaceName(name);

    const std::vector<const FieldDecl *> fields(record->field_begin(), record->field_end());

    if (fields.size() != 2) {
        report(
            DiagCode::InterfaceFieldCount,
            record->getLocation(),
            std::string(isConst ? "const interface '" : "interface '") + name +
                "' must have exactly two fields, '" +
                (isConst ? "const void* object" : "void* object") +
                "' and a pointer to a const vtable named 'vtable', not " +
                std::to_string(fields.size()),
            sm);
    }

    if (fields.size() >= 1)
        checkObjectField(fields[0], name, isConst, sm);

    if (fields.size() >= 2)
        checkVtableField(fields[1], name, sm);

    if (!rule.interfaceMustHaveFieldsThatArePrivateAlternative ||
        config.privateAlternativeRule.level == RuleLevel::Off)
    {
        return;
    }

    for (size_t i = 0; i < fields.size() && i < 2; ++i) {
        if (PrivateAlternativeRule::isPrivateTagged(fields[i]))
            continue;

        report(
            DiagCode::InterfaceFieldNotPrivate,
            fields[i]->getLocation(),
            "field '" + fields[i]->getNameAsString() + "' of interface '" + name +
                "' must be marked private",
            sm);
    }
}

InterfaceRule::InterfaceRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void InterfaceRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        recordDecl(
            isStruct(),
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("interfaceStruct"),
        this);
}

void InterfaceRule::run(const MatchFinder::MatchResult &result)
{
    if (config.interfacesRule.level == RuleLevel::Off || !result.SourceManager)
        return;

    const auto *record = result.Nodes.getNodeAs<RecordDecl>("interfaceStruct");

    if (record && config.interfacesRule.isInterfaceName(record->getNameAsString()))
        checkInterface(record, *result.SourceManager);
}
