#include "finder_and_finalizer_consumer.hpp"

FinderAndFinalizerConsumer::FinderAndFinalizerConsumer(
    MatchFinder &finder,
    StructDatabase &database,
    StructDatabaseRule *databaseRule,
    StructInitRule *initRule,
    StructCleanupRule *cleanupRule,
    StructRaiiDiscardRule *raiiDiscardRule)
    : finder(finder),
      database(database),
      databaseRule(databaseRule),
      initRule(initRule),
      cleanupRule(cleanupRule),
      raiiDiscardRule(raiiDiscardRule)
{
}

void FinderAndFinalizerConsumer::HandleTranslationUnit(
    ASTContext &context)
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

    // -------------------------------------------------
    // Phase 6:
    //
    // Check that raii struct return values are never
    // discarded or accessed directly, now that the
    // database knows which structs are raii structs.
    // -------------------------------------------------
    if (raiiDiscardRule)
        raiiDiscardRule->finalize();
}
