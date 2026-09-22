#pragma once

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include "struct_database.hpp"
#include "struct_database_rule.hpp"
#include "struct_init_rule.hpp"
#include "struct_cleanup_rule.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class FinderAndFinalizerConsumer : public ASTConsumer {
private:
    MatchFinder &finder;

    StructDatabase &database;

    StructDatabaseRule *databaseRule = nullptr;
    StructInitRule *initRule = nullptr;
    StructCleanupRule *cleanupRule = nullptr;

public:
    FinderAndFinalizerConsumer(
        MatchFinder &finder,
        StructDatabase &database,
        StructDatabaseRule *databaseRule,
        StructInitRule *initRule,
        StructCleanupRule *cleanupRule);

    void HandleTranslationUnit(
        ASTContext &context) override;
};
