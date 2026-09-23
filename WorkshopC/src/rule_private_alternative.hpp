#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

// Alternative private-field rule.
//
// A field tagged with __attribute__((annotate("workshopc_private_field")))
// (see default/private_tag.h) may only be accessed from a function that:
//
//   a) has a name starting with the owning struct's actual (tag)
//      name, not any typedef alias of it, and
//   b) either
//        - takes a pointer to the field's owning struct as its
//          first parameter, named "self" (const or non-const,
//          typedef or not), or
//        - accesses the field through a plain (non-pointer)
//          local variable or parameter of the owning struct's
//          type, via '.' rather than '->'. This lets a struct's
//          own by-value creator (e.g. a "_pod" or "_make"
//          function with no pointer parameter at all) build the
//          struct directly.
class PrivateAlternativeRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    static constexpr const char *kPrivateFieldTag = "workshopc_private_field";

private:
    bool isThirdParty(const std::string &path) const;

    bool isPrivateTagged(const FieldDecl *field) const;

    const RecordDecl *resolveRecordDecl(QualType type) const;

    bool hasValidSelfAccessor(
        const FunctionDecl *func,
        const RecordDecl *owner) const;

    bool isDirectValueAccess(
        const MemberExpr *memberExpr,
        const RecordDecl *owner) const;

public:
    PrivateAlternativeRule(const Config &cfg,
                           SuppressionManager &sup,
                           Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
