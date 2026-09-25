#include "rule_enum.hpp"

#include <algorithm>

bool EnumRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool EnumRule::isIgnoredLocation(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    const SourceLocation spellingLoc = sm.getSpellingLoc(loc);
    const SourceLocation expansionLoc = sm.getExpansionLoc(loc);

    if (sm.isInSystemHeader(spellingLoc) ||
        sm.isInSystemHeader(expansionLoc))
        return true;

    const std::string spellingPath = sm.getFilename(spellingLoc).str();
    const std::string expansionPath = sm.getFilename(expansionLoc).str();

    if (!spellingPath.empty() && isThirdParty(spellingPath))
        return true;

    if (!expansionPath.empty() && isThirdParty(expansionPath))
        return true;

    return false;
}

void EnumRule::collectTypedefs(ASTContext &ctx) {
    if (typedefsCollected)
        return;

    typedefsCollected = true;

    for (const Decl *decl : ctx.getTranslationUnitDecl()->decls()) {
        const auto *typedefDecl = dyn_cast<TypedefNameDecl>(decl);

        if (!typedefDecl)
            continue;

        const auto *enumType =
            typedefDecl->getUnderlyingType()->getAs<EnumType>();

        if (!enumType)
            continue;

        enumsWithTypedef.insert(
            enumType->getDecl()->getCanonicalDecl());
    }
}

const EnumDecl *EnumRule::getEnumDecl(QualType type) const {
    const auto *enumType =
        type.getCanonicalType()->getAs<EnumType>();

    if (!enumType)
        return nullptr;

    return enumType->getDecl()->getCanonicalDecl();
}

bool EnumRule::isGovernedEnum(
    const EnumDecl *enumDecl,
    const SourceManager &sm) const
{
    if (!enumDecl)
        return false;

    if (!enumsWithTypedef.count(enumDecl))
        return false;

    return !isIgnoredLocation(sm, enumDecl->getLocation());
}

bool EnumRule::isEnumMember(
    const Expr *expr,
    const EnumDecl *target) const
{
    if (!expr)
        return false;

    // Implicit casts are stripped on purpose: an int silently converted
    // to the enum type must be judged by what it was before the cast.
    expr = expr->IgnoreParenImpCasts();

    // In C an enumerator has type int, so it has to be recognized
    // by its declaration rather than by its type.
    if (const auto *declRef = dyn_cast<DeclRefExpr>(expr)) {
        if (const auto *enumerator =
                dyn_cast<EnumConstantDecl>(declRef->getDecl()))
        {
            const auto *owner =
                dyn_cast<EnumDecl>(enumerator->getDeclContext());

            return owner && owner->getCanonicalDecl() == target;
        }
    }

    // A variable, function result or explicit cast that already has
    // the enum type.
    if (getEnumDecl(expr->getType()) == target)
        return true;

    if (const auto *conditional = dyn_cast<ConditionalOperator>(expr)) {
        return isEnumMember(conditional->getTrueExpr(), target) &&
               isEnumMember(conditional->getFalseExpr(), target);
    }

    return false;
}

std::string EnumRule::describe(const Expr *expr) const {
    if (!expr)
        return "<expression>";

    expr = expr->IgnoreParenImpCasts();

    if (const auto *declRef = dyn_cast<DeclRefExpr>(expr))
        return declRef->getDecl()->getNameAsString();

    if (const auto *member = dyn_cast<MemberExpr>(expr))
        return member->getMemberDecl()->getNameAsString();

    if (const auto *unary = dyn_cast<UnaryOperator>(expr)) {
        if (unary->getOpcode() == UO_Deref)
            return "*" + describe(unary->getSubExpr());
    }

    return "<expression>";
}

void EnumRule::reportAt(
    const SourceManager &sm,
    SourceLocation loc,
    const std::string &message)
{
    const SourceLocation expansionLoc = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansionLoc))
        return;

    diagnostics.report(
        config.enumRule.level,
        sm,
        expansionLoc,
        message
    );
}

