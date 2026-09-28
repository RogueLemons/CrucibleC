#include "rule_null_check.hpp"

#include "reference_tag.hpp"

#include <algorithm>
#include <map>
#include <set>

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
    if (!bin || !bin->isEqualityOp())
        return false;

    const Expr *lhs = bin->getLHS()->IgnoreParenImpCasts();
    const Expr *rhs = bin->getRHS()->IgnoreParenImpCasts();

    return (isParamRef(lhs, param) && isNullLiteral(rhs)) ||
           (isParamRef(rhs, param) && isNullLiteral(lhs));
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


// =====================================================================
// Flow analysis
// =====================================================================

class NullCheckRule::FlowAnalyzer {
private:
    using ParamSet = std::set<const ParmVarDecl *>;

    // What is known at one point of the function: the parameters proven
    // to be non-null, or that the point can not be reached at all
    struct State {
        bool reachable = true;
        ParamSet nonNull;

        bool operator==(const State &other) const {
            return reachable == other.reachable && nonNull == other.nonNull;
        }
    };

    // The states after a condition, when it is true and when it is false
    struct Branches {
        State whenTrue;
        State whenFalse;
    };

    const NullCheckRule &rule;
    ASTContext &context;

    std::unordered_map<const ParmVarDecl *, ParamState> &params;

    // bool is_valid = p != NULL; the branches it stands for
    std::map<const VarDecl *, std::pair<ParamSet, ParamSet>> conditionVariables;

    // The states at the gotos to each label seen so far
    std::map<const LabelDecl *, State> gotoStates;
    std::set<const LabelDecl *> gotoTargets;

    // Where break and continue go: the states of all that reach them
    std::vector<State> breakStates;
    std::vector<State> continueStates;

    // The state before each switch, where every case label starts
    std::vector<State> switchEntries;

    static State unreachable() {
        State state;
        state.reachable = false;
        return state;
    }

    // Only what is known on both paths
    static State join(const State &a, const State &b) {
        if (!a.reachable)
            return b;

        if (!b.reachable)
            return a;

        State joined;
        std::set_intersection(
            a.nonNull.begin(), a.nonNull.end(),
            b.nonNull.begin(), b.nonNull.end(),
            std::inserter(joined.nonNull, joined.nonNull.begin()));

        return joined;
    }

    static State withNonNull(State state, const ParmVarDecl *param) {
        if (state.reachable)
            state.nonNull.insert(param);

        return state;
    }

    static State withNonNull(State state, const ParamSet &nonNull) {
        if (state.reachable)
            state.nonNull.insert(nonNull.begin(), nonNull.end());

        return state;
    }

    void collectGotoTargets(const Stmt *stmt) {
        if (!stmt)
            return;

        if (const auto *gotoStmt = dyn_cast<GotoStmt>(stmt))
            gotoTargets.insert(gotoStmt->getLabel());

        for (const Stmt *child : stmt->children())
            collectGotoTargets(child);
    }

    const ParmVarDecl *referencedParam(const Expr *expr) const {
        for (const auto &[param, state] : params) {
            if (rule.isParamRef(expr, param))
                return param;
        }

        return nullptr;
    }

    // abort(), exit(), the failure handler of assert, ...
    static bool isNoReturnCall(const CallExpr *call) {
        const FunctionDecl *callee = call->getDirectCallee();

        if (!callee)
            return false;

        if (callee->isNoReturn())
            return true;

        const std::string name = callee->getNameAsString();

        return name == "abort" || name == "exit" || name == "_Exit" ||
               name == "quick_exit" || name == "_assert" || name == "_wassert" ||
               name == "__assert_fail" || name == "__assert_rtn" ||
               name == "__assert";
    }

    void checkDeref(const Expr *expr, const State &state) {
        if (!state.reachable)
            return;

        for (auto &[param, paramState] : params) {
            if (!paramState.violation &&
                !state.nonNull.count(param) &&
                rule.isDerefOfParam(expr, param))
            {
                paramState.violation = expr;
            }
        }
    }

public:
    FlowAnalyzer(
        const NullCheckRule &rule,
        ASTContext &context,
        std::unordered_map<const ParmVarDecl *, ParamState> &params)
        : rule(rule),
          context(context),
          params(params)
    {}

