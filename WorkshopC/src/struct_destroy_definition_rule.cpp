#include "struct_destroy_definition_rule.hpp"

#include <algorithm>
#include <cstdint>

namespace {

bool endsWith(const std::string &value, const std::string &suffix)
{
    return !suffix.empty() &&
           value.size() > suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// The first return or goto inside 'stmt', or null
const Stmt *findJump(const Stmt *stmt)
{
    if (!stmt)
        return nullptr;

    if (isa<ReturnStmt>(stmt) || isa<GotoStmt>(stmt) || isa<IndirectGotoStmt>(stmt))
        return stmt;

    for (const Stmt *child : stmt->children()) {
        if (const Stmt *jump = findJump(child))
            return jump;
    }

    return nullptr;
}

// The first label inside 'stmt', or null
const LabelStmt *findLabel(const Stmt *stmt)
{
    if (!stmt)
        return nullptr;

    if (const auto *label = dyn_cast<LabelStmt>(stmt))
        return label;

    for (const Stmt *child : stmt->children()) {
        if (const LabelStmt *label = findLabel(child))
            return label;
    }

    return nullptr;
}

// The statements of a block, or the single statement of an if without braces
std::vector<const Stmt *> getStatements(const Stmt *block)
{
    if (const auto *compound = dyn_cast<CompoundStmt>(block))
        return { compound->body_begin(), compound->body_end() };

    return { block };
}

// A statement without the labels in front of it
const Stmt *stripLabels(const Stmt *stmt)
{
    while (const auto *label = dyn_cast_or_null<LabelStmt>(stmt))
        stmt = label->getSubStmt();

    return stmt;
}

} // namespace

bool StructDestroyDefinitionRule::isThirdParty(const std::string &file) const
{
    for (const auto &p : config.thirdPartyIncludes) {
        if (!p.empty() && file.find(p) != std::string::npos)
            return true;
    }

    return false;
}

bool StructDestroyDefinitionRule::shouldIgnore(
    const SourceManager &sm,
    SourceLocation loc) const
{
    if (loc.isInvalid())
        return true;

    if (suppressions.isSuppressed(sm, sm.getExpansionLoc(loc)))
        return true;

    const SourceLocation spelling = sm.getSpellingLoc(loc);

    if (sm.isInSystemHeader(spelling))
        return true;

    const std::string file = sm.getFilename(spelling).str();

    if (file.empty())
        return true;

    return isThirdParty(file);
}

void StructDestroyDefinitionRule::report(
    DiagCode code,
    SourceLocation loc,
    const std::string &message) const
{
    if (!sourceManager || shouldIgnore(*sourceManager, loc))
        return;

    diagnostics.report(
        config.structResourceManagementRule.level,
        code,
        *sourceManager,
        sourceManager->getExpansionLoc(loc),
        message);
}

std::vector<StructDestroyDefinitionRule::RaiiField>
StructDestroyDefinitionRule::getRaiiFields(const RecordDecl *record) const
{
    const auto &rule = config.structResourceManagementRule;
    std::vector<RaiiField> fields;

    for (const FieldDecl *field : record->fields()) {
        QualType type = field->getType().getCanonicalType();
        bool isArray = false;

        // An array field is destroyed with the array destroy function,
        // which only takes a single dimension
        if (const auto *array = dyn_cast<ConstantArrayType>(type.getTypePtr())) {
            type = array->getElementType().getCanonicalType();
            isArray = true;
        }

        if (type->isArrayType())
            continue;

        const RecordDecl *fieldRecord = type->getAsRecordDecl();

        if (!fieldRecord)
            continue;

        const std::string name = fieldRecord->getNameAsString();
        const StructDatabase::StructInfo *info = database.find(name);

        if (!info || info->kind != StructDatabase::Kind::Raii)
            continue;

        const std::string suffix =
            isArray ? rule.activeRaiiStructArrayDestroyerSuffix() : rule.raiiStructDestroyerSuffix;

        if (suffix.empty())
            continue;

        fields.push_back({ field, name + suffix });
    }

    return fields;
}

int StructDestroyDefinitionRule::getDestroyedField(
    const CallExpr *call,
    const ParmVarDecl *self,
    const std::vector<RaiiField> &fields) const
{
    const FunctionDecl *callee = call ? call->getDirectCallee() : nullptr;

    if (!callee || call->getNumArgs() < 1)
        return -1;

    const Expr *argument = call->getArg(0)->IgnoreParenImpCasts();

    // '&self->field', or 'self->field' for an array field
    if (const auto *address = dyn_cast<UnaryOperator>(argument)) {
        if (address->getOpcode() != UO_AddrOf)
            return -1;

        argument = address->getSubExpr()->IgnoreParenImpCasts();
    }

    const auto *member = dyn_cast<MemberExpr>(argument);

    if (!member || !member->isArrow())
        return -1;

    const auto *base = dyn_cast<DeclRefExpr>(member->getBase()->IgnoreParenImpCasts());

    if (!base || base->getDecl() != self)
        return -1;

    const std::string calleeName = callee->getNameAsString();

    for (size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].field == member->getMemberDecl() &&
            fields[i].destroyName == calleeName)
            return static_cast<int>(i);
    }

    return -1;
}

