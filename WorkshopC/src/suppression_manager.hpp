#pragma once

#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>
#include <unordered_map>

struct SuppressedRange {
    unsigned startLine;
    unsigned endLine;
};

struct MissingSuppressionReason {
    unsigned line;
    unsigned column;
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

    /*
     * True if 'line' is a comment of the form "// Reason: <text>"
     * (or the block comment equivalent) with a non-empty reason.
     */
    static bool isReasonComment(const std::string &line);

public:
    void parseFile(const std::string &path);

    /*
     * Finds every "WorkshopC off" comment in the file that is not
     * immediately followed, on the next line, by a comment starting
     * with "Reason: ".
     */
    std::vector<MissingSuppressionReason> findMissingReasons(
        const std::string &path
    );

    bool isSuppressed(
        const clang::SourceManager &sm,
        clang::SourceLocation loc
    );
};
