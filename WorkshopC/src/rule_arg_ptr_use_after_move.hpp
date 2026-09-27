#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * A pointer variable passed to a parameter tagged 'move' is owned by the
 * called function afterwards, so it may not be used again (read, moved
 * again, compared, its address taken, ...) until it is reassigned.
 * Reassigning means a plain assignment or a new declaration. Passing its
 * address to a parameter tagged 'out' does not count, since the function
 * is not guaranteed to write to it (it may fail and return early).
 *
 * The analysis follows the control flow of each function: a use is
 * reported if the pointer may have been moved on any path reaching it.
 * Paths that leave the function (return, goto) after a move do not
 * count, and moves inside loops carry over to the next iteration.
 */
class ArgumentPointerUseAfterMoveRule : public MatchFinder::MatchCallback {
private:
    static constexpr const char* kMoveTag = "workshopc_move";
    static constexpr const char* kOutTag  = "workshopc_out";

    // Moved pointer variable -> the function it was moved to
    using State = std::map<const VarDecl *, std::string>;

    enum class Flow {
        Normal,
        Exit,
        Break,
        Continue
    };

    struct JumpTarget {
        bool isLoop = false;
        std::vector<State> breaks;
        std::vector<State> continues;
    };

    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    // Per analyzed function
    const SourceManager *sourceManager = nullptr;
    const ASTContext *context = nullptr;
    std::vector<JumpTarget> jumpTargets;
    std::unordered_set<unsigned> reported;

private:
    bool isThirdParty(const std::string &path) const;

    bool shouldIgnore(SourceLocation loc) const;

    std::string getParamTag(const ParmVarDecl *param) const;

    bool isTracked(const VarDecl *var) const;

    /*
     * Strips parentheses, casts and the operator wrapper calls
     * (e.g. move_cast), returning the expression underneath.
     */
    const Expr *stripWrappers(const Expr *expr) const;

    const VarDecl *getVariable(const Expr *expr) const;

    static State merge(const State &a, const State &b);

    void reportUse(const DeclRefExpr *use, const std::string &movedTo);

    void walkExpr(const Expr *expr, State &state);

    void walkCall(const CallExpr *call, State &state);

    Flow walk(const Stmt *stmt, State &state);

    Flow walkLoop(
        const Stmt *body,
        const Expr *cond,
        const Expr *inc,
        bool conditionFirst,
        State &state);

    Flow walkSwitch(const SwitchStmt *switchStmt, State &state);

public:
    ArgumentPointerUseAfterMoveRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
