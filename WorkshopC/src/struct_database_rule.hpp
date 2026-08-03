#pragma once

#include <clang/AST/Decl.h>
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

    void registerFunction(
        const FunctionDecl *FD)
    {
        if (!FD->getIdentifier())
            return;

        std::string name =
            FD->getNameAsString();

        auto matchSuffix =
            [&](const std::string &suffix,
                StructDatabase::FunctionKind kind)
        {
            if (!endsWith(name, suffix))
                return false;

            std::string structName =
                name.substr(
                    0,
                    name.size() - suffix.size());

            if (structName.empty())
                return true;

            database.registerFunction(
                structName,
                kind);

            return true;
        };

        if (matchSuffix(
                freeSuffix,
                StructDatabase::FunctionKind::FreeCreator))
            return;

        if (matchSuffix(
                podSuffix,
                StructDatabase::FunctionKind::PodCreator))
            return;

        if (matchSuffix(
                raiiSuffix,
                StructDatabase::FunctionKind::RaiiCreator))
            return;

        if (matchSuffix(
                destroySuffix,
                StructDatabase::FunctionKind::Destroy))
            return;

        if (matchSuffix(
                copySuffix,
                StructDatabase::FunctionKind::Copy))
            return;

        if (matchSuffix(
                moveSuffix,
                StructDatabase::FunctionKind::Move))
            return;

        matchSuffix(
            validSuffix,
            StructDatabase::FunctionKind::Valid);
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

        if (const auto *RD =
            result.Nodes.getNodeAs<RecordDecl>(
                "struct")) {

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

    void onEndOfTranslationUnit() override
    {
        database.finalize();

        std::cout << "\n===== Struct Database =====\n";

    for (const auto &[name, info] : database.allStructs()) {

        std::cout << name << "\n";

        std::cout << "  kind: ";

        switch (info.kind) {
        case StructDatabase::Kind::Free:
            std::cout << "Free";
            break;

        case StructDatabase::Kind::Pod:
            std::cout << "Pod";
            break;

        case StructDatabase::Kind::Raii:
            std::cout << "Raii";
            break;

        case StructDatabase::Kind::Invalid:
            std::cout << "Invalid";
            break;
        }

        std::cout << "\n";

        std::cout << "  free creator : " << info.hasFreeCreator << "\n";
        std::cout << "  pod creator  : " << info.hasPodCreator << "\n";
        std::cout << "  raii creator : " << info.hasRaiiCreator << "\n";
        std::cout << "  destroy      : " << info.hasDestroy << "\n";
        std::cout << "  copy         : " << info.hasCopy << "\n";
        std::cout << "  move         : " << info.hasMove << "\n";
        std::cout << "  valid        : " << info.hasValid << "\n";

        std::cout << "\n";
    }

    std::cout << "===========================\n";
    }
};