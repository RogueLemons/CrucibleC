#pragma once

#include <clang/AST/Decl.h>

/*
 * True if 'param' is a parameter of a function declaration or definition,
 * and not a parameter of a function type, e.g. in a function pointer
 * typedef, variable or parameter ('void (*callback)(int* value)').
 */
inline bool isParameterOfFunctionDecl(const clang::ParmVarDecl *param) {
    if (!param)
        return false;

    const auto *function =
        llvm::dyn_cast<clang::FunctionDecl>(param->getDeclContext());

    if (!function)
        return false;

    // A function type written inside a function body gets that function
    // as its context too, so check that it is one of its own parameters
    for (const clang::ParmVarDecl *own : function->parameters()) {
        if (own == param)
            return true;
    }

    return false;
}
