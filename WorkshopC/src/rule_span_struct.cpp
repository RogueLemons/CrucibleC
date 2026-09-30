#include "rule_span_struct.hpp"

#include <clang/AST/ParentMapContext.h>

#include <algorithm>

bool SpanStructRule::isThirdParty(const std::string &path) const
{
    for (const auto &include : config.thirdPartyIncludes) {
        if (!include.empty() && path.find(include) != std::string::npos)
            return true;
    }

    return false;
}

bool SpanStructRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    const SourceLocation spelling = sm.getSpellingLoc(loc);
    const SourceLocation expansion = sm.getExpansionLoc(loc);

    if (suppressions.isSuppressed(sm, expansion))
        return true;

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string spellingPath = sm.getFilename(spelling).str();
    const std::string expansionPath = sm.getFilename(expansion).str();

    return spellingPath.empty() ||
           isThirdParty(spellingPath) ||
           (!expansionPath.empty() && isThirdParty(expansionPath));
}

namespace {

const RecordDecl *recordOf(QualType type)
{
    const auto *recordType =
        type.getCanonicalType()->getAs<RecordType>();

    return recordType ? recordType->getDecl() : nullptr;
}

} // namespace

std::string SpanStructRule::getSpanStructName(QualType type) const
{
    return getSpanStructName(recordOf(type));
}

std::string SpanStructRule::getSpanStructName(const RecordDecl *record) const
{
    if (!record)
        return "";

    const std::string name = record->getNameAsString();

    return config.spanStructRule.isSpanStructName(name) ? name : "";
}

bool SpanStructRule::isConstSpan(const RecordDecl *record) const
{
    return record &&
           config.spanStructRule.isConstSpanStructName(record->getNameAsString());
}

std::string SpanStructRule::spanKindText(const RecordDecl *record) const
{
    return isConstSpan(record) ? "const span struct" : "span struct";
}

bool SpanStructRule::isSizeT(QualType type) const
{
    type = type.getUnqualifiedType();

    const auto *typedefType = type->getAs<TypedefType>();

    return typedefType &&
           typedefType->getDecl()->getNameAsString() == "size_t";
}

bool SpanStructRule::isValidSpanDefinition(const RecordDecl *record) const
{
    if (!record)
        return false;

    const auto fields = record->fields();
    auto field = fields.begin();

    if (field == fields.end())
        return false;

    const FieldDecl *data = *field++;

    if (field == fields.end())
        return false;

    const FieldDecl *size = *field++;

    if (field != fields.end())
        return false;

    return data->getNameAsString() == "data" &&
           data->getType()->isPointerType() &&
           size->getNameAsString() == "size" &&
           isSizeT(size->getType());
}

const Expr *SpanStructRule::getArrayExpression(const Expr *expr) const
{
    if (!expr)
        return nullptr;

    const Expr *stripped = expr->IgnoreParenImpCasts();

    return stripped->getType()->isArrayType() ? stripped : nullptr;
}

std::string SpanStructRule::getExpressionName(const Expr *expr) const
{
    if (!expr)
        return "<expression>";

    expr = expr->IgnoreParenImpCasts();

    if (const auto *reference = dyn_cast<DeclRefExpr>(expr))
        return reference->getDecl()->getNameAsString();

    if (const auto *member = dyn_cast<MemberExpr>(expr)) {
        return getExpressionName(member->getBase()) +
            (member->isArrow() ? "->" : ".") +
            member->getMemberDecl()->getNameAsString();
    }

    return "<expression>";
}

const RecordDecl *SpanStructRule::getSpanOfFunction(
    const FunctionDecl *function,
    ASTContext &context) const
{
    if (!function)
        return nullptr;

    const std::string name = function->getNameAsString();
    const RecordDecl *best = nullptr;
    size_t bestLength = 0;

    for (const auto *decl : context.getTranslationUnitDecl()->decls()) {
        const auto *definition = dyn_cast<RecordDecl>(decl);

        if (!definition || !definition->isThisDeclarationADefinition())
            continue;

        const std::string spanName = getSpanStructName(definition);

        if (!spanName.empty() &&
            spanName.size() > bestLength &&
            name.rfind(spanName, 0) == 0)
        {
            best = definition;
            bestLength = spanName.size();
        }
    }

    return best;
}

