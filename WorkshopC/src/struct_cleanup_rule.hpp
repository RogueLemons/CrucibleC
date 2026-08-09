#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "struct_database.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class StructCleanupRule : public MatchFinder::MatchCallback {
private:
    struct TrackedVar {
        const VarDecl *decl = nullptr;
        std::string structName;
        bool destroyed = false;
    };

    struct ScopeState {
        std::vector<TrackedVar> vars;
        std::unordered_set<const VarDecl *> destroyed;
    };

    class CleanupAnalyzer {
    private:
        StructCleanupRule &owner;
        const FunctionDecl *function = nullptr;
        std::vector<ScopeState> scopes;
        std::vector<TrackedVar> params;
        std::unordered_set<const VarDecl *> reportedVars;

    public:
        CleanupAnalyzer(StructCleanupRule &owner, const FunctionDecl *function)
            : owner(owner), function(function) {
        }

        void scanStmt(const Stmt *stmt)
        {
            if (!stmt)
                return;
        
            // A compound statement introduces a new lexical scope.
            if (const auto *compound = dyn_cast<CompoundStmt>(stmt)) {
                scopes.emplace_back();
            
                // Scan everything in this scope.
                for (const Stmt *child : compound->body()) {
                    if (child)
                        scanStmt(child);
                }
            
                // Anything still alive when we reach the closing brace
                // must have been explicitly destroyed.
                owner.reportPendingVarsForScopeExit(
                    compound->getEndLoc(),
                    scopes.back(),
                    reportedVars);
                
                scopes.pop_back();
                return;
            }
        
            // Track local RAII struct declarations.
            if (const auto *declStmt = dyn_cast<DeclStmt>(stmt)) {
                for (const Decl *decl : declStmt->decls()) {
                    const auto *var = dyn_cast<VarDecl>(decl);
                
                    if (var)
                        trackVar(var);
                }
            }
        
            // A destroy call can occur anywhere inside an expression tree.
            if (const auto *call = dyn_cast<CallExpr>(stmt))
                markDestroyedIfNeeded(call);
        
            // These statements cause the current scopes to be exited.
            //
            // The actual scope unwinding is more complicated for nested
            // control-flow constructs, but this at least ensures that
            // variables are diagnosed before an explicit control-flow exit.
            if (const auto *returnStmt = dyn_cast<ReturnStmt>(stmt)
                // || isa<BreakStmt>(stmt) 
                // || isa<ContinueStmt>(stmt) 
                // || isa<GotoStmt>(stmt)
                ) {
                owner.reportPendingVarsForExit(
                    returnStmt->getEndLoc(),
                    scopes,
                    params,
                    reportedVars);
            }
        
            // Continue recursively through all child statements/expressions.
            for (auto it = stmt->child_begin(),
                      end = stmt->child_end();
                 it != end;
                 ++it) {
                
                if (*it)
                    scanStmt(*it);
            }
        }

        void trackVar(const VarDecl *var)
        {
            if (!var)
                return;
        
            // Function-local static variables have static storage duration and
            // are intentionally not tracked by this scope-based cleanup rule.
            if (var->isStaticLocal())
                return;
        
            if (!var->hasLocalStorage())
                return;
        
            if (!owner.shouldTrackVar(var, function))
                return;
        
            if (scopes.empty())
                return;
        
            TrackedVar tracked;
            tracked.decl = var;
            tracked.structName = owner.getStructName(var->getType());
            tracked.destroyed = false;
        
            scopes.back().vars.push_back(tracked);
        }

        void markDestroyedIfNeeded(const CallExpr *call) {
            if (!call)
                return;

            const auto *calleeDecl = call->getDirectCallee();
            if (!calleeDecl)
                return;

            const auto *functionDecl = dyn_cast<FunctionDecl>(calleeDecl);
            if (!functionDecl)
                return;

            const std::string calleeName = functionDecl->getNameAsString();
            if (calleeName.empty() || !owner.isDestroyCall(calleeName))
                return;

            const Expr *arg = call->getNumArgs() > 0 ? call->getArg(0) : nullptr;
            if (!arg)
                return;

            const auto *target = owner.getReferencedVarDecl(arg);
            if (!target)
                return;

            for (auto &scope : scopes) {
                for (auto &tracked : scope.vars) {
                    if (tracked.decl == target) {
                        tracked.destroyed = true;
                        scope.destroyed.insert(target);
                    }
                }
            }

            for (auto &tracked : params) {
                if (tracked.decl == target) {
                    tracked.destroyed = true;
                }
            }
        }

        void addParameter(const VarDecl *param) {
            if (!param || !owner.shouldTrackVar(param, function))
                return;

            TrackedVar tracked;
            tracked.decl = param;
            tracked.structName = owner.getStructName(param->getType());
            params.push_back(tracked);
        }

        void finalize() {
            owner.reportPendingParams(function, params, reportedVars);
        }
    };

    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    StructDatabase &database;

    const SourceManager *sourceManager = nullptr;

    std::vector<const FunctionDecl *> pendingFunctions;

    const std::string podSuffix;
    const std::string raiiSuffix;
    const std::string destroySuffix;
    const std::string copySuffix;
    const std::string moveSuffix;
    const std::string validSuffix;
    const std::string freeSuffix;

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

    const RecordDecl *getStructDecl(QualType type) const
    {
        QualType current = type.getUnqualifiedType();

        for (int i = 0; i < 8; ++i) {
            current = current.getCanonicalType().getUnqualifiedType();

            if (current->isRecordType()) {
                const auto *recordDecl = current->getAs<RecordType>()->getDecl();
                if (recordDecl)
                    return recordDecl;
            }

            if (current->isPointerType()) {
                current = current->getPointeeType();
                continue;
            }

            break;
        }

        return nullptr;
    }

    bool isStructType(
        QualType type,
        std::string *name = nullptr) const
    {
        const auto *recordDecl = getStructDecl(type);

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

        const std::string name =
            function->getNameAsString();

        return 
            // name == structName + podSuffix ||
            name == structName + raiiSuffix ||
            // name == structName + freeSuffix ||
            name == structName + destroySuffix ||
            name == structName + copySuffix ||
            name == structName + moveSuffix ||
            name == structName + validSuffix;
    }

    const VarDecl *getReferencedVarDecl(const Expr *expr) const
    {
        if (!expr)
            return nullptr;

        expr = expr->IgnoreParenImpCasts();

        if (const auto *declRef = dyn_cast<DeclRefExpr>(expr))
            return dyn_cast<VarDecl>(declRef->getDecl());

        if (const auto *unary = dyn_cast<UnaryOperator>(expr)) {
            if (unary->getOpcode() == UO_Deref ||
                unary->getOpcode() == UO_AddrOf)
                return getReferencedVarDecl(unary->getSubExpr());
        }

        if (const auto *memberExpr = dyn_cast<MemberExpr>(expr))
            return getReferencedVarDecl(memberExpr->getBase());

        if (const auto *arraySubscript = dyn_cast<ArraySubscriptExpr>(expr))
            return getReferencedVarDecl(arraySubscript->getBase());

        return nullptr;
    }

    bool shouldTrackVar(
        const VarDecl *var,
        const FunctionDecl *function) const
    {
        if (!var)
            return false;

        if (var->isStaticLocal())
            return false;

        // Normal local variables and function parameters can own
        // a RAII struct value.
        if (!var->isLocalVarDecl() &&
            !isa<ParmVarDecl>(var))
            return false;

        // A pointer to a RAII struct does not own the struct itself.
        // Cleanup is required for the struct value, not for pointer variables.
        QualType type = var->getType().getUnqualifiedType();

        if (type->isPointerType())
            return false;

        std::string structName;
        if (!isStructType(type, &structName))
            return false;

        if (function && isInsideHelperFunction(function, structName))
            return false;

        const auto *info = database.find(structName);
        return info && info->kind == StructDatabase::Kind::Raii;
    }

    std::string getStructName(QualType type) const
    {
        std::string name;
        isStructType(type, &name);
        return name;
    }

    bool isDestroyCall(const std::string &name) const
    {
        if (destroySuffix.empty())
            return false;

        if (name.size() < destroySuffix.size())
            return false;

        return name.compare(
            name.size() - destroySuffix.size(),
            destroySuffix.size(),
            destroySuffix) == 0;
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

    void reportPendingVarsForScopeExit(
        SourceLocation loc,
        const ScopeState &scope,
        std::unordered_set<const VarDecl *> &reportedVars) const
    {
        for (const auto &tracked : scope.vars) {
            if (tracked.destroyed || !tracked.decl)
                continue;

            if (reportedVars.count(tracked.decl))
                continue;

            reportedVars.insert(tracked.decl);

            reportUsageIssue(
                loc,
                "struct variable '" +
                tracked.decl->getNameAsString() +
                "' of type '" +
                tracked.structName +
                "' must be destroyed with '" +
                tracked.structName +
                destroySuffix +
                "' before scope exit because it is a RAII struct");
        }
    }

    void reportPendingVarsForExit(
        SourceLocation loc,
        const std::vector<ScopeState> &scopes,
        const std::vector<TrackedVar> &params,
        std::unordered_set<const VarDecl *> &reportedVars) const
    {
        for (const auto &scope : scopes) {
            reportPendingVarsForScopeExit(
                loc,
                scope,
                reportedVars);
        }

        for (const auto &tracked : params) {
            if (tracked.destroyed || !tracked.decl)
                continue;

            if (reportedVars.count(tracked.decl))
                continue;

            reportedVars.insert(tracked.decl);

            reportUsageIssue(
                loc,
                "struct parameter '" +
                tracked.decl->getNameAsString() +
                "' of type '" +
                tracked.structName +
                "' must be destroyed with '" +
                tracked.structName +
                destroySuffix +
                "' before scope exit because it is a RAII struct");
        }
    }

    void reportPendingParams(
        const FunctionDecl *function,
        const std::vector<TrackedVar> &params,
        std::unordered_set<const VarDecl *> &reportedVars) const
    {
        if (!function)
            return;

        for (const auto &tracked : params) {
            if (tracked.destroyed || !tracked.decl)
                continue;

            if (reportedVars.count(tracked.decl))
                continue;

            reportedVars.insert(tracked.decl);

            reportUsageIssue(
                function->getLocation(),
                "struct parameter '" + tracked.decl->getNameAsString() +
                    "' of type '" + tracked.structName +
                    "' must be destroyed with '" +
                tracked.structName +
                destroySuffix +
                "' before scope exit because it is a RAII struct");
        }
    }

public:
    StructCleanupRule(
        const RuleConfig &cfg,
        const Config &gc,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db)
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag),
          database(db),
          podSuffix(getOption(cfg,"pod_struct_creator_suffix")),
          raiiSuffix(getOption(cfg,"raii_struct_creator_suffix")),
          destroySuffix(getOption(cfg, "raii_struct_destroyer_suffix")),
          copySuffix(getOption(cfg,"raii_struct_copy_suffix")),
          moveSuffix(getOption(cfg,"raii_struct_move_suffix")),
          validSuffix(getOption(cfg,"raii_struct_valid_suffix")),
          freeSuffix(getOption(cfg,"free_struct_creator_suffix"))
    {
    }

    void run(const MatchFinder::MatchResult &result) override
    {
        sourceManager = result.SourceManager;

        if (!sourceManager)
            return;

        const auto *function =
            result.Nodes.getNodeAs<FunctionDecl>("function");

        if (!function || !function->doesThisDeclarationHaveABody())
            return;

        pendingFunctions.push_back(function);
    }

    void finalize()
    {
        for (const auto *function : pendingFunctions) {
            if (!function)
                continue;

            CleanupAnalyzer analyzer(*this, function);

            for (const auto *param : function->parameters())
                analyzer.addParameter(param);

            analyzer.scanStmt(function->getBody());
            analyzer.finalize();
        }
    }
};
