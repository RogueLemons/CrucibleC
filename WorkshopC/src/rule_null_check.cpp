#include "rule_null_check.hpp"

#include "reference_tag.hpp"

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

bool NullCheckRule::isNamedNullCheckCall(const Expr *expr,
                                         const ParmVarDecl *param) const {
    if (!expr)
        return false;

    const auto *call = dyn_cast<CallExpr>(expr->IgnoreParenImpCasts());

    if (!call)
        return false;

    const FunctionDecl *fd = call->getDirectCallee();

    if (!fd)
        return false;

    const std::string name = fd->getNameAsString();

    if (name.find("NULL") == std::string::npos &&
        name.find("null") == std::string::npos)
        return false;

    for (const Expr *arg : call->arguments()) {
        if (isParamRef(arg, param))
            return true;
    }

    return false;
}

bool NullCheckRule::isBooleanCheck(const Expr *expr,
                                   const ParmVarDecl *param) const {
    if (!expr || !config.nullCheckRule.allowDirectPtrInIfStatement)
        return false;

    expr = expr->IgnoreParenImpCasts();

    // !ptr, !!ptr (as produced by assert), ...
    while (const auto *un = dyn_cast<UnaryOperator>(expr)) {
        if (un->getOpcode() != UO_LNot)
            break;

        expr = un->getSubExpr()->IgnoreParenImpCasts();
    }

    return isParamRef(expr, param);
}

bool NullCheckRule::isConditionCheck(const Expr *cond,
                                     const ParmVarDecl *param) const {
    return isNullComparison(cond, param) ||
           isNamedNullCheckCall(cond, param) ||
           isBooleanCheck(cond, param);
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

    // Implicit dereference: param[index]
    if (const auto *subscript = dyn_cast<ArraySubscriptExpr>(expr))
        return isParamRef(subscript->getBase(), param);

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

    std::unordered_map<const ParmVarDecl*, ParamState> states;

    // A reference pointer always points to a real object, so it does not
    // need a null check when the reference pointer rule enforces that
    const bool skipReferencePointers =
        config.referencePointerRule.level != RuleLevel::Off &&
        config.referencePointerRule.disableNullCheckRuleForReferencePointers;

    for (const auto *p : fn->parameters()) {
        if (!isPointerParam(p))
            continue;

        if (skipReferencePointers && hasReferenceTag(p))
            continue;

        states[p] = {};
    }

    if (states.empty())
        return;

    // A null check only guards the scope it was made in and the scopes
    // nested inside it. Each entry holds the parameters checked in one
    // open scope, the innermost scope last.
    std::vector<std::unordered_set<const ParmVarDecl*>> scopes(1);

    auto isGuarded = [&](const ParmVarDecl *param) {
        for (const auto &scope : scopes) {
            if (scope.count(param))
                return true;
        }

        return false;
    };

    // A check made where a condition is evaluated (if, while, for, do,
    // ?:, && and ||) belongs to the scope that contains the condition,
    // and therefore also guards the branches and the code after it.
    auto markConditionChecks = [&](const Expr *cond) {
        if (!cond)
            return;

        for (auto &[param, st] : states) {
            if (isConditionCheck(cond, param))
                scopes.back().insert(param);
        }
    };

    // Statements coming from a macro expansion do not open scopes of
    // their own, so a check inside e.g. 'do { ... } while (0)' in a
    // macro counts for the scope where the macro is used.
    auto pushScope = [&](const Stmt *s) {
        if (!s || s->getBeginLoc().isMacroID())
            return false;

        scopes.emplace_back();
        return true;
    };

    std::function<void(const Stmt*)> walk;

    auto walkInScope = [&](const Stmt *s) {
        if (!s)
            return;

        const bool pushed = pushScope(s);

        walk(s);

        if (pushed)
            scopes.pop_back();
    };

    walk = [&](const Stmt *s) {
        if (!s)
            return;

        if (const auto *compound = dyn_cast<CompoundStmt>(s)) {
            const bool pushed = pushScope(compound);

            for (const Stmt *child : compound->body())
                walk(child);

            if (pushed)
                scopes.pop_back();

            return;
        }

        if (const auto *ifStmt = dyn_cast<IfStmt>(s)) {
            walk(ifStmt->getInit());
            walk(ifStmt->getConditionVariableDeclStmt());
            walk(ifStmt->getCond());
            markConditionChecks(ifStmt->getCond());
            walkInScope(ifStmt->getThen());
            walkInScope(ifStmt->getElse());
            return;
        }

        if (const auto *whileStmt = dyn_cast<WhileStmt>(s)) {
            walk(whileStmt->getConditionVariableDeclStmt());
            walk(whileStmt->getCond());
            markConditionChecks(whileStmt->getCond());
            walkInScope(whileStmt->getBody());
            return;
        }

        if (const auto *forStmt = dyn_cast<ForStmt>(s)) {
            const bool pushed = pushScope(forStmt);

            walk(forStmt->getInit());
            walk(forStmt->getConditionVariableDeclStmt());
            walk(forStmt->getCond());
            markConditionChecks(forStmt->getCond());
            walkInScope(forStmt->getBody());
            walk(forStmt->getInc());

            if (pushed)
                scopes.pop_back();

            return;
        }

        if (const auto *doStmt = dyn_cast<DoStmt>(s)) {
            walkInScope(doStmt->getBody());
            walk(doStmt->getCond());
            markConditionChecks(doStmt->getCond());
            return;
        }

        if (const auto *switchStmt = dyn_cast<SwitchStmt>(s)) {
            walk(switchStmt->getInit());
            walk(switchStmt->getConditionVariableDeclStmt());
            walk(switchStmt->getCond());
            walkInScope(switchStmt->getBody());
            return;
        }

        if (const auto *conditional = dyn_cast<ConditionalOperator>(s)) {
            walk(conditional->getCond());
            markConditionChecks(conditional->getCond());
            walk(conditional->getTrueExpr());
            walk(conditional->getFalseExpr());
            return;
        }

        if (const auto *binary = dyn_cast<BinaryOperator>(s)) {
            if (binary->isLogicalOp()) {
                walk(binary->getLHS());
                markConditionChecks(binary->getLHS());
                walk(binary->getRHS());
                markConditionChecks(binary->getRHS());
                return;
            }
        }

        if (const auto *expr = dyn_cast<Expr>(s)) {
            for (auto &[param, st] : states) {
                if (!st.violation &&
                    isDerefOfParam(expr, param) &&
                    !isGuarded(param))
                {
                    st.violation = expr;
                }

                // An explicit comparison with null or a null checking
                // function counts wherever it is, e.g. stored in a
                // variable that is then asserted.
                if (isNullComparison(expr, param) ||
                    isNamedNullCheckCall(expr, param))
                {
                    scopes.back().insert(param);
                }
            }
        }

        for (const Stmt *child : s->children())
            walk(child);
    };

    walk(fn->getBody());

    for (const auto &[param, st] : states) {
        if (!st.violation)
            continue;

        std::string name = param->getNameAsString();
        if (name.empty())
            name = "<unnamed>";

        diagnostics.report(
            config.nullCheckRule.level,
            DiagCode::DereferenceBeforeNullCheck,
            sm,
            st.violation->getBeginLoc(),
            "pointer parameter '" + name +
                "' is dereferenced before null check"
        );
    }
}
