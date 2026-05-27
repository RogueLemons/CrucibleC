#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>

#include <string>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class AssignmentRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    bool opt(const std::string &key) const {
        auto it = config.options.find(key);
        return it != config.options.end() && it->second == "true";
    }

    const Expr* norm(const Expr *e) const {
        if (!e) return nullptr;
        return e->IgnoreParenCasts()->IgnoreImpCasts();
    }

    bool isThirdParty(const std::string &path) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (path.find(p) != std::string::npos)
                return true;
        }
        return false;
    }

    bool shouldSkip(SourceManager &sm, SourceLocation loc) const {
        if (loc.isInvalid())
            return true;

        if (suppressions.isSuppressed(sm, loc))
            return true;

        std::string file = sm.getFilename(loc).str();
        return (!file.empty() && isThirdParty(file));
    }

    std::string nameOf(const Decl *d) const {
        if (!d) return "<null>";

        if (const auto *nd = dyn_cast<NamedDecl>(d)) {
            std::string n = nd->getNameAsString();
            return n.empty() ? "<anonymous>" : n;
        }

        return "<unnamed>";
    }

    bool isNullExpr(const Expr *e) const {
        e = norm(e);

        if (!e)
            return false;

        // NULL
        if (const auto *dre = dyn_cast<DeclRefExpr>(e))
            return dre->getNameInfo().getAsString() == "NULL";

        // pure 0
        if (const auto *il = dyn_cast<IntegerLiteral>(e))
            return il->getValue() == 0;

        // (void*)0 / (int*)0
        if (const auto *cast = dyn_cast<CStyleCastExpr>(e)) {

            const Expr *sub = norm(cast->getSubExpr());

            if (const auto *il = dyn_cast<IntegerLiteral>(sub))
                return il->getValue() == 0;
        }

        // nullptr
        if (isa<CXXNullPtrLiteralExpr>(e))
            return true;

        return false;
    }

    bool containsPointer(QualType qt) const {
        qt = qt.getUnqualifiedType();

        // direct pointer
        if (qt->isPointerType())
            return true;

        const RecordType *rt = qt->getAsStructureType();

        if (!rt)
            rt = qt->getAsUnionType();

        if (!rt)
            return false;

        const RecordDecl *rd = rt->getDecl();

        if (!rd)
            return false;

        rd = rd->getDefinition();

        if (!rd)
            return false;

        for (const FieldDecl *field : rd->fields()) {

            if (containsPointer(field->getType()))
                return true;
        }

        return false;
    }

    bool isLiteralZeroInit(
        const Expr *e,
        SourceManager &sm
    ) const {
        if (!e)
            return false;

        e = e->IgnoreImplicit();

        const auto *ile = dyn_cast<InitListExpr>(e);

        if (!ile)
            return false;

        SourceRange range = ile->getSourceRange();

        if (range.isInvalid())
            return false;

        std::string text =
            Lexer::getSourceText(
                CharSourceRange::getTokenRange(range),
                sm,
                LangOptions()
            ).str();

        return text == "{0}";
    }

