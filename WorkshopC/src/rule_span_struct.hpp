#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>

#include <llvm/ADT/APSInt.h>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class SpanStructRule : public MatchFinder::MatchCallback {
private:
    const Config &config;
    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool isThirdParty(const std::string &path) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    std::string getSpanStructName(QualType type) const;

    std::string getSpanStructName(const RecordDecl *record) const;

    /*
     * True for a span struct named with const_span_struct_suffix, which
     * must hold a pointer to const data.
     */
    bool isConstSpan(const RecordDecl *record) const;

    // "span struct" or "const span struct", for messages
    std::string spanKindText(const RecordDecl *record) const;

    bool isSizeT(QualType type) const;

    bool isValidSpanDefinition(const RecordDecl *record) const;

    const Expr *getArrayExpression(const Expr *expr) const;

    // Where the data of a span starts in an array
    struct ArrayStart {
        const Expr *array = nullptr;

        // The array itself, not '&array[index]' or 'array + index'
        bool plain = true;

        bool indexKnown = true;
        int64_t index = 0;

        bool sizeKnown = false;
        uint64_t size = 0;
    };

    /*
     * True if 'expr' is an array, '&array[index]' or 'array + index',
     * the start of the elements a span gets from the array.
     */
    bool getArrayStart(
        const Expr *expr,
        ASTContext &context,
        ArrayStart &start) const;

    // What is wrong with the count for a start inside an array
    std::string describeIndexedCount(const ArrayStart &start) const;

    /*
     * True if 'variable' is a span or const span holding all of 'array':
     * initialized from its start ('array' or '&array[0]') with its full
     * size, with an initializer or the span's pod creator.
     */
    bool isSpanOfWholeArray(
        const VarDecl *variable,
        const VarDecl *array,
        ASTContext &context) const;

    /*
     * The variables declared right after 'array': the next statement of
     * its block, or the next declaration of the file for a global.
     */
    std::vector<const VarDecl *> getNextDeclaredVariables(
        const VarDecl *array,
        ASTContext &context) const;

    /*
     * A wrapper around an unsafe function: static, with a body of a single
     * expression statement or return, that is not a comma expression.
     */
    bool isOneLineStaticFunction(const FunctionDecl *function) const;

    // The function a statement is written in
    const FunctionDecl *getEnclosingFunction(
        const Stmt *stmt,
        ASTContext &context) const;

    /*
     * The span variable holding all of 'array' right after it, or null.
     * Only for arrays that require_span_immediately_after_array covers.
     */
    const VarDecl *getSpanOfArray(
        const VarDecl *array,
        ASTContext &context) const;

    /*
     * With require_span_immediately_after_array, an array is only used
     * through its span, except in the span's initializer, its own
     * declaration, and code that is never evaluated (sizeof, typeof).
     */
    void checkArrayUse(
        const DeclRefExpr *use,
        const SourceManager &sm,
        ASTContext &context) const;

    // With require_span_immediately_after_array
    void checkSpanAfterArray(
        const VarDecl *array,
        const SourceManager &sm,
        ASTContext &context) const;

    // Where an argument hands over the data of a span
    struct SpanDataUse {
        const RecordDecl *span = nullptr;

        // 'span.data', 'span_ptr->data' or 'int_span_data(...)'
        std::string text;

        // What to pass instead: 'span', '*span_ptr' or 'the span'
        std::string passInstead;
    };

    /*
     * True if 'expr' hands over the data of a span: 'span.data',
     * 'span->data', or a call to a function named after a span that
     * returns a pointer (a getter such as 'int_span_data(&span)'), also
     * inside casts, callsite operators and pointer arithmetic
     * ('span.data + 1'). A single element, '&span.data[i]', is not.
     */
    bool getSpanDataUse(
        const Expr *expr,
        ASTContext &context,
        SpanDataUse &use) const;

    // The span struct whose name 'function' starts with, the longest match
    const RecordDecl *getSpanOfFunction(
        const FunctionDecl *function,
        ASTContext &context) const;

    // The expression as written, e.g. 'span', 'holder.span' or 'span_ptr->data'
    std::string getExpressionName(const Expr *expr) const;

    std::string getArrayName(const Expr *expr) const;

    bool getArrayElementCount(
        const Expr *expr,
        llvm::APSInt *count) const;

    /*
     * True if 'countExpr' is a constant equal to the element count of the
     * array, or at most that count when spans may be given fewer elements.
     */
    bool hasExpectedArrayCount(
        const Expr *arrayExpr,
        const Expr *countExpr,
        ASTContext &context) const;

    bool isLibraryFunction(
        const FunctionDecl *function,
        const SourceManager &sm) const;

    /*
     * The pod creator of a span: named <span name><pod suffix> and
     * returning the span, 'int_span int_span_pod(...)'.
     */
    bool isSpanPodFunction(
        const FunctionDecl *function,
        const RecordDecl *span) const;

    /*
     * True if 'call' destroys an array of raii structs, with 'argument'
     * as its first argument: '<element struct><array destroyer suffix>'
     * given the array itself, e.g. 'd_str_destroy_array(names, 2)'. The
     * callee must have the exact array destroyer signature,
     * 'void <struct><suffix>(<struct>* self, size_t n)', and the struct
     * must be raii (see hasVisibleRaiiCreator). Only with struct resource
     * management and allow_raii_struct_arrays.
     */
    bool isRaiiArrayDestroyCall(
        const CallExpr *call,
        const Expr *argument) const;

    /*
     * True if the raii creator of 'record', '<struct> <struct><raii
     * creator suffix>(...)', is declared before 'loc'. Having it is what
     * makes a struct raii, so no struct database is needed.
     */
    bool hasVisibleRaiiCreator(
        const RecordDecl *record,
        SourceLocation loc,
        ASTContext &context) const;

    bool isInsideStructInitializer(
        const Expr *expr,
        ASTContext &context) const;

    void report(
        DiagCode code,
        SourceLocation loc,
        const std::string &message,
        const SourceManager &sm) const;

public:
    SpanStructRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
