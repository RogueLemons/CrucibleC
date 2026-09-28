#include "rule_function_pointer_tags.hpp"

#include "function_pointer_tags.hpp"
#include "reference_tag.hpp"

#include <algorithm>

namespace {

// "move", "out", "modify" or "" for a movement tag annotation
std::string readableTag(const std::string &tag) {
    if (tag == "workshopc_move")
        return "move";

    if (tag == "workshopc_out")
        return "out";

    if (tag == "workshopc_modify")
        return "modify";

    return "";
}

// The array type written in 'loc', through qualifiers and typedefs
ArrayTypeLoc arrayTypeLocOf(TypeLoc loc) {
    while (!loc.isNull()) {
        if (const auto array = loc.getAs<ArrayTypeLoc>())
            return array;

        if (loc.getAs<PointerTypeLoc>() || loc.getAs<FunctionTypeLoc>())
            return {};

        if (const auto typedefLoc = loc.getAs<TypedefTypeLoc>()) {
            const TypedefNameDecl *decl = typedefLoc.getTypePtr()->getDecl();
            const TypeSourceInfo *info = decl ? decl->getTypeSourceInfo() : nullptr;

            if (!info)
                return {};

            loc = info->getTypeLoc();
            continue;
        }

        loc = loc.getNextTypeLoc();
    }

    return {};
}

std::string parameterText(unsigned index, const ParmVarDecl *param) {
    std::string text = "parameter " + std::to_string(index + 1);

    if (param && !param->getName().empty())
        text += " ('" + param->getNameAsString() + "')";

    return text;
}

} // namespace

