#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/Basic/SourceManager.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class ArgumentPointerCallsiteRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    static constexpr const char* kMoveTag = "workshopc_move";
    static constexpr const char* kOutTag  = "workshopc_out";
    static constexpr const char* kModTag  = "workshopc_modify";

private:

    bool isThirdParty(const std::string &path) const;

    std::string getCalleeName(const CallExpr *CE) const;

    bool isWrapperCall(const Expr *E,
                       const std::string &expected) const;

    bool getUsedWrapper(const Expr *E,
                    std::string &wrapperName) const;

    std::string getParamTag(const ParmVarDecl *P) const;

    void report(const std::string &msg,
                const SourceManager &sm,
                SourceLocation loc);

public:
    ArgumentPointerCallsiteRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
