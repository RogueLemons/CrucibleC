#ifndef IC_INLINE_H
#define IC_INLINE_H

/*
===============================================================================
IC Inline Abstraction Layer

C Compatibility:
- C89+ fallback supported
- C99+ inline supported
- MSVC / GCC / Clang supported

Provides:
    IC_INLINE        -> best-effort inline hint
    IC_HEADER_FUNC   -> always safe in headers (never causes linker issues)

===============================================================================
*/

/*
-------------------------------------------------------------------------------
IC_INLINE

Purpose:
- Express intent: "this function should be inlined if possible"
- Does NOT guarantee inlining
- Does NOT guarantee linkage safety in headers

Use for:
- source files (.c)
- optional performance hints
-------------------------------------------------------------------------------
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define IC_INLINE inline
#elif defined(_MSC_VER)
    #define IC_INLINE __inline
#elif defined(__GNUC__) || defined(__clang__)
    #define IC_INLINE __inline__
#else
    #define IC_INLINE
#endif


/*
-------------------------------------------------------------------------------
IC_HEADER_FUNC

Purpose:
- SAFE for header-defined functions
- Prevents multiple definition / linker errors
- Guarantees internal linkage

Rule:
- Always safe to put in headers
- Compiler may inline if it wants

Preferred for:
- small getters
- to_string functions
- tiny utility helpers
-------------------------------------------------------------------------------
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define IC_HEADER_FUNC static inline
#elif defined(_MSC_VER)
    #define IC_HEADER_FUNC static __inline
#elif defined(__GNUC__) || defined(__clang__)
    #define IC_HEADER_FUNC static __inline__
#else
    #define IC_HEADER_FUNC static
#endif

#endif