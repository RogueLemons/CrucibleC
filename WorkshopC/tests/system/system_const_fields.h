#ifndef TESTS_SYSTEM_SYSTEM_CONST_FIELDS_H
#define TESTS_SYSTEM_SYSTEM_CONST_FIELDS_H

// Clang treats the rest of this file as a system header, the way it
// treats the standard library, portably on every platform
#pragma GCC system_header

struct system_settings
{
    const int version; // not reported: a system header
    int *const buffer; // not reported: a system header
};

#endif // TESTS_SYSTEM_SYSTEM_CONST_FIELDS_H