bool SpanStructRule::getSpanDataUse(
    const Expr *expr,
    ASTContext &context,
    SpanDataUse &use) const
{
    while (expr) {
        expr = expr->IgnoreParenCasts();

        if (const auto *call = dyn_cast<CallExpr>(expr)) {
            const FunctionDecl *callee = call->getDirectCallee();
            const std::string name = callee ? callee->getNameAsString() : "";

            // give(span.data), lend(span.data), ...
            if (call->getNumArgs() == 1 &&
                (name == "workshopc_move" || name == "workshopc_out" ||
                 name == "workshopc_modify"))
            {
                expr = call->getArg(0);
                continue;
            }

            // int_span_data(&span): a function named after a span that
            // returns a pointer hands over its data, like span.data
            const QualType returned = callee ? callee->getReturnType() : QualType();

            if (returned.isNull() ||
                !returned->isPointerType() ||
                returned->isFunctionPointerType())
                return false;

            // A pointer to the span itself is the span, not its data
            if (!getSpanStructName(returned->getPointeeType()).empty())
                return false;

            const RecordDecl *span = getSpanOfFunction(callee, context);

            if (!span)
                return false;

            use.span = span;
            use.text = name + "(...)";
            use.passInstead = "the span";
            return true;
        }

        // span.data + offset, span.data - offset, offset + span.data
        if (const auto *binary = dyn_cast<BinaryOperator>(expr)) {
            if (!binary->isAdditiveOp())
                return false;

            expr = binary->getLHS()->getType()->isPointerType()
                ? binary->getLHS()
                : binary->getRHS();
            continue;
        }

        break;
    }

    const auto *member = dyn_cast_or_null<MemberExpr>(expr);

    if (!member || member->getMemberDecl()->getNameAsString() != "data")
        return false;

    QualType base = member->getBase()->getType();

    if (member->isArrow())
        base = base->getPointeeType();

    if (getSpanStructName(base).empty())
        return false;

    const std::string span = getExpressionName(member->getBase());

    use.span = recordOf(base);
    use.text = getExpressionName(member);
    use.passInstead = "'" + (member->isArrow() ? "*" + span : span) + "'";
    return true;
}

std::string SpanStructRule::getArrayName(const Expr *expr) const
{
    const Expr *array = getArrayExpression(expr);

    if (!array)
        return "<array expression>";

    if (const auto *reference = dyn_cast<DeclRefExpr>(array))
        return reference->getDecl()->getNameAsString();

    if (const auto *member = dyn_cast<MemberExpr>(array)) {
        const std::string base = getArrayName(member->getBase());

        return base + (member->isArrow() ? "->" : ".") +
            member->getMemberDecl()->getNameAsString();
    }

    return "<array expression>";
}

bool SpanStructRule::getArrayStart(
    const Expr *expr,
    ASTContext &context,
    ArrayStart &start) const
{
    if (!expr)
        return false;

    const Expr *stripped = expr->IgnoreParenImpCasts();
    const Expr *indexExpr = nullptr;

    start = ArrayStart{};

    if ((start.array = getArrayExpression(stripped))) {
        start.plain = true;
    }
    // &array[index]
    else if (const auto *address = dyn_cast<UnaryOperator>(stripped)) {
        const auto *subscript = address->getOpcode() == UO_AddrOf
            ? dyn_cast<ArraySubscriptExpr>(address->getSubExpr()->IgnoreParenImpCasts())
            : nullptr;

        if (!subscript || !(start.array = getArrayExpression(subscript->getBase())))
            return false;

        indexExpr = subscript->getIdx();
    }
    // array + index, index + array
    else if (const auto *binary = dyn_cast<BinaryOperator>(stripped)) {
        if (binary->getOpcode() != BO_Add)
            return false;

        if ((start.array = getArrayExpression(binary->getLHS())))
            indexExpr = binary->getRHS();
        else if ((start.array = getArrayExpression(binary->getRHS())))
            indexExpr = binary->getLHS();
        else
            return false;
    }
    else {
        return false;
    }

    if (indexExpr) {
        start.plain = false;

        Expr::EvalResult result;

        start.indexKnown =
            indexExpr->IgnoreParenImpCasts()->EvaluateAsInt(result, context) &&
            result.Val.isInt();

        if (start.indexKnown)
            start.index = result.Val.getInt().getExtValue();
    }

    llvm::APSInt size;
    start.sizeKnown = getArrayElementCount(start.array, &size);

    if (start.sizeKnown)
        start.size = size.getZExtValue();

    return true;
}

