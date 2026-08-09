#pragma once

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <unordered_set>

#include "config.hpp"
#include "diagnostics.hpp"
#include "struct_database.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class StructDatabaseRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    StructDatabase &database;

    const SourceManager *sourceManager = nullptr;

    std::unordered_set<const TagDecl *>seen;

    const std::string freeSuffix;
    const std::string podSuffix;
    const std::string raiiSuffix;
    const std::string destroySuffix;
    const std::string copySuffix;
    const std::string moveSuffix;
    const std::string validSuffix;

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

    bool endsWith(
        const std::string &str,
        const std::string &suffix) const
    {
        if (suffix.empty())
            return false;

        if (str.size() < suffix.size())
            return false;

        return str.compare(
            str.size() - suffix.size(),
            suffix.size(),
            suffix) == 0;
    }

    bool isThirdParty(const std::string &file) const
    {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (!p.empty() &&
                file.find(p) != std::string::npos)
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

        SourceLocation spell =
            sm.getSpellingLoc(loc);

        if (sm.isInSystemHeader(spell))
            return true;

        std::string file =
            sm.getFilename(spell).str();

        if (file.empty())
            return true;

        return isThirdParty(file);
    }

    void registerStruct(
        const RecordDecl *RD)
    {
        if (!RD->isStruct())
            return;

        if (!RD->isThisDeclarationADefinition())
            return;

        if (!RD->getIdentifier())
            return;

        const TagDecl *canon =
            RD->getCanonicalDecl();

        if (!canon)
            return;

        if (seen.count(canon))
            return;

        seen.insert(canon);

        database.registerStruct(RD);
    }

    const RecordDecl *getStructDecl(QualType type) const
    {
        QualType current = type.getUnqualifiedType();

        for (int i = 0; i < 8; ++i) {
            current = current.getCanonicalType().getUnqualifiedType();

            if (const auto *recordType = current->getAs<RecordType>())
                return recordType->getDecl();

            if (const auto *typedefType = current->getAs<TypedefType>()) {
                current = typedefType->desugar();
                continue;
            }

            if (current->isEnumeralType() || current->isRecordType()) {
                break;
            }

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
        const std::string *expectedName = nullptr) const
    {
        const auto *recordDecl = getStructDecl(type);

        if (!recordDecl || !recordDecl->isStruct())
            return false;

        if (expectedName && !expectedName->empty())
            return recordDecl->getNameAsString() == *expectedName;

        return true;
    }

    bool isPointerToStructType(
        QualType type,
        const std::string *expectedName = nullptr) const
    {
        const QualType unqualified =
            type.getUnqualifiedType();

        const auto *ptrType =
            unqualified->getAs<PointerType>();

        if (!ptrType)
            return false;

        return isStructType(ptrType->getPointeeType(), expectedName);
    }

    bool isConstPointerToStructType(
        QualType type,
        const std::string *expectedName = nullptr) const
    {
        const QualType unqualified =
            type.getUnqualifiedType();

        const auto *ptrType =
            unqualified->getAs<PointerType>();

        if (!ptrType)
            return false;

        const QualType pointee = ptrType->getPointeeType();

        return pointee.isConstQualified() &&
            isStructType(pointee, expectedName);
    }

    bool matchesFreeCreator(
        const FunctionDecl *FD,
        const std::string &structName) const
    {
        if (FD->getNumParams() < 1)
            return false;

        const auto *selfParam = FD->getParamDecl(0);

        if (!selfParam)
            return false;

        if (selfParam->getNameAsString() != "self")
            return false;

        return isPointerToStructType(selfParam->getType(), &structName);
    }

    bool matchesPodCreator(
        const FunctionDecl *FD,
        const std::string &structName) const
    {
        return isStructType(FD->getReturnType(), &structName);
    }

    bool matchesRaiiCreator(
        const FunctionDecl *FD,
        const std::string &structName) const
    {
        return isStructType(FD->getReturnType(), &structName);
    }

    bool matchesCopyCreator(
        const FunctionDecl *FD,
        const std::string &structName) const
    {
        if (FD->getNumParams() != 1)
            return false;

        const auto *selfParam = FD->getParamDecl(0);

        if (!selfParam)
            return false;

        if (selfParam->getNameAsString() != "self")
            return false;

        return isConstPointerToStructType(selfParam->getType(), &structName) &&
            isStructType(FD->getReturnType(), &structName);
    }

    bool matchesMoveCreator(
        const FunctionDecl *FD,
        const std::string &structName) const
    {
        if (FD->getNumParams() != 1)
            return false;

        const auto *selfParam = FD->getParamDecl(0);

        if (!selfParam)
            return false;

        if (selfParam->getNameAsString() != "self")
            return false;

        return isPointerToStructType(selfParam->getType(), &structName) &&
            isStructType(FD->getReturnType(), &structName);
    }

    bool matchesDestroy(
        const FunctionDecl *FD,
        const std::string &structName) const
    {
        if (FD->getNumParams() != 1)
            return false;

        const auto *selfParam = FD->getParamDecl(0);

        if (!selfParam)
            return false;

        if (selfParam->getNameAsString() != "self")
            return false;

        return FD->getReturnType()
                .getCanonicalType()
                ->isVoidType() &&
            isPointerToStructType(selfParam->getType(), &structName);
    }

    bool matchesValid(
        const FunctionDecl *FD,
        const std::string &structName) const
    {
        if (FD->getNumParams() != 1)
            return false;

        const auto *selfParam = FD->getParamDecl(0);

        if (!selfParam)
            return false;

        if (selfParam->getNameAsString() != "self")
            return false;

        const QualType returnType =
            FD->getReturnType().getCanonicalType();

        return (returnType->isBooleanType() ||
                returnType->isSpecificBuiltinType(BuiltinType::Bool) ||
                returnType->isSpecificBuiltinType(BuiltinType::Int)) &&
            isConstPointerToStructType(selfParam->getType(), &structName);
    }

    void reportInvalidStruct(
        const std::string &structName,
        const StructDatabase::StructInfo &info) const
    {
        if (!sourceManager || !info.decl)
            return;

        std::string podSignature = structName + " " + structName + podSuffix + "(...)";
        std::string raiiSignature = structName + " " + structName + raiiSuffix + "(...)";
        std::string freeSignature = "<any> " + structName + freeSuffix + "(" + structName + "* self, ...)";

        std::string message =
            "struct '" + structName + "' is invalid: it requires exactly one of the following constructor functions: '" +
            podSignature + "', '" + raiiSignature + "', or '" +
            freeSignature + "'";

        diagnostics.report(
            config.level,
            *sourceManager,
            info.decl->getLocation(),
            message);
    }

    void reportMissingRaiiHelpers(
        const std::string &structName,
        const StructDatabase::StructInfo &info) const
    {
        if (!sourceManager || !info.decl)
            return;

        if (!info.hasDestroy) {
            std::string message =
                "struct '" + structName + "' is missing required destructor function 'void " +
                structName + destroySuffix + "(" + structName + "* self)'";

            diagnostics.report(
                config.level,
                *sourceManager,
                info.decl->getLocation(),
                message);
        }

        if (!info.hasCopy) {
            std::string message =
                "struct '" + structName + "' is missing required copy function '" +
                structName + " " + structName + copySuffix + "(const " +
                structName + "* self, ...)'";

            diagnostics.report(
                config.level,
                *sourceManager,
                info.decl->getLocation(),
                message);
        }

        if (!info.hasMove) {
            std::string message =
                "struct '" + structName + "' is missing required move function '" +
                structName + " " + structName + moveSuffix + "(" +
                structName + "* self, ...)'";

            diagnostics.report(
                config.level,
                *sourceManager,
                info.decl->getLocation(),
                message);
        }

        if (!info.hasValid) {
            std::string message =
                "struct '" + structName + "' is missing required valididation function '_Bool / bool " +
                structName + validSuffix + "(const " + structName + "* self)'";

            diagnostics.report(
                config.level,
                *sourceManager,
                info.decl->getLocation(),
                message);
        }
    }

    void registerFunction(
        const FunctionDecl *FD)
    {
        if (!FD->getIdentifier())
            return;

        std::string name =
            FD->getNameAsString();

        auto matchSuffix =
            [&](const std::string &suffix,
                StructDatabase::FunctionKind kind,
                auto matcher)
        {
            if (!endsWith(name, suffix))
                return false;

            std::string structName =
                name.substr(
                    0,
                    name.size() - suffix.size());

            if (structName.empty())
                return true;

            if (!matcher(FD, structName))
                return false;

            database.registerFunction(
                structName,
                kind);

            return true;
        };

        if (matchSuffix(
                freeSuffix,
                StructDatabase::FunctionKind::FreeCreator,
                [this](const FunctionDecl *fn, const std::string &structName) {
                    return matchesFreeCreator(fn, structName);
                }))
            return;

        if (matchSuffix(
                podSuffix,
                StructDatabase::FunctionKind::PodCreator,
                [this](const FunctionDecl *fn, const std::string &structName) {
                    return matchesPodCreator(fn, structName);
                }))
            return;

        if (matchSuffix(
                raiiSuffix,
                StructDatabase::FunctionKind::RaiiCreator,
                [this](const FunctionDecl *fn, const std::string &structName) {
                    return matchesRaiiCreator(fn, structName);
                }))
            return;

        if (matchSuffix(
                destroySuffix,
                StructDatabase::FunctionKind::Destroy,
                [this](const FunctionDecl *fn, const std::string &structName) {
                    return matchesDestroy(fn, structName);
                }))
            return;

        if (matchSuffix(
                copySuffix,
                StructDatabase::FunctionKind::Copy,
                [this](const FunctionDecl *fn, const std::string &structName) {
                    return matchesCopyCreator(fn, structName);
                }))
            return;

        if (matchSuffix(
                moveSuffix,
                StructDatabase::FunctionKind::Move,
                [this](const FunctionDecl *fn, const std::string &structName) {
                    return matchesMoveCreator(fn, structName);
                }))
            return;

        matchSuffix(
            validSuffix,
            StructDatabase::FunctionKind::Valid,
            [this](const FunctionDecl *fn, const std::string &structName) {
                return matchesValid(fn, structName);
            });
    }

public:
    StructDatabaseRule(
        const RuleConfig &cfg,
        const Config &gc,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db)
        :
        config(cfg),
        globalConfig(gc),
        suppressions(sup),
        diagnostics(diag),
        database(db),

        freeSuffix(getOption(
            cfg,
            "free_struct_creator_suffix")),

        podSuffix(getOption(
            cfg,
            "pod_struct_creator_suffix")),

        raiiSuffix(getOption(
            cfg,
            "raii_struct_creator_suffix")),

        destroySuffix(getOption(
            cfg,
            "raii_struct_destroyer_suffix")),

        copySuffix(getOption(
            cfg,
            "raii_struct_copy_suffix")),

        moveSuffix(getOption(
            cfg,
            "raii_struct_move_suffix")),

        validSuffix(getOption(
            cfg,
            "raii_struct_valid_suffix"))
    {
    }

    void run(
        const MatchFinder::MatchResult &result)
        override
    {
        const auto &sm =
            *result.SourceManager;

        sourceManager = result.SourceManager;

        const auto *RD =
            result.Nodes.getNodeAs<RecordDecl>("struct");

        if (!RD)
            RD = result.Nodes.getNodeAs<RecordDecl>("record");

        if (RD) {
            if (!shouldIgnore(
                    sm,
                    RD->getLocation()))
                registerStruct(RD);

            return;
        }

        if (const auto *FD =
            result.Nodes.getNodeAs<FunctionDecl>(
                "function")) {

            if (!shouldIgnore(
                    sm,
                    FD->getLocation()))
                registerFunction(FD);

            return;
        }
    }

    void finalize()
    {
        for (const auto &[name, info] : database.allStructs()) {

            if (info.kind == StructDatabase::Kind::Invalid) {
                reportInvalidStruct(name, info);
            }
            else if (info.kind == StructDatabase::Kind::Raii) {
                reportMissingRaiiHelpers(name, info);
            }
        }
    }
};