#include "rule_arg_ptr_use_after_move.hpp"

#include <algorithm>

bool ArgumentPointerUseAfterMoveRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool ArgumentPointerUseAfterMoveRule::shouldIgnore(SourceLocation loc) const {
    if (!sourceManager || loc.isInvalid())
        return true;

    const SourceLocation expansion = sourceManager->getExpansionLoc(loc);

    if (suppressions.isSuppressed(*sourceManager, expansion))
        return true;

    if (sourceManager->isInSystemHeader(expansion))
        return true;

    const std::string path = sourceManager->getFilename(expansion).str();

    return !path.empty() && isThirdParty(path);
}

std::string ArgumentPointerUseAfterMoveRule::getParamTag(const ParmVarDecl *param) const {
    if (!param)
        return "";

    for (const auto *attr : param->attrs()) {
        if (const auto *annotate = dyn_cast<AnnotateAttr>(attr)) {
            const StringRef tag = annotate->getAnnotation();

            // Other annotations, e.g. the reference tag, can come first
            if (tag == kMoveTag || tag == kOutTag)
                return tag.str();
        }
    }

    return "";
}

bool ArgumentPointerUseAfterMoveRule::isTracked(const VarDecl *var) const {
    // Local pointer variables and pointer parameters. Globals and static
    // locals live on between calls, so they can not be followed here.
    return var &&
           var->getType()->isPointerType() &&
           !var->hasGlobalStorage();
}

