#pragma once

#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>
#include <unordered_map>

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
    );

public:
    void parseFile(const std::string &path);

    bool isSuppressed(
        const clang::SourceManager &sm,
        clang::SourceLocation loc
    );
};
