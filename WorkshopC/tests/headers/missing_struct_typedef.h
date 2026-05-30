#ifndef TESTS_HEADERS_MISSING_STRUCT_TYPEDEF_H
#define TESTS_HEADERS_MISSING_STRUCT_TYPEDEF_H

struct BadInHeader
{
    int x;
};

#define DECLARE_BAD_STRUCT_FROM_HEADER(name) \
    struct name {                \
        int value;               \
    }

DECLARE_BAD_STRUCT_FROM_HEADER(BadStructInsideHeader);

#endif // TESTS_HEADERS_MISSING_STRUCT_TYPEDEF_H