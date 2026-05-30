#ifndef TESTS_EXTERNAL_MISSING_STRUCT_TYPEDEF_H
#define TESTS_EXTERNAL_MISSING_STRUCT_TYPEDEF_H

struct BadInExternalHeader
{
    int x;
};

#define DECLARE_BAD_STRUCT_FROM_EXTERNAL_HEADER(name) \
    struct name {                \
        int value;               \
    }

DECLARE_BAD_STRUCT_FROM_EXTERNAL_HEADER(BadStructInsideExternalHeader);

#endif // TESTS_EXTERNAL_MISSING_STRUCT_TYPEDEF_H