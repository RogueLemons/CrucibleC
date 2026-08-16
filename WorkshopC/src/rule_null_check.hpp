#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_map>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class NullCheckRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    struct ParamState {
        bool seenGuard = false;
        const Expr *violation = nullptr;
    };

private:
    bool isThirdParty(const std::string &path) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (path.find(p) != std::string::npos)
                return true;
        }
        return false;
    }

    bool isPointerParam(const ParmVarDecl *p) const {
        return p && p->getType()->isPointerType();
    }

    bool isParamRef(const Expr *expr,
                    const ParmVarDecl *param) const {
        if (!expr)
            return false;

        expr = expr->IgnoreParenImpCasts();

        if (const auto *dr = dyn_cast<DeclRefExpr>(expr))
            return dr->getDecl() == param;

        return false;
    }

    bool isNullLiteral(const Expr *expr) const {
        if (!expr)
            return false;

        expr = expr->IgnoreParenImpCasts();

        if (isa<CXXNullPtrLiteralExpr>(expr))
            return true;

        if (const auto *i = dyn_cast<IntegerLiteral>(expr))
            return i->getValue() == 0;

        if (const auto *cast = dyn_cast<CastExpr>(expr))
            return isNullLiteral(cast->getSubExpr());

        return false;
    }

    bool isNullComparison(const Expr *expr,
                          const ParmVarDecl *param) const {
        if (!expr)
            return false;

        expr = expr->IgnoreParenImpCasts();

        const auto *bin = dyn_cast<BinaryOperator>(expr);
        if (!bin || !bin->isComparisonOp())
            return false;

        const Expr *lhs = bin->getLHS()->IgnoreParenImpCasts();
        const Expr *rhs = bin->getRHS()->IgnoreParenImpCasts();

        return (isParamRef(lhs, param) && isNullLiteral(rhs)) ||
               (isParamRef(rhs, param) && isNullLiteral(lhs));
    }

    bool isMacroNullCheck(const Expr *expr) const {
        if (!expr)
            return false;

        if (const auto *call = dyn_cast<CallExpr>(expr)) {
            if (const FunctionDecl *fd = call->getDirectCallee()) {
                std::string name = fd->getNameAsString();

                return name.find("NULL") != std::string::npos ||
                       name.find("null") != std::string::npos;
            }
        }

        return false;
    }

    bool getBoolOption(const std::string &key,
                       bool defaultValue) const {
        auto it = config.options.find(key);
        if (it == config.options.end())
            return defaultValue;

        const std::string &v = it->second;

        return (v == "true" ||
                v == "True" ||
                v == "1" ||
                v == "yes");
    }

    bool isAllowedBooleanUse(const Expr *expr,
                             const ParmVarDecl *param,
                             bool allowBool) const {
        if (!allowBool)
            return false;

        if (!expr)
            return false;

        expr = expr->IgnoreParenImpCasts();

        // if (ptr)
        if (isParamRef(expr, param))
            return true;

        // if (!ptr)
        if (const auto *un = dyn_cast<UnaryOperator>(expr)) {
            if (un->getOpcode() == UO_LNot &&
                isParamRef(un->getSubExpr(), param)) {
                return true;
            }
        }

        // ptr ? a : b
        if (const auto *cond = dyn_cast<ConditionalOperator>(expr)) {
            const Expr *c = cond->getCond()->IgnoreParenImpCasts();

            if (isParamRef(c, param))
                return true;

            if (const auto *un = dyn_cast<UnaryOperator>(c)) {
                if (un->getOpcode() == UO_LNot &&
                    isParamRef(un->getSubExpr(), param)) {
                    return true;
                }
            }
        }

        return false;
    }

    bool isNullGuard(const Expr *expr,
                     const ParmVarDecl *param) const {
        return isNullComparison(expr, param) ||
               isMacroNullCheck(expr);
    }

    bool isDerefOfParam(
        const Expr *expr,
        const ParmVarDecl *param
    ) const {
        if (!expr)
            return false;
    
        expr = expr->IgnoreParenImpCasts();
    
        // Explicit: *param
        if (const auto *un = dyn_cast<UnaryOperator>(expr)) {
            if (un->getOpcode() == UO_Deref)
                return isParamRef(un->getSubExpr(), param);
        }
    
        // Implicit dereference: param->member
        if (const auto *member = dyn_cast<MemberExpr>(expr)) {
            if (member->isArrow())
                return isParamRef(member->getBase(), param);
        }
    
        return false;
    }

public:
    NullCheckRule(const RuleConfig &cfg,
                  const Config &gc,
                  SuppressionManager &sup,
                  Diagnostics &diag)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag) {}

    void run(const MatchFinder::MatchResult &result) override {
        const auto *fn =
            result.Nodes.getNodeAs<FunctionDecl>("function");

        if (!fn || !fn->hasBody())
            return;

        auto &sm = *result.SourceManager;

        SourceLocation loc = fn->getLocation();
        SourceLocation expansionLoc = sm.getExpansionLoc(loc);

        if (suppressions.isSuppressed(sm, expansionLoc))
            return;

        std::string path = sm.getFilename(expansionLoc).str();
        if (!path.empty() && isThirdParty(path))
            return;

        const bool allowBool =
            getBoolOption("allow_direct_ptr_in_if_statement", false);

        std::unordered_map<const ParmVarDecl*, ParamState> states;

        for (const auto *p : fn->parameters()) {
            if (isPointerParam(p))
                states[p] = {};
        }

        if (states.empty())
            return;

        const Stmt *body = fn->getBody();

        std::function<bool(const Stmt*)> walk =
            [&](const Stmt *s) -> bool {
                if (!s)
                    return true;

                const Expr *expr = dyn_cast<Expr>(s);

                for (auto &[param, st] : states) {
                    if (!param)
                        continue;

                    if (!st.seenGuard &&
                        expr &&
                        isNullGuard(expr, param)) {
                        st.seenGuard = true;
                    }

                    if (expr &&
                        isAllowedBooleanUse(expr, param, allowBool)) {
                        st.seenGuard = true;
                    }

                    if (expr &&
                        isDerefOfParam(expr, param)) {

                        if (!st.seenGuard) {
                            st.violation = expr;
                            return false;
                        }
                    }
                }

                for (const Stmt *c : s->children()) {
                    if (!walk(c))
                        return false;
                }

                return true;
            };

        walk(body);

        for (const auto &[param, st] : states) {
            if (!st.violation)
                continue;

            std::string name = param->getNameAsString();
            if (name.empty())
                name = "<unnamed>";

            diagnostics.report(
                config.level,
                sm,
                st.violation->getBeginLoc(),
                "pointer parameter '" + name +
                    "' is dereferenced before null check"
            );
        }
    }
};