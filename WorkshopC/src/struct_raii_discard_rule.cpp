#include "struct_raii_discard_rule.hpp"

#include <clang/AST/ParentMapContext.h>

bool StructRaiiDiscardRule::isThirdParty(const std::string &file) const
{
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && file.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool StructRaiiDiscardRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    if (suppressions.isSuppressed(sm, loc))
        return true;

    const SourceLocation spelling = sm.getSpellingLoc(loc);

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string file = sm.getFilename(spelling).str();

    if (file.empty())
        return true;

    return isThirdParty(file);
}

std::string StructRaiiDiscardRule::getReturnedStructName(const CallExpr *call) const
{
    const QualType type = call->getType().getCanonicalType();

    const auto *recordType = type->getAs<RecordType>();

    if (!recordType)
        return "";

    const RecordDecl *record = recordType->getDecl();

    if (!record || !record->isStruct())
        return "";

    return record->getNameAsString();
}

std::string StructRaiiDiscardRule::getCalleeName(const CallExpr *call) const
{
    if (const FunctionDecl *callee = call->getDirectCallee())
        return callee->getNameAsString();

    return "<function pointer>";
}

StructRaiiDiscardRule::Use StructRaiiDiscardRule::classifyUse(
    const CallExpr *call,
    ASTContext &context,
    std::string *memberName) const
{
    DynTypedNode current = DynTypedNode::create(*call);

    for (int depth = 0; depth < 64; ++depth) {
        const auto parents = context.getParents(current);

        if (parents.empty())
            return Use::Owned;

        const DynTypedNode &parent = parents[0];
        const Stmt *currentStmt = current.get<Stmt>();

        // Initializing a variable
        if (parent.get<VarDecl>())
            return Use::Owned;

        if (parent.get<Decl>())
            return Use::Owned;

        const Stmt *parentStmt = parent.get<Stmt>();

        if (!parentStmt)
            return Use::Owned;

        // Ownership moves to the caller
        if (isa<ReturnStmt>(parentStmt))
            return Use::Owned;

        if (!isa<Expr>(parentStmt)) {
            // An expression statement: its value is thrown away
            return Use::Discarded;
        }

        // Transparent wrappers around the value. Clang wraps a struct
        // returned by value in a MaterializeTemporaryExpr when a member
        // is accessed on it, even in C.
        if (isa<ParenExpr>(parentStmt) ||
            isa<ImplicitCastExpr>(parentStmt) ||
            isa<MaterializeTemporaryExpr>(parentStmt) ||
            isa<FullExpr>(parentStmt) ||
            isa<ConditionalOperator>(parentStmt) ||
            isa<BinaryConditionalOperator>(parentStmt) ||
            isa<ChooseExpr>(parentStmt) ||
            isa<GenericSelectionExpr>(parentStmt) ||
            isa<OpaqueValueExpr>(parentStmt))
        {
            current = parent;
            continue;
        }

        if (const auto *cast = dyn_cast<CStyleCastExpr>(parentStmt)) {
            if (cast->getType()->isVoidType())
                return Use::VoidCast;

            current = parent;
            continue;
        }

        if (const auto *member = dyn_cast<MemberExpr>(parentStmt)) {
            if (memberName)
                *memberName = member->getMemberDecl()->getNameAsString();

            return Use::MemberAccess;
        }

        if (const auto *binary = dyn_cast<BinaryOperator>(parentStmt)) {
            if (binary->getOpcode() == BO_Comma) {
                // The left side of a comma is evaluated and dropped
                if (binary->getLHS() == currentStmt)
                    return Use::Discarded;

                current = parent;
                continue;
            }

            // Assigned to something
            return Use::Owned;
        }

        // Passed as an argument, placed in an initializer list or
        // compound literal, or used in an unevaluated context such as
        // sizeof. None of them leave the value without an owner.
        return Use::Owned;
    }

    return Use::Owned;
}

StructRaiiDiscardRule::StructRaiiDiscardRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag,
    StructDatabase &db)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag),
      database(db)
{
}

void StructRaiiDiscardRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("call"),
        this);
}

void StructRaiiDiscardRule::run(const MatchFinder::MatchResult &result)
{
    sourceManager = result.SourceManager;

    const auto *call = result.Nodes.getNodeAs<CallExpr>("call");

    if (!call || !result.Context)
        return;

    // Only calls returning a struct by value can be affected. Whether
    // that struct is a raii struct is only known once the struct
    // database is complete, so the check happens in finalize().
    if (getReturnedStructName(call).empty())
        return;

    PendingCall pending;
    pending.call = call;
    pending.use = classifyUse(call, *result.Context, &pending.memberName);

    if (pending.use != Use::Owned)
        pendingCalls.push_back(pending);
}

void StructRaiiDiscardRule::finalize()
{
    if (!sourceManager)
        return;

    for (const auto &pending : pendingCalls) {
        const std::string structName =
            getReturnedStructName(pending.call);

        const auto *info = database.find(structName);

        if (!info || info->kind != StructDatabase::Kind::Raii)
            continue;

        const SourceLocation loc = pending.call->getExprLoc();

        if (shouldIgnore(*sourceManager, loc))
            continue;

        const std::string callee = getCalleeName(pending.call);

        std::string message;

        switch (pending.use) {
        case Use::MemberAccess:
            message =
                "member '" + pending.memberName +
                "' may not be accessed directly on the raii struct '" +
                structName + "' returned by '" + callee +
                "', assign it to a variable first so that it can be destroyed";
            break;

        case Use::VoidCast:
        case Use::Discarded:
            message =
                "raii struct '" + structName + "' returned by '" + callee +
                "' may not be discarded" +
                (pending.use == Use::VoidCast ? " (not even with a void cast)" : "") +
                ", assign it to a variable so that it can be destroyed";
            break;

        case Use::Owned:
            continue;
        }

        diagnostics.report(
            config.structResourceManagementRule.level,
            *sourceManager,
            loc,
            message);
    }
}