std::string SpanStructRule::describeIndexedCount(const ArrayStart &start) const
{
    const std::string name = getArrayName(start.array);

    if (!start.indexKnown || !start.sizeKnown)
        return "must use a constant index into '" + name + "', so that its count can be checked";

    if (start.index < 0 || static_cast<uint64_t>(start.index) > start.size)
        return "starts at index " + std::to_string(start.index) + ", outside of '" + name +
            "', which has " + std::to_string(start.size) + " elements";

    return std::string("must use a constant count of ") +
        (config.spanStructRule.allowSpansToBeGivenFewerElementsThanTheirSize ? "at most " : "") +
        std::to_string(start.size - static_cast<uint64_t>(start.index)) +
        ", the elements left in '" + name + "' from index " + std::to_string(start.index);
}

bool SpanStructRule::isSpanOfWholeArray(
    const VarDecl *variable,
    const VarDecl *array,
    ASTContext &context) const
{
    if (!variable || !variable->getInit() ||
        getSpanStructName(variable->getType()).empty())
        return false;

    const Expr *init = variable->getInit()->IgnoreParenImpCasts();
    const Expr *dataExpr = nullptr;
    const Expr *countExpr = nullptr;

    // { array, size }
    if (const auto *list = dyn_cast<InitListExpr>(init)) {
        if (list->getNumInits() < 2)
            return false;

        dataExpr = list->getInit(0);
        countExpr = list->getInit(1);
    }
    // span_pod(array, size)
    else if (const auto *call = dyn_cast<CallExpr>(init)) {
        const RecordDecl *span = recordOf(variable->getType());

        if (call->getNumArgs() < 2 ||
            !isSpanPodFunction(call->getDirectCallee(), span))
            return false;

        dataExpr = call->getArg(0);
        countExpr = call->getArg(1);
    }
    else {
        return false;
    }

    ArrayStart start;

    if (!getArrayStart(dataExpr, context, start) ||
        !start.indexKnown || start.index != 0)
        return false;

    const auto *reference = dyn_cast<DeclRefExpr>(start.array);

    if (!reference || reference->getDecl() != array)
        return false;

    // An array without a constant size, e.g. 'int values[n]', can not be
    // checked, so any count is taken
    if (!start.sizeKnown)
        return true;

    Expr::EvalResult result;

    return countExpr->IgnoreParenImpCasts()->EvaluateAsInt(result, context) &&
           result.Val.isInt() &&
           !result.Val.getInt().isNegative() &&
           result.Val.getInt().getZExtValue() == start.size;
}

std::vector<const VarDecl *> SpanStructRule::getNextDeclaredVariables(
    const VarDecl *array,
    ASTContext &context) const
{
    std::vector<const VarDecl *> next;

    // A global: the declarations after it in the file. Declarations
    // written together ('int a[2], b[3];') share their start.
    if (isa<TranslationUnitDecl>(array->getDeclContext())) {
        bool found = false;
        SourceLocation group;

        for (const Decl *decl : context.getTranslationUnitDecl()->decls()) {
            if (!found) {
                found = decl == array;
                continue;
            }

            if (decl->getBeginLoc() == array->getBeginLoc())
                continue;

            const auto *variable = dyn_cast<VarDecl>(decl);

            if (!variable)
                break;

            if (group.isInvalid())
                group = variable->getBeginLoc();
            else if (variable->getBeginLoc() != group)
                break;

            next.push_back(variable);
        }

        return next;
    }

    // A local: the next statement of its block
    const auto declParents = context.getParents(*array);
    const auto *declStmt = declParents.empty() ? nullptr : declParents[0].get<DeclStmt>();

    if (!declStmt)
        return next;

    const auto stmtParents = context.getParents(*declStmt);
    const auto *block = stmtParents.empty() ? nullptr : stmtParents[0].get<CompoundStmt>();

    if (!block)
        return next;

    for (auto it = block->body_begin(); it != block->body_end(); ++it) {
        if (*it != declStmt)
            continue;

        if (++it == block->body_end())
            break;

        if (const auto *following = dyn_cast<DeclStmt>(*it)) {
            for (const Decl *decl : following->decls()) {
                if (const auto *variable = dyn_cast<VarDecl>(decl))
                    next.push_back(variable);
            }
        }

        break;
    }

    return next;
}

