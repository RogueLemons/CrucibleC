#include "rule_function_discard.hpp"

#include <clang/AST/ParentMapContext.h>

#include "function_pointer_tags.hpp"

bool FunctionDiscardRule::isThirdParty(const std::string &path) const
{
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool FunctionDiscardRule::shouldIgnore(
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

bool FunctionDiscardRule::isDiscarded(
    const CallExpr *call,
    ASTContext &context) const
{
    DynTypedNode current = DynTypedNode::create(*call);

    for (int depth = 0; depth < 64; ++depth) {
        const auto parents = context.getParents(current);

        if (parents.empty())
            return false;

        const DynTypedNode &parent = parents[0];
        const Stmt *currentStmt = current.get<Stmt>();
        const Stmt *parentStmt = parent.get<Stmt>();

        if (!parentStmt)
            return false;

        // An explicit cast to void is the opt-in spelling for discarding
        // a result. A non-void cast still leaves the value in use.
        if (const auto *cast = dyn_cast<CStyleCastExpr>(parentStmt)) {
            if (cast->getType()->isVoidType())
                return false;

            current = parent;
            continue;
        }

        if (isa<ParenExpr>(parentStmt) ||
            isa<ImplicitCastExpr>(parentStmt) ||
            isa<ExprWithCleanups>(parentStmt) ||
            isa<MaterializeTemporaryExpr>(parentStmt) ||
            isa<FullExpr>(parentStmt) ||
            isa<OpaqueValueExpr>(parentStmt))
        {
            current = parent;
            continue;
        }

        if (isa<ReturnStmt>(parentStmt))
            return false;

        if (const auto *ifStmt = dyn_cast<IfStmt>(parentStmt)) {
            if (ifStmt->getCond() == currentStmt)
                return false;
        }

        if (const auto *whileStmt = dyn_cast<WhileStmt>(parentStmt)) {
            if (whileStmt->getCond() == currentStmt)
                return false;
        }

        if (const auto *doStmt = dyn_cast<DoStmt>(parentStmt)) {
            if (doStmt->getCond() == currentStmt)
                return false;
        }

        if (const auto *forStmt = dyn_cast<ForStmt>(parentStmt)) {
            if (forStmt->getCond() == currentStmt)
                return false;
        }

        if (const auto *switchStmt = dyn_cast<SwitchStmt>(parentStmt)) {
            if (switchStmt->getCond() == currentStmt)
                return false;
        }

        if (const auto *binary = dyn_cast<BinaryOperator>(parentStmt)) {
            if (binary->getOpcode() == BO_Comma) {
                if (binary->getLHS() == currentStmt)
                    return true;

                // The right operand has the value of the comma expression;
                // continue upward to determine whether that value is used.
                current = parent;
                continue;
            }

            return false;
        }

        // Calls in sizeof/alignof and similar unevaluated contexts do not
        // actually discard a runtime result.
        if (isa<UnaryExprOrTypeTraitExpr>(parentStmt))
            return false;

        if (!isa<Expr>(parentStmt))
            return true;

        // Any other expression parent consumes the value: arguments,
        // assignments, conditions, member access, and so on.
        return false;
    }

    return false;
}

FunctionDiscardRule::FunctionDiscardRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void FunctionDiscardRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("functionDiscardCall"),
        this);
}

void FunctionDiscardRule::run(const MatchFinder::MatchResult &result)
{
    if (config.functionDiscardRule.level == RuleLevel::Off ||
        !result.Context ||
        !result.SourceManager)
    {
        return;
    }

    const auto *call = result.Nodes.getNodeAs<CallExpr>("functionDiscardCall");

    // Struct return values have dedicated ownership/discard diagnostics,
    // including the RAII-specific rules, so leave them to that subsystem.
    if (!call || call->getType()->isVoidType() ||
        call->getType()->isRecordType() ||
        !isDiscarded(call, *result.Context))
    {
        return;
    }

    const SourceManager &sm = *result.SourceManager;
    const SourceLocation loc = sm.getExpansionLoc(call->getExprLoc());

    if (shouldIgnore(sm, loc))
        return;

    diagnostics.report(
        config.functionDiscardRule.level,
        DiagCode::FunctionReturnDiscarded,
        sm,
        loc,
        "return value of function '" + callNameOf(call) +
            "' must not be discarded; cast it to void to explicitly discard it"
    );
}