    void analyze(const Stmt *body) {
        collectGotoTargets(body);
        statement(body, State{});
    }

    /*
     * Evaluates an expression for its value, reporting unchecked
     * dereferences, and returns the state after it.
     */
    State expression(const Expr *expr, State state) {
        if (!expr || !state.reachable)
            return state;

        // sizeof and _Alignof do not evaluate their operand
        if (isa<UnaryExprOrTypeTraitExpr>(expr))
            return state;

        if (const auto *binary = dyn_cast<BinaryOperator>(expr)) {
            if (binary->isLogicalOp()) {
                const Branches branches = condition(binary, state);
                return join(branches.whenTrue, branches.whenFalse);
            }

            if (binary->getOpcode() == BO_Comma)
                return expression(binary->getRHS(), expression(binary->getLHS(), state));

            if (binary->isAssignmentOp()) {
                state = expression(binary->getRHS(), state);
                state = expression(binary->getLHS(), state);

                // A parameter given a new value may be null again
                const Expr *target = binary->getLHS()->IgnoreParenImpCasts();

                if (const auto *ref = dyn_cast<DeclRefExpr>(target)) {
                    if (const auto *param = dyn_cast<ParmVarDecl>(ref->getDecl()))
                        state.nonNull.erase(param);

                    if (const auto *var = dyn_cast<VarDecl>(ref->getDecl()))
                        conditionVariables.erase(var);
                }

                return state;
            }
        }

        if (const auto *conditional = dyn_cast<ConditionalOperator>(expr)) {
            const Branches branches = condition(conditional->getCond(), state);

            return join(
                expression(conditional->getTrueExpr(), branches.whenTrue),
                expression(conditional->getFalseExpr(), branches.whenFalse));
        }

        if (const auto *call = dyn_cast<CallExpr>(expr)) {
            state = expression(call->getCallee(), state);

            for (const Expr *arg : call->arguments())
                state = expression(arg, state);

            return isNoReturnCall(call) ? unreachable() : state;
        }

        // ({ ... }), a statement used as an expression
        if (const auto *statementExpr = dyn_cast<StmtExpr>(expr))
            return statement(statementExpr->getSubStmt(), state);

        checkDeref(expr, state);

        for (const Stmt *child : expr->children()) {
            if (const auto *childExpr = dyn_cast_or_null<Expr>(child))
                state = expression(childExpr, state);
        }

        return state;
    }

