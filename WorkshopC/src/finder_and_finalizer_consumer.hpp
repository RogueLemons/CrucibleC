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
        StructCleanupRule *cleanupRule)
        : finder(finder),
          database(database),
          databaseRule(databaseRule),
          initRule(initRule),
          cleanupRule(cleanupRule)
    {
    }

    void HandleTranslationUnit(
        ASTContext &context) override
    {
        // -------------------------------------------------
        // Phase 1:
        //
        // Run all registered matchers. The struct rules
        // only collect information at this stage.
        // -------------------------------------------------
        finder.matchAST(context);

        // -------------------------------------------------
        // Phase 2:
        //
        // Finalize the shared struct database.
        //
        // This MUST happen before StructInitRule and
        // StructCleanupRule perform any checks.
        // -------------------------------------------------
        database.finalize();

        // -------------------------------------------------
        // Phase 3:
        //
        // Run the database rule's diagnostics now that
        // the database is complete.
        // -------------------------------------------------
        if (databaseRule)
            databaseRule->finalize();

        // -------------------------------------------------
        // Phase 4:
        //
        // Run initialization checks against the finished
        // database.
        // -------------------------------------------------
        if (initRule)
            initRule->finalize();

        // -------------------------------------------------
        // Phase 5:
        //
        // Run cleanup checks against the finished
        // database.
        // -------------------------------------------------
        if (cleanupRule)
            cleanupRule->finalize();
    }
};
