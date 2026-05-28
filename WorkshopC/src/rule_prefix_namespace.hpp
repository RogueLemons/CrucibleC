#pragma once

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "config.hpp"
#include "diagnostics.hpp"
#include "suppression_manager.hpp"

using namespace clang;
using namespace clang::ast_matchers;

class PrefixNamespaceRule : public MatchFinder::MatchCallback {
private:
    RuleConfig config;
    const Config &globalConfig;

    SuppressionManager &suppressions;
    Diagnostics &diagnostics;

private:
    bool opt(const std::string &key) const {
        auto it = config.options.find(key);

        return it != config.options.end() &&
               it->second == "true";
    }

    std::string strOpt(const std::string &key,
                       const std::string &fallback = "") const {
        auto it = config.options.find(key);

        return it == config.options.end()
            ? fallback
            : it->second;
    }

    int intOpt(const std::string &key,
               int fallback) const {
        auto it = config.options.find(key);

        if (it == config.options.end())
            return fallback;

        try {
            return std::stoi(it->second);
        }
        catch (...) {
            return fallback;
        }
    }

    bool isThirdParty(const std::string &path) const {
        for (const auto &p : globalConfig.getThirdPartyIncludes()) {
            if (path.find(p) != std::string::npos)
                return true;
        }

        return false;
    }

    bool isHeaderFile(const std::string &path) const {
        std::string lower = path;

        std::transform(
            lower.begin(),
            lower.end(),
            lower.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

        return lower.ends_with(".h")   ||
               lower.ends_with(".hpp") ||
               lower.ends_with(".hh")  ||
               lower.ends_with(".hxx");
    }

    std::string sanitize(const std::string &s) const {
        std::string out;

        for (char c : s) {

            if (std::isalnum(static_cast<unsigned char>(c))) {
                out += static_cast<char>(
                    std::tolower(static_cast<unsigned char>(c)));
            }
            else {
                out += '_';
            }
        }

        return out;
    }

    std::string upper(const std::string &s) const {
        std::string out = s;

        std::transform(
            out.begin(),
            out.end(),
            out.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });

        return out;
    }

    std::vector<std::string> splitPath(
        const std::string &path) const {

        std::vector<std::string> out;

        std::filesystem::path p(path);

        for (const auto &part : p) {

            std::string s = part.string();

            if (!s.empty())
                out.push_back(s);
        }

        return out;
    }

    // ------------------------------------------------------------
    // Extract dirs after top_dir and before filename
    // ------------------------------------------------------------
    std::vector<std::string> extractDirs(
        const std::string &path) const {

        const std::string topDir =
            sanitize(strOpt("top_dir", "src"));

        std::vector<std::string> parts =
            splitPath(path);

        std::vector<std::string> dirs;

        bool collecting = topDir.empty();

        for (const std::string &raw : parts) {

            std::string part = sanitize(raw);

            if (part.empty())
                continue;

            if (!collecting) {

                if (part == topDir)
                    collecting = true;

                continue;
            }

            // skip filename
            if (raw.find('.') != std::string::npos)
                break;

            dirs.push_back(part);
        }

        return dirs;
    }

    // ------------------------------------------------------------
    // build namespace prefix
    //
    // work_from_top = true
    //   a/b/c -> a__b__
    //
    // work_from_top = false
    //   a/b/c -> b__c__
    // ------------------------------------------------------------
    std::string buildPrefix(
        const std::string &path) const {

        const bool workFromTop =
            opt("work_from_top");

        const int stopAtCount =
            intOpt("stop_at_count", 10);

        const bool useSeparator =
            opt("use_seperator");

        const std::string separator =
            useSeparator
                ? strOpt("seperator", "_")
                : "";

        std::vector<std::string> dirs =
            extractDirs(path);

        if (dirs.empty())
            return "";

        std::vector<std::string> selected;

        if (workFromTop) {

            for (size_t i = 0;
                 i < dirs.size() &&
                 (int)i < stopAtCount;
                 ++i)
            {
                selected.push_back(dirs[i]);
            }
        }
        else {

            int begin =
                std::max(
                    0,
                    (int)dirs.size() - stopAtCount);

            for (size_t i = begin;
                 i < dirs.size();
                 ++i)
            {
                selected.push_back(dirs[i]);
            }
        }

        if (selected.empty())
            return "";

        std::ostringstream oss;

        for (size_t i = 0; i < selected.size(); ++i) {

            oss << selected[i];

            if (i + 1 < selected.size())
                oss << separator;
        }

        oss << separator;

        return oss.str();
    }

    // ------------------------------------------------------------
    // build include guard
    //
    // Uses FULL path after top_dir
    //
    // Example:
    //
    // WorkshopC/tests/headers/a/b/c/header.h
    //
    // =>
    //
    // TESTS_HEADERS_A_B_C_HEADER_H
    // ------------------------------------------------------------
    std::string buildIncludeGuardName(
        const std::string &path) const {

        const std::string topDir =
            sanitize(strOpt("top_dir", "src"));

        std::vector<std::string> parts =
            splitPath(path);

        std::vector<std::string> collected;

        bool collecting = topDir.empty();

        for (const std::string &raw : parts) {

            std::string part =
                sanitize(raw);

            if (part.empty())
                continue;

            if (!collecting) {

                if (part == topDir)
                    collecting = true;

                continue;
            }

            collected.push_back(
                upper(part));
        }

        if (collected.empty())
            return "";

        std::ostringstream oss;

        for (size_t i = 0;
             i < collected.size();
             ++i)
        {
            oss << collected[i];

            if (i + 1 < collected.size())
                oss << "_";
        }

        return oss.str();
    }

