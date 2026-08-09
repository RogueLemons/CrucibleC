#include "config_parser.hpp"
#include "config.hpp"
#include "suppression_manager.hpp"
#include "rule_enum.hpp"
#include "rule_private.hpp"
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
        if (config.hasRule("enum")) {
            enumRule = std::make_unique<EnumRule>(
                config.getRuleConfig("enum"),
                config,
                suppressions,
                *diagnostics
            );
        
            finder.addMatcher(
                enumDecl().bind("enum"),
                enumRule.get()
            );
        }

        // Private rule
        if (config.hasRule("private")) {
            privateRule = std::make_unique<PrivateRule>(
                config.getRuleConfig("private"),
                config,
                suppressions,
                *diagnostics
            );
        
            finder.addMatcher(
                memberExpr(
                    hasAncestor(functionDecl().bind("parentFunction"))
                ).bind("privateAccess"),
                privateRule.get()
            );
        }

        // Function pointer rule
        if (config.hasRule("function_pointer")) {
            functionPointerRule = std::make_unique<FunctionPointerRule>(
                config.getRuleConfig("function_pointer"),
                config,
                suppressions,
                *diagnostics
            );
        
            finder.addMatcher(
                parmVarDecl().bind("funcptr"),
                functionPointerRule.get()
            );
            
            finder.addMatcher(
                varDecl().bind("funcptr"),
                functionPointerRule.get()
            );
        }

        // Typedef struct rule
        if (config.hasRule("typedef_struct")) {
        
            typedefStructRule =
                std::make_unique<TypedefStructRule>(
                    config.getRuleConfig("typedef_struct"),
                    config,
                    suppressions,
                    *diagnostics
                );
            
            finder.addMatcher(
                recordDecl(
                    isStruct(),
                    unless(isExpansionInSystemHeader())
                ).bind("struct"),
                typedefStructRule.get()
            );
        
            finder.addMatcher(
                typedefDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("typedef"),
                typedefStructRule.get()
            );
        }

        // Assignment rule
        if (config.hasRule("assignment")) {
            assignmentRule = std::make_unique<AssignmentRule>(
                config.getRuleConfig("assignment"),
                config,
                suppressions,
                *diagnostics
            );
        
            finder.addMatcher(
                varDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("varDecl"),
                assignmentRule.get()
            );
        
            finder.addMatcher(
                binaryOperator(
                    isAssignmentOperator()
                ).bind("assignmentOp"),
                assignmentRule.get()
            );

            finder.addMatcher(
                callExpr(
                    unless(isExpansionInSystemHeader())
                ).bind("callExpr"),
                assignmentRule.get()
            );
        }

        // Prefix namespace rule
        if (config.hasRule("prefix_namespace")) {

            prefixNamespaceRule =
                std::make_unique<PrefixNamespaceRule>(
                    config.getRuleConfig(
                        "prefix_namespace"
                    ),
                    config,
                    suppressions,
                    *diagnostics
                );

            finder.addMatcher(
                functionDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("function"),
                prefixNamespaceRule.get()
            );

            finder.addMatcher(
                recordDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("record"),
                prefixNamespaceRule.get()
            );

            finder.addMatcher(
                typedefDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("typedef"),
                prefixNamespaceRule.get()
            );
        }

        // Null check rule
        if (config.hasRule("null_check")) {
        
            nullCheckRule =
                std::make_unique<NullCheckRule>(
                    config.getRuleConfig("null_check"),
                    config,
                    suppressions,
                    *diagnostics
                );
            
            finder.addMatcher(
                functionDecl(
                    isDefinition(),
                    unless(isExpansionInSystemHeader())
                ).bind("function"),
                nullCheckRule.get()
            );
        }

        // Argument pointer movement rule
        if (config.hasRule("argument_pointer_movement")) {
        
            argumentPointerMovementRule =
                std::make_unique<
                    ArgumentPointerMovementRule>(
                        config.getRuleConfig(
                            "argument_pointer_movement"
                        ),
                        config,
                        suppressions,
                        *diagnostics
                    );
                
            finder.addMatcher(
                functionDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("func"),
                argumentPointerMovementRule.get()
            );

            finder.addMatcher(
                callExpr(
                    unless(isExpansionInSystemHeader())
                ).bind("call"),
                argumentPointerMovementRule.get()
            );
        }

        // Argument pointer movement rule for callsite
        if (config.hasRule("argument_pointer_movement")) {

            argumentPointerCallsiteRule =
                std::make_unique<ArgumentPointerCallsiteRule>(
                    config.getRuleConfig("argument_pointer_movement"),
                    config,
                    suppressions,
                    *diagnostics
                );
            
            finder.addMatcher(
                callExpr(unless(isExpansionInSystemHeader()))
                    .bind("call"),
                argumentPointerCallsiteRule.get()
            );
        }

        // struct resource management rule
        if (config.hasRule("struct_resource_management")) {
            
            // Struct kind database rule
            structDatabaseRule =
                std::make_unique<StructDatabaseRule>(
                    config.getRuleConfig("struct_resource_management"),
                    config,
                    suppressions,
                    *diagnostics,
                    structDatabase
                );

            finder.addMatcher(
                recordDecl(
                    isStruct(),
                    unless(isExpansionInSystemHeader())
                ).bind("struct"),
                structDatabaseRule.get());
            
            finder.addMatcher(
                functionDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("function"),
                structDatabaseRule.get());

            structInitRule =
                std::make_unique<StructInitRule>(
                    config.getRuleConfig("struct_resource_management"),
                    config,
                    suppressions,
                    *diagnostics,
                    structDatabase
                );

            finder.addMatcher(
                varDecl(
                    unless(isExpansionInSystemHeader())
                ).bind("varDecl"),
                structInitRule.get());

            finder.addMatcher(
                binaryOperator(
                    isAssignmentOperator()
                ).bind("assignment"),
                structInitRule.get());

            finder.addMatcher(
                callExpr(
                    unless(isExpansionInSystemHeader())
                ).bind("call"),
                structInitRule.get());

            finder.addMatcher(
                recordDecl(
                    isStruct(),
                    unless(isExpansionInSystemHeader())
                ).bind("record"),
                structInitRule.get());
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