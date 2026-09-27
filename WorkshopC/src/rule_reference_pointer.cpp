#include "rule_reference_pointer.hpp"

#include "reference_tag.hpp"
#include "tag_placement.hpp"

#include <algorithm>

bool ReferencePointerRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool ReferencePointerRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    const SourceLocation expansion = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansion))
        return true;

    if (sm.isInSystemHeader(expansion))
        return true;

    const std::string path = sm.getFilename(expansion).str();

    return !path.empty() && isThirdParty(path);
}

const Expr *ReferencePointerRule::stripWrappers(const Expr *expr) const {
    while (expr) {
        expr = expr->IgnoreParenCasts();

        const auto *call = dyn_cast<CallExpr>(expr);
        const FunctionDecl *callee = call ? call->getDirectCallee() : nullptr;

        if (!callee || call->getNumArgs() != 1)
            break;

        const std::string name = callee->getNameAsString();

        if (name != "workshopc_move" &&
            name != "workshopc_out" &&
            name != "workshopc_modify")
            break;

        expr = call->getArg(0);
    }

    return expr;
}

bool ReferencePointerRule::isReference(const Expr *expr) const {
    const auto *ref = dyn_cast_or_null<DeclRefExpr>(stripWrappers(expr));

    return ref && hasReferenceTag(dyn_cast<ParmVarDecl>(ref->getDecl()));
}

bool ReferencePointerRule::isObject(const Expr *expr) const {
    if (!expr)
        return false;

    expr = expr->IgnoreParenImpCasts();

    // variable
    if (const auto *ref = dyn_cast<DeclRefExpr>(expr))
        return isa<VarDecl>(ref->getDecl());

    // variable.field, reference->field
    if (const auto *member = dyn_cast<MemberExpr>(expr)) {
        return member->isArrow()
            ? isReference(member->getBase())
            : isObject(member->getBase());
    }

    // array[index], only on a real array, not on a pointer
    if (const auto *subscript = dyn_cast<ArraySubscriptExpr>(expr)) {
        const Expr *base = subscript->getBase()->IgnoreParenImpCasts();

        return base->getType()->isArrayType() && isObject(base);
    }

    // *reference
    if (const auto *unary = dyn_cast<UnaryOperator>(expr)) {
        if (unary->getOpcode() == UO_Deref)
            return isReference(unary->getSubExpr());
    }

    return false;
}

bool ReferencePointerRule::isValidArgument(const Expr *arg) const {
    const Expr *stripped = stripWrappers(arg);

    if (!stripped)
        return false;

    // Another reference passed on
    if (isReference(stripped))
        return true;

    // &object
    if (const auto *addressOf = dyn_cast<UnaryOperator>(stripped)) {
        if (addressOf->getOpcode() == UO_AddrOf)
            return isObject(addressOf->getSubExpr());
    }

    // A string literal, or an array decaying to a pointer to its first element
    if (isa<StringLiteral>(stripped))
        return true;

    if (stripped->getType()->isArrayType())
        return isObject(stripped);

    return false;
}

void ReferencePointerRule::report(
    const SourceManager &sm,
    SourceLocation loc,
    const std::string &message)
{
    if (shouldIgnore(sm, loc))
        return;

    diagnostics.report(
        config.referencePointerRule.level,
        sm,
        sm.getExpansionLoc(loc),
        message
    );
}

void ReferencePointerRule::checkCall(const CallExpr *call, const SourceManager &sm) {
    const FunctionDecl *callee = call->getDirectCallee();

    if (!callee)
        return;

    const unsigned count = std::min(call->getNumArgs(), callee->getNumParams());

    for (unsigned i = 0; i < count; ++i) {
        const ParmVarDecl *param = callee->getParamDecl(i);

        if (!hasReferenceTag(param))
            continue;

        const Expr *arg = call->getArg(i);

        if (isValidArgument(arg))
            continue;

        report(sm, arg->getExprLoc(),
            "argument for reference parameter '" + param->getNameAsString() +
            "' of function '" + callee->getNameAsString() +
            "' must be the address of an object (e.g. '&variable') or "
            "another reference pointer, since a reference can never be null");
    }
}

