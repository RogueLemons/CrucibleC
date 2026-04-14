#ifndef EC_OPAQUE_STORAGE_H
#define EC_OPAQUE_STORAGE_H

/*
===============================================================================
EC Opaque Storage

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

    #include "ec_opaque_storage.h"

    #define COLOR_SIZE   (sizeof(int) * 3)
    #define COLOR_ALIGN  (EC_ALIGNOF(int))

    EC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)

    void color_init(Color* c, int r, int g, int b);
    int  color_get_red(const Color* c);


--- In your source (.c) ---

    #include "color.h"

    struct Color {
        int r, g, b;
    };

    EC_OPAQUE_DEFINE(Color, COLOR_ALIGN, COLOR_SIZE)

    void color_init(Color* c, int r, int g, int b) {
        struct Color* real = (struct Color*)c;
        real->r = r;
        real->g = g;
        real->b = b;
    }

    int color_get_red(const Color* c) {
        const struct Color* real = (const struct Color*)c;
        return real->r;
    }


--- In user code ---

    Color c;                    // stack allocation
    color_init(&c, 1, 2, 3);   // no malloc needed

-------------------------------------------------------------------------------
IMPORTANT RULES

1) ALWAYS call EC_OPAQUE_DEFINE in the .c file
2) NEVER expose struct fields in headers
3) alignment must be <= alignof(max_align_t) for portability
4) access data only via implementation (casting internally)

===============================================================================
*/

#include <stddef.h>
#include "ec_static_assert.h"

/* Basic byte type */
typedef unsigned char ec_byte;


/*
===============================================================================
Alignment support
===============================================================================
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define EC_ALIGNAS(x) _Alignas(x)
    #define EC_ALIGNOF(x) _Alignof(x)

#elif defined(_MSC_VER)
    #define EC_ALIGNAS(x) __declspec(align(x))
    #define EC_ALIGNOF(x) __alignof(x)

#elif defined(__GNUC__) || defined(__clang__)
    #define EC_ALIGNAS(x) __attribute__((aligned(x)))
    #define EC_ALIGNOF(x) __alignof__(x)

#else
    #define EC_ALIGNAS(x)
    #define EC_ALIGNOF(x) (offsetof(struct { char c; x member; }, member))
#endif


/*
===============================================================================
Opaque storage definition (for headers)
===============================================================================
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L || \
    defined(_MSC_VER) || defined(__GNUC__)

    #define EC_OPAQUE_STORAGE(name, alignment, size) \
        EC_STATIC_ASSERT((alignment) <= EC_ALIGNOF(max_align_t), alignment_too_large); \
        typedef struct { \
            EC_ALIGNAS(alignment) ec_byte data[(size)]; \
        } name;

#else

    #define EC_OPAQUE_STORAGE(name, alignment, size) \
        EC_STATIC_ASSERT((alignment) <= sizeof(max_align_t), alignment_too_large); \
        typedef union { \
            ec_byte data[(size)]; \
            max_align_t _align; \
        } name;

#endif


/*
===============================================================================
Definition check (for .c files)
===============================================================================
*/

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

    #define EC_OPAQUE_DEFINE(name, alignment, size) \
        EC_STATIC_ASSERT(sizeof(struct name) == (size), size_mismatch); \
        EC_STATIC_ASSERT(EC_ALIGNOF(struct name) == (alignment), alignment_mismatch);

#else

    #define EC_OPAQUE_DEFINE(name, alignment, size) \
        EC_STATIC_ASSERT(sizeof(struct name) == (size), size_mismatch);

#endif

#endif