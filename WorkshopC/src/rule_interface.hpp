#pragma once

#include <clang/AST/Decl.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

/*
 * The interface part of the interfaces rule. An interface struct, named
 * with interface_suffix, holds exactly two fields:
 *
 *   void* object;                    the object the functions work on
 *   const <some>_vtable* vtable;     its functions
 *
 * A const interface, named with const_interface_suffix, holds a
 * 'const void* object' instead. With
 * interface_must_have_fields_that_are_private_alternative and the
 * private alternative rule on, both fields are marked private.
 */
class InterfaceRule : public MatchFinder::MatchCallback {
private:
    const Config &config;
    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    bool isThirdParty(const std::string &path) const;
    bool shouldIgnore(const SourceManager &sm, SourceLocation loc) const;

    void report(
        DiagCode code,
        SourceLocation loc,
        const std::string &message,
        const SourceManager &sm) const;

    void checkObjectField(
        const FieldDecl *field,
        const std::string &interfaceName,
        bool isConst,
        const SourceManager &sm) const;

    void checkVtableField(
        const FieldDecl *field,
        const std::string &interfaceName,
        const SourceManager &sm) const;

    void checkInterface(
        const RecordDecl *record,
        const SourceManager &sm) const;

public:
    InterfaceRule(const Config &cfg,
                  SuppressionManager &sup,
                  Diagnostics &diag);

    void bindFinder(MatchFinder &finder);
    void run(const MatchFinder::MatchResult &result) override;
};
