#include "config_parser.hpp"
#include "config.hpp"
#include "suppression_manager.hpp"
#include "rule_suppression_reason.hpp"
#include "rule_suppression_balance.hpp"
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
#include "rule_arg_ptr_use_after_move.hpp"
#include "rule_restricted_malloc.hpp"
#include "rule_single_return.hpp"
#include "rule_strict_switch.hpp"
#include "rule_global_variable.hpp"
#include "rule_reference_pointer.hpp"
#include "rule_function_pointer_tags.hpp"
#include "rule_function_discard.hpp"
#include "rule_array_struct.hpp"
#include "rule_span_struct.hpp"
#include "rule_const_field.hpp"
#include "rule_no_goto.hpp"
#include "rule_vtable.hpp"
#include "rule_interface.hpp"

#include "struct_database.hpp"
#include "struct_database_rule.hpp"
#include "struct_init_rule.hpp"
#include "struct_cleanup_rule.hpp"
#include "struct_raii_discard_rule.hpp"
#include "finder_and_finalizer_consumer.hpp"

#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Options/OptionUtils.h>

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

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

    Diagnostics &diagnostics;
    SuppressionManager suppressions{};

    std::unique_ptr<SuppressionReasonRule> suppressionReasonRule{};
    std::unique_ptr<SuppressionBalanceRule> suppressionBalanceRule{};
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
    std::unique_ptr<ArgumentPointerUseAfterMoveRule> argumentPointerUseAfterMoveRule{};
    std::unique_ptr<RestrictedMallocRule> restrictedMallocRule{};
    std::unique_ptr<SingleReturnRule> singleReturnRule{};
    std::unique_ptr<StrictSwitchRule> strictSwitchRule{};
    std::unique_ptr<GlobalVariableRule> globalVariableRule{};
    std::unique_ptr<ReferencePointerRule> referencePointerRule{};
    std::unique_ptr<FunctionPointerTagRule> functionPointerTagRule{};
    std::unique_ptr<FunctionDiscardRule> functionDiscardRule{};
    std::unique_ptr<ArrayStructRule> arrayStructRule{};
    std::unique_ptr<SpanStructRule> spanStructRule{};
    std::unique_ptr<ConstFieldRule> constFieldRule{};
    std::unique_ptr<NoGotoRule> noGotoRule{};
    std::unique_ptr<VtableRule> vtableRule{};
    std::unique_ptr<InterfaceRule> interfaceRule{};
    
    StructDatabase structDatabase{};
    std::unique_ptr<StructDatabaseRule> structDatabaseRule{};
    std::unique_ptr<StructInitRule> structInitRule{};
    std::unique_ptr<StructCleanupRule> structCleanupRule{};
    std::unique_ptr<StructRaiiDiscardRule> structRaiiDiscardRule{};
    std::unique_ptr<StructDestroyDefinitionRule> structDestroyDefinitionRule{};

