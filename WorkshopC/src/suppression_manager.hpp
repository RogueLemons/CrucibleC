#pragma once

#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>

struct SuppressedRange {
    unsigned startLine;
    unsigned endLine;
};

class SuppressionManager {
private:
    std::unordered_map<
        std::string,
        std::vector<SuppressedRange>
    > suppressedRanges;

private:
    static bool contains(
        const std::string &line,
        const std::string &text
    ) {
        return line.find(text) != std::string::npos;
    }

public:
    void parseFile(const std::string &path) {
        if (suppressedRanges.contains(path))
            return;

        std::ifstream file(path);

        if (!file.is_open())
            return;

        std::vector<SuppressedRange> ranges;

        std::string line;

        bool disabled = false;
        unsigned startLine = 0;
        unsigned currentLine = 0;

        while (std::getline(file, line)) {
            currentLine++;

            if (contains(line, "WorkshopC off")) {
                if (!disabled) {
                    disabled = true;
                    startLine = currentLine;
                }
            }

            if (contains(line, "WorkshopC on")) {
                if (disabled) {
                    ranges.push_back({
                        startLine,
                        currentLine
                    });

                    disabled = false;
                }
            }
        }

        // unfinished block until EOF
        if (disabled) {
            ranges.push_back({
                startLine,
                currentLine
            });
        }

        suppressedRanges[path] = ranges;
    }

    bool isSuppressed(
        const clang::SourceManager &sm,
        clang::SourceLocation loc
    ) {
        if (loc.isInvalid())
            return false;

        clang::SourceLocation expansionLoc =
            sm.getExpansionLoc(loc);

        std::string path =
            sm.getFilename(expansionLoc).str();

        if (path.empty())
            return false;

        parseFile(path);

        unsigned line =
            sm.getSpellingLineNumber(expansionLoc);

        auto it = suppressedRanges.find(path);

        if (it == suppressedRanges.end())
            return false;

        for (const auto &range : it->second) {
            if (line >= range.startLine &&
                line <= range.endLine)
            {
                return true;
            }
        }

        return false;
    }
};