void ReferencePointerRule::checkReassignment(
    const Expr *target,
    SourceLocation loc,
    const SourceManager &sm)
{
    const auto *ref =
        dyn_cast_or_null<DeclRefExpr>(target ? target->IgnoreParenImpCasts() : nullptr);

    const auto *param = ref ? dyn_cast<ParmVarDecl>(ref->getDecl()) : nullptr;

    if (!hasReferenceTag(param))
        return;

    report(sm, loc,
        "reference pointer '" + param->getNameAsString() +
        "' may not be reassigned");
}

void ReferencePointerRule::checkFunction(
    const FunctionDecl *function,
    const SourceManager &sm)
{
    for (const ParmVarDecl *param : function->parameters()) {
        if (!hasReferenceTag(param) || param->getType()->isPointerType())
            continue;

        // Only report where the tag is actually written
        bool writtenHere = false;

        for (const auto *attr : param->attrs()) {
            if (const auto *annotate = dyn_cast<AnnotateAttr>(attr)) {
                if (annotate->getAnnotation() == kReferencePointerTag &&
                    !annotate->isInherited())
                    writtenHere = true;
            }
        }

        if (!writtenHere)
            continue;

        report(sm, param->getLocation(),
            "reference tag on parameter '" + param->getNameAsString() +
            "' of function '" + function->getNameAsString() +
            "' may only be used on a pointer");
    }
}

void ReferencePointerRule::checkTagPlacement(
    const Decl *decl,
    const SourceManager &sm)
{
    if (!decl || decl->isImplicit())
        return;

    const auto *param = dyn_cast<ParmVarDecl>(decl);

    // The tag belongs on the parameters of function declarations
    if (param && isParameterOfFunctionDecl(param))
        return;

    for (const auto *attr : decl->attrs()) {
        const auto *annotate = dyn_cast<AnnotateAttr>(attr);

        // Inherited attributes were already reported where written
        if (!annotate || annotate->isInherited() ||
            annotate->getAnnotation() != kReferencePointerTag)
            continue;

        std::string name = "<unnamed>";

        if (const auto *named = dyn_cast<NamedDecl>(decl)) {
            if (!named->getName().empty())
                name = named->getNameAsString();
        }

        report(sm, decl->getLocation(),
            param
                ? "reference tag on '" + name +
                  "' may not be used on a parameter of a function pointer "
                  "type, calls through function pointers are not checked"
                : "reference tag on '" + name +
                  "' may only be used on function parameters");
    }
}

ReferencePointerRule::ReferencePointerRule(const Config &cfg,
                                           SuppressionManager &sup,
                                           Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void ReferencePointerRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        callExpr(unless(isExpansionInSystemHeader())).bind("call"),
        this);

    finder.addMatcher(
        binaryOperator(
            isAssignmentOperator(),
            unless(isExpansionInSystemHeader())
        ).bind("assignment"),
        this);

    finder.addMatcher(
        unaryOperator(
            anyOf(hasOperatorName("++"), hasOperatorName("--")),
            unless(isExpansionInSystemHeader())
        ).bind("incDec"),
        this);

    finder.addMatcher(
        functionDecl(unless(isExpansionInSystemHeader())).bind("function"),
        this);

    finder.addMatcher(
        decl(
            hasAttr(clang::attr::Annotate),
            unless(isExpansionInSystemHeader())
        ).bind("taggedDecl"),
        this);
}

void ReferencePointerRule::run(const MatchFinder::MatchResult &result) {
    if (config.referencePointerRule.level == RuleLevel::Off)
        return;

    const SourceManager &sm = *result.SourceManager;

    if (const auto *call = result.Nodes.getNodeAs<CallExpr>("call")) {
        checkCall(call, sm);
        return;
    }

    if (const auto *assignment = result.Nodes.getNodeAs<BinaryOperator>("assignment")) {
        checkReassignment(assignment->getLHS(), assignment->getOperatorLoc(), sm);
        return;
    }

    if (const auto *incDec = result.Nodes.getNodeAs<UnaryOperator>("incDec")) {
        checkReassignment(incDec->getSubExpr(), incDec->getOperatorLoc(), sm);
        return;
    }

    if (const auto *tagged = result.Nodes.getNodeAs<Decl>("taggedDecl")) {
        checkTagPlacement(tagged, sm);
        return;
    }

    if (const auto *function = result.Nodes.getNodeAs<FunctionDecl>("function"))
        checkFunction(function, sm);
}
