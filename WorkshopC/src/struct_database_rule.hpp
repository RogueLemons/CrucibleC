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
    const Config &config;

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
    const std::string returnSuffix;
    const std::string validSuffix;

private:
    bool endsWith(
        const std::string &str,
        const std::string &suffix) const;

    bool isThirdParty(const std::string &file) const;

    bool shouldIgnore(
        const SourceManager &sm,
        SourceLocation loc) const;

    void registerStruct(
        const RecordDecl *RD);

    const RecordDecl *getStructDecl(QualType type) const;

    bool isStructType(
        QualType type,
        const std::string *expectedName = nullptr) const;

    bool isPointerToStructType(
        QualType type,
        const std::string *expectedName = nullptr) const;

    bool isConstPointerToStructType(
        QualType type,
        const std::string *expectedName = nullptr) const;

    bool matchesFreeCreator(
        const FunctionDecl *FD,
        const std::string &structName) const;

    bool matchesPodCreator(
        const FunctionDecl *FD,
        const std::string &structName) const;

    bool matchesRaiiCreator(
        const FunctionDecl *FD,
        const std::string &structName) const;

    bool matchesCopyCreator(
        const FunctionDecl *FD,
        const std::string &structName) const;

    bool matchesMoveCreator(
        const FunctionDecl *FD,
        const std::string &structName) const;

    bool matchesReturn(
        const FunctionDecl *FD,
        const std::string &structName) const;

    bool matchesDestroy(
        const FunctionDecl *FD,
        const std::string &structName) const;

    bool matchesValid(
        const FunctionDecl *FD,
        const std::string &structName) const;

    void reportInvalidStruct(
        const std::string &structName,
        const StructDatabase::StructInfo &info) const;

    void reportMissingRaiiHelpers(
        const std::string &structName,
        const StructDatabase::StructInfo &info) const;

    void registerFunction(
        const FunctionDecl *FD);

public:
    StructDatabaseRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag,
        StructDatabase &db);

    void bindFinder(MatchFinder &finder);

    void run(
        const MatchFinder::MatchResult &result)
        override;

    void finalize();
};
