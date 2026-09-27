#pragma once

#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>
#include <unordered_map>

#include "diagnostic_codes.hpp"

struct SuppressedRange {
    unsigned startLine;
    unsigned endLine;
};

struct MissingSuppressionReason {
    unsigned line;
    unsigned column;
};

struct UnbalancedSuppression {
    DiagCode code;
    unsigned line;
    unsigned column;
    std::string message;
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

    /*
     * Finds every 'WorkshopC off' that is never turned back on in the
     * same file, every 'WorkshopC on' without a preceding 'WorkshopC
     * off', and every 'WorkshopC off' while already turned off.
     */
    std::vector<UnbalancedSuppression> findUnbalanced(
        const std::string &path
    );

    /*
     * Every file that took part in the translation unit, except system
     * headers and files in the given third party folders.
     */
    static std::vector<std::string> projectFiles(
        const clang::SourceManager &sm,
        const std::vector<std::string> &thirdPartyIncludes
    );

    bool isSuppressed(
        const clang::SourceManager &sm,
        clang::SourceLocation loc
    );
};