public:
    WorkshopFrontendAction(const Config &cfg, Diagnostics &diag)
        : config(cfg), diagnostics(diag) {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &CI,
        StringRef) override
    {
        // Every 'WorkshopC off' must be turned back on, always enabled
        suppressionBalanceRule = std::make_unique<SuppressionBalanceRule>(
            config,
            suppressions,
            diagnostics
        );

        suppressionBalanceRule->bindFinder(finder);

        // Suppression reason rule
        if (config.suppressionReasonRule.level != RuleLevel::Off) {
            suppressionReasonRule = std::make_unique<SuppressionReasonRule>(
                config,
                suppressions,
                diagnostics
            );

            suppressionReasonRule->bindFinder(finder);
        }

        // Enum rule
        if (config.enumRule.level != RuleLevel::Off) {
            enumRule = std::make_unique<EnumRule>(
                config,
                suppressions,
                diagnostics
            );

            enumRule->bindFinder(finder);
        }

        // Private rule
        if (config.privateRule.level != RuleLevel::Off) {
            privateRule = std::make_unique<PrivateRule>(
                config,
                suppressions,
                diagnostics
            );

            privateRule->bindFinder(finder);
        }

        // Private alternative rule
        if (config.privateAlternativeRule.level != RuleLevel::Off) {
            privateAlternativeRule = std::make_unique<PrivateAlternativeRule>(
                config,
                suppressions,
                diagnostics
            );

            privateAlternativeRule->bindFinder(finder);
        }

        // Function pointer rule
        if (config.functionPointerRule.level != RuleLevel::Off) {
            functionPointerRule = std::make_unique<FunctionPointerRule>(
                config,
                suppressions,
                diagnostics
            );

            functionPointerRule->bindFinder(finder);
        }

        // Typedef struct rule
        if (config.typedefStructRule.level != RuleLevel::Off) {

            typedefStructRule =
                std::make_unique<TypedefStructRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            typedefStructRule->bindFinder(finder);
        }

        // Assignment rule
        if (config.assignmentRule.level != RuleLevel::Off) {
            assignmentRule = std::make_unique<AssignmentRule>(
                config,
                suppressions,
                diagnostics
            );

            assignmentRule->bindFinder(finder);
        }

        // Prefix namespace rule
        if (config.prefixNamespaceRule.level != RuleLevel::Off) {

            prefixNamespaceRule =
                std::make_unique<PrefixNamespaceRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            prefixNamespaceRule->bindFinder(finder);
        }

        // Null check rule
        if (config.nullCheckRule.level != RuleLevel::Off) {

            nullCheckRule =
                std::make_unique<NullCheckRule>(
                    config,
                    suppressions,
                    diagnostics
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
                        diagnostics
                    );

            argumentPointerMovementRule->bindFinder(finder);
        }

        // Argument pointer movement rule for callsite
        if (config.argumentPointerMovementRule.level != RuleLevel::Off) {

            argumentPointerCallsiteRule =
                std::make_unique<ArgumentPointerCallsiteRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            argumentPointerCallsiteRule->bindFinder(finder);

            // No use of a pointer after it has been moved
            argumentPointerUseAfterMoveRule =
                std::make_unique<ArgumentPointerUseAfterMoveRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            argumentPointerUseAfterMoveRule->bindFinder(finder);
        }

        // Restricted malloc rule
        if (config.restrictedMallocRule.level != RuleLevel::Off) {

            restrictedMallocRule =
                std::make_unique<RestrictedMallocRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            restrictedMallocRule->bindFinder(finder);
        }

        // Single return rule
        if (config.singleReturnRule.level != RuleLevel::Off) {

            singleReturnRule =
                std::make_unique<SingleReturnRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            singleReturnRule->bindFinder(finder);
        }

        // Strict switch rule
        if (config.strictSwitchRule.level != RuleLevel::Off) {

            strictSwitchRule =
                std::make_unique<StrictSwitchRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            strictSwitchRule->bindFinder(finder);
        }

        // Global variable rule
        if (config.globalVariableRule.level != RuleLevel::Off) {

            globalVariableRule =
                std::make_unique<GlobalVariableRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            globalVariableRule->bindFinder(finder);
        }

        // Reference pointer rule
        if (config.referencePointerRule.level != RuleLevel::Off) {

            referencePointerRule =
                std::make_unique<ReferencePointerRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            referencePointerRule->bindFinder(finder);
        }

        // Function discard rule
        if (config.functionDiscardRule.level != RuleLevel::Off) {
            functionDiscardRule = std::make_unique<FunctionDiscardRule>(
                config,
                suppressions,
                diagnostics
            );

            functionDiscardRule->bindFinder(finder);
        }

        // Array struct rule
        if (config.arrayStructRule.level != RuleLevel::Off) {
            arrayStructRule = std::make_unique<ArrayStructRule>(
                config,
                suppressions,
                diagnostics
            );

            arrayStructRule->bindFinder(finder);
        }

        // Span struct rule
        if (config.spanStructRule.level != RuleLevel::Off) {
            spanStructRule = std::make_unique<SpanStructRule>(
                config,
                suppressions,
                diagnostics
            );

            spanStructRule->bindFinder(finder);
        }

        // Const field rule
        if (config.constFieldRule.level != RuleLevel::Off) {
            constFieldRule = std::make_unique<ConstFieldRule>(
                config,
                suppressions,
                diagnostics
            );

            constFieldRule->bindFinder(finder);
        }

        // No goto rule
        if (config.noGotoRule.level != RuleLevel::Off) {
            noGotoRule = std::make_unique<NoGotoRule>(
                config,
                suppressions,
                diagnostics
            );

            noGotoRule->bindFinder(finder);
        }

        // Interfaces rule: vtables
        if (config.interfacesRule.level != RuleLevel::Off) {
            vtableRule = std::make_unique<VtableRule>(
                config,
                suppressions,
                diagnostics
            );

            vtableRule->bindFinder(finder);

            // Interfaces rule: interfaces
            interfaceRule = std::make_unique<InterfaceRule>(
                config,
                suppressions,
                diagnostics
            );

            interfaceRule->bindFinder(finder);
        }

        // Tags of functions assigned or passed to function pointers,
        // for both the movement tags and the reference tag
        if (config.argumentPointerMovementRule.level != RuleLevel::Off ||
            config.referencePointerRule.level != RuleLevel::Off) {

            functionPointerTagRule =
                std::make_unique<FunctionPointerTagRule>(
                    config,
                    suppressions,
                    diagnostics
                );

            functionPointerTagRule->bindFinder(finder);
        }

        // struct resource management rule
        if (config.structResourceManagementRule.level != RuleLevel::Off) {

            // Struct kind database rule
            structDatabaseRule =
                std::make_unique<StructDatabaseRule>(
                    config,
                    suppressions,
                    diagnostics,
                    structDatabase
                );

            structDatabaseRule->bindFinder(finder);

            // Struct initialization and assignment rule
            structInitRule =
                std::make_unique<StructInitRule>(
                    config,
                    suppressions,
                    diagnostics,
                    structDatabase
                );

            structInitRule->bindFinder(finder);

            // Struct cleanup rule
            structCleanupRule =
                std::make_unique<StructCleanupRule>(
                    config,
                    suppressions,
                    diagnostics,
                    structDatabase
                );

            structCleanupRule->bindFinder(finder);

            // Raii struct return value discard rule
            structRaiiDiscardRule =
                std::make_unique<StructRaiiDiscardRule>(
                    config,
                    suppressions,
                    diagnostics,
                    structDatabase
                );

            structRaiiDiscardRule->bindFinder(finder);

            // Raii struct destroy function definition rule
            if (config.structResourceManagementRule.raiiStandardizedDestroyDefinitions) {
                structDestroyDefinitionRule =
                    std::make_unique<StructDestroyDefinitionRule>(
                        config,
                        suppressions,
                        diagnostics,
                        structDatabase
                    );

                structDestroyDefinitionRule->bindFinder(finder);
            }
        }

        return std::make_unique<FinderAndFinalizerConsumer>(
            finder,
            structDatabase,
            structDatabaseRule.get(),
            structInitRule.get(),
            structCleanupRule.get(),
            structRaiiDiscardRule.get(),
            structDestroyDefinitionRule.get()
        );
    }
};

