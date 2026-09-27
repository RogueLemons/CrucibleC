#pragma once

#include <clang/Basic/SourceManager.h>

#include <ostream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "config.hpp"
#include "diagnostic_codes.hpp"

struct DiagnosticRecord {
    std::string file;
    unsigned line = 0;
    unsigned column = 0;
    RuleLevel level = RuleLevel::Warning;
    DiagCode code = DiagCode::SuppressionNotTurnedBackOn;
    std::string message;
};

/*
 * Collects the diagnostics of every analyzed file. A diagnostic reported
 * more than once, e.g. for a header included by several analyzed files,
 * is only kept once.
 */
class Diagnostics {
private:
    // Print each diagnostic as text to stderr as soon as it is reported
    bool streamText = true;

    int warnings = 0;
    int errors = 0;

    std::vector<DiagnosticRecord> records;

    std::set<std::tuple<std::string, unsigned, unsigned, int, int, std::string>> seen;

public:
    void setStreamText(bool enabled);

    void report(
        RuleLevel level,
        DiagCode code,
        const clang::SourceManager &sm,
        clang::SourceLocation loc,
        const std::string &message
    );

    void report(
        RuleLevel level,
        DiagCode code,
        const std::string &file,
        unsigned line,
        unsigned column,
        const std::string &message
    );

    int getWarnings() const;
    int getErrors() const;

    void writeText(std::ostream &out) const;
    void writeJson(std::ostream &out) const;
    void writeSarif(std::ostream &out) const;
};