bool SpanStructRule::isOneLineStaticFunction(const FunctionDecl *function) const
{
    if (!function || function->getStorageClass() != SC_Static)
        return false;

    const auto *body = dyn_cast_or_null<CompoundStmt>(function->getBody());

    if (!body || body->size() != 1)
        return false;

    const Stmt *statement = body->body_front();
    const Expr *expr = dyn_cast<Expr>(statement);

    if (const auto *ret = dyn_cast<ReturnStmt>(statement))
        expr = ret->getRetValue();

    if (!expr)
        return false;

    // 'return first(), second();' is two statements written as one
    const auto *comma = dyn_cast<BinaryOperator>(expr->IgnoreParenImpCasts());

    return !comma || comma->getOpcode() != BO_Comma;
}

const FunctionDecl *SpanStructRule::getEnclosingFunction(
    const Stmt *stmt,
    ASTContext &context) const
{
    DynTypedNode current = DynTypedNode::create(*stmt);

    for (int depth = 0; depth < 256; ++depth) {
        const auto parents = context.getParents(current);

        if (parents.empty())
            return nullptr;

        if (const auto *function = parents[0].get<FunctionDecl>())
            return function;

        current = parents[0];
    }

    return nullptr;
}

const VarDecl *SpanStructRule::getSpanOfArray(
    const VarDecl *array,
    ASTContext &context) const
{
    if (!array)
        return nullptr;

    // The definition is the declaration the span follows
    const VarDecl *definition = array->getDefinition();

    if (!definition || isa<ParmVarDecl>(definition) ||
        !definition->getType()->isArrayType())
        return nullptr;

    for (const VarDecl *variable : getNextDeclaredVariables(definition, context)) {
        if (isSpanOfWholeArray(variable, definition, context))
            return variable;
    }

    return nullptr;
}

bool SpanStructRule::isRaiiArrayDestroyCall(
    const CallExpr *call,
    const Expr *argument) const
{
    const StructResourceManagementRuleConfig &rule =
        config.structResourceManagementRule;
    const std::string suffix = rule.activeRaiiStructArrayDestroyerSuffix();

    if (rule.level == RuleLevel::Off || suffix.empty() ||
        !call || !argument ||
        call->getNumArgs() != 2 || call->getArg(0) != argument)
        return false;

    const FunctionDecl *callee = call->getDirectCallee();
    const auto *array = dyn_cast<DeclRefExpr>(argument->IgnoreParenImpCasts());

    if (!callee || !array || !array->getType()->isArrayType())
        return false;

    const RecordDecl *element = array->getType()
        ->getAsArrayTypeUnsafe()
        ->getElementType()
        ->getAsRecordDecl();

    if (!element ||
        callee->getNameAsString() != element->getNameAsString() + suffix)
        return false;

    // The signature the struct database accepts for an array destroyer,
    // 'void <struct><suffix>(<struct>* self, size_t n)', so only a real
    // array destroyer is trusted with the array
    if (callee->getNumParams() != 2 ||
        !callee->getReturnType().getCanonicalType()->isVoidType())
        return false;

    const ParmVarDecl *self = callee->getParamDecl(0);
    const ParmVarDecl *count = callee->getParamDecl(1);
    const auto *selfPointer = self->getType().getCanonicalType()->getAs<PointerType>();

    const QualType sizeType =
        callee->getASTContext().getSizeType().getCanonicalType();

    if (self->getNameAsString() != "self" ||
        !selfPointer ||
        !selfPointer->getPointeeType()->getAsRecordDecl() ||
        selfPointer->getPointeeType()->getAsRecordDecl()->getCanonicalDecl() !=
            element->getCanonicalDecl() ||
        count->getType().getCanonicalType().getUnqualifiedType() != sizeType)
        return false;

    return hasVisibleRaiiCreator(element, call->getBeginLoc(), callee->getASTContext());
}

