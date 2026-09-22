#pragma once

#include <cstddef>
#include <string>
#include <unordered_set>
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

        bool operator==(const ReportKey &other) const;
    };

    struct ReportKeyHash {
        std::size_t operator()(const ReportKey &key) const;
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
            const FunctionDecl *function);

        Flow scanStmt(const Stmt *stmt);

        void trackVar(
            const VarDecl *var);

        void markDestroyedIfNeeded(const CallExpr *call);

        void addParameter(
            const VarDecl *param);

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
        Flow functionFlow);

    private:
        /*
         * Handle cleanup required by a continue.
         */
        void reportContinueCleanup(
            SourceLocation loc);

        /*
         * Handle cleanup required by a break.
         */
        void reportBreakCleanup(
            SourceLocation loc);

        StateSnapshot captureState() const;

        void restoreState(
            const StateSnapshot &snapshot);

        StateSnapshot mergeStates(
            const StateSnapshot &a,
            const StateSnapshot &b) const;
    };

    const Config &config;

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
    const std::string returnSuffix;
    const std::string validSuffix;
    const std::string freeSuffix;

private:
    bool isThirdParty(
        const std::string &file) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    const RecordDecl *getStructDecl(
        QualType type) const;

    bool isStructType(
        QualType type,
        std::string *name = nullptr) const;

    bool isInsideHelperFunction(
        const FunctionDecl *function,
        const std::string &structName) const;

    const VarDecl *getReferencedVarDecl(
        const Expr *expr) const;

    bool shouldTrackVar(
        const VarDecl *var,
        const FunctionDecl *function) const;

    std::string getStructName(
        QualType type) const;

    bool isDestroyCall(
        const std::string &name,
        const std::string &structName) const;

    bool isReturnCall(
        const std::string &name,
        const std::string &structName) const;

    void reportUsageIssue(
        SourceLocation loc,
        const std::string &message) const;

    void reportPendingVarsForScopeExit(
        SourceLocation loc,
        const ScopeState &scope,
        std::unordered_set<
            ReportKey,
            ReportKeyHash> &reportedVars) const;

    void reportPendingVarsForExit(
        SourceLocation loc,
        const std::vector<ScopeState> &scopes,
        const std::vector<TrackedVar> &params,
        std::unordered_set<
            ReportKey,
            ReportKeyHash> &reportedVars) const;

    void reportPendingParams(
        SourceLocation loc,
        const std::vector<TrackedVar> &params,
        std::unordered_set<
            ReportKey,
            ReportKeyHash> &reportedVars) const;

public:
    StructCleanupRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db);

    void bindFinder(MatchFinder &finder);

    void run(
        const MatchFinder::MatchResult &result)
        override;

    void finalize();
};
