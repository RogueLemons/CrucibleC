#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Attr.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class ArgumentPointerMovementRule : public MatchFinder::MatchCallback {
private:
    const Config &config;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

    static constexpr const char* kMoveTag = "workshopc_move";
    static constexpr const char* kOutTag  = "workshopc_out";
    static constexpr const char* kModTag  = "workshopc_modify";

private:

    struct FunctionState {
        std::vector<std::string> declTags;
        bool hasDecl = false;
    };

    std::unordered_map<std::string, FunctionState> functions;

private:

    bool isNonConstPointer(QualType qt) const;

    bool isThirdParty(const std::string &path) const;

    std::string getTag(const ParmVarDecl *param) const;

    std::string makeKey(const FunctionDecl *FD) const;

    void report(const std::string &msg,
                const ParmVarDecl *P,
                const SourceManager &sm,
                SourceLocation loc);

    // =====================================================
    // NEW: EXCLUDE INTERNAL WORKSHOPC HELPERS
    // =====================================================
    bool isWorkshopCInternal(const FunctionDecl *FD) const;

public:
    ArgumentPointerMovementRule(
        const Config &cfg,
        SuppressionManager &sup,
        Diagnostics &diag);

    void bindFinder(MatchFinder &finder);

    void run(const MatchFinder::MatchResult &result) override;
};