bool FunctionPointerTagRule::isThirdParty(const std::string &path) const {
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && path.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool FunctionPointerTagRule::shouldIgnore(SourceLocation loc) const {
    if (!sourceManager || loc.isInvalid())
        return true;

    const SourceLocation expansion = sourceManager->getExpansionLoc(loc);

    if (suppressions.isSuppressed(*sourceManager, expansion))
        return true;

    if (sourceManager->isInSystemHeader(expansion))
        return true;

    const std::string path = sourceManager->getFilename(expansion).str();

    return !path.empty() && isThirdParty(path);
}

FunctionPointerTagRule::BoundFunction FunctionPointerTagRule::boundFunctionOf(
    const Expr *source) const
{
    BoundFunction bound;

    // A cast can not hide a mismatch: look at what is cast
    while (source) {
        source = source->IgnoreParenCasts();

        const auto *addressOf = dyn_cast<UnaryOperator>(source);

        if (!addressOf || addressOf->getOpcode() != UO_AddrOf)
            break;

        source = addressOf->getSubExpr();
    }

    if (!source)
        return bound;

    if (const auto *ref = dyn_cast<DeclRefExpr>(source)) {
        if (const auto *function = dyn_cast<FunctionDecl>(ref->getDecl())) {
            bound.known = true;
            bound.description = "function '" + function->getNameAsString() + "'";
            bound.parameters.assign(function->param_begin(), function->param_end());
            return bound;
        }
    }

    const FunctionProtoTypeLoc type = functionTypeLocOfExpr(source);

    if (!type)
        return bound;

    bound.known = true;

    const std::string name = functionPointerNameOf(source);

    bound.description = name.empty()
        ? std::string("the function pointer")
        : "function pointer '" + name + "'";

    for (unsigned i = 0; i < type.getNumParams(); ++i)
        bound.parameters.push_back(type.getParam(i));

    return bound;
}

void FunctionPointerTagRule::checkInitializer(TypeLoc target, const Expr *source) {
    if (target.isNull() || !source)
        return;

    source = source->IgnoreParenImpCasts();

    if (const auto *list = dyn_cast<InitListExpr>(source)) {

        // An array: every element against the element type
        if (const ArrayTypeLoc array = arrayTypeLocOf(target)) {
            for (const Expr *element : list->inits())
                checkInitializer(array.getElementLoc(), element);

            return;
        }

        // A struct or union: every field against the field's type
        const RecordDecl *record = list->getType()->getAsRecordDecl();

        // A single value in braces, e.g. (callback_t){ function }
        if (!record) {
            if (list->getNumInits() == 1)
                checkInitializer(target, list->getInit(0));

            return;
        }

        if (record->isUnion()) {
            const FieldDecl *field = list->getInitializedFieldInUnion();
            const TypeSourceInfo *info = field ? field->getTypeSourceInfo() : nullptr;

            if (info && list->getNumInits() > 0)
                checkInitializer(info->getTypeLoc(), list->getInit(0));

            return;
        }

        unsigned index = 0;

        for (const FieldDecl *field : record->fields()) {
            if (index >= list->getNumInits())
                break;

            if (const TypeSourceInfo *info = field->getTypeSourceInfo())
                checkInitializer(info->getTypeLoc(), list->getInit(index));

            ++index;
        }

        return;
    }

    const FunctionProtoTypeLoc function = functionTypeLocOf(target);

    if (function)
        checkBinding(function, target.getType().getAsString(), source);
}

void FunctionPointerTagRule::checkBinding(
    FunctionProtoTypeLoc target,
    const std::string &targetName,
    const Expr *source)
{
    if (!target || !source)
        return;

    source = source->IgnoreParenImpCasts();

    // flag ? first : second, both may end up in the function pointer
    if (const auto *conditional = dyn_cast<ConditionalOperator>(source)) {
        checkBinding(target, targetName, conditional->getTrueExpr());
        checkBinding(target, targetName, conditional->getFalseExpr());
        return;
    }

    // A type from a system header or a third party library has no tags
    if (shouldIgnore(target.getBeginLoc()))
        return;

    const SourceLocation loc = source->getExprLoc();

    if (shouldIgnore(loc))
        return;

    const BoundFunction bound = boundFunctionOf(source);

    if (!bound.known)
        return;

    const bool checkMovement =
        config.argumentPointerMovementRule.level != RuleLevel::Off;

    const bool checkReference =
        config.referencePointerRule.level != RuleLevel::Off;

    const unsigned count = std::min<unsigned>(
        bound.parameters.size(), target.getNumParams());

    const std::string prefix =
        bound.description + " can not be used as '" + targetName + "': ";

    const SourceLocation reportLoc = sourceManager->getExpansionLoc(loc);

    for (unsigned i = 0; i < count; ++i) {
        const ParmVarDecl *own = bound.parameters[i];
        const ParmVarDecl *expected = target.getParam(i);

        if (!own || !expected)
            continue;

        if (checkMovement) {
            const std::string ownTag = readableTag(movementTagOf(own));
            const std::string expectedTag = readableTag(movementTagOf(expected));

            if (ownTag != expectedTag) {
                std::string detail;

                if (ownTag.empty())
                    detail = "is not tagged, but '" + targetName + "' tags it " + expectedTag;
                else if (expectedTag.empty())
                    detail = "is tagged " + ownTag + ", but '" + targetName + "' does not tag it";
                else
                    detail = "is tagged " + ownTag + ", but '" + targetName + "' tags it " + expectedTag;

                diagnostics.report(
                    config.argumentPointerMovementRule.level,
                    DiagCode::FunctionPointerMovementTagMismatch,
                    *sourceManager,
                    reportLoc,
                    prefix + parameterText(i, own) + " " + detail
                );
            }
        }

        if (checkReference) {
            const bool ownReference = hasReferenceTag(own);
            const bool expectedReference = hasReferenceTag(expected);

            if (ownReference != expectedReference) {
                diagnostics.report(
                    config.referencePointerRule.level,
                    DiagCode::FunctionPointerReferenceTagMismatch,
                    *sourceManager,
                    reportLoc,
                    prefix + parameterText(i, own) +
                    (ownReference
                        ? " is a reference, but not in '" + targetName + "'"
                        : " is not a reference, but is one in '" + targetName + "'")
                );
            }
        }
    }
}

void FunctionPointerTagRule::checkConditionalCallee(const CallExpr *call) {
    if (call->getDirectCallee())
        return;

    std::vector<const Expr *> branches;
    collectCalleeBranches(call->getCallee(), branches);

    if (branches.size() < 2)
        return;

    const SourceLocation loc = call->getCallee()->getExprLoc();

    if (shouldIgnore(loc))
        return;

    const Expr *first = branches.front();
    const FunctionProtoTypeLoc firstType = functionTypeLocOfExpr(first);

    if (!firstType)
        return;

    std::string firstName = functionPointerNameOf(first);

    if (firstName.empty())
        firstName = "<function pointer>";

    const SourceLocation reportLoc = sourceManager->getExpansionLoc(loc);

    // Every other branch against the first, the call can only follow
    // one set of tags
    for (size_t b = 1; b < branches.size(); ++b) {
        const FunctionProtoTypeLoc type = functionTypeLocOfExpr(branches[b]);

        if (!type)
            continue;

        std::string name = functionPointerNameOf(branches[b]);

        if (name.empty())
            name = "<function pointer>";

        const std::string prefix =
            "the conditional call can go to '" + firstName + "' or '" + name + "', ";

        const unsigned count = std::min(firstType.getNumParams(), type.getNumParams());

        for (unsigned i = 0; i < count; ++i) {
            const ParmVarDecl *a = firstType.getParam(i);
            const ParmVarDecl *other = type.getParam(i);

            if (!a || !other)
                continue;

            if (config.argumentPointerMovementRule.level != RuleLevel::Off) {
                std::string tagA = readableTag(movementTagOf(a));
                std::string tagB = readableTag(movementTagOf(other));

                if (tagA != tagB) {
                    diagnostics.report(
                        config.argumentPointerMovementRule.level,
                        DiagCode::FunctionPointerMovementTagMismatch,
                        *sourceManager,
                        reportLoc,
                        prefix + "which tag " + parameterText(i, a) + " differently (" +
                        (tagA.empty() ? "not tagged" : tagA) + " and " +
                        (tagB.empty() ? "not tagged" : tagB) + ")"
                    );
                }
            }

            if (config.referencePointerRule.level != RuleLevel::Off &&
                hasReferenceTag(a) != hasReferenceTag(other))
            {
                diagnostics.report(
                    config.referencePointerRule.level,
                    DiagCode::FunctionPointerReferenceTagMismatch,
                    *sourceManager,
                    reportLoc,
                    prefix + "but " + parameterText(i, a) + " is a reference in only one of them"
                );
            }
        }
    }
}

FunctionPointerTagRule::FunctionPointerTagRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{}

void FunctionPointerTagRule::bindFinder(MatchFinder &finder) {
    // callback_t callback = function;
    finder.addMatcher(
        varDecl(
            hasInitializer(expr()),
            unless(isExpansionInSystemHeader())
        ).bind("var"),
        this
    );

    // callback = function;
    finder.addMatcher(
        binaryOperator(
            hasOperatorName("="),
            unless(isExpansionInSystemHeader())
        ).bind("assign"),
        this
    );

    // register_callback(function);
    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("call"),
        this
    );

    // (handlers){ .on_event = function }
    finder.addMatcher(
        compoundLiteralExpr(
            unless(isExpansionInSystemHeader())
        ).bind("literal"),
        this
    );

    // return function; in a function returning a function pointer
    finder.addMatcher(
        returnStmt(
            hasAncestor(functionDecl().bind("function")),
            unless(isExpansionInSystemHeader())
        ).bind("return"),
        this
    );
}