// -------------------------
// Factory
// -------------------------
class WorkshopActionFactory : public FrontendActionFactory {
private:
    const Config &config;
    Diagnostics &diagnostics;

public:
    WorkshopActionFactory(const Config &cfg, Diagnostics &diag)
        : config(cfg), diagnostics(diag) {}

    std::unique_ptr<FrontendAction> create() override {
        return std::make_unique<WorkshopFrontendAction>(
            config, diagnostics
        );
    }
};

// -------------------------
// Exit codes
//
// 0-3 report analysis results as a bitmask (1 = errors found,
// 2 = warnings found). 64 and above mean the tool itself could
// not run the analysis, so they never collide with rule results.
// -------------------------
enum ExitCode {
    ExitClean = 0,
    ExitErrors = 1,
    ExitWarnings = 2,
    ExitErrorsAndWarnings = 3,
    ExitBadUsage = 64,
    ExitConfigFailed = 65,
    ExitCompilationDatabaseFailed = 66,
    ExitAnalysisFailed = 67
};

// -------------------------
// Clang resource directory
// -------------------------
static int mainExecutableAnchor;

// Clang's builtin headers (stddef.h, mm_malloc.h, ...) are looked up in
// its resource directory, which by default is expected next to this
// executable. When it is not there, fall back to the resource directory
// of the clang the tool was built against. Returns the argument to add,
// or an empty string if the default location works or nothing is found.
static std::string findResourceDirArgument(const char *argv0) {
    const std::string defaultDir =
        clang::GetResourcesPath(argv0, &mainExecutableAnchor);

    if (llvm::sys::fs::is_directory(defaultDir + "/include"))
        return "";

#ifdef WORKSHOPC_CLANG_RESOURCE_DIR
    const std::string fallbackDir = WORKSHOPC_CLANG_RESOURCE_DIR;

    if (llvm::sys::fs::is_directory(fallbackDir + "/include"))
        return "-resource-dir=" + fallbackDir;
#endif

    return "";
}

