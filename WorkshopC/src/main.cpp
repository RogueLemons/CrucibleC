#include "config_parser.hpp"
#include "config.hpp"
#include "suppression_manager.hpp"
#include "rule_enum.hpp"

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

public:
    WorkshopFrontendAction(const Config &cfg, int &w, int &e)
        : config(cfg), warnings(w), errors(e) {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &CI,
        StringRef) override
    {
        diagnostics = std::make_unique<Diagnostics>(
            warnings,
            errors
        );

        if (config.hasRule("enum")) {
            enumRule = std::make_unique<EnumRule>(
                config.getRuleConfig("enum"),
                config,
                suppressions,
                *diagnostics
            );

            finder.addMatcher(enumDecl().bind("enum"), enumRule.get());
        }

        return finder.newASTConsumer();
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