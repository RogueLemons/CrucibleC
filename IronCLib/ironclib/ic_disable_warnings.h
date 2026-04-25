#ifndef IC_DISABLE_WARNINGS_H
#define IC_DISABLE_WARNINGS_H

#if defined(_MSC_VER)
// MSVC
#define IC_DISABLE_WARNINGS \
    __pragma(warning(push)) \
    __pragma(warning(disable: 4127))  /* constant conditional */ \
    __pragma(warning(disable: 4100))  /* unused parameter */ \
    __pragma(warning(disable: 4189))  /* unused variable */ \
    __pragma(warning(disable: 4056))  /* FP overflow */
#define IC_ENABLE_WARNINGS \
    __pragma(warning(pop));

// Clang
#elif defined(__clang__)
#define IC_DISABLE_WARNINGS \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Weverything\"")
#define IC_ENABLE_WARNINGS \
    _Pragma("clang diagnostic pop")
    
// GCC
#elif defined(__GNUC__)
#define IC_DISABLE_WARNINGS \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wtype-limits\"") \
    _Pragma("GCC diagnostic ignored \"-Wunused\"") \
    _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")
#define IC_ENABLE_WARNINGS \
    _Pragma("GCC diagnostic pop")
    
// Unknown compiler: do nothing
#else
#define IC_DISABLE_WARNINGS
#define IC_ENABLE_WARNINGS

#endif

#endif // IC_DISABLE_WARNINGS_H