// -------------------------
// Command line
// -------------------------
// Found automatically. Other configs, e.g. the presets or
// 'ci.workshopc.yaml', are given with --config
static const char *kDefaultConfigName = "workshopc.yaml";

static const char *kUsage =
    "Usage: workshopc [options] <files or folders...>\n"
    "\n"
    "Analyzes the given C files, and every .c file found in the given folders.\n"
    "\n"
    "Options:\n"
    "  --config <file>                   The config file. Without it, workshopc.yaml\n"
    "                                    is searched for in the current folder and its parents.\n"
    "  -p, --build-path <folder>         The folder containing compile_commands.json.\n"
    "                                    Overrides compile_commands_dir from the config.\n"
    "  --text <file>                     Also write the diagnostics as text to a file.\n"
    "  --json <file>                     Also write the diagnostics as JSON to a file.\n"
    "  --sarif <file>                    Also write the diagnostics as SARIF to a file.\n"
    "                                    Use - as the file to write to stdout instead.\n"
    "  -q, --quiet                       Print nothing but clang's compile errors and\n"
    "                                    problems that stop the analysis.\n"
    "  -h, --help                        Show this help.\n";

struct Options {
    std::string configPath;
    std::string compileCommandsDir;
    std::string textPath;
    std::string jsonPath;
    std::string sarifPath;
    std::vector<std::string> inputs;
    bool quiet = false;
    bool help = false;
};

// Returns false and prints an error on invalid arguments
static bool parseArguments(int argc, const char **argv, Options &options) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        // --name value, or --name=value
        auto valueOf = [&](const std::string &name, std::string &value) {
            if (arg == name) {
                if (i + 1 >= argc) {
                    std::cerr << "Missing value for " << name << "\n";
                    return -1;
                }

                value = argv[++i];
                return 1;
            }

            if (arg.rfind(name + "=", 0) == 0) {
                value = arg.substr(name.size() + 1);
                return 1;
            }

            return 0;
        };

        int matched = 0;

        if (arg == "-h" || arg == "--help") {
            options.help = true;
            continue;
        }

        if (arg == "-q" || arg == "--quiet") {
            options.quiet = true;
            continue;
        }

        if ((matched = valueOf("--config", options.configPath)) ||
            (matched = valueOf("-p", options.compileCommandsDir)) ||
            (matched = valueOf("--build-path", options.compileCommandsDir)) ||
            (matched = valueOf("--text", options.textPath)) ||
            (matched = valueOf("--json", options.jsonPath)) ||
            (matched = valueOf("--sarif", options.sarifPath)))
        {
            if (matched < 0)
                return false;

            continue;
        }

        if (arg.size() > 1 && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }

        options.inputs.push_back(arg);
    }

    const int toStdout =
        (options.textPath == "-") +
        (options.jsonPath == "-") +
        (options.sarifPath == "-");

    if (toStdout > 1) {
        std::cerr << "Only one of --text, --json and --sarif can write to stdout (-)\n";
        return false;
    }

    return true;
}

// An output file given with --text, --json or --sarif, '-' being stdout
class OutputFile {
private:
    std::string path;
    std::ofstream file;

public:
    // Returns false and prints an error when the file can not be written
    bool open(const std::string &outputPath) {
        path = outputPath;

        if (path.empty() || path == "-")
            return true;

        file.open(path);

        if (!file) {
            std::cerr << "Failed to write " << path << "\n";
            return false;
        }

        return true;
    }

