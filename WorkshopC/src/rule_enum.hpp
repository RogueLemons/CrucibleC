#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include <iostream>

#include "config.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class EnumRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    int &warnings;
    int &errors;

    bool isThirdParty(const std::string &path) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (path.find(p) != std::string::npos)
                return true;
        }
        return false;
    }

public:
    EnumRule(const RuleConfig &cfg,
             const Config &gc,
             int &w,
             int &e)
        : config(cfg),
          globalConfig(gc),
          warnings(w),
          errors(e) {}

    void setSourceManager(SourceManager*) {}

    void run(const MatchFinder::MatchResult &result) override {
        const auto *e = result.Nodes.getNodeAs<EnumDecl>("enum");
        if (!e) return;

        if (config.level == RuleLevel::Off)
            return;

        auto &sm = *result.SourceManager;
        auto loc = e->getLocation();

        std::string path = sm.getFilename(loc).str();

        if (path.empty())
            return;

        // 🚫 ignore third-party code
        if (isThirdParty(path))
            return;

        std::string name = e->getNameAsString();
        if (name.empty())
            name = "<anonymous>";

        std::string msg = "enum '" + name + "' is not allowed";

        if (config.level == RuleLevel::Warning) {
            std::cerr << path << ": warning: " << msg << "\n";
            warnings++;
        } else {
            std::cerr << path << ": error: " << msg << "\n";
            errors++;
        }
    }
};