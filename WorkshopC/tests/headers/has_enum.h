#ifndef TESTS_DETECT_HAS_ENUM_H
#define TESTS_DETECT_HAS_ENUM_H

enum detected_enum 
{
    X,
    Y, 
    Z
};

#define DEFINE_SOME_DETECTED_ENUM_VALUE enum macro_detected \
{ \
    I, \
    J, \
    K \
};

#endif // TESTS_DETECT_HAS_ENUM_H