    bool isWanted() const {
        return !path.empty();
    }

    std::ostream &stream() {
        return path == "-" ? std::cout : file;
    }
};

// Looks for the default config file in the current folder and its parents
static std::string findConfigFile() {
    llvm::SmallString<256> dir;

    if (llvm::sys::fs::current_path(dir))
        return "";

    while (!dir.empty()) {
        llvm::SmallString<256> candidate(dir);
        llvm::sys::path::append(candidate, kDefaultConfigName);

        if (llvm::sys::fs::is_regular_file(candidate))
            return std::string(candidate);

        const llvm::StringRef parent = llvm::sys::path::parent_path(dir);

        if (parent == dir)
            break;

        dir = parent;
    }

    return "";
}

static std::string realPathOf(const std::string &path) {
    llvm::SmallString<256> real;

    if (!llvm::sys::fs::real_path(path, real))
        return std::string(real);

    llvm::SmallString<256> absolute(path);
    llvm::sys::fs::make_absolute(absolute);
    return std::string(absolute);
}

static bool isInside(const std::string &path, const std::string &folder) {
    if (folder.empty() || path.size() <= folder.size())
        return false;

    if (path.compare(0, folder.size(), folder) != 0)
        return false;

    const char next = path[folder.size()];
    return next == '/' || next == '\\';
}

/*
 * Expands the inputs into the list of source files to analyze. Folders are
 * searched recursively for .c files, skipping third party folders and the
 * compilation database folder (which contains CMake's own test sources).
 */
static bool collectSources(
    const std::vector<std::string> &inputs,
    const Config &config,
    const std::string &compileCommandsDir,
    std::vector<std::string> &sources)
{
    const std::string skipDir = realPathOf(compileCommandsDir);

    auto isThirdParty = [&](const std::string &path) {
        for (const auto &p : config.thirdPartyIncludes) {
            if (!p.empty() && path.find(p) != std::string::npos)
                return true;
        }

        return false;
    };

    std::set<std::string> unique;

    for (const auto &input : inputs) {
        if (llvm::sys::fs::is_regular_file(input)) {
            const std::string path = realPathOf(input);

            if (unique.insert(path).second)
                sources.push_back(path);

            continue;
        }

        if (!llvm::sys::fs::is_directory(input)) {
            std::cerr << "No such file or folder: " << input << "\n";
            return false;
        }

        std::vector<std::string> found;
        std::error_code ec;

        for (llvm::sys::fs::recursive_directory_iterator it(input, ec), end;
             it != end && !ec;
             it.increment(ec))
        {
            const std::string path = realPathOf(it->path());

            if (llvm::sys::fs::is_directory(it->path())) {
                if (path == skipDir || isThirdParty(path + "/"))
                    it.no_push();

                continue;
            }

            if (llvm::sys::path::extension(path) != ".c" ||
                isThirdParty(path) ||
                isInside(path, skipDir))
                continue;

            found.push_back(path);
        }

        if (ec) {
            std::cerr << "Failed to read folder " << input << ": "
                      << ec.message() << "\n";
            return false;
        }

        std::sort(found.begin(), found.end());

        for (const auto &path : found) {
            if (unique.insert(path).second)
                sources.push_back(path);
        }
    }

    return true;
}

