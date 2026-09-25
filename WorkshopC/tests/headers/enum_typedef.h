#ifndef TESTS_HEADERS_ENUM_TYPEDEF_H
#define TESTS_HEADERS_ENUM_TYPEDEF_H

enum HeaderBadEnum // bad: no typedef
{
    HEADER_BAD_A,
    HEADER_BAD_B
};

typedef enum // good
{
    HEADER_GOOD_A,
    HEADER_GOOD_B
} HeaderGoodEnum;

#endif // TESTS_HEADERS_ENUM_TYPEDEF_H
