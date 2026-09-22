#include "rule_null_check.hpp"

#include <functional>

bool NullCheckRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }
    return false;
}

bool NullCheckRule::isPointerParam(const ParmVarDecl *p) const {
    return p && p->getType()->isPointerType();
}

bool NullCheckRule::isParamRef(const Expr *expr,
                const ParmVarDecl *param) const {
    if (!expr)
        return false;

    expr = expr->IgnoreParenImpCasts();

    if (const auto *dr = dyn_cast<DeclRefExpr>(expr))
        return dr->getDecl() == param;

    return false;
}

bool NullCheckRule::isNullLiteral(const Expr *expr) const {
    if (!expr)
        return false;

    expr = expr->IgnoreParenImpCasts();

    if (isa<CXXNullPtrLiteralExpr>(expr))
        return true;

    if (const auto *i = dyn_cast<IntegerLiteral>(expr))
        return i->getValue() == 0;

    if (const auto *cast = dyn_cast<CastExpr>(expr))
        return isNullLiteral(cast->getSubExpr());

    return false;
}

bool NullCheckRule::isNullComparison(const Expr *expr,
                      const ParmVarDecl *param) const {
    if (!expr)
        return false;

    expr = expr->IgnoreParenImpCasts();

    const auto *bin = dyn_cast<BinaryOperator>(expr);
    if (!bin || !bin->isComparisonOp())
        return false;

    const Expr *lhs = bin->getLHS()->IgnoreParenImpCasts();
    const Expr *rhs = bin->getRHS()->IgnoreParenImpCasts();

    return (isParamRef(lhs, param) && isNullLiteral(rhs)) ||
           (isParamRef(rhs, param) && isNullLiteral(lhs));
}

bool NullCheckRule::isMacroNullCheck(const Expr *expr) const {
    if (!expr)
        return false;

    if (const auto *call = dyn_cast<CallExpr>(expr)) {
        if (const FunctionDecl *fd = call->getDirectCallee()) {
            std::string name = fd->getNameAsString();

            return name.find("NULL") != std::string::npos ||
                   name.find("null") != std::string::npos;
        }
    }

    return false;
}

bool NullCheckRule::isAllowedBooleanUse(const Expr *expr,
                         const ParmVarDecl *param,
                         bool allowBool) const {
    if (!allowBool)
        return false;

    if (!expr)
        return false;

    expr = expr->IgnoreParenImpCasts();

    // if (ptr)
    if (isParamRef(expr, param))
        return true;

    // if (!ptr)
    if (const auto *un = dyn_cast<UnaryOperator>(expr)) {
        if (un->getOpcode() == UO_LNot &&
            isParamRef(un->getSubExpr(), param)) {
            return true;
        }
    }

    // ptr ? a : b
    if (const auto *cond = dyn_cast<ConditionalOperator>(expr)) {
        const Expr *c = cond->getCond()->IgnoreParenImpCasts();

        if (isParamRef(c, param))
            return true;

        if (const auto *un = dyn_cast<UnaryOperator>(c)) {
            if (un->getOpcode() == UO_LNot &&
                isParamRef(un->getSubExpr(), param)) {
                return true;
            }
        }
    }

    return false;
}

bool NullCheckRule::isNullGuard(const Expr *expr,
                 const ParmVarDecl *param) const {
    return isNullComparison(expr, param) ||
           isMacroNullCheck(expr);
}

bool NullCheckRule::isDerefOfParam(
    const Expr *expr,
    const ParmVarDecl *param
) const {
    if (!expr)
        return false;

    expr = expr->IgnoreParenImpCasts();

    // Explicit: *param
    if (const auto *un = dyn_cast<UnaryOperator>(expr)) {
        if (un->getOpcode() == UO_Deref)
            return isParamRef(un->getSubExpr(), param);
    }

    // Implicit dereference: param->member
    if (const auto *member = dyn_cast<MemberExpr>(expr)) {
        if (member->isArrow())
            return isParamRef(member->getBase(), param);
    }

    return false;
}

NullCheckRule::NullCheckRule(const Config &cfg,
              SuppressionManager &sup,
              Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag) {}

void NullCheckRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        functionDecl(
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("function"),
        this
    );
}

void NullCheckRule::run(const MatchFinder::MatchResult &result) {
    const auto *fn =
        result.Nodes.getNodeAs<FunctionDecl>("function");

    if (!fn || !fn->hasBody())
        return;

    auto &sm = *result.SourceManager;

    SourceLocation loc = fn->getLocation();
    SourceLocation expansionLoc = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansionLoc))
        return;

    std::string path = sm.getFilename(expansionLoc).str();
    if (!path.empty() && isThirdParty(path))
        return;

    const bool allowBool =
        config.nullCheckRule.allowDirectPtrInIfStatement;

    std::unordered_map<const ParmVarDecl*, ParamState> states;

    for (const auto *p : fn->parameters()) {
        if (isPointerParam(p))
            states[p] = {};
    }

    if (states.empty())
        return;

    const Stmt *body = fn->getBody();

    std::function<bool(const Stmt*)> walk =
        [&](const Stmt *s) -> bool {
            if (!s)
                return true;

            const Expr *expr = dyn_cast<Expr>(s);

            for (auto &[param, st] : states) {
                if (!param)
                    continue;

                if (!st.seenGuard &&
                    expr &&
                    isNullGuard(expr, param)) {
                    st.seenGuard = true;
                }

                if (expr &&
                    isAllowedBooleanUse(expr, param, allowBool)) {
                    st.seenGuard = true;
                }

                if (expr &&
                    isDerefOfParam(expr, param)) {

                    if (!st.seenGuard) {
                        st.violation = expr;
                        return false;
                    }
                }
            }

            for (const Stmt *c : s->children()) {
                if (!walk(c))
                    return false;
            }

            return true;
        };

    walk(body);

    for (const auto &[param, st] : states) {
        if (!st.violation)
            continue;

        std::string name = param->getNameAsString();
        if (name.empty())
            name = "<unnamed>";

        diagnostics.report(
            config.nullCheckRule.level,
            sm,
            st.violation->getBeginLoc(),
            "pointer parameter '" + name +
                "' is dereferenced before null check"
        );
    }
}
