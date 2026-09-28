#pragma once

#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/TypeLoc.h>

#include <algorithm>
#include <string>
#include <vector>

#include "reference_tag.hpp"
#include "tag_placement.hpp"

/*
 * The tags of a function pointer are written on the parameters of its
 * type, e.g. 'typedef void (*callback_t)(MOVED int* value);'. Types do
 * not carry attributes, so the tags are read from the parameter
 * declarations of the type as it was written, following typedefs.
 */

/*
 * The function type written in 'loc', through pointers, arrays,
 * parentheses, qualifiers and typedefs, e.g. the function type of
 * 'callback_t', 'callback_t table[2]' or 'void (**callback)(int*)'.
 */
inline clang::FunctionProtoTypeLoc functionTypeLocOf(clang::TypeLoc loc) {
    while (!loc.isNull()) {
        if (const auto function = loc.getAs<clang::FunctionProtoTypeLoc>())
            return function;

        if (const auto typedefLoc = loc.getAs<clang::TypedefTypeLoc>()) {
            const clang::TypedefNameDecl *decl = typedefLoc.getTypePtr()->getDecl();
            const clang::TypeSourceInfo *info = decl ? decl->getTypeSourceInfo() : nullptr;

            if (!info)
                return {};

            loc = info->getTypeLoc();
            continue;
        }

        loc = loc.getNextTypeLoc();
    }

    return {};
}

/*
 * The function type of what 'expr' refers to, as it was written: a
 * variable, parameter or field holding a function pointer, an element of
 * an array of them, a function returning one, or a cast to one. For a
 * function it is the function's own type.
 */
inline clang::FunctionProtoTypeLoc functionTypeLocOfExpr(const clang::Expr *expr) {
    while (expr) {
        expr = expr->IgnoreParenImpCasts();

        // An explicit cast decides the type: (callback_t)function
        if (const auto *cast = llvm::dyn_cast<clang::ExplicitCastExpr>(expr)) {
            const clang::TypeSourceInfo *info = cast->getTypeInfoAsWritten();
            return info ? functionTypeLocOf(info->getTypeLoc()) : clang::FunctionProtoTypeLoc{};
        }

        // So does a compound literal: (callback_t){ function }
        if (const auto *literal = llvm::dyn_cast<clang::CompoundLiteralExpr>(expr)) {
            const clang::TypeSourceInfo *info = literal->getTypeSourceInfo();
            return info ? functionTypeLocOf(info->getTypeLoc()) : clang::FunctionProtoTypeLoc{};
        }

        if (const auto *ref = llvm::dyn_cast<clang::DeclRefExpr>(expr)) {
            const auto *decl = llvm::dyn_cast<clang::DeclaratorDecl>(ref->getDecl());
            const clang::TypeSourceInfo *info = decl ? decl->getTypeSourceInfo() : nullptr;
            return info ? functionTypeLocOf(info->getTypeLoc()) : clang::FunctionProtoTypeLoc{};
        }

        if (const auto *member = llvm::dyn_cast<clang::MemberExpr>(expr)) {
            const auto *decl = llvm::dyn_cast<clang::DeclaratorDecl>(member->getMemberDecl());
            const clang::TypeSourceInfo *info = decl ? decl->getTypeSourceInfo() : nullptr;
            return info ? functionTypeLocOf(info->getTypeLoc()) : clang::FunctionProtoTypeLoc{};
        }

        // table[i], the element type is found through the array type
        if (const auto *subscript = llvm::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
            expr = subscript->getBase();
            continue;
        }

        // *pointer and &function
        if (const auto *unary = llvm::dyn_cast<clang::UnaryOperator>(expr)) {
            if (unary->getOpcode() == clang::UO_Deref ||
                unary->getOpcode() == clang::UO_AddrOf)
            {
                expr = unary->getSubExpr();
                continue;
            }

            return {};
        }

        // get_callback()(...), the function type of what it returns
        if (const auto *call = llvm::dyn_cast<clang::CallExpr>(expr)) {
            const clang::FunctionProtoTypeLoc callee =
                functionTypeLocOfExpr(call->getCallee());

            return callee ? functionTypeLocOf(callee.getReturnLoc())
                          : clang::FunctionProtoTypeLoc{};
        }

        return {};
    }

    return {};
}

/*
 * The name of what a function pointer expression refers to, e.g.
 * 'callback' for 'callback', 'self->callback' or 'callbacks[i]'.
 */
inline std::string functionPointerNameOf(const clang::Expr *expr) {
    while (expr) {
        expr = expr->IgnoreParenCasts();

        if (const auto *ref = llvm::dyn_cast<clang::DeclRefExpr>(expr))
            return ref->getDecl()->getNameAsString();

        if (const auto *member = llvm::dyn_cast<clang::MemberExpr>(expr))
            return member->getMemberDecl()->getNameAsString();

        if (const auto *subscript = llvm::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
            expr = subscript->getBase();
            continue;
        }

        if (const auto *unary = llvm::dyn_cast<clang::UnaryOperator>(expr)) {
            expr = unary->getSubExpr();
            continue;
        }

        break;
    }

    return "";
}

