#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/StmtCXX.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include "config.hpp"
#include "diagnostics.hpp"
#include "struct_database.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class StructCleanupRule : public MatchFinder::MatchCallback {
private:
    struct TrackedVar {
        const VarDecl *decl = nullptr;
        std::string structName;
        bool destroyed = false;
    };

    struct ScopeState {
        std::vector<TrackedVar> vars;
        std::unordered_set<const VarDecl *> destroyed;
    };

    /*
     * A variable may legitimately produce more than one diagnostic
     * when it reaches multiple different bad exits.
     *
     * Therefore a report is identified by both:
     *
     *     - the variable
     *     - the source location of the exit
     */
    struct ReportKey {
        const VarDecl *decl = nullptr;
        unsigned location = 0;

        bool operator==(const ReportKey &other) const
        {
            return decl == other.decl &&
                   location == other.location;
        }
    };

    struct ReportKeyHash {
        std::size_t operator()(const ReportKey &key) const
        {
            const std::size_t declHash =
                std::hash<const VarDecl *>{}(key.decl);

            const std::size_t locationHash =
                std::hash<unsigned>{}(key.location);

            return declHash ^
                   (locationHash +
                    0x9e3779b9 +
                    (declHash << 6) +
                    (declHash >> 2));
        }
    };

    class CleanupAnalyzer {
    private:
        enum class Flow {
            Normal,
            Return,
            Break,
            Continue
        };

        struct StateSnapshot {
            std::vector<std::vector<bool>> scopeDestroyed;
            std::vector<bool> paramDestroyed;
        };

        StructCleanupRule &owner;
        const FunctionDecl *function = nullptr;

        std::vector<ScopeState> scopes;
        std::vector<TrackedVar> params;

    /*
     * IMPORTANT:
     *
     * Do not use a global "hasNormalExit" flag here.
     *
     * A return inside one branch does not mean that the whole
     * function has no normal exit.
         *
     * Example:
     *
     *     if (condition)
     *         return;
     *
     *     // normal path still exists
     *
     * The final Flow returned by scanStmt(functionBody) is
     * what tells us whether the function as a whole can reach
     * its closing brace.
         */

        std::unordered_set<ReportKey, ReportKeyHash> reportedVars;

        /*
         * Each entry is the index in 'scopes' belonging to the
         * corresponding active loop.
         */
        std::vector<size_t> loopScopeStack;

        /*
         * Switch statements are break targets too.
         */
        std::vector<size_t> switchScopeStack;

    public:
        CleanupAnalyzer(
            StructCleanupRule &owner,
            const FunctionDecl *function)
            : owner(owner),
              function(function)
        {
        }

        Flow scanStmt(const Stmt *stmt)
        {
            if (!stmt)
                return Flow::Normal;

            /*
             * A compound statement is a lexical scope.
             *
             * If normal execution reaches the closing brace,
             * variables belonging to this scope must have been
             * explicitly destroyed.
             *
             * Abnormal control flow is handled by the actual jump
             * statement itself.
             */
            if (const auto *compound =
                    dyn_cast<CompoundStmt>(stmt)) {

                scopes.emplace_back();

                Flow flow = Flow::Normal;

                for (const Stmt *child :
                     compound->body()) {

                    if (!child)
                        continue;

                    flow = scanStmt(child);

                    if (flow != Flow::Normal)
                        break;
                }

                if (flow == Flow::Normal) {

                    owner.reportPendingVarsForScopeExit(
                        compound->getEndLoc(),
                        scopes.back(),
                        reportedVars);
                }

                scopes.pop_back();

                return flow;
            }

            /*
             * Explicit return.
             *
             * Return exits every currently active lexical scope.
             * Therefore every active local variable, plus every
             * tracked parameter, must already have been destroyed.
             */
            if (const auto *returnStmt =
                    dyn_cast<ReturnStmt>(stmt)) {

                /*
                 * Scan the return expression first so that a destroy
                 * call appearing inside the expression is recognized.
                 */
                for (auto it = returnStmt->child_begin(),
                          end = returnStmt->child_end();
                     it != end;
                     ++it) {

                    if (*it)
                        scanStmt(*it);
                }

                owner.reportPendingVarsForExit(
                    returnStmt->getEndLoc(),
                    scopes,
                    params,
                    reportedVars);

                return Flow::Return;
            }

            /*
             * break exits the nearest enclosing loop or switch.
             */
            if (isa<BreakStmt>(stmt)) {

                reportBreakCleanup(
                    stmt->getEndLoc());

                return Flow::Break;
            }

            /*
             * continue exits the current iteration but remains
             * inside the loop's controlling scope.
             */
            if (isa<ContinueStmt>(stmt)) {

                reportContinueCleanup(
                    stmt->getEndLoc());

                return Flow::Continue;
            }

            /*
             * Local RAII struct declarations.
             */
            if (const auto *declStmt =
                    dyn_cast<DeclStmt>(stmt)) {

                for (const Decl *decl :
                     declStmt->decls()) {

                    const auto *var =
                        dyn_cast<VarDecl>(decl);

                    if (var)
                        trackVar(var);
                }

                /*
                 * Also scan initializers because a destroy call can
                 * technically occur inside an initializer expression.
                 */
                for (auto it = declStmt->child_begin(),
                          end = declStmt->child_end();
                     it != end;
                     ++it) {

                    if (*it)
                        scanStmt(*it);
                }

                return Flow::Normal;
            }

            /*
             * if statement.
             *
             * Each branch is analyzed independently.
             */
            if (const auto *ifStmt =
                    dyn_cast<IfStmt>(stmt)) {

                /*
                 * The init statement and condition are evaluated
                 * before either branch.
                 */
                if (const Stmt *init =
                        ifStmt->getInit())
                    scanStmt(init);

                if (const Stmt *condVar =
                        ifStmt->getConditionVariableDeclStmt())
                    scanStmt(condVar);

                if (const Expr *cond =
                        ifStmt->getCond())
                    scanStmt(cond);

                const StateSnapshot before =
                    captureState();

                /*
                 * THEN branch.
                 */
                Flow thenFlow = Flow::Normal;

                if (const Stmt *thenStmt =
                        ifStmt->getThen()) {

                    thenFlow =
                        scanStmt(thenStmt);
                }

                StateSnapshot thenState =
                    captureState();

                /*
                 * Restore state before THEN before analyzing ELSE.
                 */
                restoreState(before);

                /*
                 * ELSE branch.
                 *
                 * With no else branch, the implicit else path is
                 * simply the original state.
                 */
                Flow elseFlow = Flow::Normal;

                StateSnapshot elseState =
                    before;

                if (const Stmt *elseStmt =
                        ifStmt->getElse()) {

                    elseFlow =
                        scanStmt(elseStmt);

                    elseState =
                        captureState();
                }

                const bool thenNormal =
                    thenFlow == Flow::Normal;

                const bool elseNormal =
                    elseFlow == Flow::Normal;

                /*
                 * Both branches reach the following statement.
                 *
                 * A variable is definitely destroyed only if both
                 * paths destroyed it.
                 */
                if (thenNormal && elseNormal) {

                    restoreState(
                        mergeStates(
                            thenState,
                            elseState));

                    return Flow::Normal;
                }

                /*
                 * Only THEN reaches following code.
                 *
                 * ELSE exited through return/break/continue.
                 */
                if (thenNormal && !elseNormal) {

                    restoreState(
                        thenState);

                    return Flow::Normal;
                }

                /*
                 * Only ELSE reaches following code.
                 *
                 * THEN exited through return/break/continue.
                 */
                if (!thenNormal && elseNormal) {

                    restoreState(
                        elseState);

                    return Flow::Normal;
                }

                /*
                 * Neither branch reaches following code.
                 */
                restoreState(before);

                /*
             * Both branches exit abnormally.
                 *
             * If they use the same kind of exit, preserve that
             * flow. Otherwise treat the entire if as abnormal.
                 */
            if (thenFlow == elseFlow)
                return thenFlow;

                return Flow::Return;
            }

            /*
             * for-loop.
             */
            if (const auto *forStmt =
                    dyn_cast<ForStmt>(stmt)) {

                const StateSnapshot beforeLoop =
                    captureState();

                scopes.emplace_back();

                const size_t loopScopeIndex =
                    scopes.size() - 1;

                loopScopeStack.push_back(
                    loopScopeIndex);

                /*
                 * for initializer.
                 */
                if (const Stmt *init =
                        forStmt->getInit())
                    scanStmt(init);

                /*
                 * Optional condition variable.
                 */
                if (const Stmt *condVar =
                        forStmt->getConditionVariableDeclStmt())
                    scanStmt(condVar);

                /*
                 * Condition.
                 */
                if (const Expr *cond =
                        forStmt->getCond())
                    scanStmt(cond);

                /*
                 * Analyze one representative loop execution.
                 */
                Flow bodyFlow = Flow::Normal;

                if (const Stmt *body =
                        forStmt->getBody()) {

                    bodyFlow =
                        scanStmt(body);
                }

                /*
             * A continue executes the increment expression.
                 */
                if (bodyFlow == Flow::Continue) {

                    if (const Expr *inc =
                            forStmt->getInc())
                        scanStmt(inc);

                    bodyFlow =
                        Flow::Normal;
                }
                else if (bodyFlow == Flow::Normal) {

                    if (const Expr *inc =
                            forStmt->getInc())
                        scanStmt(inc);
                }

                /*
                 * The loop scope itself is exited when the loop
                 * terminates normally.
                 */
                if (bodyFlow == Flow::Normal) {

                    owner.reportPendingVarsForScopeExit(
                        forStmt->getEndLoc(),
                        scopes.back(),
                        reportedVars);
                }

                loopScopeStack.pop_back();

                scopes.pop_back();

                /*
                 * A loop may execute zero times, so destruction
                 * performed inside the loop cannot be considered
                 * to have happened after the loop.
                 */
                restoreState(beforeLoop);

                /*
                 * A return from the loop body exits the function.
                 */
                if (bodyFlow == Flow::Return)
                    return Flow::Return;

                /*
                 * break and continue are consumed by this loop.
                 */
                return Flow::Normal;
            }

            /*
             * while-loop.
             */
            if (const auto *whileStmt =
                    dyn_cast<WhileStmt>(stmt)) {

                const StateSnapshot beforeLoop =
                    captureState();

                scopes.emplace_back();

                const size_t loopScopeIndex =
                    scopes.size() - 1;

                loopScopeStack.push_back(
                    loopScopeIndex);

                if (const Stmt *condVar =
                        whileStmt->getConditionVariableDeclStmt())
                    scanStmt(condVar);

                if (const Expr *cond =
                        whileStmt->getCond())
                    scanStmt(cond);

                Flow bodyFlow = Flow::Normal;

                if (const Stmt *body =
                        whileStmt->getBody())
                    bodyFlow =
                        scanStmt(body);

                /*
                 * The loop scope is exited when the while-loop
                 * ends normally.
                 */
                if (bodyFlow == Flow::Normal) {

                    owner.reportPendingVarsForScopeExit(
                        whileStmt->getEndLoc(),
                        scopes.back(),
                        reportedVars);
                }

                loopScopeStack.pop_back();

                scopes.pop_back();

                restoreState(beforeLoop);

                if (bodyFlow == Flow::Return)
                    return Flow::Return;

                /*
                 * break and continue are consumed by this loop.
                 */
                return Flow::Normal;
            }

            /*
             * do-while loop.
             */
            if (const auto *doStmt =
                    dyn_cast<DoStmt>(stmt)) {

                const StateSnapshot beforeLoop =
                    captureState();

                scopes.emplace_back();

                const size_t loopScopeIndex =
                    scopes.size() - 1;

                loopScopeStack.push_back(
                    loopScopeIndex);

                Flow bodyFlow = Flow::Normal;

                if (const Stmt *body =
                        doStmt->getBody())
                    bodyFlow =
                        scanStmt(body);

                /*
             * continue reaches the condition.
                 */
                if (bodyFlow == Flow::Normal ||
                    bodyFlow == Flow::Continue) {

                    if (const Expr *cond =
                            doStmt->getCond())
                        scanStmt(cond);
                }

                if (bodyFlow == Flow::Normal) {

                    owner.reportPendingVarsForScopeExit(
                        doStmt->getEndLoc(),
                        scopes.back(),
                        reportedVars);
                }

                loopScopeStack.pop_back();

                scopes.pop_back();

                restoreState(beforeLoop);

                if (bodyFlow == Flow::Return)
                    return Flow::Return;

                return Flow::Normal;
            }

            /*
             * C++ range-for.
             */
            if (const auto *rangeFor =
                    dyn_cast<CXXForRangeStmt>(stmt)) {

                const StateSnapshot beforeLoop =
                    captureState();

                scopes.emplace_back();

                const size_t loopScopeIndex =
                    scopes.size() - 1;

                loopScopeStack.push_back(
                    loopScopeIndex);

                /*
                 * Range initialization.
                 */
                if (const Expr *rangeInit =
                        rangeFor->getRangeInit())
                    scanStmt(rangeInit);

                if (const Stmt *init =
                        rangeFor->getInit())
                    scanStmt(init);

                if (const Stmt *rangeStmt =
                        rangeFor->getRangeStmt())
                    scanStmt(rangeStmt);

                if (const Stmt *beginStmt =
                        rangeFor->getBeginStmt())
                    scanStmt(beginStmt);

                if (const Stmt *endStmt =
                        rangeFor->getEndStmt())
                    scanStmt(endStmt);

                Flow bodyFlow = Flow::Normal;

                if (const Stmt *body =
                        rangeFor->getBody())
                    bodyFlow =
                        scanStmt(body);

                if (bodyFlow == Flow::Normal) {

                    owner.reportPendingVarsForScopeExit(
                        rangeFor->getEndLoc(),
                        scopes.back(),
                        reportedVars);
                }

                loopScopeStack.pop_back();

                scopes.pop_back();

                restoreState(beforeLoop);

                if (bodyFlow == Flow::Return)
                    return Flow::Return;

                return Flow::Normal;
            }

            /*
             * Switch statement.
             *
             * A switch is a break target, so we give it its own
             * tracked scope.
             */
            if (const auto *switchStmt =
                    dyn_cast<SwitchStmt>(stmt)) {

                scopes.emplace_back();

                const size_t switchScopeIndex =
                    scopes.size() - 1;

                switchScopeStack.push_back(
                    switchScopeIndex);

                if (const Stmt *init =
                        switchStmt->getInit())
                    scanStmt(init);

                if (const Stmt *condVar =
                        switchStmt->getConditionVariableDeclStmt())
                    scanStmt(condVar);

                if (const Expr *cond =
                        switchStmt->getCond())
                    scanStmt(cond);

                Flow bodyFlow = Flow::Normal;

                if (const Stmt *body =
                        switchStmt->getBody())
                    bodyFlow =
                        scanStmt(body);

                /*
                 * A normal switch exit leaves the switch scope.
                 */
                if (bodyFlow == Flow::Normal) {

                    owner.reportPendingVarsForScopeExit(
                        switchStmt->getEndLoc(),
                        scopes.back(),
                        reportedVars);
                }

                switchScopeStack.pop_back();

                scopes.pop_back();

                /*
                 * break is consumed by the switch.
                 */
                if (bodyFlow == Flow::Break)
                    return Flow::Normal;

                return bodyFlow;
            }

            /*
             * A destroy call can occur anywhere inside an expression tree.
             */
            if (const auto *call =
                    dyn_cast<CallExpr>(stmt)) {

                markDestroyedIfNeeded(call);
            }

            /*
             * Generic recursive traversal.
             */
            for (auto it = stmt->child_begin(),
                      end = stmt->child_end();
                 it != end;
                 ++it) {

                if (*it)
                    scanStmt(*it);
            }

            return Flow::Normal;
        }

        void trackVar(
            const VarDecl *var)
        {
            if (!var)
                return;

            /*
             * Function-local static variables have static storage
             * duration and are intentionally not tracked.
             */
            if (var->isStaticLocal())
                return;

            if (!var->hasLocalStorage())
                return;

            if (!owner.shouldTrackVar(
                    var,
                    function))
                return;

            if (scopes.empty())
                return;

            TrackedVar tracked;

            tracked.decl =
                var;

            tracked.structName =
                owner.getStructName(
                    var->getType());

            tracked.destroyed =
                false;

            scopes.back().vars.push_back(
                tracked);
        }

        void markDestroyedIfNeeded(
            const CallExpr *call)
        {
            if (!call)
                return;

            const auto *calleeDecl =
                call->getDirectCallee();

            if (!calleeDecl)
                return;

            const auto *functionDecl =
                dyn_cast<FunctionDecl>(
                    calleeDecl);

            if (!functionDecl)
                return;

            const std::string calleeName =
                functionDecl->getNameAsString();

            if (calleeName.empty())
                return;

            if (!owner.isDestroyCall(
                    calleeName))
                return;

            if (call->getNumArgs() < 1)
                return;

            const Expr *arg =
                call->getArg(0);

            if (!arg)
                return;

            const auto *target =
                owner.getReferencedVarDecl(
                    arg);

            if (!target)
                return;

            for (auto &scope :
                 scopes) {

                for (auto &tracked :
                     scope.vars) {

                    if (tracked.decl ==
                        target) {

                        tracked.destroyed =
                            true;

                        scope.destroyed.insert(
                            target);
                    }
                }
            }

            for (auto &tracked :
                 params) {

                if (tracked.decl ==
                    target) {

                    tracked.destroyed =
                        true;
                }
            }
        }

        void addParameter(
            const VarDecl *param)
        {
            if (!param)
                return;

            if (!owner.shouldTrackVar(
                    param,
                    function))
                return;

            TrackedVar tracked;

            tracked.decl =
                param;

            tracked.structName =
                owner.getStructName(
                    param->getType());

            tracked.destroyed =
                false;

            params.push_back(
                tracked);
        }

    /*
     * Finalize the analysis using the flow of the entire
     * function body.
     *
     * If the body can reach the closing brace, parameters
     * must be checked there.
     *
     * If the body cannot reach the closing brace, every
     * reachable return path has already checked parameters.
     */
    void finalize(
        Flow functionFlow)
        {
        if (functionFlow != Flow::Normal)
            return;
            
        if (!function)
            return;
            
        const Stmt *body =
            function->getBody();
            
        if (!body)
            return;
            
        owner.reportPendingParams(
            body->getEndLoc(),
            params,
            reportedVars);
            }

    private:
        /*
         * Handle cleanup required by a continue.
         */
        void reportContinueCleanup(
            SourceLocation loc)
        {
            if (loopScopeStack.empty())
                return;

            const size_t loopScope =
                loopScopeStack.back();

            if (scopes.empty())
                return;

            /*
             * Report scopes strictly inside the loop scope.
             */
            for (size_t i = scopes.size();
                 i > loopScope + 1;
                 --i) {

                owner.reportPendingVarsForScopeExit(
                    loc,
                    scopes[i - 1],
                    reportedVars);
            }
        }

        /*
         * Handle cleanup required by a break.
         */
        void reportBreakCleanup(
            SourceLocation loc)
        {
            if (scopes.empty())
                return;

            const size_t invalidScope =
                static_cast<size_t>(-1);

            size_t targetScope =
                invalidScope;

            /*
             * Find the nearest break target.
             */
            if (!switchScopeStack.empty())
                targetScope =
                    switchScopeStack.back();

            if (!loopScopeStack.empty()) {

                const size_t loopScope =
                    loopScopeStack.back();

                if (targetScope ==
                        invalidScope ||
                    loopScope > targetScope) {

                    targetScope =
                        loopScope;
                }
            }

            if (targetScope ==
                invalidScope)
                return;

            /*
             * break exits the target construct as well as every
             * nested lexical scope inside it.
             */
            for (size_t i = scopes.size();
                 i > targetScope;
                 --i) {

                owner.reportPendingVarsForScopeExit(
                    loc,
                    scopes[i - 1],
                    reportedVars);
            }
        }

        StateSnapshot captureState() const
        {
            StateSnapshot snapshot;

            snapshot.scopeDestroyed.reserve(
                scopes.size());

            for (const auto &scope :
                 scopes) {

                std::vector<bool> states;

                states.reserve(
                    scope.vars.size());

                for (const auto &tracked :
                     scope.vars) {

                    states.push_back(
                        tracked.destroyed);
                }

                snapshot.scopeDestroyed.push_back(
                    std::move(states));
            }

            snapshot.paramDestroyed.reserve(
                params.size());

            for (const auto &tracked :
                 params) {

                snapshot.paramDestroyed.push_back(
                    tracked.destroyed);
            }

            return snapshot;
        }

        void restoreState(
            const StateSnapshot &snapshot)
        {
            const size_t scopeCount =
                std::min(
                    scopes.size(),
                    snapshot.scopeDestroyed.size());

            for (size_t i = 0;
                 i < scopeCount;
                 ++i) {

                const size_t varCount =
                    std::min(
                        scopes[i].vars.size(),
                        snapshot.scopeDestroyed[i].size());

                for (size_t j = 0;
                     j < varCount;
                     ++j) {

                    scopes[i].vars[j].destroyed =
                        snapshot.scopeDestroyed[i][j];
                }

                scopes[i].destroyed.clear();

                for (const auto &tracked :
                     scopes[i].vars) {

                    if (tracked.destroyed &&
                        tracked.decl) {

                        scopes[i].destroyed.insert(
                            tracked.decl);
                    }
                }
            }

            const size_t paramCount =
                std::min(
                    params.size(),
                    snapshot.paramDestroyed.size());

            for (size_t i = 0;
                 i < paramCount;
                 ++i) {

                params[i].destroyed =
                    snapshot.paramDestroyed[i];
            }
        }

        StateSnapshot mergeStates(
            const StateSnapshot &a,
            const StateSnapshot &b) const
        {
            StateSnapshot merged =
                a;

            const size_t scopeCount =
                std::min(
                    merged.scopeDestroyed.size(),
                    b.scopeDestroyed.size());

            for (size_t i = 0;
                 i < scopeCount;
                 ++i) {

                const size_t varCount =
                    std::min(
                        merged.scopeDestroyed[i].size(),
                        b.scopeDestroyed[i].size());

                for (size_t j = 0;
                     j < varCount;
                     ++j) {

                    /*
                     * Definitely destroyed means destroyed on
                     * every path reaching this point.
                     */
                    merged.scopeDestroyed[i][j] =
                        merged.scopeDestroyed[i][j] &&
                        b.scopeDestroyed[i][j];
                }
            }

            const size_t paramCount =
                std::min(
                    merged.paramDestroyed.size(),
                    b.paramDestroyed.size());

            for (size_t i = 0;
                 i < paramCount;
                 ++i) {

                merged.paramDestroyed[i] =
                    merged.paramDestroyed[i] &&
                    b.paramDestroyed[i];
            }

            return merged;
        }
    };

    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    StructDatabase &database;

    const SourceManager *sourceManager =
        nullptr;

    std::vector<const FunctionDecl *>
        pendingFunctions;

    const std::string podSuffix;
    const std::string raiiSuffix;
    const std::string destroySuffix;
    const std::string copySuffix;
    const std::string moveSuffix;
    const std::string validSuffix;
    const std::string freeSuffix;

