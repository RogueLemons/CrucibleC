#pragma once

#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>

#include "tag_placement.hpp"

// Annotation written by the REF macro (see default/ref_tag.h)
inline constexpr const char *kReferencePointerTag = "workshopc_reference_pointer";

/*
 * True if the parameter carries the reference tag, either itself or on
 * the matching parameter of another declaration of its function.
 */
inline bool hasReferenceTag(const clang::ParmVarDecl *param) {
    if (!param)
        return false;

    auto hasTag = [](const clang::ParmVarDecl *p) {
        for (const auto *attr : p->attrs()) {
            if (const auto *annotate = llvm::dyn_cast<clang::AnnotateAttr>(attr)) {
                if (annotate->getAnnotation() == kReferencePointerTag)
                    return true;
            }
        }

        return false;
    };

    if (hasTag(param))
        return true;

    // A parameter of a function pointer type has no other declarations.
    // Its context can still be a function, when the type is written
    // inside that function's body.
    if (!isParameterOfFunctionDecl(param))
        return false;

    const auto *owner =
        llvm::cast<clang::FunctionDecl>(param->getDeclContext());

    const unsigned index = param->getFunctionScopeIndex();

    for (const clang::FunctionDecl *redecl : owner->redecls()) {
        if (index < redecl->getNumParams() &&
            hasTag(redecl->getParamDecl(index)))
            return true;
    }

    return false;
}