void StructDestroyDefinitionRule::collectNested(
    const Stmt *stmt,
    const ParmVarDecl *self,
    const std::vector<RaiiField> &fields,
    std::vector<FieldDestroy> &destroys) const
{
    if (!stmt)
        return;

    if (const auto *call = dyn_cast<CallExpr>(stmt)) {
        const int index = getDestroyedField(call, self, fields);

        if (index >= 0)
            destroys.push_back({ call, static_cast<size_t>(index), nullptr, 0 });
    }

    for (const Stmt *child : stmt->children())
        collectNested(child, self, fields, destroys);
}

void StructDestroyDefinitionRule::collectBlock(
    const Stmt *block,
    const ParmVarDecl *self,
    const std::vector<RaiiField> &fields,
    bool isBody,
    std::vector<FieldDestroy> &destroys) const
{
    const std::vector<const Stmt *> statements = getStatements(block);

    for (size_t position = 0; position < statements.size(); ++position) {
        const Stmt *statement = statements[position];
        const Stmt *stripped = stripLabels(statement);

        if (const auto *expression = dyn_cast_or_null<Expr>(stripped))
            stripped = expression->IgnoreParenCasts();

        const auto *call = dyn_cast_or_null<CallExpr>(stripped);
        const int index = getDestroyedField(call, self, fields);

        if (index >= 0) {
            destroys.push_back({ call, static_cast<size_t>(index), block, position });
        }
        else if (const auto *ifStmt = dyn_cast_or_null<IfStmt>(stripped); ifStmt && isBody) {
            collectNested(ifStmt->getInit(), self, fields, destroys);
            collectNested(ifStmt->getCond(), self, fields, destroys);

            if (ifStmt->getThen())
                collectBlock(ifStmt->getThen(), self, fields, false, destroys);

            collectNested(ifStmt->getElse(), self, fields, destroys);
        }
        else {
            collectNested(statement, self, fields, destroys);
        }
    }
}

