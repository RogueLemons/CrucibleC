#pragma once

#include <clang/Basic/SourceManager.h>

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
    );

    void report(
        RuleLevel level,
        const std::string &file,
        unsigned line,
        unsigned column,
        const std::string &message
    );
};