public:
    AssignmentRule(
        const RuleConfig &cfg,
        const Config &gc,
        SuppressionManager &sup,
        Diagnostics &diag
    )
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag)
    {}

    void run(const MatchFinder::MatchResult &result) override {
        SourceManager &sm = *result.SourceManager;

        // =====================================================
        // VAR DECL INITIALIZATION RULE
        // =====================================================
        if (const auto *vd =
                result.Nodes.getNodeAs<VarDecl>("varDecl"))
        {
            SourceLocation loc = vd->getLocation();

            if (shouldSkip(sm, loc))
                return;

            if (vd->isImplicit())
                return;

            if (isa<ParmVarDecl>(vd))
                return;

            QualType qt = vd->getType().getCanonicalType();

            if ((qt->isBuiltinType() || qt->isPointerType()) && !vd->hasInit()) {

                diagnostics.report(
                    config.level,
                    sm,
                    loc,
                    "variable '" + nameOf(vd) +
                    "' must be initialized at declaration"
                );
            }

            // =====================================================
            // forbid_zero_init_for_objects_with_pointers
            // =====================================================
            if (opt("forbid_zero_init_for_objects_with_pointers")) {

                const Expr *init = vd->getInit();

                if (init &&
                    containsPointer(vd->getType()) &&
                    isLiteralZeroInit(init, sm))
                {
                    diagnostics.report(
                        config.level,
                        sm,
                        vd->getLocation(),
                        "object '" + nameOf(vd) +
                        "' containing pointers may not be initialized with {0}"
                    );
                }
            }

            // =====================================================
            // forbid_mut_arg_pointer
            // =====================================================
            if (opt("forbid_mut_arg_pointer")) {

                const Expr *init = vd->getInit();

                if (!init)
                    return;

                init = norm(init);

                if (const auto *uop = dyn_cast<UnaryOperator>(init)) {

                    if (uop->getOpcode() != UO_AddrOf)
                        return;

                    const Expr *sub = norm(uop->getSubExpr());

                    if (const auto *dre =
                            dyn_cast<DeclRefExpr>(sub))
                    {
                        if (const auto *pd =
                                dyn_cast<ParmVarDecl>(dre->getDecl()))
                        {
                            if (!pd->getType().isConstQualified()) {

                                diagnostics.report(
                                    config.level,
                                    sm,
                                    vd->getLocation(),
                                    "taking address of non-const argument '" +
                                    nameOf(pd) +
                                    "' is forbidden"
                                );
                            }
                        }
                    }
                }
            }
        }

        // =====================================================
        // ASSIGNMENT RULES
        // =====================================================
        if (const auto *op =
                result.Nodes.getNodeAs<BinaryOperator>("assignmentOp"))
        {
            if (!op->isAssignmentOp())
                return;

            SourceLocation loc =
                sm.getSpellingLoc(op->getOperatorLoc());

            if (shouldSkip(sm, loc))
                return;

            const Expr *lhs = norm(op->getLHS());
            const Expr *rhs = norm(op->getRHS());

            // -------------------------------------------------
            // NULL ASSIGNMENT
            // -------------------------------------------------
            if (opt("forbid_null_assign")) {

                if (lhs &&
                    lhs->getType()->isPointerType() &&
                    isNullExpr(rhs))
                {
                    if (const auto *dre = dyn_cast<DeclRefExpr>(lhs)) {

                        diagnostics.report(
                            config.level,
                            sm,
                            loc,
                            "pointer '" +
                            nameOf(dre->getDecl()) +
                            "' cannot be assigned NULL"
                        );
                    }
                    else if (const auto *me = dyn_cast<MemberExpr>(lhs)) {

                        diagnostics.report(
                            config.level,
                            sm,
                            loc,
                            "pointer field '" +
                            me->getMemberDecl()->getNameAsString() +
                            "' cannot be assigned NULL"
                        );
                    }
                }
            }

            // -------------------------------------------------
            // ARGUMENT REASSIGNMENT + STRUCT RULE
            // -------------------------------------------------
            if (opt("forbid_arg_reassign")) {

                // arg = value
                if (const auto *dre = dyn_cast<DeclRefExpr>(lhs)) {

                    if (const auto *pd =
                            dyn_cast<ParmVarDecl>(dre->getDecl()))
                    {
                        diagnostics.report(
                            config.level,
                            sm,
                            loc,
                            "function argument '" +
                            nameOf(pd) +
                            "' reassignment is forbidden"
                        );
                    }
                }

                // arg.field = value (by-value only)
                if (const auto *me = dyn_cast<MemberExpr>(lhs)) {

                    const Expr *base = norm(me->getBase());

                    if (const auto *dre =
                            dyn_cast<DeclRefExpr>(base))
                    {
                        if (const auto *pd =
                                dyn_cast<ParmVarDecl>(dre->getDecl()))
                        {
                            QualType qt =
                                pd->getType().getCanonicalType();

                            if (!qt->isPointerType()) {

                                diagnostics.report(
                                    config.level,
                                    sm,
                                    loc,
                                    "fields of by-value argument '" +
                                    nameOf(pd) +
                                    "' may not be modified"
                                );
                            }
                        }
                    }
                }
            }
        }

        // =====================================================
        // CALL RULE (NULL ARGUMENTS)
        // =====================================================
        if (const auto *call =
                result.Nodes.getNodeAs<CallExpr>("callExpr"))
        {
            SourceLocation loc =
                sm.getSpellingLoc(call->getExprLoc());

            if (shouldSkip(sm, loc))
                return;

            std::string funcName = "<unknown>";

            if (const auto *callee = call->getDirectCallee()) {
                funcName = callee->getNameAsString();
            }

            for (const Expr *arg : call->arguments()) {

                arg = norm(arg);

                if (!arg)
                    continue;

                if (opt("forbid_null_as_arg")) {

                    if (isNullExpr(arg)) {

                        diagnostics.report(
                            config.level,
                            sm,
                            arg->getExprLoc(),
                            "NULL passed as argument to function '" +
                            funcName + "'"
                        );
                    }
                }
            }
        }
    }
};