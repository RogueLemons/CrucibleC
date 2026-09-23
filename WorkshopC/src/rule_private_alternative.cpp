#include "rule_private_alternative.hpp"

bool PrivateAlternativeRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool PrivateAlternativeRule::isPrivateTagged(const FieldDecl *field) const {
    for (const auto *attr : field->attrs()) {
        if (const auto *A = dyn_cast<AnnotateAttr>(attr)) {
            if (A->getAnnotation() == kPrivateFieldTag)
                return true;
        }
    }

    return false;
}

const RecordDecl *PrivateAlternativeRule::resolveRecordDecl(QualType type) const {
    QualType canonical = type.getCanonicalType().getUnqualifiedType();

    const auto *recordType = canonical->getAs<RecordType>();

    return recordType ? recordType->getDecl() : nullptr;
}

bool PrivateAlternativeRule::hasValidSelfAccessor(
    const FunctionDecl *func,
    const RecordDecl *owner) const
{
    if (!func || !owner)
        return false;

    if (func->getNumParams() < 1)
        return false;

    const auto *self = func->getParamDecl(0);

    if (self->getNameAsString() != "self")
        return false;

    const auto *ptrType = self->getType()->getAs<PointerType>();

    if (!ptrType)
        return false;

    const RecordDecl *paramRecord =
        resolveRecordDecl(ptrType->getPointeeType());

    if (!paramRecord)
        return false;

    return paramRecord->getCanonicalDecl() == owner->getCanonicalDecl();
}

bool PrivateAlternativeRule::isDirectValueAccess(
    const MemberExpr *memberExpr,
    const RecordDecl *owner) const
{
    if (memberExpr->isArrow())
        return false;

    const Expr *base = memberExpr->getBase();

    if (!base)
        return false;

    base = base->IgnoreParenImpCasts();

    const auto *declRef = dyn_cast<DeclRefExpr>(base);

    if (!declRef)
        return false;

    const auto *varDecl = dyn_cast<VarDecl>(declRef->getDecl());

    if (!varDecl)
        return false;

    const RecordDecl *baseRecord = resolveRecordDecl(varDecl->getType());

    if (!baseRecord)
        return false;

    return baseRecord->getCanonicalDecl() == owner->getCanonicalDecl();
}

PrivateAlternativeRule::PrivateAlternativeRule(const Config &cfg,
                       SuppressionManager &sup,
                       Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void PrivateAlternativeRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        ast_matchers::memberExpr(
            hasAncestor(functionDecl().bind("parentFunction"))
        ).bind("privateAccess"),
        this
    );
}

void PrivateAlternativeRule::run(const MatchFinder::MatchResult &result) {
    const auto *memberExpr =
        result.Nodes.getNodeAs<MemberExpr>("privateAccess");

    if (!memberExpr)
        return;

    if (config.privateAlternativeRule.level == RuleLevel::Off)
        return;

    const auto *memberDecl = memberExpr->getMemberDecl();

    if (!memberDecl)
        return;

    const auto *fieldDecl = dyn_cast<FieldDecl>(memberDecl);

    if (!fieldDecl)
        return;

    if (!isPrivateTagged(fieldDecl))
        return;

    auto &sm = *result.SourceManager;

    SourceLocation loc = memberExpr->getExprLoc();

    SourceLocation spellingLoc = sm.getSpellingLoc(loc);
    SourceLocation expansionLoc = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansionLoc))
        return;

    std::string spellingPath = sm.getFilename(spellingLoc).str();
    std::string expansionPath = sm.getFilename(expansionLoc).str();

    if (!spellingPath.empty() && isThirdParty(spellingPath))
        return;

    if (!expansionPath.empty() && isThirdParty(expansionPath))
        return;

    const RecordDecl *owner = fieldDecl->getParent();

    if (!owner)
        return;

    std::string fieldName = fieldDecl->getNameAsString();
    std::string ownerName = owner->getNameAsString();

    const auto *func =
        result.Nodes.getNodeAs<FunctionDecl>("parentFunction");

    if (!func) {
        diagnostics.report(
            config.privateAlternativeRule.level,
            sm,
            expansionLoc,
            "direct access to private field '" +
            fieldName +
            "' is not allowed"
        );

        return;
    }

    std::string functionName = func->getNameAsString();

    bool nameOk = functionName.starts_with(ownerName);
    bool selfOk = hasValidSelfAccessor(func, owner) ||
                  isDirectValueAccess(memberExpr, owner);

    if (nameOk && selfOk)
        return;

    diagnostics.report(
        config.privateAlternativeRule.level,
        sm,
        expansionLoc,
        "private field '" +
        fieldName +
        "' may only be accessed from a function named starting with '" +
        ownerName +
        "' that takes a pointer to '" +
        ownerName +
        "' named 'self' as its first parameter"
    );
}
