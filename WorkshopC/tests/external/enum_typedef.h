#ifndef TESTS_EXTERNAL_ENUM_TYPEDEF_H
#define TESTS_EXTERNAL_ENUM_TYPEDEF_H

enum ExternalBadEnum // good: third party enums are not checked
{
    EXTERNAL_BAD_A,
    EXTERNAL_BAD_B
};

typedef enum // good
{
    EXTERNAL_A,
    EXTERNAL_B
} ExternalEnum;

#endif // TESTS_EXTERNAL_ENUM_TYPEDEF_H