    /*
     * Evaluates an expression used as a condition and returns the states
     * when it is true and when it is false.
     */
    Branches condition(const Expr *expr, State state) {
        if (!expr || !state.reachable)
            return {state, state};

        const Expr *stripped = expr->IgnoreParenImpCasts();

        // !condition
        if (const auto *unary = dyn_cast<UnaryOperator>(stripped)) {
            if (unary->getOpcode() == UO_LNot) {
                const Branches inner = condition(unary->getSubExpr(), state);
                return {inner.whenFalse, inner.whenTrue};
            }
        }

        if (const auto *binary = dyn_cast<BinaryOperator>(stripped)) {
            // The right side is only evaluated when the left side is true
            if (binary->getOpcode() == BO_LAnd) {
                const Branches left = condition(binary->getLHS(), state);
                const Branches right = condition(binary->getRHS(), left.whenTrue);

                return {right.whenTrue, join(left.whenFalse, right.whenFalse)};
            }

            // The right side is only evaluated when the left side is false
            if (binary->getOpcode() == BO_LOr) {
                const Branches left = condition(binary->getLHS(), state);
                const Branches right = condition(binary->getRHS(), left.whenFalse);

                return {join(left.whenTrue, right.whenTrue), right.whenFalse};
            }

            if (binary->getOpcode() == BO_Comma)
                return condition(binary->getRHS(), expression(binary->getLHS(), state));

            // param == NULL, param != NULL
            for (const auto &[param, paramState] : params) {
                if (!rule.isNullComparison(binary, param))
                    continue;

                state = expression(binary, state);

                return binary->getOpcode() == BO_EQ
                    ? Branches{state, withNonNull(state, param)}
                    : Branches{withNonNull(state, param), state};
            }
        }

        if (const auto *conditional = dyn_cast<ConditionalOperator>(stripped)) {
            const Branches choice = condition(conditional->getCond(), state);
            const Branches first = condition(conditional->getTrueExpr(), choice.whenTrue);
            const Branches second = condition(conditional->getFalseExpr(), choice.whenFalse);

            return {join(first.whenTrue, second.whenTrue),
                    join(first.whenFalse, second.whenFalse)};
        }

        if (const auto *ref = dyn_cast<DeclRefExpr>(stripped)) {
            // param, when allowed as a condition
            if (const ParmVarDecl *param = referencedParam(ref)) {
                if (rule.config.nullCheckRule.allowDirectPtrInIfStatement)
                    return {withNonNull(state, param), state};

                return {state, state};
            }

            // A variable holding a null check
            if (const auto *var = dyn_cast<VarDecl>(ref->getDecl())) {
                const auto it = conditionVariables.find(var);

                if (it != conditionVariables.end())
                    return {withNonNull(state, it->second.first),
                            withNonNull(state, it->second.second)};
            }
        }

        // while (1), for a loop that only ends with break
        Expr::EvalResult constant;

        if (!stripped->isValueDependent() &&
            stripped->EvaluateAsInt(constant, context))
        {
            return constant.Val.getInt().getBoolValue()
                ? Branches{state, unreachable()}
                : Branches{unreachable(), state};
        }

        state = expression(expr, state);
        return {state, state};
    }

