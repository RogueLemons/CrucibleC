#include "config_parser.hpp"
#include "config.hpp"
#include "suppression_manager.hpp"
#include "rule_enum.hpp"
#include "rule_private.hpp"
#include "rule_private_alternative.hpp"
#include "rule_function_pointer.hpp"
#include "rule_typedef_struct.hpp"
#include "rule_assignment.hpp"
#include "rule_prefix_namespace.hpp"
#include "rule_null_check.hpp"
#include "rule_arg_ptr_move.hpp"
#include "rule_arg_ptr_move_callsite.hpp"

#include "struct_database.hpp"
#include "struct_database_rule.hpp"
#include "struct_init_rule.hpp"
#include "struct_cleanup_rule.hpp"
#include "finder_and_finalizer_consumer.hpp"

#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

#include <iostream>
#include <memory>

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

// -------------------------
// Frontend Action
// -------------------------
class WorkshopFrontendAction : public ASTFrontendAction {
private:
    MatchFinder finder{};
    const Config &config;

    int &warnings;
    int &errors;

    std::unique_ptr<Diagnostics> diagnostics{};
    SuppressionManager suppressions{};

    std::unique_ptr<EnumRule> enumRule{};
    std::unique_ptr<PrivateRule> privateRule{};
    std::unique_ptr<PrivateAlternativeRule> privateAlternativeRule{};
    std::unique_ptr<FunctionPointerRule> functionPointerRule{};
    std::unique_ptr<AssignmentRule> assignmentRule{};
    std::unique_ptr<PrefixNamespaceRule> prefixNamespaceRule{};
    std::unique_ptr<NullCheckRule> nullCheckRule{};
    std::unique_ptr<TypedefStructRule> typedefStructRule{};
    std::unique_ptr<ArgumentPointerMovementRule> argumentPointerMovementRule{};
    std::unique_ptr<ArgumentPointerCallsiteRule> argumentPointerCallsiteRule{};
    
    StructDatabase structDatabase{};
    std::unique_ptr<StructDatabaseRule> structDatabaseRule{};
    std::unique_ptr<StructInitRule> structInitRule{};
    std::unique_ptr<StructCleanupRule> structCleanupRule{};

public:
    WorkshopFrontendAction(const Config &cfg, int &w, int &e)
        : config(cfg), warnings(w), errors(e) {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &CI,
        StringRef) override
    {
        // Diagnostics
        diagnostics = std::make_unique<Diagnostics>(
            warnings,
            errors
        );

        // Enum rule
        if (config.enumRule.level != RuleLevel::Off) {
            enumRule = std::make_unique<EnumRule>(
                config,
                suppressions,
                *diagnostics
            );

            enumRule->bindFinder(finder);
        }

        // Private rule
        if (config.privateRule.level != RuleLevel::Off) {
            privateRule = std::make_unique<PrivateRule>(
                config,
                suppressions,
                *diagnostics
            );

            privateRule->bindFinder(finder);
        }

        // Private alternative rule
        if (config.privateAlternativeRule.level != RuleLevel::Off) {
            privateAlternativeRule = std::make_unique<PrivateAlternativeRule>(
                config,
                suppressions,
                *diagnostics
            );

            privateAlternativeRule->bindFinder(finder);
        }

        // Function pointer rule
        if (config.functionPointerRule.level != RuleLevel::Off) {
            functionPointerRule = std::make_unique<FunctionPointerRule>(
                config,
                suppressions,
                *diagnostics
            );

            functionPointerRule->bindFinder(finder);
        }

        // Typedef struct rule
        if (config.typedefStructRule.level != RuleLevel::Off) {

            typedefStructRule =
                std::make_unique<TypedefStructRule>(
                    config,
                    suppressions,
                    *diagnostics
                );

            typedefStructRule->bindFinder(finder);
        }

        // Assignment rule
        if (config.assignmentRule.level != RuleLevel::Off) {
            assignmentRule = std::make_unique<AssignmentRule>(
                config,
                suppressions,
                *diagnostics
            );

            assignmentRule->bindFinder(finder);
        }

        // Prefix namespace rule
        if (config.prefixNamespaceRule.level != RuleLevel::Off) {

            prefixNamespaceRule =
                std::make_unique<PrefixNamespaceRule>(
                    config,
                    suppressions,
                    *diagnostics
                );

            prefixNamespaceRule->bindFinder(finder);
        }

        // Null check rule
        if (config.nullCheckRule.level != RuleLevel::Off) {

            nullCheckRule =
                std::make_unique<NullCheckRule>(
                    config,
                    suppressions,
                    *diagnostics
                );

            nullCheckRule->bindFinder(finder);
        }

        // Argument pointer movement rule
        if (config.argumentPointerMovementRule.level != RuleLevel::Off) {

            argumentPointerMovementRule =
                std::make_unique<
                    ArgumentPointerMovementRule>(
                        config,
                        suppressions,
                        *diagnostics
                    );

            argumentPointerMovementRule->bindFinder(finder);
        }

        // Argument pointer movement rule for callsite
        if (config.argumentPointerMovementRule.level != RuleLevel::Off) {

            argumentPointerCallsiteRule =
                std::make_unique<ArgumentPointerCallsiteRule>(
                    config,
                    suppressions,
                    *diagnostics
                );

            argumentPointerCallsiteRule->bindFinder(finder);
        }

        // struct resource management rule
        if (config.structResourceManagementRule.level != RuleLevel::Off) {

            // Struct kind database rule
            structDatabaseRule =
                std::make_unique<StructDatabaseRule>(
                    config,
                    suppressions,
                    *diagnostics,
                    structDatabase
                );

            structDatabaseRule->bindFinder(finder);

            // Struct initialization and assignment rule
            structInitRule =
                std::make_unique<StructInitRule>(
                    config,
                    suppressions,
                    *diagnostics,
                    structDatabase
                );

            structInitRule->bindFinder(finder);

            // Struct cleanup rule
            structCleanupRule =
                std::make_unique<StructCleanupRule>(
                    config,
                    suppressions,
                    *diagnostics,
                    structDatabase
                );

            structCleanupRule->bindFinder(finder);
        }

        return std::make_unique<FinderAndFinalizerConsumer>(
            finder,
            structDatabase,
            structDatabaseRule.get(),
            structInitRule.get(),
            structCleanupRule.get()
        );
    }
};