const Expr *ArgumentPointerUseAfterMoveRule::stripWrappers(const Expr *expr) const {
    while (expr) {
        expr = expr->IgnoreParenCasts();

        const auto *call = dyn_cast<CallExpr>(expr);

        if (!call || call->getNumArgs() != 1)
            break;

        const FunctionDecl *callee = call->getDirectCallee();

        if (!callee)
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

const VarDecl *ArgumentPointerUseAfterMoveRule::getVariable(const Expr *expr) const {
    if (const auto *ref = dyn_cast_or_null<DeclRefExpr>(stripWrappers(expr)))
        return dyn_cast<VarDecl>(ref->getDecl());

    return nullptr;
}

ArgumentPointerUseAfterMoveRule::State ArgumentPointerUseAfterMoveRule::merge(
    const State &a,
    const State &b)
{
    State merged = a;

    for (const auto &[var, movedTo] : b)
        merged.emplace(var, movedTo);

    return merged;
}

void ArgumentPointerUseAfterMoveRule::reportUse(
    const DeclRefExpr *use,
    const std::string &movedTo)
{
    const SourceLocation loc = use->getLocation();

    if (shouldIgnore(loc))
        return;

    // Loop bodies are walked twice, report each use once
    if (!reported.insert(loc.getRawEncoding()).second)
        return;

    diagnostics.report(
        config.argumentPointerMovementRule.level,
        DiagCode::UseAfterMove,
        *sourceManager,
        sourceManager->getExpansionLoc(loc),
        "pointer '" + use->getDecl()->getNameAsString() +
        "' may have been moved to '" + movedTo +
        "' and can not be used until it is reassigned"
    );
}

void ArgumentPointerUseAfterMoveRule::walkCall(const CallExpr *call, State &state) {
    // Everything passed to the call is evaluated first
    walkExpr(call->getCallee(), state);

    for (const Expr *arg : call->arguments())
        walkExpr(arg, state);

    // A function, or a function pointer whose type gives the tags
    const CalleeParameters callee = calleeParametersOf(call);

    if (!callee.known)
        return;

    const unsigned count = std::min<unsigned>(
        call->getNumArgs(), callee.parameters.size());

    for (unsigned i = 0; i < count; ++i) {
        const std::string tag = getParamTag(callee.parameters[i]);
        const Expr *arg = stripWrappers(call->getArg(i));

        // Passing '&ptr' to an out parameter does not count as reassigning
        // it: the function may fail and return early without writing a
        // new value, so a moved pointer must be reassigned explicitly.
        if (tag == kMoveTag) {
            const VarDecl *var = getVariable(arg);

            if (isTracked(var))
                state[var] = callee.name;
        }
    }
}

void ArgumentPointerUseAfterMoveRule::walkExpr(const Expr *expr, State &state) {
    if (!expr)
        return;

    // sizeof and _Alignof do not evaluate their operand
    if (isa<UnaryExprOrTypeTraitExpr>(expr))
        return;

    if (const auto *ref = dyn_cast<DeclRefExpr>(expr)) {
        if (const auto *var = dyn_cast<VarDecl>(ref->getDecl())) {
            auto it = state.find(var);

            if (it != state.end())
                reportUse(ref, it->second);
        }

        return;
    }

    if (const auto *binary = dyn_cast<BinaryOperator>(expr)) {
        if (binary->getOpcode() == BO_Assign) {
            const auto *target =
                dyn_cast<DeclRefExpr>(binary->getLHS()->IgnoreParens());

            const auto *var =
                target ? dyn_cast<VarDecl>(target->getDecl()) : nullptr;

            if (isTracked(var)) {
                walkExpr(binary->getRHS(), state);
                state.erase(var);
                return;
            }
        }
    }

    if (const auto *call = dyn_cast<CallExpr>(expr)) {
        walkCall(call, state);
        return;
    }

    for (const Stmt *child : expr->children())
        walk(child, state);
}

ArgumentPointerUseAfterMoveRule::Flow ArgumentPointerUseAfterMoveRule::walkLoop(
    const Stmt *body,
    const Expr *cond,
    const Expr *inc,
    bool conditionFirst,
    State &state)
{
    State exitState;
    bool canExit = false;

    State entry = state;

    // The second pass starts from everything that can reach the top of
    // the loop again, so a move late in the body is seen by a use early
    // in the next iteration.
    for (int pass = 0; pass < 2; ++pass) {
        State current = entry;

        if (conditionFirst) {
            walkExpr(cond, current);

            // No condition, or a constant true one: for (;;), while (1)
            bool alwaysTrue = !cond;
            bool value = false;

            if (cond && cond->EvaluateAsBooleanCondition(value, *context))
                alwaysTrue = value;

            if (!alwaysTrue) {
                exitState = merge(exitState, current);
                canExit = true;
            }
        }

        jumpTargets.push_back({true, {}, {}});

        const Flow flow = walk(body, current);

        JumpTarget target = std::move(jumpTargets.back());
        jumpTargets.pop_back();

        State back;
        bool reachesBack = false;

        if (flow == Flow::Normal) {
            back = current;
            reachesBack = true;
        }

        for (const auto &continued : target.continues) {
            back = merge(back, continued);
            reachesBack = true;
        }

        if (reachesBack) {
            walkExpr(inc, back);

            if (!conditionFirst) {
                walkExpr(cond, back);
                exitState = merge(exitState, back);
                canExit = true;
            }

            entry = merge(entry, back);
        }

        for (const auto &broken : target.breaks) {
            exitState = merge(exitState, broken);
            canExit = true;
        }
    }

    if (!canExit)
        return Flow::Exit;

    state = exitState;
    return Flow::Normal;
}

ArgumentPointerUseAfterMoveRule::Flow ArgumentPointerUseAfterMoveRule::walkSwitch(
    const SwitchStmt *switchStmt,
    State &state)
{
    walk(switchStmt->getInit(), state);
    walk(switchStmt->getConditionVariableDeclStmt(), state);
    walkExpr(switchStmt->getCond(), state);

    const State head = state;

    bool hasDefault = false;

    for (const SwitchCase *sc = switchStmt->getSwitchCaseList();
         sc;
         sc = sc->getNextSwitchCase())
    {
        if (isa<DefaultStmt>(sc))
            hasDefault = true;
    }

    jumpTargets.push_back({false, {}, {}});

    State current = head;
    bool reachable = false;

    auto walkSection = [&](const Stmt *stmt) {
        // Each case label can be jumped to from the switch head
        while (const auto *label = dyn_cast_or_null<SwitchCase>(stmt)) {
            current = reachable ? merge(current, head) : head;
            reachable = true;
            stmt = label->getSubStmt();
        }

        if (!reachable)
            return;

        if (walk(stmt, current) != Flow::Normal)
            reachable = false;
    };

    if (const auto *block = dyn_cast_or_null<CompoundStmt>(switchStmt->getBody())) {
        for (const Stmt *child : block->body())
            walkSection(child);
    }
    else {
        walkSection(switchStmt->getBody());
    }

    JumpTarget target = std::move(jumpTargets.back());
    jumpTargets.pop_back();

    State after;
    bool reachesAfter = false;

    if (reachable) {
        after = current;
        reachesAfter = true;
    }

    for (const auto &broken : target.breaks) {
        after = merge(after, broken);
        reachesAfter = true;
    }

    if (!hasDefault) {
        after = merge(after, head);
        reachesAfter = true;
    }

    if (!reachesAfter)
        return Flow::Exit;

    state = after;
    return Flow::Normal;
}

ArgumentPointerUseAfterMoveRule::Flow ArgumentPointerUseAfterMoveRule::walk(
    const Stmt *stmt,
    State &state)
{
    if (!stmt)
        return Flow::Normal;

    if (const auto *expr = dyn_cast<Expr>(stmt)) {
        walkExpr(expr, state);
        return Flow::Normal;
    }

    if (const auto *block = dyn_cast<CompoundStmt>(stmt)) {
        for (const Stmt *child : block->body()) {
            const Flow flow = walk(child, state);

            if (flow != Flow::Normal)
                return flow;
        }

        return Flow::Normal;
    }

    if (const auto *declStmt = dyn_cast<DeclStmt>(stmt)) {
        for (const Decl *decl : declStmt->decls()) {
            if (const auto *var = dyn_cast<VarDecl>(decl)) {
                walkExpr(var->getInit(), state);

                // A declaration always starts out with a fresh value
                state.erase(var);
            }
        }

        return Flow::Normal;
    }

    if (const auto *ret = dyn_cast<ReturnStmt>(stmt)) {
        walkExpr(ret->getRetValue(), state);
        return Flow::Exit;
    }

    if (isa<GotoStmt>(stmt) || isa<IndirectGotoStmt>(stmt))
        return Flow::Exit;

    if (isa<BreakStmt>(stmt)) {
        if (!jumpTargets.empty())
            jumpTargets.back().breaks.push_back(state);

        return Flow::Break;
    }

    if (isa<ContinueStmt>(stmt)) {
        for (auto it = jumpTargets.rbegin(); it != jumpTargets.rend(); ++it) {
            if (it->isLoop) {
                it->continues.push_back(state);
                break;
            }
        }

        return Flow::Continue;
    }

    if (const auto *ifStmt = dyn_cast<IfStmt>(stmt)) {
        walk(ifStmt->getInit(), state);
        walk(ifStmt->getConditionVariableDeclStmt(), state);
        walkExpr(ifStmt->getCond(), state);

        State thenState = state;
        const Flow thenFlow = walk(ifStmt->getThen(), thenState);

        State elseState = state;
        const Flow elseFlow = walk(ifStmt->getElse(), elseState);

        const bool thenFalls = thenFlow == Flow::Normal;
        const bool elseFalls = elseFlow == Flow::Normal;

        if (thenFalls && elseFalls)
            state = merge(thenState, elseState);
        else if (thenFalls)
            state = thenState;
        else if (elseFalls)
            state = elseState;
        else
            return thenFlow == elseFlow ? thenFlow : Flow::Exit;

        return Flow::Normal;
    }

    if (const auto *whileStmt = dyn_cast<WhileStmt>(stmt)) {
        walk(whileStmt->getConditionVariableDeclStmt(), state);
        return walkLoop(whileStmt->getBody(), whileStmt->getCond(), nullptr, true, state);
    }

    if (const auto *forStmt = dyn_cast<ForStmt>(stmt)) {
        walk(forStmt->getInit(), state);
        return walkLoop(forStmt->getBody(), forStmt->getCond(), forStmt->getInc(), true, state);
    }

    if (const auto *doStmt = dyn_cast<DoStmt>(stmt))
        return walkLoop(doStmt->getBody(), doStmt->getCond(), nullptr, false, state);

    if (const auto *switchStmt = dyn_cast<SwitchStmt>(stmt))
        return walkSwitch(switchStmt, state);

    if (const auto *label = dyn_cast<LabelStmt>(stmt))
        return walk(label->getSubStmt(), state);

    if (const auto *attributed = dyn_cast<AttributedStmt>(stmt))
        return walk(attributed->getSubStmt(), state);

    for (const Stmt *child : stmt->children())
        walk(child, state);

    return Flow::Normal;
}

ArgumentPointerUseAfterMoveRule::ArgumentPointerUseAfterMoveRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void ArgumentPointerUseAfterMoveRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        functionDecl(
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("function"),
        this
    );
}

void ArgumentPointerUseAfterMoveRule::run(const MatchFinder::MatchResult &result) {
    if (config.argumentPointerMovementRule.level == RuleLevel::Off)
        return;

    const auto *fn = result.Nodes.getNodeAs<FunctionDecl>("function");

    if (!fn || !fn->doesThisDeclarationHaveABody())
        return;

    sourceManager = result.SourceManager;
    context = result.Context;
    jumpTargets.clear();
    reported.clear();

    if (shouldIgnore(fn->getLocation()))
        return;

    State state;
    walk(fn->getBody(), state);
}
