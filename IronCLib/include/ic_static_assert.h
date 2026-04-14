#ifndef IC_STATIC_ASSERT_H
#define IC_STATIC_ASSERT_H

/*
===============================================================================
EC Static Assert

C Version Compatibility:
- C11+: uses _Static_assert
- MSVC: uses static_assert
- Pre-C11: fallback typedef trick

Usage:
    IC_STATIC_ASSERT(sizeof(int) == 4, "int must be 4 bytes");

For compilers without native static assert support, this will cause a compile-time error if the condition is false, but the error message may be less clear.
===============================================================================
*/

// glue helpers
#define IC_INTERNAL_SA_GLUE(a,b) a##b
#define IC_INTERNAL_SA_XGLUE(a,b) IC_INTERNAL_SA_GLUE(a,b)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

    #define IC_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

#elif defined(_MSC_VER) && _MSC_VER >= 1600

    #define IC_STATIC_ASSERT(cond, msg) static_assert(cond, msg)

#else

    #if defined(__COUNTER__)
        #define IC_INTERNAL_SA_UNIQUE_ID __COUNTER__
    #else
        #define IC_INTERNAL_SA_UNIQUE_ID __LINE__
    #endif

    #define IC_STATIC_ASSERT(cond, msg) \
        typedef char IC_INTERNAL_SA_XGLUE(static_assert_failed_, IC_INTERNAL_SA_UNIQUE_ID) \
        [(cond) ? 1 : -1]

#endif

#endif