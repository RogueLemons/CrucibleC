#include "suppression_manager.hpp"

#include <fstream>

bool SuppressionManager::contains(
    const std::string &line,
    const std::string &text
) {
    return line.find(text) != std::string::npos;
}

void SuppressionManager::parseFile(const std::string &path) {
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

bool SuppressionManager::isReasonComment(const std::string &line) {
    const size_t first = line.find_first_not_of(" \t");

    if (first == std::string::npos)
        return false;

    if (line.compare(first, 2, "//") != 0 &&
        line.compare(first, 2, "/*") != 0)
        return false;

    const size_t textStart = line.find_first_not_of(" \t", first + 2);

    if (textStart == std::string::npos)
        return false;

    const std::string prefix = "Reason: ";

    if (line.compare(textStart, prefix.size(), prefix) != 0)
        return false;

    const size_t reasonStart = textStart + prefix.size();
    size_t reasonEnd = line.size();

    // A block comment's closing marker is not part of the reason.
    const size_t blockEnd = line.find("*/", reasonStart);

    if (blockEnd != std::string::npos)
        reasonEnd = blockEnd;

    for (size_t i = reasonStart; i < reasonEnd; ++i) {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r')
            return true;
    }

    return false;
}

std::vector<MissingSuppressionReason> SuppressionManager::findMissingReasons(
    const std::string &path
) {
    std::vector<MissingSuppressionReason> missing;

    std::ifstream file(path);

    if (!file.is_open())
        return missing;

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line))
        lines.push_back(line);

    bool disabled = false;

    for (size_t i = 0; i < lines.size(); ++i) {
        if (contains(lines[i], "WorkshopC off")) {
            // Only the comment that actually opens a suppressed range
            // needs a reason, a repeated "off" inside it is redundant.
            if (!disabled) {
                disabled = true;

                const bool hasReason =
                    i + 1 < lines.size() &&
                    isReasonComment(lines[i + 1]);

                if (!hasReason) {
                    missing.push_back({
                        static_cast<unsigned>(i + 1),
                        static_cast<unsigned>(
                            lines[i].find("WorkshopC off") + 1)
                    });
                }
            }
        }

        if (contains(lines[i], "WorkshopC on"))
            disabled = false;
    }

    return missing;
}

bool SuppressionManager::isSuppressed(
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
