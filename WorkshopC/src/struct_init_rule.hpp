#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "struct_database.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class StructInitRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    const std::string freeSuffix;
    const std::string podSuffix;
    const std::string raiiSuffix;
    const std::string destroySuffix;
    const std::string copySuffix;
    const std::string moveSuffix;
    const std::string returnSuffix;
    const std::string validSuffix;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    StructDatabase &database;

    const SourceManager *sourceManager = nullptr;

    std::vector<std::pair<const VarDecl *, const FunctionDecl *>> pendingVarDecls;
    std::vector<std::pair<const BinaryOperator *, const FunctionDecl *>> pendingAssignments;
    struct PendingCall {
        const CallExpr *call = nullptr;
        const FunctionDecl *function = nullptr;
        ASTContext *context = nullptr;
    };
    std::vector<PendingCall> pendingCalls;
    std::vector<std::pair<const ReturnStmt *, const FunctionDecl *>> pendingReturns;
    std::vector<const RecordDecl *> pendingRecords;

private:
    static std::string getOption(
        const RuleConfig &cfg,
        const std::string &name)
    {
        auto it = cfg.options.find(name);

        if (it == cfg.options.end())
            return "";

        return it->second;
    }

    bool isThirdParty(const std::string &file) const
    {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (!p.empty() && file.find(p) != std::string::npos)
                return true;
        }

        return false;
    }

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const
    {
        if (loc.isInvalid())
            return true;

        if (suppressions.isSuppressed(sm, loc))
            return true;

        SourceLocation spell = sm.getSpellingLoc(loc);

        if (sm.isInSystemHeader(spell))
            return true;

        std::string file = sm.getFilename(spell).str();

        if (file.empty())
            return true;

        return isThirdParty(file);
    }


    const RecordDecl *getStructDecl(
        QualType type,
        bool *isArray = nullptr) const
    {
        if (isArray)
            *isArray = false;

        QualType current = type.getUnqualifiedType();

        for (int i = 0; i < 8; ++i) {
            if (const auto *arrayType = current->getAsArrayTypeUnsafe()) {
                if (isArray)
                    *isArray = true;

                current = arrayType->getElementType().getUnqualifiedType();
                continue;
            }

            current = current.getCanonicalType().getUnqualifiedType();

            if (const auto *recordType = current->getAs<RecordType>())
                return recordType->getDecl();

            if (const auto *typedefType = current->getAs<TypedefType>()) {
                current = typedefType->desugar();
                continue;
            }

            if (const auto *tagType = current->getAs<TagType>()) {
                current = tagType->desugar();
                continue;
            }

            break;
        }

        return nullptr;
    }

    bool isStructType(
        QualType type,
        std::string *name = nullptr,
        bool *isArray = nullptr) const
    {
        const auto *recordDecl = getStructDecl(type, isArray);

        if (!recordDecl || !recordDecl->isStruct())
            return false;

        if (name)
            *name = recordDecl->getNameAsString();

        return true;
    }

    bool isInsideHelperFunction(
        const FunctionDecl *function,
        const std::string &structName) const
    {
        if (!function || structName.empty())
            return false;

        const std::string name = function->getNameAsString();

        return name == structName + podSuffix ||
            name == structName + raiiSuffix ||
            name == structName + copySuffix ||
            name == structName + moveSuffix ||
            name == structName + destroySuffix ||
            name == structName + returnSuffix ||
            name == structName + validSuffix;
    }

    const FunctionDecl *findEnclosingFunction(
        ASTContext &context,
        const DynTypedNode &node) const
    {
        DynTypedNode current = node;

        for (int i = 0; i < 16; ++i) {
            const auto parents = context.getParents(current);

            if (parents.empty())
                return nullptr;

            bool foundParent = false;

            for (const auto &parentNode : parents) {
                if (const auto *function = parentNode.get<FunctionDecl>())
                    return function;

                if (const auto *decl = parentNode.get<Decl>()) {
                    current = DynTypedNode::create(*decl);
                    foundParent = true;
                    break;
                }

                if (const auto *stmt = parentNode.get<Stmt>()) {
                    current = DynTypedNode::create(*stmt);
                    foundParent = true;
                    break;
                }
            }

            if (!foundParent)
                return nullptr;
        }

        return nullptr;
    }

    const Expr *unwrapExpr(const Expr *expr) const
    {
        while (expr) {
            if (const auto *implicit = dyn_cast<ImplicitCastExpr>(expr)) {
                expr = implicit->getSubExpr();
                continue;
            }

            if (const auto *paren = dyn_cast<ParenExpr>(expr)) {
                expr = paren->getSubExpr();
                continue;
            }

            break;
        }

        return expr;
    }

    bool isStructReturnFunction(
        const CallExpr *call,
        std::string *structName = nullptr) const
    {
        if (!call)
            return false;

        const auto *callee =
            call->getDirectCallee();

        if (!callee)
            return false;

        const std::string functionName =
            callee->getNameAsString();

        if (returnSuffix.empty())
            return false;

        if (functionName.size() <= returnSuffix.size())
            return false;

        if (functionName.compare(
                functionName.size() - returnSuffix.size(),
                returnSuffix.size(),
                returnSuffix) != 0)
            return false;

        const std::string candidateName =
            functionName.substr(
                0,
                functionName.size() - returnSuffix.size());

        if (candidateName.empty())
            return false;

        std::string actualReturnStructName;

        if (!isStructType(
                callee->getReturnType(),
                &actualReturnStructName))
            return false;

        if (actualReturnStructName != candidateName)
            return false;

        if (callee->getNumParams() != 1)
            return false;

        const auto *selfParam =
            callee->getParamDecl(0);

        if (!selfParam)
            return false;

        if (selfParam->getNameAsString() != "self")
            return false;

        if (!selfParam->getType()->isPointerType())
            return false;

        std::string parameterStructName;

        if (!isStructType(
                selfParam->getType()->getPointeeType(),
                &parameterStructName))
            return false;

        if (parameterStructName != actualReturnStructName)
            return false;

        if (structName)
            *structName = actualReturnStructName;

        return true;
    }

    bool isDirectReturnExpression(
        const CallExpr *call,
        ASTContext &context) const
    {
        if (!call)
            return false;

        const Expr *current = call;

        for (int i = 0; i < 8; ++i) {

            const auto parents =
                context.getParents(*current);

            if (parents.empty())
                return false;

            bool movedThroughWrapper = false;

            for (const auto &parent : parents) {

                if (const auto *returnStmt =
                        parent.get<ReturnStmt>()) {

                    const Expr *returnValue =
                        returnStmt->getRetValue();

                    return unwrapExpr(returnValue) == call;
                }

                if (const auto *expr =
                        parent.get<Expr>()) {

                    if (isa<ParenExpr>(expr) ||
                        isa<ImplicitCastExpr>(expr)) {

                        current = expr;
                        movedThroughWrapper = true;
                        break;
                    }
                }
            }

            if (!movedThroughWrapper)
                return false;
        }

        return false;
    }

    void checkReturnFunctionUsage(
        const CallExpr *call,
        const FunctionDecl *enclosingFunction,
        ASTContext &context) const
    {
        if (!call)
            return;
    
        std::string structName;
    
        if (!isStructReturnFunction(
                call,
                &structName))
            return;
        
        // The return helper itself is exempt from the
        // normal rules for its own struct.
        if (enclosingFunction &&
            isInsideHelperFunction(
                enclosingFunction,
                structName))
            return;
            
        const auto *info =
            database.find(structName);
            
        if (!info ||
            info->kind != StructDatabase::Kind::Raii)
            return;
        
        if (isDirectReturnExpression(
                call,
                context))
            return;
        
        reportUsageIssue(
            call->getExprLoc(),
            "RAII return function '" +
            call->getDirectCallee()->getNameAsString() +
            "' may only be used directly in a return statement");
    }

    bool exprIsStructReturnValue(const Expr *expr, std::string *structName = nullptr) const
    {
        expr = unwrapExpr(expr);

        if (!expr)
            return false;

        if (const auto *call = dyn_cast<CallExpr>(expr)) {
            if (const auto *callee = call->getDirectCallee()) {
                const QualType returnType = callee->getReturnType();
                return isStructType(returnType, structName);
            }
        }

        return false;
    }

    bool exprIsStructVariableReference(
        const Expr *expr,
        std::string *structName = nullptr) const
    {
        expr = unwrapExpr(expr);

        if (!expr)
            return false;

        // Direct struct variable:
        //
        //     pos2 = pos;
        //
        if (const auto *declRef = dyn_cast<DeclRefExpr>(expr)) {
            if (const auto *varDecl = dyn_cast<VarDecl>(declRef->getDecl()))
                return isStructType(varDecl->getType(), structName);

            return false;
        }

        // Dereferenced struct pointer:
        //
        //     pos2 = *pos_ptr;
        //
        if (const auto *unary = dyn_cast<UnaryOperator>(expr)) {
            if (unary->getOpcode() == UO_Deref) {
                const Expr *subExpr =
                    unwrapExpr(unary->getSubExpr());

                if (!subExpr)
                    return false;

                // The dereferenced expression itself must be a struct.
                return isStructType(expr->getType(), structName);
            }
        }

        // Struct element from an array:
        //
        //     pos2 = positions[1];
        //     pos2 = pos_array.positions[1];
        //     pos2 = pos_arrays[0][1];
        //
        if (const auto *arraySubscript =
                dyn_cast<ArraySubscriptExpr>(expr)) {

            // The resulting array element must be a struct.
            if (!isStructType(expr->getType(), structName))
                return false;

            const Expr *base =
                unwrapExpr(arraySubscript->getBase());

            if (!base)
                return false;

            // Direct array variable:
            //
            //     positions[1]
            //
            if (const auto *declRef = dyn_cast<DeclRefExpr>(base)) {
                const auto *varDecl =
                    dyn_cast<VarDecl>(declRef->getDecl());

                if (!varDecl)
                    return false;

                bool isArray = false;
                std::string baseStructName;

                if (!isStructType(
                        varDecl->getType(),
                        &baseStructName,
                        &isArray))
                    return false;

                return isArray;
            }

            // Struct member containing an array:
            //
            //     pos_array.positions[1]
            //
            if (const auto *memberExpr = dyn_cast<MemberExpr>(base)) {
                const auto *memberDecl =
                    memberExpr->getMemberDecl();

                if (!memberDecl)
                    return false;

                const auto *fieldDecl =
                    dyn_cast<FieldDecl>(memberDecl);

                if (!fieldDecl)
                    return false;

                bool isArray = false;
                std::string fieldStructName;

                if (!isStructType(
                        fieldDecl->getType(),
                        &fieldStructName,
                        &isArray))
                    return false;

                return isArray;
            }

            // Nested array:
            //
            //     pos_arrays[0][1]
            //
            if (isa<ArraySubscriptExpr>(base))
                return exprIsStructVariableReference(
                    base,
                    structName);

            return false;
        }

        return false;
    }

    bool exprIsStructValue(const Expr *expr, std::string *structName = nullptr) const
    {
        return exprIsStructReturnValue(expr, structName) ||
            exprIsStructVariableReference(expr, structName);
    }

    bool exprIsNonPointerStructValue(const Expr *expr, std::string *structName = nullptr) const
    {
        expr = unwrapExpr(expr);

        if (!expr)
            return false;

        if (exprIsStructReturnValue(expr, structName))
            return true;

        if (const auto *declRef = dyn_cast<DeclRefExpr>(expr)) {
            if (const auto *varDecl = dyn_cast<VarDecl>(declRef->getDecl())) {
                const QualType type = varDecl->getType();
                return !type->getAs<PointerType>() && isStructType(type, structName);
            }
        }

        return false;
    }

    void reportUsageIssue(
        SourceLocation loc,
        const std::string &message) const
    {
        if (!sourceManager || shouldIgnore(*sourceManager, loc))
            return;

        diagnostics.report(
            config.level,
            *sourceManager,
            loc,
            message);
    }

    void checkVarDecl(
        const VarDecl *varDecl,
        const FunctionDecl *enclosingFunction) const
    {
        if (!varDecl)
            return;

        // Function parameters are not local variable declarations.
        // A by-value struct parameter is already initialized by the
        // caller, so it must not be checked by the variable
        // initialization rule.
        if (isa<ParmVarDecl>(varDecl))
            return;

        std::string structName;
        bool isArray = false;
        if (!isStructType(varDecl->getType(), &structName, &isArray))
            return;

        if (enclosingFunction &&
            isInsideHelperFunction(enclosingFunction, structName))
            return;

        if (isArray) {
            const std::string variableName =
                varDecl->getNameAsString();

            reportUsageIssue(
                varDecl->getLocation(),
                "array variable '" + variableName + "' of type '" + structName + "' is not allowed outside of structs");
            return;
        }

        const auto *info = database.find(structName);
        if (!info)
            return;

        const std::string variableName =
            varDecl->getNameAsString();

        const Expr *init = varDecl->getInit();

        if (info->kind == StructDatabase::Kind::Pod) {
            if (!init) {
                reportUsageIssue(
                    varDecl->getLocation(),
                    "struct variable '" + variableName +
                        "' of type '" + structName +
                        "' should be initialized from a function return value or another struct variable");
            }
            else if (!exprIsStructValue(init)) {
                reportUsageIssue(
                    varDecl->getLocation(),
                    "struct variable '" + variableName +
                        "' of type '" + structName +
                        "' should be initialized from a function return value or another struct variable");
            }
        }
        else if (info->kind == StructDatabase::Kind::Raii) {
            if (!init) {
                reportUsageIssue(
                    varDecl->getLocation(),
                    "struct variable '" + variableName +
                        "' of type '" + structName +
                        "' should be initialized from a function return value");
            }
            else if (!exprIsStructReturnValue(init)) {
                reportUsageIssue(
                    varDecl->getLocation(),
                    "struct variable '" + variableName +
                        "' of type '" + structName +
                        "' should be initialized from a function return value");
            }
        }
    }

    const VarDecl *getReferencedVarDecl(const Expr *expr) const
    {
        expr = unwrapExpr(expr);

        if (!expr)
            return nullptr;

        if (const auto *declRef = dyn_cast<DeclRefExpr>(expr))
            return dyn_cast<VarDecl>(declRef->getDecl());

        if (const auto *unary = dyn_cast<UnaryOperator>(expr)) {
            if (unary->getOpcode() == UO_Deref)
                return getReferencedVarDecl(unary->getSubExpr());
        }

        if (const auto *arraySubscript = dyn_cast<ArraySubscriptExpr>(expr))
            return getReferencedVarDecl(arraySubscript->getBase());

        if (const auto *memberExpr = dyn_cast<MemberExpr>(expr))
            return getReferencedVarDecl(memberExpr->getBase());

        return nullptr;
    }

    std::string getAssignmentTargetName(const Expr *expr) const
    {
        expr = unwrapExpr(expr);

        if (!expr)
            return "";

        if (const auto *declRef = dyn_cast<DeclRefExpr>(expr)) {
            if (const auto *varDecl =
                    dyn_cast<VarDecl>(declRef->getDecl()))
                return varDecl->getNameAsString();

            return "";
        }

        if (const auto *memberExpr = dyn_cast<MemberExpr>(expr)) {
            std::string baseName =
                getAssignmentTargetName(memberExpr->getBase());

            const auto *memberDecl = memberExpr->getMemberDecl();
            if (!memberDecl)
                return baseName;

            const std::string memberName =
                memberDecl->getNameAsString();

            if (baseName.empty())
                return memberName;

            return baseName + "." + memberName;
        }

        if (const auto *arraySubscript =
                dyn_cast<ArraySubscriptExpr>(expr)) {

            std::string baseName =
                getAssignmentTargetName(arraySubscript->getBase());

            if (baseName.empty())
                return "";

            return baseName + "[...]";
        }

        if (const auto *unary =
                dyn_cast<UnaryOperator>(expr)) {

            if (unary->getOpcode() == UO_Deref) {
                std::string name =
                    getAssignmentTargetName(unary->getSubExpr());

                if (!name.empty())
                    return "*" + name;
            }
        }

        return "";
    }

    bool isPointerDereference(const Expr *expr) const
    {
        expr = unwrapExpr(expr);

        if (!expr)
            return false;

        return isa<UnaryOperator>(expr) &&
            cast<UnaryOperator>(expr)->getOpcode() == UO_Deref;
    }

    void checkAssignment(
        const BinaryOperator *assignment,
        const FunctionDecl *enclosingFunction) const
    {
        if (!assignment || !assignment->isAssignmentOp())
            return;
    
        const Expr *lhs = assignment->getLHS();
        const Expr *rhs = assignment->getRHS();
    
        if (!lhs || !rhs)
            return;
    
        lhs = unwrapExpr(lhs);
    
        // Direct member assignments such as:
        //
        //     obj.field = value;
        //
        // are intentionally allowed unless the member itself is
        // a RAII struct. Array elements are checked separately.
        if (isa<MemberExpr>(lhs))
            return;
    
        std::string structName;
        if (!isStructType(lhs->getType(), &structName))
            return;
    
        if (enclosingFunction &&
            isInsideHelperFunction(enclosingFunction, structName))
            return;
        
        const auto *info = database.find(structName);
        if (!info)
            return;
        
        if (!exprIsNonPointerStructValue(rhs))
            return;
        
        if (info->kind != StructDatabase::Kind::Raii)
            return;
        
        // A plain variable:
        //
        //     vec = other_vec;
        //
        // A struct array element:
        //
        //     vec_array.vecs[0] = vec;
        //
        // getAssignmentTargetName() gives us the actual destination.
        const std::string targetName =
            getAssignmentTargetName(lhs);

        if (targetName.empty())
            return;

        if (isPointerDereference(lhs)) {
            reportUsageIssue(
                assignment->getOperatorLoc(),
                "pointer dereference '" + targetName +
                    "' of struct '" + structName +
                    "' cannot be reassigned with another struct value (raii)");
        }
        else if (isa<ArraySubscriptExpr>(lhs)) {
            reportUsageIssue(
                assignment->getOperatorLoc(),
                "struct '" + structName +
                    "' array element '" + targetName +
                    "' cannot be reassigned with another struct value (raii)");
        }
        else {
            reportUsageIssue(
                assignment->getOperatorLoc(),
                "variable '" + targetName +
                    "' of type struct '" + structName +
                    "' cannot be reassigned with another struct value (raii)");
        }
    }

    void checkCallArguments(
        const CallExpr *call,
        const FunctionDecl *enclosingFunction) const
    {
        if (!call)
            return;

        const auto *calleeDecl = call->getDirectCallee();
        if (!calleeDecl)
            return;

        const auto *functionDecl =
            dyn_cast<FunctionDecl>(calleeDecl);

        if (!functionDecl)
            return;

        const std::string functionName =
            functionDecl->getNameAsString();

        for (unsigned i = 0; i < call->getNumArgs(); ++i) {
            const Expr *arg = call->getArg(i);

            if (!arg)
                continue;

            const auto *paramDecl =
                functionDecl->getParamDecl(i);

            if (!paramDecl)
                continue;

            std::string structName;

            if (!isStructType(
                    paramDecl->getType(),
                    &structName))
                continue;

            if (enclosingFunction &&
                isInsideHelperFunction(
                    enclosingFunction,
                    structName))
                continue;

            const auto *info =
                database.find(structName);

            if (!info)
                continue;

            // Get the argument name when the argument is a
            // simple variable reference.
            std::string argumentName;

            const Expr *unwrappedArg =
                unwrapExpr(arg);

            if (const auto *declRef =
                    dyn_cast<DeclRefExpr>(unwrappedArg)) {

                if (const auto *varDecl =
                        dyn_cast<VarDecl>(
                            declRef->getDecl())) {

                    argumentName =
                        varDecl->getNameAsString();
                }
            }

            // Fall back to the parameter name if the argument
            // is not a simple variable reference.
            if (argumentName.empty())
                argumentName =
                    paramDecl->getNameAsString();

            if (info->kind == StructDatabase::Kind::Pod) {

                if (!exprIsStructValue(arg)) {

                    reportUsageIssue(
                        arg->getExprLoc(),
                        "struct '" +
                        structName +
                        "' argument '" +
                        argumentName +
                        "' passed to function '" +
                        functionName +
                        "' should be passed from a function return value or another struct variable");
                }
            }
            else if (info->kind ==
                     StructDatabase::Kind::Raii) {

                if (!exprIsStructReturnValue(arg)) {

                    reportUsageIssue(
                        arg->getExprLoc(),
                        "struct '" +
                        structName +
                        "' argument '" +
                        argumentName +
                        "' passed to function '" +
                        functionName +
                        "' should be passed from a function return value");
                }
            }
        }
    }

    void checkReturnStmt(
        const ReturnStmt *returnStmt,
        const FunctionDecl *enclosingFunction) const
    {
        if (!returnStmt || !enclosingFunction)
            return;

        std::string structName;

        // The function itself must return a struct by value.
        if (!isStructType(
                enclosingFunction->getReturnType(),
                &structName))
            return;

        if (isInsideHelperFunction(enclosingFunction, structName))
            return;

        const auto *info = database.find(structName);
        if (!info)
            return;

        const Expr *returnValue =
            returnStmt->getRetValue();

        // A non-void struct-returning function must have
        // a return expression.
        if (!returnValue) {
            if (info->kind == StructDatabase::Kind::Pod) {
                reportUsageIssue(
                    returnStmt->getReturnLoc(),
                    "function '" +
                    enclosingFunction->getNameAsString() +
                    "' returning pod struct '" +
                    structName +
                    "' should return a function return value or another struct variable");
            }
            else if (info->kind == StructDatabase::Kind::Raii) {
                reportUsageIssue(
                    returnStmt->getReturnLoc(),
                    "function '" +
                    enclosingFunction->getNameAsString() +
                    "' returning RAII struct '" +
                    structName +
                    "' should return a function call");
            }

            return;
        }

        if (info->kind == StructDatabase::Kind::Pod) {

            // Pod structs may be returned from:
            //
            //     return make_pos();
            //     return pos;
            //     return *pos_ptr;
            //     return positions[0];
            //
            // but not from:
            //
            //     return (pos_t){5, 6};
            //
            if (!exprIsStructValue(returnValue)) {
                reportUsageIssue(
                    returnValue->getExprLoc(),
                    "function '" +
                    enclosingFunction->getNameAsString() +
                    "' returning struct '" +
                    structName +
                    "' should return a function return value or another struct variable");
            }
        }
        else if (info->kind == StructDatabase::Kind::Raii) {

            // RAII structs may only be returned directly from
            // another function returning the same kind of struct:
            //
            //     return make_int_vector();
            //
            // but not:
            //
            //     return vec;
            //     return *vec_ptr;
            //     return vec_array.vecs[0];
            //     return (int_vector_t){...};
            //
            if (!exprIsStructReturnValue(returnValue)) {
                reportUsageIssue(
                    returnValue->getExprLoc(),
                    "function '" +
                    enclosingFunction->getNameAsString() +
                    "' returning RAII struct '" +
                    structName +
                    "' should return a function call");
            }
        }
    }

    void checkRecordDecl(const RecordDecl *recordDecl) const
    {
        if (!recordDecl || !recordDecl->isStruct())
            return;

        if (!recordDecl->isThisDeclarationADefinition())
            return;

        const auto *info = database.find(recordDecl->getNameAsString());
        if (!info)
            return;

        if (info->kind != StructDatabase::Kind::Pod)
            return;

        for (const auto *field : recordDecl->fields()) {
            if (!field)
                continue;

            std::string fieldStructName;
            if (!isStructType(field->getType(), &fieldStructName))
                continue;

            if (field->getType()->getAs<PointerType>())
                continue;

            const auto *fieldInfo = database.find(fieldStructName);
            if (fieldInfo && fieldInfo->kind != StructDatabase::Kind::Pod) {
                reportUsageIssue(
                    field->getLocation(),
                    "struct field '" + field->getNameAsString() + "' inside pod struct '" + recordDecl->getNameAsString() + "' must be a pod struct");
            }
        }
    }