    // Returns the state after the statement
    State statement(const Stmt *stmt, State state) {
        if (!stmt)
            return state;

        if (const auto *expr = dyn_cast<Expr>(stmt))
            return expression(expr, state);

        if (const auto *compound = dyn_cast<CompoundStmt>(stmt)) {
            for (const Stmt *child : compound->body()) {
                // Code after a jump is only reached again through a label
                const bool entry =
                    isa<LabelStmt>(child) || isa<SwitchCase>(child);

                if (!state.reachable && !entry)
                    continue;

                state = statement(child, state);
            }

            return state;
        }

        if (const auto *label = dyn_cast<LabelStmt>(stmt)) {
            const LabelDecl *decl = label->getDecl();
            const auto it = gotoStates.find(decl);

            State entry = it != gotoStates.end() ? join(state, it->second) : state;

            // Only reached through a goto further down, nothing is known
            if (!entry.reachable && gotoTargets.count(decl))
                entry = State{};

            return statement(label->getSubStmt(), entry);
        }

        if (const auto *switchCase = dyn_cast<SwitchCase>(stmt)) {
            const State entry = switchEntries.empty()
                ? state
                : join(state, switchEntries.back());

            return statement(switchCase->getSubStmt(), entry);
        }

        if (const auto *declStmt = dyn_cast<DeclStmt>(stmt)) {
            for (const Decl *decl : declStmt->decls()) {
                const auto *var = dyn_cast<VarDecl>(decl);

                if (!var || !var->getInit())
                    continue;

                // int valid = p != NULL; is remembered for 'if (valid)'
                if (var->getType()->isIntegerType()) {
                    const Branches branches = condition(var->getInit(), state);

                    conditionVariables[var] = {
                        branches.whenTrue.nonNull, branches.whenFalse.nonNull};

                    state = join(branches.whenTrue, branches.whenFalse);
                }
                else {
                    state = expression(var->getInit(), state);
                }
            }

            return state;
        }

        if (const auto *returnStmt = dyn_cast<ReturnStmt>(stmt)) {
            expression(returnStmt->getRetValue(), state);
            return unreachable();
        }

        if (isa<BreakStmt>(stmt)) {
            if (!breakStates.empty())
                breakStates.back() = join(breakStates.back(), state);

            return unreachable();
        }

        if (isa<ContinueStmt>(stmt)) {
            if (!continueStates.empty())
                continueStates.back() = join(continueStates.back(), state);

            return unreachable();
        }

        if (const auto *gotoStmt = dyn_cast<GotoStmt>(stmt)) {
            const LabelDecl *label = gotoStmt->getLabel();
            const auto it = gotoStates.find(label);

            gotoStates[label] = it != gotoStates.end() ? join(it->second, state) : state;

            return unreachable();
        }

        if (const auto *ifStmt = dyn_cast<IfStmt>(stmt)) {
            state = statement(ifStmt->getInit(), state);
            state = statement(ifStmt->getConditionVariableDeclStmt(), state);

            const Branches branches = condition(ifStmt->getCond(), state);

            return join(
                statement(ifStmt->getThen(), branches.whenTrue),
                ifStmt->getElse()
                    ? statement(ifStmt->getElse(), branches.whenFalse)
                    : branches.whenFalse);
        }

        if (const auto *whileStmt = dyn_cast<WhileStmt>(stmt))
            return loop(whileStmt->getConditionVariableDeclStmt(),
                        whileStmt->getCond(), whileStmt->getBody(), nullptr,
                        false, state);

        if (const auto *forStmt = dyn_cast<ForStmt>(stmt)) {
            state = statement(forStmt->getInit(), state);

            return loop(forStmt->getConditionVariableDeclStmt(),
                        forStmt->getCond(), forStmt->getBody(), forStmt->getInc(),
                        false, state);
        }

        if (const auto *doStmt = dyn_cast<DoStmt>(stmt))
            return loop(nullptr, doStmt->getCond(), doStmt->getBody(),
                        nullptr, true, state);

        if (const auto *switchStmt = dyn_cast<SwitchStmt>(stmt)) {
            state = statement(switchStmt->getInit(), state);
            state = statement(switchStmt->getConditionVariableDeclStmt(), state);
            state = expression(switchStmt->getCond(), state);

            bool hasDefault = false;

            for (const SwitchCase *c = switchStmt->getSwitchCaseList(); c; c = c->getNextSwitchCase()) {
                if (isa<DefaultStmt>(c))
                    hasDefault = true;
            }

            switchEntries.push_back(state);
            breakStates.push_back(unreachable());

            // The body is only entered through its case labels
            State after = statement(switchStmt->getBody(), unreachable());
            after = join(after, breakStates.back());

            breakStates.pop_back();
            switchEntries.pop_back();

            // Without a default, no case may match at all
            return hasDefault ? after : join(after, state);
        }

        if (const auto *attributed = dyn_cast<AttributedStmt>(stmt))
            return statement(attributed->getSubStmt(), state);

        return state;
    }

    /*
     * A loop is walked until what is known at its start no longer
     * changes: a dereference in the body must be safe in every iteration.
     */
    State loop(
        const Stmt *conditionVariable,
        const Expr *cond,
        const Stmt *body,
        const Expr *increment,
        bool bodyFirst,
        State state)
    {
        State head = state;
        State after = unreachable();

        for (int iteration = 0; iteration < 16; ++iteration) {
            breakStates.push_back(unreachable());
            continueStates.push_back(unreachable());

            State next;

            if (bodyFirst) {
                // do { body } while (cond);
                State end = statement(body, head);
                end = join(end, continueStates.back());

                const Branches branches = condition(cond, end);

                after = join(branches.whenFalse, breakStates.back());
                next = join(state, branches.whenTrue);
            }
            else {
                State checked = statement(conditionVariable, head);

                const Branches branches = cond
                    ? condition(cond, checked)
                    : Branches{checked, unreachable()};

                State end = statement(body, branches.whenTrue);
                end = join(end, continueStates.back());
                end = expression(increment, end);

                after = join(branches.whenFalse, breakStates.back());
                next = join(head, end);
            }

            breakStates.pop_back();
            continueStates.pop_back();

            if (next == head)
                break;

            head = next;
        }

        return after;
    }
};

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

    FlowAnalyzer analyzer(*this, *result.Context, states);
    analyzer.analyze(fn->getBody());

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
