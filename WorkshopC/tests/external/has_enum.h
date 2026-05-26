#ifndef TESTS_NOT_DETECT_HAS_ENUM_H
#define TESTS_NOT_DETECT_HAS_ENUM_H

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

#endif // TESTS_NOT_DETECT_HAS_ENUM_H