EnumRule::EnumRule(const Config &cfg,
         SuppressionManager &sup,
         Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void EnumRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        enumDecl().bind("enum"),
        this
    );

    if (!config.enumRule.allowEnumTypedef)
        return;

    finder.addMatcher(
        varDecl(
            hasInitializer(expr()),
            unless(isExpansionInSystemHeader())
        ).bind("varInit"),
        this
    );

    finder.addMatcher(
        binaryOperator(
            isAssignmentOperator(),
            unless(isExpansionInSystemHeader())
        ).bind("assign"),
        this
    );

    finder.addMatcher(
        unaryOperator(
            anyOf(
                hasOperatorName("++"),
                hasOperatorName("--")
            ),
            unless(isExpansionInSystemHeader())
        ).bind("incDec"),
        this
    );

    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("call"),
        this
    );
}

void EnumRule::checkEnumDeclaration(
    const EnumDecl *e,
    const MatchFinder::MatchResult &result)
{
    auto &sm = *result.SourceManager;

    SourceLocation loc = e->getLocation();

    bool fromMacro = loc.isMacroID();

    // -------------------------
    // Resolve locations
    // -------------------------

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
    // File paths
    // -------------------------

    std::string spellingPath =
        sm.getFilename(spellingLoc).str();

    std::string expansionPath =
        sm.getFilename(expansionLoc).str();

    // -------------------------
    // Ignore third-party macro definitions
    // -------------------------

    if (!spellingPath.empty() &&
        isThirdParty(spellingPath))
    {
        return;
    }

    // -------------------------
    // Ignore third-party expansion sites
    // -------------------------

    if (!expansionPath.empty() &&
        isThirdParty(expansionPath))
    {
        return;
    }

    // -------------------------
    // Build message
    // -------------------------

    std::string name = e->getNameAsString();

    if (name.empty()) {
        if (const auto *typedefName = e->getTypedefNameForAnonDecl())
            name = typedefName->getNameAsString();
    }

    if (name.empty())
        name = "<anonymous>";

    std::string msg;

    if (!config.enumRule.allowEnumTypedef) {
        msg = "enum '" + name + "' is not allowed";
    }
    else {
        if (sm.isInSystemHeader(spellingLoc) ||
            sm.isInSystemHeader(expansionLoc))
            return;

        if (!e->isCompleteDefinition())
            return;

        collectTypedefs(*result.Context);

        if (enumsWithTypedef.count(e->getCanonicalDecl()))
            return;

        msg = "enum '" + name + "' must have a typedef";
    }

    if (fromMacro)
        msg += " (macro expansion)";

    // -------------------------
    // Report diagnostic
    // -------------------------

    diagnostics.report(
        config.enumRule.level,
        sm,
        expansionLoc,
        msg
    );
}

void EnumRule::checkVariableInit(
    const VarDecl *var,
    const MatchFinder::MatchResult &result)
{
    const auto &sm = *result.SourceManager;

    if (var->isImplicit())
        return;

    const Expr *init = var->getInit();

    if (!init || isa<InitListExpr>(init->IgnoreParenImpCasts()))
        return;

    collectTypedefs(*result.Context);

    const EnumDecl *target = getEnumDecl(var->getType());

    if (!isGovernedEnum(target, sm))
        return;

    if (isIgnoredLocation(sm, var->getLocation()))
        return;

    if (isEnumMember(init, target))
        return;

    reportAt(
        sm,
        init->getExprLoc(),
        "variable '" + var->getNameAsString() +
        "' of type '" + var->getType().getAsString() +
        "' must be initialized with a member of its enum or an explicit cast"
    );
}

