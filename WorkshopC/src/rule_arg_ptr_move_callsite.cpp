#include "rule_arg_ptr_move_callsite.hpp"

#include <algorithm>

bool ArgumentPointerCallsiteRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (path.find(p) != std::string::npos)
            return true;
    }
    return false;
}

std::string ArgumentPointerCallsiteRule::getCalleeName(const CallExpr *CE) const {
    if (!CE)
        return "";

    const FunctionDecl *FD = CE->getDirectCallee();
    if (!FD)
        return "";

    return FD->getNameAsString();
}

bool ArgumentPointerCallsiteRule::isWrapperCall(const Expr *E,
                   const std::string &expected) const
{
    if (!E)
        return false;

    E = E->IgnoreParenImpCasts();

    while (true) {

        if (const auto *Cast = dyn_cast<CastExpr>(E)) {
            E = Cast->getSubExpr()->IgnoreParenImpCasts();
            continue;
        }

        if (const auto *Paren = dyn_cast<ParenExpr>(E)) {
            E = Paren->getSubExpr()->IgnoreParenImpCasts();
            continue;
        }

        break;
    }

    const auto *Call = dyn_cast<CallExpr>(E);
    if (!Call)
        return false;

    const FunctionDecl *FD = Call->getDirectCallee();
    if (!FD)
        return false;

    const std::string name = FD->getNameAsString();

    return name == expected;
}

bool ArgumentPointerCallsiteRule::getUsedWrapper(const Expr *E,
                std::string &wrapperName) const
{
    wrapperName.clear();

    if (!E)
        return false;

    E = E->IgnoreParenImpCasts();

    while (true) {

        if (const auto *Cast =
            dyn_cast<CastExpr>(E))
        {
            E = Cast->getSubExpr()
                ->IgnoreParenImpCasts();
            continue;
        }

        if (const auto *Paren =
            dyn_cast<ParenExpr>(E))
        {
            E = Paren->getSubExpr()
                ->IgnoreParenImpCasts();
            continue;
        }

        break;
    }

    const auto *Call =
        dyn_cast<CallExpr>(E);

    if (!Call)
        return false;

    const FunctionDecl *FD =
        Call->getDirectCallee();

    if (!FD)
        return false;

    wrapperName =
        FD->getNameAsString();

    return
        wrapperName == "workshopc_move" ||
        wrapperName == "workshopc_out" ||
        wrapperName == "workshopc_modify";
}

std::string ArgumentPointerCallsiteRule::getParamTag(const ParmVarDecl *P) const {
    for (const auto *attr : P->attrs()) {

        if (const auto *A =
            dyn_cast<AnnotateAttr>(attr))
        {
            StringRef t = A->getAnnotation();

            if (t == kMoveTag ||
                t == kOutTag  ||
                t == kModTag)
            {
                return t.str();
            }
        }
    }

    return "";
}

std::string ArgumentPointerCallsiteRule::getOwnParamTag(const ParmVarDecl *P) const {
    if (!P)
        return "";

    const std::string tag = getParamTag(P);

    if (!tag.empty())
        return tag;

    const auto *owner = dyn_cast<FunctionDecl>(P->getDeclContext());

    if (!owner)
        return "";

    const unsigned index = P->getFunctionScopeIndex();

    for (const FunctionDecl *redecl : owner->redecls()) {
        if (index < redecl->getNumParams()) {
            const std::string redeclTag = getParamTag(redecl->getParamDecl(index));

            if (!redeclTag.empty())
                return redeclTag;
        }
    }

    return "";
}

void ArgumentPointerCallsiteRule::checkMoveOfBorrowedParam(
    const CallExpr *CE,
    const FunctionDecl *FD,
    const SourceManager &sm)
{
    const unsigned count = std::min(CE->getNumArgs(), FD->getNumParams());

    for (unsigned i = 0; i < count; ++i) {
        if (getParamTag(FD->getParamDecl(i)) != kMoveTag)
            continue;

        // Strip parentheses, casts and the operator wrapper calls
        const Expr *arg = CE->getArg(i);

        while (arg) {
            arg = arg->IgnoreParenCasts();

            const auto *call = dyn_cast<CallExpr>(arg);
            const FunctionDecl *wrapper = call ? call->getDirectCallee() : nullptr;

            if (!wrapper || call->getNumArgs() != 1)
                break;

            const std::string name = wrapper->getNameAsString();

            if (name != "workshopc_move" &&
                name != "workshopc_out" &&
                name != "workshopc_modify")
                break;

            arg = call->getArg(0);
        }

        // param, or *param for what an out parameter points to
        if (const auto *deref = dyn_cast_or_null<UnaryOperator>(arg)) {
            if (deref->getOpcode() == UO_Deref)
                arg = deref->getSubExpr()->IgnoreParenCasts();
        }

        const auto *ref = dyn_cast_or_null<DeclRefExpr>(arg);
        const auto *param = ref ? dyn_cast<ParmVarDecl>(ref->getDecl()) : nullptr;

        if (!param)
            continue;

        const std::string ownTag = getOwnParamTag(param);

        if (ownTag != kModTag && ownTag != kOutTag)
            continue;

        report(
            DiagCode::BorrowedPointerMoved,
            std::string(ownTag == kModTag ? "modify" : "out") +
            " parameter '" + param->getNameAsString() +
            "' may not be moved to parameter '" +
            FD->getParamDecl(i)->getNameAsString() +
            "' of function '" + FD->getNameAsString() +
            "', the function only borrows it and does not own it",
            sm,
            CE->getBeginLoc()
        );
    }
}