bool SpanStructRule::hasVisibleRaiiCreator(
    const RecordDecl *record,
    SourceLocation loc,
    ASTContext &context) const
{
    const std::string &suffix =
        config.structResourceManagementRule.raiiStructCreatorSuffix;

    if (!record || suffix.empty())
        return false;

    const std::string name = record->getNameAsString() + suffix;
    const SourceManager &sm = context.getSourceManager();

    for (const NamedDecl *decl :
         context.getTranslationUnitDecl()->lookup(&context.Idents.get(name)))
    {
        const auto *function = dyn_cast<FunctionDecl>(decl);

        if (!function)
            continue;

        const RecordDecl *returned =
            function->getReturnType()->getAsRecordDecl();

        if (!returned ||
            returned->getCanonicalDecl() != record->getCanonicalDecl())
            continue;

        // Any declaration of it before the call
        for (const FunctionDecl *redeclaration : function->redecls()) {
            if (sm.isBeforeInTranslationUnit(redeclaration->getLocation(), loc))
                return true;
        }
    }

    return false;
}

void SpanStructRule::checkArrayUse(
    const DeclRefExpr *use,
    const SourceManager &sm,
    ASTContext &context) const
{
    const auto *array = dyn_cast<VarDecl>(use->getDecl());
    const VarDecl *span = getSpanOfArray(array, context);

    // Without a span the missing span is reported instead
    if (!span)
        return;

    const Decl *definition = array->getDefinition()->getCanonicalDecl();
    DynTypedNode current = DynTypedNode::create(*use);

    for (int depth = 0; depth < 256; ++depth) {
        const auto parents = context.getParents(current);

        if (parents.empty())
            break;

        const DynTypedNode &parent = parents[0];

        // sizeof(values), _Alignof(values), __typeof__(values)
        if (parent.get<UnaryExprOrTypeTraitExpr>() || parent.get<TypeLoc>())
            return;

        // An array of raii structs is destroyed through the array itself,
        // which the struct resource management rule tracks
        if (const auto *call = parent.get<CallExpr>()) {
            if (isRaiiArrayDestroyCall(call, current.get<Expr>()))
                return;
        }

        // The span's initializer, or the array's own declaration
        if (const auto *variable = parent.get<VarDecl>()) {
            if (variable == span ||
                variable->getCanonicalDecl() == definition)
                return;

            break;
        }

        if (parent.get<FunctionDecl>())
            break;

        current = parent;
    }

    report(
        DiagCode::ArrayUsedAfterSpan,
        use->getLocation(),
        "array '" + array->getNameAsString() +
            "' may only be used through its span, use '" +
            span->getNameAsString() + "' instead",
        sm);
}

void SpanStructRule::checkSpanAfterArray(
    const VarDecl *array,
    const SourceManager &sm,
    ASTContext &context) const
{
    if (!config.spanStructRule.requireSpanImmediatelyAfterArray ||
        !array->getType()->isArrayType() ||
        array->isThisDeclarationADefinition() == VarDecl::DeclarationOnly)
        return;

    for (const VarDecl *variable : getNextDeclaredVariables(array, context)) {
        if (isSpanOfWholeArray(variable, array, context))
            return;
    }

    const auto *constantArray =
        dyn_cast_or_null<ConstantArrayType>(array->getType()->getAsArrayTypeUnsafe());

    const std::string name = array->getNameAsString();
    const std::string size = constantArray
        ? std::to_string(constantArray->getSize().getZExtValue())
        : "size";

    report(
        DiagCode::SpanMissingAfterArray,
        array->getLocation(),
        "array '" + name + "' must be followed right away by a span or const span variable "
            "holding the whole array, e.g. '{ " + name + ", " + size + " }'",
        sm);
}