void EnumRule::checkAssignment(
    const BinaryOperator *op,
    const MatchFinder::MatchResult &result)
{
    const auto &sm = *result.SourceManager;

    collectTypedefs(*result.Context);

    const Expr *lhs = op->getLHS();

    const EnumDecl *target = getEnumDecl(lhs->getType());

    if (!isGovernedEnum(target, sm))
        return;

    if (isIgnoredLocation(sm, op->getOperatorLoc()))
        return;

    if (op->getOpcode() == BO_Assign) {
        if (isEnumMember(op->getRHS(), target))
            return;

        reportAt(
            sm,
            op->getRHS()->getExprLoc(),
            "'" + describe(lhs) +
            "' of type '" + lhs->getType().getAsString() +
            "' must be assigned a member of its enum or an explicit cast"
        );

        return;
    }

    // Compound assignment (+=, |=, ...) can produce values that are not
    // members of the enum whatever the right hand side is.
    reportAt(
        sm,
        op->getOperatorLoc(),
        "'" + describe(lhs) +
        "' of type '" + lhs->getType().getAsString() +
        "' may not be modified with arithmetic, assign a member of its "
        "enum or an explicit cast"
    );
}

void EnumRule::checkIncrementDecrement(
    const UnaryOperator *op,
    const MatchFinder::MatchResult &result)
{
    const auto &sm = *result.SourceManager;

    collectTypedefs(*result.Context);

    const Expr *operand = op->getSubExpr();

    const EnumDecl *target = getEnumDecl(operand->getType());

    if (!isGovernedEnum(target, sm))
        return;

    if (isIgnoredLocation(sm, op->getOperatorLoc()))
        return;

    reportAt(
        sm,
        op->getOperatorLoc(),
        "'" + describe(operand) +
        "' of type '" + operand->getType().getAsString() +
        "' may not be modified with arithmetic, assign a member of its "
        "enum or an explicit cast"
    );
}

void EnumRule::checkCall(
    const CallExpr *call,
    const MatchFinder::MatchResult &result)
{
    const auto &sm = *result.SourceManager;

    const FunctionDecl *callee = call->getDirectCallee();

    if (!callee)
        return;

    collectTypedefs(*result.Context);

    const unsigned count =
        std::min(call->getNumArgs(), callee->getNumParams());

    for (unsigned i = 0; i < count; ++i) {
        const ParmVarDecl *param = callee->getParamDecl(i);
        const Expr *arg = call->getArg(i);

        const EnumDecl *target = getEnumDecl(param->getType());

        if (!isGovernedEnum(target, sm))
            continue;

        if (isIgnoredLocation(sm, arg->getExprLoc()))
            continue;

        if (isEnumMember(arg, target))
            continue;

        const std::string argName =
            param->getNameAsString().empty()
                ? "#" + std::to_string(i + 1)
                : "'" + param->getNameAsString() + "'";

        reportAt(
            sm,
            arg->getExprLoc(),
            "argument " + argName +
            " of type '" + param->getType().getAsString() +
            "' passed to function '" + callee->getNameAsString() +
            "' must be a member of its enum or an explicit cast"
        );
    }
}

void EnumRule::run(const MatchFinder::MatchResult &result) {
    if (config.enumRule.level == RuleLevel::Off)
        return;

    if (const auto *e =
            result.Nodes.getNodeAs<EnumDecl>("enum"))
    {
        checkEnumDeclaration(e, result);
        return;
    }

    if (!config.enumRule.allowEnumTypedef)
        return;

    if (const auto *var =
            result.Nodes.getNodeAs<VarDecl>("varInit"))
    {
        checkVariableInit(var, result);
        return;
    }

    if (const auto *op =
            result.Nodes.getNodeAs<BinaryOperator>("assign"))
    {
        checkAssignment(op, result);
        return;
    }

    if (const auto *op =
            result.Nodes.getNodeAs<UnaryOperator>("incDec"))
    {
        checkIncrementDecrement(op, result);
        return;
    }

    if (const auto *call =
            result.Nodes.getNodeAs<CallExpr>("call"))
    {
        checkCall(call, result);
        return;
    }
}