/*
 * What a call goes to: a function, or a function pointer whose type
 * gives the parameters and their tags.
 */
struct CalleeParameters {
    // False when the parameters could not be found, e.g. for a call
    // through a conditional expression
    bool known = false;

    // "function 'name'" or "function pointer 'name'"
    std::string description;

    // The name alone, e.g. for "moved to 'name'". For a conditional
    // callee the names of all branches: first' or 'second
    std::string name;

    // May contain null for parameters without a declaration
    std::vector<const clang::ParmVarDecl *> parameters;
};

/*
 * The annotation of a parameter with the given names, "" if none.
 * For a parameter of a function declaration, the other declarations of
 * the function are looked at too.
 */
inline std::string parameterTagOf(
    const clang::ParmVarDecl *param,
    const std::vector<std::string> &tags)
{
    if (!param)
        return "";

    auto ownTag = [&tags](const clang::ParmVarDecl *p) -> std::string {
        for (const auto *attr : p->attrs()) {
            if (const auto *annotate = llvm::dyn_cast<clang::AnnotateAttr>(attr)) {
                for (const std::string &tag : tags) {
                    if (annotate->getAnnotation() == tag)
                        return tag;
                }
            }
        }

        return "";
    };

    const std::string tag = ownTag(param);

    if (!tag.empty() || !isParameterOfFunctionDecl(param))
        return tag;

    const auto *owner = llvm::cast<clang::FunctionDecl>(param->getDeclContext());
    const unsigned index = param->getFunctionScopeIndex();

    for (const clang::FunctionDecl *redecl : owner->redecls()) {
        if (index < redecl->getNumParams()) {
            const std::string redeclTag = ownTag(redecl->getParamDecl(index));

            if (!redeclTag.empty())
                return redeclTag;
        }
    }

    return "";
}

// "workshopc_move", "workshopc_out", "workshopc_modify" or ""
inline std::string movementTagOf(const clang::ParmVarDecl *param) {
    return parameterTagOf(param, {"workshopc_move", "workshopc_out", "workshopc_modify"});
}

/*
 * What a call may go to when its callee is a conditional expression,
 * e.g. 'first' and 'second' for (flag ? first : second)(value), also
 * through nested conditionals. Otherwise the callee itself.
 */
inline void collectCalleeBranches(
    const clang::Expr *callee,
    std::vector<const clang::Expr *> &branches)
{
    if (!callee)
        return;

    callee = callee->IgnoreParenImpCasts();

    if (const auto *conditional =
            llvm::dyn_cast<clang::AbstractConditionalOperator>(callee))
    {
        collectCalleeBranches(conditional->getTrueExpr(), branches);
        collectCalleeBranches(conditional->getFalseExpr(), branches);
        return;
    }

    branches.push_back(callee);
}

// True if both function types have the same movement and reference tags
inline bool haveSameTags(
    clang::FunctionProtoTypeLoc first,
    clang::FunctionProtoTypeLoc second)
{
    const unsigned count = std::min(first.getNumParams(), second.getNumParams());

    for (unsigned i = 0; i < count; ++i) {
        const clang::ParmVarDecl *a = first.getParam(i);
        const clang::ParmVarDecl *b = second.getParam(i);

        if (!a || !b)
            continue;

        if (movementTagOf(a) != movementTagOf(b) ||
            hasReferenceTag(a) != hasReferenceTag(b))
            return false;
    }

    return true;
}

/*
 * The parameters a call goes through: those of the called function, or
 * those written in the type of the called function pointer. A call
 * through a conditional expression uses the tags its branches agree on,
 * and is unknown when they do not (reported by the function pointer tag
 * rule) or when a branch can not be traced to a declaration.
 */
inline CalleeParameters calleeParametersOf(const clang::CallExpr *call) {
    CalleeParameters callee;

    if (!call)
        return callee;

    if (const clang::FunctionDecl *function = call->getDirectCallee()) {
        callee.known = true;
        callee.name = function->getNameAsString();
        callee.description = "function '" + callee.name + "'";
        callee.parameters.assign(function->param_begin(), function->param_end());
        return callee;
    }

    std::vector<const clang::Expr *> branches;
    collectCalleeBranches(call->getCallee(), branches);

    clang::FunctionProtoTypeLoc type;
    std::string names;

    for (const clang::Expr *branch : branches) {
        const clang::FunctionProtoTypeLoc branchType = functionTypeLocOfExpr(branch);

        if (!branchType)
            return callee;

        if (!type)
            type = branchType;
        else if (!haveSameTags(type, branchType))
            return callee;

        std::string name = functionPointerNameOf(branch);

        if (name.empty())
            name = "<function pointer>";

        // Quoted one by one where used: 'first' or 'second'
        names += (names.empty() ? "" : "' or '") + name;
    }

    if (!type)
        return callee;

    callee.known = true;
    callee.name = names;
    callee.description = "function pointer '" + names + "'";

    for (unsigned i = 0; i < type.getNumParams(); ++i)
        callee.parameters.push_back(type.getParam(i));

    return callee;
}