void FunctionPointerTagRule::run(const MatchFinder::MatchResult &result) {
    sourceManager = result.SourceManager;

    if (const auto *var = result.Nodes.getNodeAs<VarDecl>("var")) {
        if (const TypeSourceInfo *info = var->getTypeSourceInfo())
            checkInitializer(info->getTypeLoc(), var->getInit());

        return;
    }

    if (const auto *assign = result.Nodes.getNodeAs<BinaryOperator>("assign")) {
        const Expr *target = assign->getLHS();

        checkBinding(
            functionTypeLocOfExpr(target),
            target->getType().getAsString(),
            assign->getRHS());

        return;
    }

    if (const auto *literal = result.Nodes.getNodeAs<CompoundLiteralExpr>("literal")) {
        if (const TypeSourceInfo *info = literal->getTypeSourceInfo())
            checkInitializer(info->getTypeLoc(), literal->getInitializer());

        return;
    }

    if (const auto *call = result.Nodes.getNodeAs<CallExpr>("call")) {
        checkConditionalCallee(call);

        const CalleeParameters callee = calleeParametersOf(call);

        if (!callee.known)
            return;

        const unsigned count = std::min<unsigned>(
            call->getNumArgs(), callee.parameters.size());

        for (unsigned i = 0; i < count; ++i) {
            const ParmVarDecl *param = callee.parameters[i];
            const TypeSourceInfo *info = param ? param->getTypeSourceInfo() : nullptr;

            if (info)
                checkInitializer(info->getTypeLoc(), call->getArg(i));
        }

        return;
    }

    if (const auto *ret = result.Nodes.getNodeAs<ReturnStmt>("return")) {
        const auto *function = result.Nodes.getNodeAs<FunctionDecl>("function");
        const FunctionTypeLoc type = function ? function->getFunctionTypeLoc() : FunctionTypeLoc{};

        if (type && ret->getRetValue())
            checkInitializer(type.getReturnLoc(), ret->getRetValue());
    }
}