void StructDestroyDefinitionRule::checkDestroyDefinition(const FunctionDecl *function) const
{
    const auto &rule = config.structResourceManagementRule;
    const std::string name = function->getNameAsString();

    if (!endsWith(name, rule.raiiStructDestroyerSuffix))
        return;

    const std::string structName =
        name.substr(0, name.size() - rule.raiiStructDestroyerSuffix.size());

    const StructDatabase::StructInfo *info = database.find(structName);

    if (!info || info->kind != StructDatabase::Kind::Raii || !info->decl)
        return;

    const RecordDecl *record = info->decl->getDefinition();
    const auto *body = dyn_cast_or_null<CompoundStmt>(function->getBody());

    if (!record || !body || function->getNumParams() < 1)
        return;

    const ParmVarDecl *self = function->getParamDecl(0);
    const std::vector<RaiiField> fields = getRaiiFields(record);

    if (fields.empty())
        return;

    std::vector<FieldDestroy> destroys;
    collectBlock(body, self, fields, true, destroys);

    const auto fieldName = [&fields](size_t index) {
        return "raii field '" + fields[index].field->getNameAsString() + "'";
    };

    // The first destroy of each field, and the block they all belong in:
    // the block of the first field destroy that is in an allowed place
    std::vector<const FieldDestroy *> first(fields.size(), nullptr);
    const Stmt *groupBlock = nullptr;

    for (const FieldDestroy &destroy : destroys) {
        if (first[destroy.fieldIndex]) {
            report(
                DiagCode::RaiiFieldMultipleDestroyCalls,
                destroy.call->getExprLoc(),
                fieldName(destroy.fieldIndex) + " of struct '" + structName +
                    "' has multiple destroy calls in '" + name + "'");
            continue;
        }

        first[destroy.fieldIndex] = &destroy;

        if (!groupBlock)
            groupBlock = destroy.block;
    }

    // Destroys in the group block, which the order check looks at
    std::vector<const FieldDestroy *> placed(fields.size(), nullptr);

    for (size_t i = 0; i < fields.size(); ++i) {
        const FieldDestroy *destroy = first[i];

        if (!destroy) {
            const bool isArray = fields[i].field->getType()->isArrayType();
            const std::string fieldText = fields[i].field->getNameAsString();

            report(
                DiagCode::RaiiFieldNotDestroyed,
                function->getLocation(),
                fieldName(i) + " of struct '" + structName +
                    "' is never destroyed in '" + name + "', call '" +
                    fields[i].destroyName +
                    (isArray ? "(self->" + fieldText + ", ...)'"
                             : "(&self->" + fieldText + ")'"));
            continue;
        }

        const SourceLocation loc = destroy->call->getExprLoc();

        if (!destroy->block) {
            report(
                DiagCode::RaiiFieldDestroyMisplaced,
                loc,
                fieldName(i) + " must be destroyed directly in the body of '" + name +
                    "' or directly inside an if statement there, not in a nested block, loop, switch or else branch");
            continue;
        }

        if (destroy->block != groupBlock) {
            report(
                DiagCode::RaiiFieldDestroyMisplaced,
                loc,
                fieldName(i) + " must be destroyed in the same block as the other raii fields of struct '" +
                    structName + "'");
            continue;
        }

        placed[i] = destroy;
    }

    // A return or goto before all the field destroys skips the struct as
    // a whole, like an if statement around them. One between them, or a
    // label a goto could jump to, leaves the struct partly destroyed.
    size_t firstPosition = SIZE_MAX;
    size_t lastPosition = 0;

    for (const FieldDestroy *destroy : placed) {
        if (!destroy)
            continue;

        firstPosition = std::min(firstPosition, destroy->position);
        lastPosition = std::max(lastPosition, destroy->position);
    }

    if (groupBlock && firstPosition < lastPosition) {
        const std::vector<const Stmt *> statements = getStatements(groupBlock);

        for (size_t position = firstPosition + 1; position <= lastPosition; ++position) {
            if (position < lastPosition) {
                if (const Stmt *jump = findJump(statements[position])) {
                    report(
                        DiagCode::RaiiFieldDestroysInterrupted,
                        jump->getBeginLoc(),
                        std::string(isa<ReturnStmt>(jump) ? "a return" : "a goto") +
                            " between the raii field destroys of struct '" + structName +
                            "' may leave it partly destroyed");
                }
            }

            if (const LabelStmt *label = findLabel(statements[position])) {
                report(
                    DiagCode::RaiiFieldDestroysInterrupted,
                    label->getBeginLoc(),
                    "label '" + std::string(label->getName()) +
                        "' between the raii field destroys of struct '" + structName +
                        "' lets a goto skip some of them and leave it partly destroyed");
            }
        }
    }

    if (!rule.raiiDestroyInReverseOrder)
        return;

    // A field declared earlier is destroyed later: after every field
    // declared after it
    for (size_t i = 0; i < fields.size(); ++i) {
        for (size_t j = i + 1; j < fields.size(); ++j) {
            if (!placed[i] || !placed[j] || placed[i] > placed[j])
                continue;

            report(
                DiagCode::RaiiDestroyNotReverseOrder,
                placed[i]->call->getExprLoc(),
                fieldName(i) + " must be destroyed after " + fieldName(j) +
                    " (raii destruction must follow reverse declaration order)");
        }
    }
}

StructDestroyDefinitionRule::StructDestroyDefinitionRule(
    const Config &cfg,
    SuppressionManager &sup,
    Diagnostics &diag,
    StructDatabase &db)
    : config(cfg),
      suppressions(sup),
      diagnostics(diag),
      database(db)
{
}

void StructDestroyDefinitionRule::bindFinder(MatchFinder &finder)
{
    finder.addMatcher(
        functionDecl(
            isDefinition(),
            unless(isExpansionInSystemHeader())
        ).bind("destroyDefinition"),
        this);
}

void StructDestroyDefinitionRule::run(const MatchFinder::MatchResult &result)
{
    sourceManager = result.SourceManager;

    // Whether it destroys a raii struct is only known once the struct
    // database is complete, so the check happens in finalize()
    if (const auto *function = result.Nodes.getNodeAs<FunctionDecl>("destroyDefinition"))
        destroyDefinitions.push_back(function);
}

void StructDestroyDefinitionRule::finalize()
{
    if (!sourceManager ||
        !config.structResourceManagementRule.raiiStandardizedDestroyDefinitions)
        return;

    for (const FunctionDecl *function : destroyDefinitions)
        checkDestroyDefinition(function);
}
