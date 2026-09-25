#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "struct_database.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class StructInitRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    const std::string freeSuffix;
    const std::string podSuffix;
    const std::string raiiSuffix;
    const std::string destroySuffix;
    const std::string copySuffix;
    const std::string moveSuffix;
    const std::string returnSuffix;
    const std::string validSuffix;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    StructDatabase &database;

    const SourceManager *sourceManager = nullptr;

    std::vector<std::pair<const VarDecl *, const FunctionDecl *>> pendingVarDecls;
    std::vector<std::pair<const BinaryOperator *, const FunctionDecl *>> pendingAssignments;
    struct PendingCall {
        const CallExpr *call = nullptr;
        const FunctionDecl *function = nullptr;
        ASTContext *context = nullptr;
    };
    std::vector<PendingCall> pendingCalls;
    std::vector<std::pair<const ReturnStmt *, const FunctionDecl *>> pendingReturns;
    std::vector<const RecordDecl *> pendingRecords;

private:
    bool isThirdParty(const std::string &file) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    const RecordDecl *getStructDecl(
        QualType type,
        bool *isArray = nullptr) const;

    bool isStructType(
        QualType type,
        std::string *name = nullptr,
        bool *isArray = nullptr) const;

    bool isInsideHelperFunction(
        const FunctionDecl *function,
        const std::string &structName) const;

    /*
     * True if 'function' is the free struct creator of a known free
     * struct (e.g. 'Name_init') and 'target' is a direct member of
     * that same struct (e.g. 'self->member'). A free creator sets up
     * its own struct, so it may assign its members freely, even when
     * a member is a raii struct.
     */
    bool isOwnMemberOfFreeCreator(
        const Expr *target,
        const FunctionDecl *function) const;

    const FunctionDecl *findEnclosingFunction(
        ASTContext &context,
        const DynTypedNode &node) const;

    const Expr *unwrapExpr(const Expr *expr) const;

    bool isStructReturnFunction(
        const CallExpr *call,
        std::string *structName = nullptr) const;

    bool isDirectReturnExpression(
        const CallExpr *call,
        ASTContext &context) const;

    void checkReturnFunctionUsage(
        const CallExpr *call,
        const FunctionDecl *enclosingFunction,
        ASTContext &context) const;

    bool exprIsStructReturnValue(const Expr *expr, std::string *structName = nullptr) const;

    bool exprIsStructVariableReference(
        const Expr *expr,
        std::string *structName = nullptr) const;

    bool exprIsStructValue(const Expr *expr, std::string *structName = nullptr) const;

    bool exprIsNonPointerStructValue(const Expr *expr, std::string *structName = nullptr) const;

    void reportUsageIssue(
        SourceLocation loc,
        const std::string &message) const;

    void checkVarDecl(
        const VarDecl *varDecl,
        const FunctionDecl *enclosingFunction) const;

    const VarDecl *getReferencedVarDecl(const Expr *expr) const;

    std::string getAssignmentTargetName(const Expr *expr) const;

    bool isPointerDereference(const Expr *expr) const;

    void checkAssignment(
        const BinaryOperator *assignment,
        const FunctionDecl *enclosingFunction) const;

    void checkCallArguments(
        const CallExpr *call,
        const FunctionDecl *enclosingFunction) const;

    void checkReturnStmt(
        const ReturnStmt *returnStmt,
        const FunctionDecl *enclosingFunction) const;

    void checkRecordDecl(const RecordDecl *recordDecl) const;

public:
    StructInitRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;

    void finalize();
};