void ArgumentPointerCallsiteRule::report(DiagCode code,
            const std::string &msg,
            const SourceManager &sm,
            SourceLocation loc)
{
    diagnostics.report(
        config.argumentPointerMovementRule.level,
        code,
        sm,
        loc,
        msg
    );
}

ArgumentPointerCallsiteRule::ArgumentPointerCallsiteRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{}

void ArgumentPointerCallsiteRule::bindFinder(MatchFinder &finder) {
    finder.addMatcher(
        callExpr(unless(isExpansionInSystemHeader()))
            .bind("call"),
        this
    );
}

void ArgumentPointerCallsiteRule::run(const MatchFinder::MatchResult &result) {

    const auto *CE =
        result.Nodes.getNodeAs<CallExpr>("call");

    if (!CE)
        return;

    const auto &sm = *result.SourceManager;

    SourceLocation loc =
        CE->getBeginLoc();

    SourceLocation expLoc =
        sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expLoc))
        return;

    if (sm.isInSystemHeader(expLoc))
        return;

    std::string path =
        sm.getFilename(expLoc).str();

    if (!path.empty() &&
        isThirdParty(path))
    {
        return;
    }

    const FunctionDecl *FD =
        CE->getDirectCallee();

    if (!FD)
        return;

    const std::string calleeName =
        FD->getNameAsString();

    // Ignore wrapper helper calls
    if (calleeName == "workshopc_modify" ||
        calleeName == "workshopc_move" ||
        calleeName == "workshopc_out")
    {
        return;
    }

    checkMoveOfBorrowedParam(CE, FD, sm);

    for (unsigned i = 0;
         i < CE->getNumArgs();
         ++i)
    {
        const ParmVarDecl *P = nullptr;

        if (i < FD->getNumParams())
            P = FD->getParamDecl(i);

        if (!P)
            continue;

        const Expr *Arg =
            CE->getArg(i);

        std::string usedWrapper;

        bool hasWrapper =
            getUsedWrapper(
                Arg,
                usedWrapper
            );

        std::string tag =
            getParamTag(P);

        if (tag.empty()) {

            if (hasWrapper) {

                std::string operatorName;

                if (usedWrapper ==
                    "workshopc_move")
                {
                    operatorName = "move_operator";
                }
                else if (usedWrapper ==
                         "workshopc_out")
                {
                    operatorName = "out_operator";
                }
                else if (usedWrapper ==
                         "workshopc_modify")
                {
                    operatorName = "modify_operator";
                }

                report(
                    DiagCode::OperatorOnUntaggedParameter,
                    operatorName +
                    "(...) used for parameter '" +
                    P->getNameAsString() +
                    "' in function '" +
                    FD->getNameAsString() +
                    "', but the parameter is not tagged",
                    sm,
                    CE->getBeginLoc()
                );
            }

            continue;
        }

        std::string expected;

        if (tag == kModTag) {

            bool enabled =
                config.argumentPointerMovementRule.requireOperatorForModifyCallsite;

            if (!enabled) {

                if (hasWrapper) {

                    report(
                        DiagCode::OperatorDisabled,
                        "modify_operator(...) used for parameter '" +
                        P->getNameAsString() +
                        "' in function '" +
                        FD->getNameAsString() +
                        "', but modify callsite operators are disabled",
                        sm,
                        CE->getBeginLoc()
                    );
                }

                continue;
            }

            expected = "workshopc_modify";
        }
        else if (tag == kMoveTag) {

            bool enabled =
                config.argumentPointerMovementRule.requireOperatorForMoveCallsite;

            if (!enabled) {

                if (hasWrapper) {

                    report(
                        DiagCode::OperatorDisabled,
                        "move_operator(...) used for parameter '" +
                        P->getNameAsString() +
                        "' in function '" +
                        FD->getNameAsString() +
                        "', but move callsite operators are disabled",
                        sm,
                        CE->getBeginLoc()
                    );
                }

                continue;
            }

            expected = "workshopc_move";
        }
        else if (tag == kOutTag) {

            bool enabled =
                config.argumentPointerMovementRule.requireOperatorForOutCallsite;

            if (!enabled) {

                if (hasWrapper) {

                    report(
                        DiagCode::OperatorDisabled,
                        "out_operator(...) used for parameter '" +
                        P->getNameAsString() +
                        "' in function '" +
                        FD->getNameAsString() +
                        "', but out callsite operators are disabled",
                        sm,
                        CE->getBeginLoc()
                    );
                }

                continue;
            }

            expected = "workshopc_out";
        }

        bool wrapped = false;

        if (!expected.empty())
            wrapped =
                isWrapperCall(
                    Arg,
                    expected
                );

        if (!wrapped) {

            std::string operatorName;

            if (tag == kModTag)
                operatorName = "modify_operator";

            else if (tag == kMoveTag)
                operatorName = "move_operator";

            else if (tag == kOutTag)
                operatorName = "out_operator";

            report(
                DiagCode::OperatorMissing,
                "missing " +
                operatorName +
                "(...) at call site for parameter '" +
                P->getNameAsString() +
                "' in function '" +
                FD->getNameAsString() +
                "'",
                sm,
                CE->getBeginLoc()
            );
        }
    }
}
