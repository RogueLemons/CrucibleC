#pragma once

#include <clang/Basic/SourceManager.h>

#include <iostream>
#include <string>

#include "config.hpp"

class Diagnostics {
private:
    int &warnings;
    int &errors;

public:
    Diagnostics(int &w, int &e)
        : warnings(w), errors(e) {}

    void report(
        RuleLevel level,
        const clang::SourceManager &sm,
        clang::SourceLocation loc,
        const std::string &message
    ) {
        if (level == RuleLevel::Off)
            return;

        if (loc.isInvalid())
            return;

        clang::PresumedLoc presumed =
            sm.getPresumedLoc(loc);

        if (presumed.isInvalid())
            return;

        const char *levelStr =
            (level == RuleLevel::Warning)
                ? "warning"
                : "error";

        std::cerr
            << presumed.getFilename()
            << ":"
            << presumed.getLine()
            << ":"
            << presumed.getColumn()
            << ":\t"
            << levelStr
            << ": "
            << message
            << "\n";

        if (level == RuleLevel::Warning)
            warnings++;
        else if (level == RuleLevel::Error)
            errors++;
    }
};