bool SpanStructRule::getArrayElementCount(
    const Expr *expr,
    llvm::APSInt *count) const
{
    const Expr *array = getArrayExpression(expr);

    if (!array || !count)
        return false;

    const ArrayType *arrayType =
        array->getType()->getAsArrayTypeUnsafe();
    const auto *constantArray =
        dyn_cast_or_null<ConstantArrayType>(arrayType);

    if (!constantArray)
        return false;

    *count = constantArray->getSize();
    return true;
}

bool SpanStructRule::hasExpectedArrayCount(
    const Expr *arrayExpr,
    const Expr *countExpr,
    ASTContext &context) const
{
    // The elements left from where the span starts in the array
    ArrayStart start;

    if (!getArrayStart(arrayExpr, context, start) ||
        !start.sizeKnown || !start.indexKnown || !countExpr ||
        start.index < 0 || static_cast<uint64_t>(start.index) > start.size)
        return false;

    const llvm::APSInt expected(
        llvm::APInt(64, start.size - static_cast<uint64_t>(start.index)),
        /*isUnsigned=*/true);

    Expr::EvalResult result;

    if (!countExpr->IgnoreParenImpCasts()->EvaluateAsInt(result, context) ||
        !result.Val.isInt())
        return false;

    const llvm::APSInt &actual = result.Val.getInt();

    if (!config.spanStructRule.allowSpansToBeGivenFewerElementsThanTheirSize)
        return llvm::APSInt::compareValues(actual, expected) == 0;

    // At most the array's element count. A negative count becomes a huge
    // size_t, so it is too many.
    return !actual.isNegative() &&
           llvm::APSInt::compareValues(actual, expected) <= 0;
}

bool SpanStructRule::isLibraryFunction(
    const FunctionDecl *function,
    const SourceManager &sm) const
{
    if (!function)
        return false;

    const SourceLocation spelling =
        sm.getSpellingLoc(function->getLocation());

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string path = sm.getFilename(spelling).str();
    return !path.empty() && isThirdParty(path);
}

bool SpanStructRule::isSpanPodFunction(
    const FunctionDecl *function,
    const RecordDecl *span) const
{
    if (!function || !span ||
        config.structResourceManagementRule.level == RuleLevel::Off)
    {
        return false;
    }

    // <span name> <span name><pod suffix>(...): the name alone is not
    // enough, the function must also return the span itself
    const std::string expectedName =
        getSpanStructName(span) +
        config.structResourceManagementRule.podStructCreatorSuffix;

    if (function->getNameAsString() != expectedName)
        return false;

    const RecordDecl *returned = recordOf(function->getReturnType());

    return returned &&
           returned->getCanonicalDecl() == span->getCanonicalDecl();
}

bool SpanStructRule::isInsideStructInitializer(
    const Expr *expr,
    ASTContext &context) const
{
    DynTypedNode current = DynTypedNode::create(*expr);

    for (int depth = 0; depth < 64; ++depth) {
        const auto parents = context.getParents(current);

        if (parents.empty())
            return false;

        const DynTypedNode &parent = parents[0];

        if (const auto *initializer = parent.get<InitListExpr>()) {
            if (initializer->getType()->isRecordType())
                return true;
        }

        current = parent;
    }

    return false;
}

void SpanStructRule::report(
    DiagCode code,
    SourceLocation loc,
    const std::string &message,
    const SourceManager &sm) const
{
    if (shouldIgnore(sm, loc))
        return;

    diagnostics.report(
        config.spanStructRule.level,
        code,
        sm,
        sm.getExpansionLoc(loc),
        message);
}

SpanStructRule::SpanStructRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag)
{
}

void SpanStructRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        recordDecl(
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("spanRecord"),
        this);

    finder.addMatcher(
        varDecl(
            unless(isExpansionInSystemHeader())
        ).bind("spanVariable"),
        this);

    finder.addMatcher(
        callExpr(
            unless(isExpansionInSystemHeader())
        ).bind("spanCall"),
        this);

    finder.addMatcher(
        declRefExpr(
            to(varDecl(hasType(hasCanonicalType(arrayType())))),
            unless(isExpansionInSystemHeader())
        ).bind("arrayUse"),
        this);
}

