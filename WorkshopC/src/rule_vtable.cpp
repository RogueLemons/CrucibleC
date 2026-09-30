#include "rule_vtable.hpp"

#include <vector>

namespace {

// 'void*' or 'const void*', the object a vtable function works on
bool isObjectPointer(QualType type)
{
    const auto *pointer = type.getCanonicalType()->getAs<PointerType>();

    if (!pointer)
        return false;

    const QualType pointee = pointer->getPointeeType();

    return pointee->isVoidType() &&
           !pointee.isVolatileQualified() &&
           !pointee.isRestrictQualified();
}

// The function a vtable initializer element names, or null
const FunctionDecl *getFunction(const Expr *element)
{
    const Expr *stripped = element->IgnoreParenImpCasts();

    // '&function' names the function too
    if (const auto *address = dyn_cast<UnaryOperator>(stripped)) {
        if (address->getOpcode() == UO_AddrOf)
            stripped = address->getSubExpr()->IgnoreParenImpCasts();
    }

    const auto *reference = dyn_cast<DeclRefExpr>(stripped);
    return reference ? dyn_cast<FunctionDecl>(reference->getDecl()) : nullptr;
}

} // namespace

bool VtableRule::isThirdParty(const std::string &path) const
{
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool VtableRule::shouldIgnore(
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

void VtableRule::report(
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

const RecordDecl *VtableRule::getVtable(QualType type) const
{
    const RecordDecl *record = type.getCanonicalType()->getAsRecordDecl();

    if (!record || !record->isStruct() ||
        !config.interfacesRule.isVtableName(record->getNameAsString()))
    {
        return nullptr;
    }

    return record;
}

void VtableRule::checkVtableStruct(
    const RecordDecl *record,
    const SourceManager &sm) const
{
    const std::string vtableName = record->getNameAsString();

    for (const FieldDecl *field : record->fields()) {
        const std::string fieldText =
            "field '" + field->getNameAsString() + "' of vtable struct '" + vtableName + "'";

        const auto *pointer = field->getType().getCanonicalType()->getAs<PointerType>();
        const auto *function =
            pointer ? pointer->getPointeeType()->getAs<FunctionType>() : nullptr;

        if (!function) {
            report(
                DiagCode::VtableFieldNotFunctionPointer,
                field->getLocation(),
                fieldText + " must be a function pointer",
                sm);
            continue;
        }

        const auto *prototype = dyn_cast<FunctionProtoType>(function);

        if (!prototype || prototype->getNumParams() == 0 ||
            !isObjectPointer(prototype->getParamType(0)))
        {
            report(
                DiagCode::VtableFunctionMissingObjectParameter,
                field->getLocation(),
                fieldText + " must take a 'void*' or 'const void*' as its first parameter, the object it works on",
                sm);
        }
    }
}

void VtableRule::checkInitializer(
    const VarDecl *variable,
    const RecordDecl *vtable,
    const SourceManager &sm) const
{
    const std::string variableText = "vtable '" + variable->getNameAsString() + "'";
    const Expr *init = variable->getInit();
    const auto *list = init ? dyn_cast<InitListExpr>(init->IgnoreImplicit()) : nullptr;

    if (!list) {
        report(
            DiagCode::VtableNotInitialized,
            variable->getLocation(),
            variableText + " must be initialized at declaration with braces, with a function for every function pointer",
            sm);
        return;
    }

    // The semantic form holds an element per field, an implicit one
    // for every field the braces leave out
    std::vector<std::string> missing;
    unsigned index = 0;

    for (const FieldDecl *field : vtable->fields()) {
        const Expr *element = index < list->getNumInits() ? list->getInit(index) : nullptr;
        ++index;

        if (!element || isa<ImplicitValueInitExpr>(element)) {
            missing.push_back("'" + field->getNameAsString() + "'");
            continue;
        }

        // Where the element is written, e.g. 'NULL', not inside the
        // system header that defines the macro
        if (!getFunction(element)) {
            report(
                DiagCode::VtableElementNotFunction,
                sm.getExpansionLoc(element->getBeginLoc()),
                "function pointer '" + field->getNameAsString() + "' of " + variableText +
                    " must be initialized with a function, not e.g. NULL or 0",
                sm);
        }
    }

    if (missing.empty())
        return;

    std::string names;

    for (size_t i = 0; i < missing.size(); ++i)
        names += (i == 0 ? "" : ", ") + missing[i];

    report(
        DiagCode::VtableMissingFunction,
        variable->getLocation(),
        variableText + " must give a function for every function pointer, missing " + names,
        sm);
}

void VtableRule::checkVtableVariable(
    const VarDecl *variable,
    const RecordDecl *vtable,
    const SourceManager &sm) const
{
    const std::string variableText = "vtable '" + variable->getNameAsString() + "'";

    if (isa<ParmVarDecl>(variable)) {
        report(
            DiagCode::VtableNotStaticConst,
            variable->getLocation(),
            variableText + " may not be passed by value, pass a pointer to a static const vtable instead",
            sm);
        return;
    }

    // An array is const through its elements
    const QualType element = variable->getASTContext().getBaseElementType(variable->getType());

    if (variable->getStorageClass() != SC_Static || !element.isConstQualified()) {
        report(
            DiagCode::VtableNotStaticConst,
            variable->getLocation(),
            variableText + " must be declared static const",
            sm);
    }

    // An extern declaration has no initializer of its own, and an array
    // of vtables is only checked for static const
    if (variable->isThisDeclarationADefinition() != VarDecl::DeclarationOnly &&
        !variable->getType()->isArrayType())
    {
        checkInitializer(variable, vtable, sm);
    }
}

VtableRule::VtableRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void VtableRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        recordDecl(
            isStruct(),
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("vtableStruct"),
        this);

    finder.addMatcher(
        varDecl(
            unless(isExpansionInSystemHeader())
        ).bind("vtableVariable"),
        this);
}

void VtableRule::run(const MatchFinder::MatchResult &result)
{
    if (config.interfacesRule.level == RuleLevel::Off ||
        config.interfacesRule.vtableSuffix.empty() ||
        !result.SourceManager)
    {
        return;
    }

    const SourceManager &sm = *result.SourceManager;

    if (const auto *record = result.Nodes.getNodeAs<RecordDecl>("vtableStruct")) {
        if (config.interfacesRule.isVtableName(record->getNameAsString()))
            checkVtableStruct(record, sm);

        return;
    }

    const auto *variable = result.Nodes.getNodeAs<VarDecl>("vtableVariable");

    if (!variable)
        return;

    // Parameters of function pointer types are not variables of their own
    if (isa<ParmVarDecl>(variable) && !isa_and_nonnull<FunctionDecl>(variable->getDeclContext()))
        return;

    QualType type = variable->getType();

    if (const ArrayType *array = variable->getASTContext().getAsArrayType(type))
        type = array->getElementType();

    if (const RecordDecl *vtable = getVtable(type))
        checkVtableVariable(variable, vtable, sm);
}