private:
    static std::string getOption(
        const RuleConfig &cfg,
        const std::string &name)
    {
        auto it =
            cfg.options.find(name);

        if (it ==
            cfg.options.end())
            return "";

        return it->second;
    }

    bool isThirdParty(
        const std::string &file) const
    {
        for (const auto &p :
             globalConfig.getThirdPartyIncludes()) {

            if (!p.empty() &&
                file.find(p) !=
                    std::string::npos)
                return true;
        }

        return false;
    }

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const
    {
        if (loc.isInvalid())
            return true;

        if (suppressions.isSuppressed(
                sm,
                loc))
            return true;

        SourceLocation spell =
            sm.getSpellingLoc(loc);

        if (sm.isInSystemHeader(spell))
            return true;

        std::string file =
            sm.getFilename(spell).str();

        if (file.empty())
            return true;

        return isThirdParty(file);
    }

    const RecordDecl *getStructDecl(
        QualType type) const
    {
        QualType current =
            type.getUnqualifiedType();

        for (int i = 0;
             i < 8;
             ++i) {

            current =
                current.getCanonicalType()
                    .getUnqualifiedType();

            if (current->isRecordType()) {

                const auto *recordType =
                    current->getAs<RecordType>();

                if (recordType) {

                    const auto *recordDecl =
                        recordType->getDecl();

                    if (recordDecl)
                        return recordDecl;
                }
            }

        /*
         * IMPORTANT:
         *
         * QualType does not provide getPointeeType().
         * The pointee API belongs to PointerType.
         */
            if (current->isPointerType()) {

            const auto *pointerType =
                current->getAs<PointerType>();

            if (!pointerType)
                break;

                current =
                pointerType->getPointeeType();

                continue;
            }

            break;
        }

        return nullptr;
    }

    bool isStructType(
        QualType type,
        std::string *name = nullptr) const
    {
        const auto *recordDecl =
            getStructDecl(type);

        if (!recordDecl ||
            !recordDecl->isStruct())
            return false;

        if (name)
            *name =
                recordDecl->getNameAsString();

        return true;
    }

    bool isInsideHelperFunction(
        const FunctionDecl *function,
        const std::string &structName) const
    {
        if (!function ||
            structName.empty())
            return false;

        const std::string name =
            function->getNameAsString();

        const auto matches =
            [&](const std::string &suffix) {

                if (suffix.empty())
                    return false;

                return name ==
                    structName + suffix;
            };

        return
            matches(podSuffix) ||
            matches(raiiSuffix) ||
            matches(freeSuffix) ||
            matches(destroySuffix) ||
            matches(copySuffix) ||
            matches(moveSuffix) ||
            matches(validSuffix);
    }

    const VarDecl *getReferencedVarDecl(
        const Expr *expr) const
    {
        if (!expr)
            return nullptr;

        expr =
            expr->IgnoreParenImpCasts();

        if (const auto *declRef =
                dyn_cast<DeclRefExpr>(expr)) {

            return dyn_cast<VarDecl>(
                declRef->getDecl());
        }

        if (const auto *unary =
                dyn_cast<UnaryOperator>(expr)) {

            if (unary->getOpcode() ==
                    UO_Deref ||
                unary->getOpcode() ==
                    UO_AddrOf) {

                return getReferencedVarDecl(
                    unary->getSubExpr());
            }
        }

        if (const auto *memberExpr =
                dyn_cast<MemberExpr>(expr)) {

            return getReferencedVarDecl(
                memberExpr->getBase());
        }

        if (const auto *arraySubscript =
                dyn_cast<ArraySubscriptExpr>(expr)) {

            return getReferencedVarDecl(
                arraySubscript->getBase());
        }

        return nullptr;
    }

    bool shouldTrackVar(
        const VarDecl *var,
        const FunctionDecl *function) const
    {
        if (!var)
            return false;

        if (var->isStaticLocal())
            return false;

        /*
         * Normal local variables and function parameters can own
         * a RAII struct value.
         */
        if (!var->isLocalVarDecl() &&
            !isa<ParmVarDecl>(var))
            return false;

        /*
         * A pointer to a RAII struct does not own the struct itself.
         */
        QualType type =
            var->getType().getUnqualifiedType();

        if (type->isPointerType())
            return false;

        std::string structName;

        if (!isStructType(
                type,
                &structName))
            return false;

        /*
         * Do not require helper functions themselves to destroy
         * their temporary/local RAII structs.
         */
        if (function &&
            isInsideHelperFunction(
                function,
                structName))
            return false;

        const auto *info =
            database.find(structName);

        return info &&
            info->kind ==
                StructDatabase::Kind::Raii;
    }

    std::string getStructName(
        QualType type) const
    {
        std::string name;

        isStructType(
            type,
            &name);

        return name;
    }

    bool isDestroyCall(
        const std::string &name) const
    {
        if (destroySuffix.empty())
            return false;

        if (name.size() <
            destroySuffix.size())
            return false;

        return name.compare(
            name.size() -
                destroySuffix.size(),
            destroySuffix.size(),
            destroySuffix) == 0;
    }

    void reportUsageIssue(
        SourceLocation loc,
        const std::string &message) const
    {
        if (!sourceManager)
            return;

        if (shouldIgnore(
                *sourceManager,
                loc))
            return;

        diagnostics.report(
            config.level,
            *sourceManager,
            loc,
            message);
    }

    void reportPendingVarsForScopeExit(
        SourceLocation loc,
        const ScopeState &scope,
        std::unordered_set<
            ReportKey,
            ReportKeyHash> &reportedVars) const
    {
        for (const auto &tracked :
             scope.vars) {

            if (tracked.destroyed ||
                !tracked.decl)
                continue;

            const ReportKey key{
                tracked.decl,
                loc.getRawEncoding()
            };

            if (reportedVars.count(key))
                continue;

            reportedVars.insert(key);

            reportUsageIssue(
                loc,
                "struct variable '" +
                tracked.decl->getNameAsString() +
                "' of type '" +
                tracked.structName +
                "' must be destroyed with '" +
                tracked.structName +
                destroySuffix +
                "' before scope exit (raii)");
        }
    }

    void reportPendingVarsForExit(
        SourceLocation loc,
        const std::vector<ScopeState> &scopes,
        const std::vector<TrackedVar> &params,
        std::unordered_set<
            ReportKey,
            ReportKeyHash> &reportedVars) const
    {
        /*
         * Check all active lexical scopes.
         */
        for (auto it = scopes.rbegin();
             it != scopes.rend();
             ++it) {

            reportPendingVarsForScopeExit(
                loc,
                *it,
                reportedVars);
        }

        /*
         * Parameters live for the whole function and therefore
         * must also be destroyed before return.
         */
        for (const auto &tracked :
             params) {

            if (tracked.destroyed ||
                !tracked.decl)
                continue;

            const ReportKey key{
                tracked.decl,
                loc.getRawEncoding()
            };

            if (reportedVars.count(key))
                continue;

            reportedVars.insert(key);

            reportUsageIssue(
                loc,
                "struct parameter '" +
                tracked.decl->getNameAsString() +
                "' of type '" +
                tracked.structName +
                "' must be destroyed with '" +
                tracked.structName +
                destroySuffix +
                "' before scope exit (raii)");
        }
    }

    void reportPendingParams(
    SourceLocation loc,
    const std::vector<TrackedVar> &params,
    std::unordered_set<
        ReportKey,
        ReportKeyHash> &reportedVars) const
{
    if (loc.isInvalid())
        return;

    for (const auto &tracked :
         params) {

        if (tracked.destroyed ||
            !tracked.decl)
            continue;

        const ReportKey key{
            tracked.decl,
            loc.getRawEncoding()
        };

        if (reportedVars.count(key))
            continue;

        reportedVars.insert(key);

        reportUsageIssue(
            loc,
            "struct parameter '" +
            tracked.decl->getNameAsString() +
            "' of type '" +
            tracked.structName +
            "' must be destroyed with '" +
            tracked.structName +
            destroySuffix +
            "' before scope exit (raii)");
    }
}

