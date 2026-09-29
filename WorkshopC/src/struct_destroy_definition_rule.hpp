#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
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

/*
 * With raii_standardized_destroy_definitions, the destroy function of a
 * raii struct destroys each of its raii fields exactly once, with
 * '<field struct>_destroy(&self->field)' or, for an array field,
 * '<field struct>_destroy_array(self->field, count)'.
 *
 * All the field destroys are statements side by side in one block:
 * either the body of the destroy function, or the then-block of an if
 * statement directly in that body, with any condition. Never in a
 * nested block, loop, switch or else branch. A return or goto before
 * them skips the struct as a whole and is allowed, but not one between
 * them, nor a label a goto could jump to. With
 * raii_destroy_in_reverse_order they come in reverse field declaration
 * order.
 *
 * A raii struct is always either fully alive or fully destroyed: a
 * condition decides whether the struct as a whole is destroyed, never
 * which of its fields are.
 */
class StructDestroyDefinitionRule : public MatchFinder::MatchCallback {
private:
    // A raii field of the struct being destroyed
    struct RaiiField {
        const FieldDecl *field = nullptr;
        std::string destroyName;
    };

    // A destroy call of a raii field found in the destroy function
    struct FieldDestroy {
        const CallExpr *call = nullptr;
        size_t fieldIndex = 0;

        // The block it is a direct statement of, or null when nested
        // deeper or not a statement of its own
        const Stmt *block = nullptr;

        // Its statement's position in its block
        size_t position = 0;
    };

    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    StructDatabase &database;

    const SourceManager *sourceManager = nullptr;

    std::vector<const FunctionDecl *> destroyDefinitions;

private:
    bool isThirdParty(const std::string &file) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    void report(
        DiagCode code,
        SourceLocation loc,
        const std::string &message) const;

    // The raii fields of 'record', in declaration order
    std::vector<RaiiField> getRaiiFields(const RecordDecl *record) const;

    // The index of the raii field 'call' destroys on 'self', or -1
    int getDestroyedField(
        const CallExpr *call,
        const ParmVarDecl *self,
        const std::vector<RaiiField> &fields) const;

    // Every field destroy inside 'stmt', which is not a direct statement
    // of an allowed block
    void collectNested(
        const Stmt *stmt,
        const ParmVarDecl *self,
        const std::vector<RaiiField> &fields,
        std::vector<FieldDestroy> &destroys) const;

    /*
     * The field destroys of an allowed block, the body or the then-block
     * of an if statement in the body
     */
    void collectBlock(
        const Stmt *block,
        const ParmVarDecl *self,
        const std::vector<RaiiField> &fields,
        bool isBody,
        std::vector<FieldDestroy> &destroys) const;

    void checkDestroyDefinition(const FunctionDecl *function) const;

public:
    StructDestroyDefinitionRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;

    void finalize();
};