    bool shouldCheckPath(
        const std::string &path) const {

        if (path.empty())
            return false;

        if (isThirdParty(path))
            return false;

        if (!isHeaderFile(path))
            return false;

        return true;
    }

    void checkName(SourceManager &sm,
                   SourceLocation loc,
                   const std::string &path,
                   const std::string &kind,
                   const std::string &name) {

        std::string prefix =
            buildPrefix(path);

        if (prefix.empty())
            return;

        if (name.starts_with(prefix))
            return;

        diagnostics.report(
            config.level,
            sm,
            loc,
            kind + " '" + name +
            "' must use namespace prefix '" +
            prefix + "'"
        );
    }

    void checkIncludeGuard(SourceManager &sm,
                           SourceLocation loc,
                           const std::string &path) {

        if (!opt("require_ifndef_for_filepath"))
            return;

        std::ifstream file(path);

        if (!file.is_open())
            return;

        std::string expected =
            buildIncludeGuardName(path);

        if (expected.empty())
            return;

        std::string line;
        int lineNo = 0;

        while (std::getline(file, line)) {

            ++lineNo;

            if (lineNo > 10)
                break;

            size_t start =
                line.find_first_not_of(" \t");

            if (start == std::string::npos)
                continue;

            line = line.substr(start);

            if (!line.starts_with("#ifndef"))
                continue;

            std::string actual =
                line.substr(7);

            start =
                actual.find_first_not_of(" \t");

            if (start == std::string::npos)
                break;

            actual =
                actual.substr(start);

            size_t end =
                actual.find_last_not_of(" \t\r\n");

            if (end != std::string::npos)
                actual =
                    actual.substr(0, end + 1);

            if (actual != expected) {

                diagnostics.report(
                    config.level,
                    sm,
                    loc,
                    "missing include guard '" +
                    expected + "'"
                );
            }

            return;
        }

        diagnostics.report(
            config.level,
            sm,
            loc,
            "missing include guard '" +
            expected + "'"
        );
    }

    void checkFileIncludeGuard(SourceManager &sm,
                               SourceLocation loc,
                               const std::string &path) {

        static std::unordered_set<std::string> checked;

        if (!checked.insert(path).second)
            return;

        checkIncludeGuard(sm, loc, path);
    }

public:
    PrefixNamespaceRule(
        const RuleConfig &cfg,
        const Config &gc,
        SuppressionManager &sup,
        Diagnostics &diag
    )
        : config(cfg),
          globalConfig(gc),
          suppressions(sup),
          diagnostics(diag)
    {}

    void run(
        const MatchFinder::MatchResult &result) override {

        if (config.level == RuleLevel::Off)
            return;

        SourceManager &sm =
            *result.SourceManager;

        // =====================================================
        // FUNCTIONS
        // =====================================================

        if (opt("apply_to_functions")) {

            if (const auto *fd =
                    result.Nodes.getNodeAs<FunctionDecl>(
                        "function"))
            {
                if (fd->isImplicit())
                    return;

                SourceLocation loc =
                    fd->getLocation();

                SourceLocation expLoc =
                    sm.getExpansionLoc(loc);

                if (suppressions.isSuppressed(sm, expLoc))
                    return;

                std::string path =
                    sm.getFilename(
                        sm.getSpellingLoc(loc)).str();

                if (!shouldCheckPath(path))
                    return;

                checkFileIncludeGuard(
                    sm,
                    expLoc,
                    path);

                if (fd->getStorageClass() == SC_Static)
                    return;

                checkName(
                    sm,
                    expLoc,
                    path,
                    "function",
                    fd->getNameAsString()
                );
            }
        }

        // =====================================================
        // STRUCTS
        // =====================================================

        if (opt("apply_to_structs")) {

            if (const auto *rd =
                    result.Nodes.getNodeAs<RecordDecl>(
                        "record"))
            {
                if (!rd->isStruct() ||
                    rd->isImplicit())
                {
                    return;
                }

                SourceLocation loc =
                    rd->getLocation();

                SourceLocation expLoc =
                    sm.getExpansionLoc(loc);

                if (suppressions.isSuppressed(sm, expLoc))
                    return;

                std::string path =
                    sm.getFilename(
                        sm.getSpellingLoc(loc)).str();

                if (!shouldCheckPath(path))
                    return;

                checkFileIncludeGuard(
                    sm,
                    expLoc,
                    path);

                checkName(
                    sm,
                    expLoc,
                    path,
                    "struct",
                    rd->getNameAsString()
                );
            }
        }

        // =====================================================
        // TYPEDEFS
        // =====================================================

        if (opt("apply_to_typedefs")) {

            if (const auto *td =
                    result.Nodes.getNodeAs<TypedefDecl>(
                        "typedef"))
            {
                SourceLocation loc =
                    td->getLocation();

                SourceLocation expLoc =
                    sm.getExpansionLoc(loc);

                if (suppressions.isSuppressed(sm, expLoc))
                    return;

                std::string path =
                    sm.getFilename(
                        sm.getSpellingLoc(loc)).str();

                if (!shouldCheckPath(path))
                    return;

                checkFileIncludeGuard(
                    sm,
                    expLoc,
                    path);

                checkName(
                    sm,
                    expLoc,
                    path,
                    "typedef",
                    td->getNameAsString()
                );
            }
        }
    }
};