public:
    StructCleanupRule(
        const RuleConfig &cfg,
        const Config &gc,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag),
          database(db),
          podSuffix(
              getOption(
                  cfg,
                  "pod_struct_creator_suffix")),
          raiiSuffix(
              getOption(
                  cfg,
                  "raii_struct_creator_suffix")),
          destroySuffix(
              getOption(
                  cfg,
                  "raii_struct_destroyer_suffix")),
          copySuffix(
              getOption(
                  cfg,
                  "raii_struct_copy_suffix")),
          moveSuffix(
              getOption(
                  cfg,
                  "raii_struct_move_suffix")),
          validSuffix(
              getOption(
                  cfg,
                  "raii_struct_valid_suffix")),
          freeSuffix(
              getOption(
                  cfg,
                  "free_struct_creator_suffix"))
    {
    }

    void run(
        const MatchFinder::MatchResult &result)
        override
    {
        sourceManager =
            result.SourceManager;

        if (!sourceManager)
            return;

        const auto *function =
            result.Nodes.getNodeAs<FunctionDecl>(
                "function");

        if (!function ||
            !function->doesThisDeclarationHaveABody())
            return;

        pendingFunctions.push_back(
            function);
    }

    void finalize()
    {
        for (const auto *function :
             pendingFunctions) {

            if (!function)
                continue;

            CleanupAnalyzer analyzer(
                *this,
                function);

            for (const auto *param :
                 function->parameters()) {

                analyzer.addParameter(
                    param);
            }

            const auto functionFlow =
                analyzer.scanStmt(
                    function->getBody());

            analyzer.finalize(
                functionFlow);
        }
    }
};