#ifndef TESTS_EXTERNAL_HAS_ENUM_H
#define TESTS_EXTERNAL_HAS_ENUM_H

enum undetected_enum 
{
    A,
    B, 
    C
};

#define DEFINE_SOME_UNDETECTED_ENUM_VALUE enum macro_undetected \
{ \
    D, \
    E, \
    F \
};

#endif // TESTS_EXTERNAL_HAS_ENUM_H