// -------------------------
// MAIN
// -------------------------
int main(int argc, const char **argv) {
    Options options;

    if (!parseArguments(argc, argv, options)) {
        std::cerr << "\n" << kUsage;
        return ExitBadUsage;
    }

    if (options.help) {
        std::cout << kUsage;
        return ExitClean;
    }

    if (options.inputs.empty()) {
        std::cerr << "No files or folders to analyze\n\n" << kUsage;
        return ExitBadUsage;
    }

    // -------------------------
    // Load config
    // -------------------------
    if (options.configPath.empty()) {
        options.configPath = findConfigFile();

        if (options.configPath.empty()) {
            std::cerr << "No " << kDefaultConfigName
                      << " found in the current folder or its parents, "
                         "use --config to point at one\n";
            return ExitConfigFailed;
        }
    }

    Config config;
    if (!ConfigParser::loadFromFile(options.configPath, config)) {
        std::cerr << "Failed to load config: " << options.configPath << "\n";
        return ExitConfigFailed;
    }

    // -------------------------
    // Locate the compilation database: -p, or the config setting which
    // is relative to the config file
    // -------------------------
    std::string compdbDir = options.compileCommandsDir;

    if (compdbDir.empty() && !config.compileCommandsDir.empty()) {
        llvm::SmallString<256> dir(config.compileCommandsDir);

        if (llvm::sys::path::is_relative(dir)) {
            llvm::SmallString<256> base(
                llvm::sys::path::parent_path(realPathOf(options.configPath)));
            llvm::sys::path::append(base, dir);
            dir = base;
        }

        compdbDir = std::string(dir);
    }

    if (compdbDir.empty()) {
        std::cerr << "No compilation database: pass -p/--build-path <folder> or set "
                     "compile_commands_dir in the config\n";
        return ExitCompilationDatabaseFailed;
    }

    std::string errorMsg;
    auto compilationDB =
        CompilationDatabase::loadFromDirectory(compdbDir, errorMsg);

    if (!compilationDB) {
        std::cerr << "Failed to load compilation database: "
                  << errorMsg << "\n";
        return ExitCompilationDatabaseFailed;
    }

    // -------------------------
    // Collect the source files
    // -------------------------
    std::vector<std::string> sources;

    if (!collectSources(options.inputs, config, compdbDir, sources))
        return ExitBadUsage;

    if (sources.empty()) {
        std::cerr << "No .c files found in the given folders\n";
        return ExitBadUsage;
    }

    // -------------------------
    // Run tool
    // -------------------------

    // Opened before the analysis so that a bad path fails right away
    OutputFile textOutput;
    OutputFile jsonOutput;
    OutputFile sarifOutput;

    if (!textOutput.open(options.textPath) ||
        !jsonOutput.open(options.jsonPath) ||
        !sarifOutput.open(options.sarifPath))
        return ExitBadUsage;

    // Diagnostics are printed to the terminal (stderr) as they are found,
    // unless quiet or the text already goes to stdout. The output files
    // are written all at once at the end.
    Diagnostics diagnostics;

    diagnostics.setStreamText(!options.quiet && options.textPath != "-");

    WorkshopActionFactory factory(config, diagnostics);

    const std::string resourceDirArgument = findResourceDirArgument(argv[0]);

    // One file at a time: clang's tooling prints progress lines on stderr
    // when it is given several files, which would mix with the diagnostics
    int result = 0;

    for (const auto &source : sources) {
        ClangTool tool(*compilationDB, {source});

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

        // Make clang's builtin headers findable
        if (!resourceDirArgument.empty()) {
            tool.appendArgumentsAdjuster(
                getInsertArgumentAdjuster(
                    resourceDirArgument.c_str(),
                    ArgumentInsertPosition::END
                )
            );
        }

        // Only WorkshopC's own rules should report warnings. Clang's warnings
        // are dropped (they belong to the project's normal build) but real
        // compile errors are still printed. Added last so it overrides any
        // -W flags and -Werror from the compilation database.
        tool.appendArgumentsAdjuster(
            getInsertArgumentAdjuster(
                {"-w"},
                ArgumentInsertPosition::END
            )
        );

        if (tool.run(&factory) != 0)
            result = 1;
    }

    // -------------------------
    // Output
    // -------------------------
    if (textOutput.isWanted())
        diagnostics.writeText(textOutput.stream());

    if (jsonOutput.isWanted())
        diagnostics.writeJson(jsonOutput.stream());

    if (sarifOutput.isWanted())
        diagnostics.writeSarif(sarifOutput.stream());

    // On stderr, next to the diagnostics, so stdout only carries an output
    // file written to '-'
    if (!options.quiet) {
        std::cerr << "\nWarnings: " << diagnostics.getWarnings() << "\n";
        std::cerr << "Errors: " << diagnostics.getErrors() << "\n";
    }

    if (result != 0)
        return ExitAnalysisFailed;

    int exitCode = ExitClean;

    if (diagnostics.getErrors() > 0)
        exitCode |= ExitErrors;

    if (diagnostics.getWarnings() > 0)
        exitCode |= ExitWarnings;

    return exitCode;
}