void SpanStructRule::run(const MatchFinder::MatchResult &result)
{
    if (config.spanStructRule.level == RuleLevel::Off ||
        !result.SourceManager ||
        !result.Context)
    {
        return;
    }

    const SourceManager &sm = *result.SourceManager;
    ASTContext &context = *result.Context;

    if (const auto *use = result.Nodes.getNodeAs<DeclRefExpr>("arrayUse")) {
        if (config.spanStructRule.requireSpanImmediatelyAfterArray)
            checkArrayUse(use, sm, context);

        return;
    }

    if (const auto *record =
            result.Nodes.getNodeAs<RecordDecl>("spanRecord"))
    {
        const std::string spanName =
            getSpanStructName(record);

        if (!spanName.empty() && !isValidSpanDefinition(record)) {
            report(
                DiagCode::SpanInvalidDefinition,
                record->getLocation(),
                spanKindText(record) + " '" + spanName +
                    "' must contain exactly a pointer field 'data' and then a size_t field 'size'",
                sm);
        }
        else if (!spanName.empty()) {
            // A span may change the data it points to, a const span may not
            const QualType pointee =
                record->fields().begin()->getType()->getPointeeType();

            const bool constData = pointee.isConstQualified();
            const std::string &constSuffix =
                config.spanStructRule.constSpanStructSuffix;

            if (isConstSpan(record) && !constData) {
                report(
                    DiagCode::SpanConstMismatch,
                    record->getLocation(),
                    "const span struct '" + spanName +
                        "' must hold a pointer to const data, e.g. 'const " +
                        pointee.getAsString() + "* data'",
                    sm);
            }
            else if (!isConstSpan(record) && constData) {
                report(
                    DiagCode::SpanConstMismatch,
                    record->getLocation(),
                    "span struct '" + spanName + "' holds a pointer to const data, " +
                        (constSuffix.empty()
                            ? std::string("which a span may not")
                            : "which makes it a const span struct, whose name must end with '" +
                              constSuffix + "'"),
                    sm);
            }
        }
    }

    if (const auto *variable =
            result.Nodes.getNodeAs<VarDecl>("spanVariable"))
    {
        if (isa<ParmVarDecl>(variable))
            return;

        checkSpanAfterArray(variable, sm, context);

        const std::string spanName =
            getSpanStructName(variable->getType());

        const std::string kind =
            spanKindText(recordOf(variable->getType()));

        if (!spanName.empty()) {
            if (!variable->hasInit()) {
                report(
                    DiagCode::SpanUninitialized,
                    variable->getLocation(),
                    kind + " variable '" + variable->getNameAsString() +
                        "' of type '" + spanName +
                        "' must be initialized at declaration",
                    sm);
            }
            else if (const auto *list = dyn_cast<InitListExpr>(
                         variable->getInit()->IgnoreParenImpCasts()))
            {
                ArrayStart start;

                if (list->getNumInits() >= 1 &&
                    getArrayStart(list->getInit(0), context, start) &&
                    (list->getNumInits() < 2 ||
                     !hasExpectedArrayCount(
                         list->getInit(0),
                         list->getInit(1),
                         context)))
                {
                    const std::string subject =
                        kind + " variable '" + variable->getNameAsString() + "'";

                    report(
                        DiagCode::SpanArrayCount,
                        variable->getLocation(),
                        !start.plain
                            ? subject + " " + describeIndexedCount(start)
                            : subject +
                              (config.spanStructRule.allowSpansToBeGivenFewerElementsThanTheirSize
                                  ? " must use a constant count of at most the array element count in its initializer"
                                  : " must use the array element count in its initializer"),
                        sm);
                }
            }
        }
    }

    const auto *call =
        result.Nodes.getNodeAs<CallExpr>("spanCall");

    if (!call)
        return;

    const FunctionDecl *callee = call->getDirectCallee();
    const std::string calleeName =
        callee ? callee->getNameAsString() : "";

    // The data pointer of a span is the array it views, so handing it to
    // a project function splits the data from its size again
    const bool isOperatorCall =
        calleeName == "workshopc_move" || calleeName == "workshopc_out" ||
        calleeName == "workshopc_modify";

    // Not even to a pod creator: the size given with it could not be
    // checked, so a span can only be made from the data of another span
    // in the span's own functions, e.g. with an initializer
    if (config.spanStructRule.onlyAllowArrayPassingToLibraryFunctionsAndSpans &&
        !isOperatorCall &&
        !isLibraryFunction(callee, sm))
    {
        for (const Expr *argument : call->arguments()) {
            SpanDataUse use;

            if (!getSpanDataUse(argument, context, use))
                continue;

            report(
                DiagCode::SpanDataPassedToNonLibraryFunction,
                argument->getExprLoc(),
                "'" + use.text + "' of " + spanKindText(use.span) + " '" +
                    getSpanStructName(use.span) +
                    "' may only be passed to a standard-library or third-party function, pass " +
                    use.passInstead + " itself instead",
                sm);
        }
    }

    // Raw data is only handed on inside small wrappers that take spans,
    // which keeps every such line easy to review
    if (config.spanStructRule.onlyAllowSpanDataPassingInOneLineStaticFunctions &&
        !isOperatorCall &&
        !isOneLineStaticFunction(getEnclosingFunction(call, context)))
    {
        for (const Expr *argument : call->arguments()) {
            SpanDataUse use;

            if (!getSpanDataUse(argument, context, use))
                continue;

            report(
                DiagCode::SpanDataOutsideWrapper,
                argument->getExprLoc(),
                "'" + use.text + "' of " + spanKindText(use.span) + " '" +
                    getSpanStructName(use.span) +
                    "' may only be passed on inside a static function with a single statement, "
                    "a wrapper that takes the span",
                sm);
        }
    }

    for (size_t i = 0; i < call->getNumArgs(); ++i) {
        const Expr *argument = call->getArg(i);

        // The array, or where in it the span starts: &array[3], array + 3
        ArrayStart start;

        if (!getArrayStart(argument, context, start))
            continue;

        if (i == 0) {
            for (const auto *record : context.getTranslationUnitDecl()->decls()) {
                const auto *definition = dyn_cast<RecordDecl>(record);

                if (!definition || !definition->isThisDeclarationADefinition())
                    continue;

                const std::string spanName =
                    getSpanStructName(definition);

                if (!spanName.empty() &&
                    isSpanPodFunction(callee, definition))
                {
                    if (call->getNumArgs() < 2 ||
                        !hasExpectedArrayCount(
                            argument,
                            call->getArg(1),
                            context))
                    {
                        report(
                            DiagCode::SpanArrayCount,
                            argument->getExprLoc(),
                            !start.plain
                                ? "the part of array '" + getArrayName(start.array) +
                                  "' passed to '" + calleeName + "' " +
                                  describeIndexedCount(start)
                                : "array '" + getArrayName(argument) +
                                  "' passed to '" + calleeName +
                                  (config.spanStructRule.allowSpansToBeGivenFewerElementsThanTheirSize
                                      ? "' must use a constant count of at most its element count"
                                      : "' must use its constant element count"),
                            sm);
                    }

                    break;
                }
            }
        }

        // &array[3] passed to another function is a single element, only
        // the array itself is restricted
        if (!start.plain)
            continue;

        if (!config.spanStructRule.onlyAllowArrayPassingToLibraryFunctionsAndSpans ||
            isLibraryFunction(callee, sm) ||
            isInsideStructInitializer(argument, context) ||
            isRaiiArrayDestroyCall(call, argument))
        {
            continue;
        }

        bool isPodCall = false;

        for (const auto *record : context.getTranslationUnitDecl()->decls()) {
            const auto *definition = dyn_cast<RecordDecl>(record);

            if (!definition || !definition->isThisDeclarationADefinition())
                continue;

            const std::string spanName =
                getSpanStructName(definition);

            if (!spanName.empty() && isSpanPodFunction(callee, definition)) {
                isPodCall = true;
                break;
            }
        }

        if (isPodCall)
            continue;

        report(
            DiagCode::SpanArrayPassedToNonLibraryFunction,
            argument->getExprLoc(),
            "array '" + getArrayName(argument) +
                "' may only be passed to a standard-library, third-party, span, or pod function",
            sm);
    }
}