public:
    StructInitRule(
        const RuleConfig &cfg,
        const Config &gc,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db)
        : config(cfg),
          globalConfig(gc),
          freeSuffix(getOption(cfg, "free_struct_creator_suffix")),
          podSuffix(getOption(cfg, "pod_struct_creator_suffix")),
          raiiSuffix(getOption(cfg, "raii_struct_creator_suffix")),
          destroySuffix(getOption(cfg, "raii_struct_destroyer_suffix")),
          copySuffix(getOption(cfg, "raii_struct_copy_suffix")),
          moveSuffix(getOption(cfg, "raii_struct_move_suffix")),
          returnSuffix(getOption(cfg, "raii_struct_return_suffix")),
          validSuffix(getOption(cfg, "raii_struct_valid_suffix")),
          suppressions(sup),
          diagnostics(diag),
          database(db)
    {
    }

    void run(const MatchFinder::MatchResult &result) override
    {
        sourceManager = result.SourceManager;

        if (!sourceManager)
            return;

        if (const auto *varDecl = result.Nodes.getNodeAs<VarDecl>("varDecl")) {
            pendingVarDecls.push_back({
                varDecl,
                findEnclosingFunction(*result.Context, DynTypedNode::create(*varDecl))});
        }

        if (const auto *assignment = result.Nodes.getNodeAs<BinaryOperator>("assignment")) {
            pendingAssignments.push_back({
                assignment,
                findEnclosingFunction(*result.Context, DynTypedNode::create(*assignment))});
        }

        if (const auto *call = result.Nodes.getNodeAs<CallExpr>("call")) {
            pendingCalls.push_back({
                call,
                findEnclosingFunction(
                    *result.Context,
                    DynTypedNode::create(*call)),
                result.Context
            });
        }

        if (const auto *returnStmt =
            result.Nodes.getNodeAs<ReturnStmt>("returnStmt")) {
                
                pendingReturns.push_back({
                    returnStmt,
                    findEnclosingFunction(
                        *result.Context,
                        DynTypedNode::create(*returnStmt))
                });
        }

        if (const auto *recordDecl = result.Nodes.getNodeAs<RecordDecl>("record")) {
            pendingRecords.push_back(recordDecl);
        }
    }

    void finalize()
    {
        for (const auto &[varDecl, function] : pendingVarDecls)
            checkVarDecl(varDecl, function);

        for (const auto &[assignment, function] : pendingAssignments)
            checkAssignment(assignment, function);

        for (const auto &pending : pendingCalls) {
            checkCallArguments(
                pending.call,
                pending.function);
            
            if (pending.context) {
                checkReturnFunctionUsage(
                    pending.call,
                    pending.function,
                    *pending.context);
            }
        }

        for (const auto &[returnStmt, function] : pendingReturns)
            checkReturnStmt(returnStmt, function);

        for (const auto *recordDecl : pendingRecords)
            checkRecordDecl(recordDecl);
    }
};
