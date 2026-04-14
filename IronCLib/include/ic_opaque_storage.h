#ifndef IC_OPAQUE_STORAGE_H
#define IC_OPAQUE_STORAGE_H

/*
===============================================================================
IC Opaque Storage

C Version Compatibility:
- Works on C89+ (reduced guarantees)
- Full correctness on C11+ or compilers with alignment support

-------------------------------------------------------------------------------
WHAT THIS DOES

Provides a way to:
- Hide struct implementation (encapsulation)
- Allow stack allocation (no malloc required)
- Enforce size and alignment at compile time (ABI safety)

-------------------------------------------------------------------------------
HOW TO USE

--- In your header (.h) ---

    #include "ic_opaque_storage.h"

    #define COLOR_SIZE   (sizeof(int) * 3)
    #define COLOR_ALIGN  (IC_ALIGNOF(int))

    IC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)

    void color_init(Color* c, int r, int g, int b);
    int  color_get_red(const Color* c);


--- In your source (.c) ---

    #include "color.h"

    struct ColorImpl {
        int r, g, b;
    };
    typedef struct ColorImpl ColorImpl;
    
    IC_OPAQUE_IMPL_ASSERT(Color, COLOR_ALIGN, COLOR_SIZE)
    
    void color_init(Color* c, int r, int g, int b) {
        ColorImpl* real = (ColorImpl*)c;
        real->r = r;
        real->g = g;
        real->b = b;
    }
    
    int color_get_red(const Color* c) {
        const ColorImpl* real = (const ColorImpl*)c;
        return real->r;
    }


--- In user code ---

    Color c;                    // stack allocation
    color_init(&c, 1, 2, 3);   // no malloc needed

-------------------------------------------------------------------------------
IMPORTANT RULES

1) ALWAYS call IC_OPAQUE_IMPL_ASSERT in the .c file
2) NEVER expose struct fields in headers
3) alignment must be <= alignof(max_align_t) for portability
4) access data only via implementation (casting internally)

===============================================================================
*/

#include <stddef.h>
#include "ic_static_assert.h"

/* Basic byte type */
typedef unsigned char ic_byte;


/*
===============================================================================
Alignment support
===============================================================================
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define IC_ALIGNAS(x) _Alignas(x)
    #define IC_ALIGNOF(x) _Alignof(x)

#elif defined(_MSC_VER)
    #define IC_ALIGNAS(x) __declspec(align(x))
    #define IC_ALIGNOF(x) __alignof(x)

#elif defined(__GNUC__) || defined(__clang__)
    #define IC_ALIGNAS(x) __attribute__((aligned(x)))
    #define IC_ALIGNOF(x) __alignof__(x)

#else
    #define IC_ALIGNAS(x)
    #define IC_ALIGNOF(x) (offsetof(struct { char c; x member; }, member))
#endif


/*
===============================================================================
Opaque storage definition (for headers)
===============================================================================
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L || \
    defined(_MSC_VER) || defined(__GNUC__)

    #define IC_OPAQUE_STORAGE(name, alignment, size) \
        IC_STATIC_ASSERT((alignment) <= IC_ALIGNOF(max_align_t), alignment_too_large); \
        typedef struct { \
            IC_ALIGNAS(alignment) ic_byte data[(size)]; \
        } name;

#else

    #define IC_OPAQUE_STORAGE(name, alignment, size) \
        IC_STATIC_ASSERT((alignment) <= sizeof(max_align_t), alignment_too_large); \
        typedef union { \
            ic_byte data[(size)]; \
            max_align_t _align; \
        } name;

#endif


/*
===============================================================================
Definition check (for .c files)
===============================================================================
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

    #define IC_OPAQUE_IMPL_ASSERT(name, alignment, size) \
        IC_STATIC_ASSERT(sizeof(name) == (size), "size mismatch"); \
        IC_STATIC_ASSERT(IC_ALIGNOF(name) == (alignment), "alignment mismatch");

#else

    #define IC_OPAQUE_IMPL_ASSERT(name, alignment, size) \
        IC_STATIC_ASSERT(sizeof(name) == (size), "size mismatch"); \

#endif

#endif