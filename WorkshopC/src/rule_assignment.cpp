#include "rule_assignment.hpp"

const Expr* AssignmentRule::norm(const Expr *e) const {
    if (!e) return nullptr;
    return e->IgnoreParenCasts()->IgnoreImpCasts();
}

bool AssignmentRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }
    return false;
}

bool AssignmentRule::shouldSkip(SourceManager &sm, SourceLocation loc) const {
    if (loc.isInvalid())
        return true;

    if (suppressions.isSuppressed(sm, loc))
        return true;

    std::string file = sm.getFilename(loc).str();
    return (!file.empty() && isThirdParty(file));
}

std::string AssignmentRule::nameOf(const Decl *d) const {
    if (!d) return "<null>";

    if (const auto *nd = dyn_cast<NamedDecl>(d)) {
        std::string n = nd->getNameAsString();
        return n.empty() ? "<anonymous>" : n;
    }

    return "<unnamed>";
}

bool AssignmentRule::isNullExpr(const Expr *e) const {
    e = norm(e);

    if (!e)
        return false;

    if (const auto *dre = dyn_cast<DeclRefExpr>(e))
        return dre->getNameInfo().getAsString() == "NULL";

    if (const auto *il = dyn_cast<IntegerLiteral>(e))
        return il->getValue() == 0;

    if (const auto *cast = dyn_cast<CStyleCastExpr>(e)) {

        const Expr *sub = norm(cast->getSubExpr());

        if (const auto *il = dyn_cast<IntegerLiteral>(sub))
            return il->getValue() == 0;
    }

    if (isa<CXXNullPtrLiteralExpr>(e))
        return true;

    return false;
}

bool AssignmentRule::containsPointer(QualType qt) const {
    qt = qt.getUnqualifiedType();

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

bool AssignmentRule::isLiteralZeroInit(const Expr *e, SourceManager &sm) const {
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

AssignmentRule::AssignmentRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag
)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{}

void AssignmentRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        varDecl(
            unless(isExpansionInSystemHeader())
        ).bind("varDecl"),
        this
    );

    finder.addMatcher(
        binaryOperator(
            isAssignmentOperator()
        ).bind("assignmentOp"),
        this
    );

    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("callExpr"),
        this
    );
}

void AssignmentRule::run(const MatchFinder::MatchResult &result) {
    SourceManager &sm = *result.SourceManager;

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
                config.assignmentRule.level,
                sm,
                loc,
                "variable '" + nameOf(vd) +
                "' must be initialized at declaration"
            );
        }
        else if (qt->isArrayType() && !vd->hasInit()) {

            QualType elementType =
                result.Context->getBaseElementType(qt)
                    .getCanonicalType();

            if (elementType->isBuiltinType() ||
                elementType->isPointerType())
            {
                diagnostics.report(
                    config.assignmentRule.level,
                    sm,
                    loc,
                    "array '" + nameOf(vd) +
                    "' must be initialized at declaration"
                );
            }
        }

        if (config.assignmentRule.forbidZeroInitForObjectsWithPointers) {

            const Expr *init = vd->getInit();

            if (init &&
                containsPointer(vd->getType()) &&
                isLiteralZeroInit(init, sm))
            {
                diagnostics.report(
                    config.assignmentRule.level,
                    sm,
                    vd->getLocation(),
                    "object '" + nameOf(vd) +
                    "' containing pointers may not be initialized with {0}"
                );
            }
        }

        if (config.assignmentRule.forbidNullAssign) {

            const Expr *init = vd->getInit();
            if (!init)
                return;

            init = init->IgnoreImplicit();

            if (const auto *ile = dyn_cast<InitListExpr>(init)) {

                const RecordDecl *rd =
                    vd->getType()->getAsRecordDecl();

                unsigned idx = 0;

                for (const Expr *child : ile->inits()) {

                    child = norm(child);
                    if (!child)
                        continue;

                    const FieldDecl *field = nullptr;

                    if (rd) {
                        unsigned i = 0;
                        for (const FieldDecl *f : rd->fields()) {
                            if (i == idx) {
                                field = f;
                                break;
                            }
                            ++i;
                        }
                    }

                    ++idx;

                    if (field && field->getType()->isPointerType()) {

                        if (isNullExpr(child)) {

                            diagnostics.report(
                                config.assignmentRule.level,
                                sm,
                                child->getExprLoc(),
                                "NULL used in pointer field initializer for '" +
                                nameOf(vd) + "." +
                                field->getNameAsString() + "'"
                            );
                        }
                    }
                }
            }
        }

        // FIXED: forbid_mut_arg_pointer (now handles all Clang wrapping)
        if (config.assignmentRule.forbidMutArgPointer) {

            const Expr *init = vd->getInit();
            if (!init)
                return;

            init = norm(init);
            init = init->IgnoreImplicit();

            const Expr *sub = init;

            if (const auto *uop = dyn_cast<UnaryOperator>(sub)) {
                if (uop->getOpcode() == UO_AddrOf)
                    sub = norm(uop->getSubExpr());
            }

            if (const auto *dre = dyn_cast<DeclRefExpr>(sub)) {

                if (const auto *pd =
                        dyn_cast<ParmVarDecl>(dre->getDecl()))
                {
                    if (!pd->getType().isConstQualified()) {

                        diagnostics.report(
                            config.assignmentRule.level,
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

        if (config.assignmentRule.forbidNullAssign) {

            if (lhs &&
                lhs->getType()->isPointerType() &&
                isNullExpr(rhs))
            {
                if (const auto *dre = dyn_cast<DeclRefExpr>(lhs)) {

                    diagnostics.report(
                        config.assignmentRule.level,
                        sm,
                        loc,
                        "pointer '" +
                        nameOf(dre->getDecl()) +
                        "' cannot be assigned NULL"
                    );
                }
                else if (const auto *me = dyn_cast<MemberExpr>(lhs)) {

                    diagnostics.report(
                        config.assignmentRule.level,
                        sm,
                        loc,
                        "pointer field '" +
                        me->getMemberDecl()->getNameAsString() +
                        "' cannot be assigned NULL"
                    );
                }
            }
        }

        if (config.assignmentRule.forbidArgReassign) {

            const Expr *nLHS = norm(lhs);

            if (const auto *dre = dyn_cast<DeclRefExpr>(nLHS)) {

                if (const auto *pd =
                        dyn_cast<ParmVarDecl>(dre->getDecl()))
                {
                    diagnostics.report(
                        config.assignmentRule.level,
                        sm,
                        loc,
                        "function argument '" +
                        nameOf(pd) +
                        "' reassignment is forbidden"
                    );
                }
            }

            if (const auto *me = dyn_cast<MemberExpr>(nLHS)) {

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
                                config.assignmentRule.level,
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

            if (config.assignmentRule.forbidNullAsArg) {

                if (isNullExpr(arg)) {

                    diagnostics.report(
                        config.assignmentRule.level,
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