// -------------------------
// Factory
// -------------------------
class WorkshopActionFactory : public FrontendActionFactory {
private:
    const Config &config;
    int warnings = 0;
    int errors = 0;

public:
    WorkshopActionFactory(const Config &cfg)
        : config(cfg) {}

    std::unique_ptr<FrontendAction> create() override {
        return std::make_unique<WorkshopFrontendAction>(
            config, warnings, errors
        );
    }

    int getWarnings() const { return warnings; }
    int getErrors() const { return errors; }
};

// -------------------------
// MAIN
// -------------------------
int main(int argc, const char **argv) {
    if (argc < 4) {
        std::cerr << "Usage: workshopc <config.yaml> <source-file> <compdb-dir>\n";
        return 1;
    }

    // -------------------------
    // Load config
    // -------------------------
    Config config;
    if (!ConfigParser::loadFromFile(argv[1], config)) {
        std::cerr << "Failed to load config\n";
        return 1;
    }

    std::string file = argv[2];
    std::string compdbDir = argv[3];

    // -------------------------
    // Load compilation database
    // -------------------------
    std::string errorMsg;
    auto compilationDB =
        CompilationDatabase::loadFromDirectory(compdbDir, errorMsg);

    if (!compilationDB) {
        std::cerr << "Failed to load compilation database: "
                  << errorMsg << "\n";
        return 1;
    }

    // -------------------------
    // Run tool
    // -------------------------
    std::vector<std::string> sources = { file };

    ClangTool tool(*compilationDB, sources);

    // Force C mode for .c files
    tool.appendArgumentsAdjuster(
        getInsertArgumentAdjuster(
            {"-x", "c"},
            ArgumentInsertPosition::BEGIN
        )
    );

    // Provide WorkshopC macro tag
    tool.appendArgumentsAdjuster(
        getInsertArgumentAdjuster(
            {"-DWORKSHOPC_PARSING=1"},
            ArgumentInsertPosition::BEGIN
        )
    );

    WorkshopActionFactory factory(config);

    int result = tool.run(&factory);

    // -------------------------
    // Final reporting
    // -------------------------
    std::cout << "\nWarnings: " << factory.getWarnings() << "\n";
    std::cout << "Errors: " << factory.getErrors() << "\n";

    if (factory.getErrors() > 0)
        return 1;

    return result;
}