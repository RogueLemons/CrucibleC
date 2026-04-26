/*
===============================================================================
IC CAST SYSTEM
===============================================================================

This header provides a macro-based, overflow-safe numeric casting system
for integer and floating-point conversions up to 64-bit types.

-------------------------------------------------------------------------------
CONFIGURATION (USER OVERRIDABLE)

The following macros may be defined by the user BEFORE including this header:

1. IC_NUM_BITS_PER_BYTE
   - Default: 8
   - Purpose: Defines the number of bits per byte on the target platform.
   - Used for static validation of type size assumptions.

2. IC_CAST_ASSERT_FUNC(expr)
   - Default: undefined, making cast functions clamp casts instead of asserting.
   - Called when a conversion is outside safe bounds before casting.

Example:
    #define IC_CAST_ASSERT_FUNC(expr) assert(expr)

-------------------------------------------------------------------------------
SAFETY MODEL

- All conversions are guaranteed to avoid undefined behavior (UB)
  when used through generated cast functions.
- Overflow and underflow are never permitted in intermediate operations.
- Precision loss and rounding are allowed where required.
- Floating-point inputs safely handle NaN and infinity values.
- All numeric conversions are constrained to types up to 64-bit width.

-------------------------------------------------------------------------------
BASIC USAGE

1. Define your type conversion matrix with the required parameters (type, mold, min, max) - > (type, mold, min, max).
   Mold types: IC_MOLD_SIGNED_INT, IC_MOLD_UNSIGNED_INT, IC_MOLD_FLOAT. 
   Types must be full symbolic values.
   Example:

    #define MY_MATRIX(X) \
        X(int32_t,  SIGNED_INT,   INT32_MIN, INT32_MAX,  uint32_t, UNSIGNED_INT, 0,         UINT32_MAX) \
        X(uint32_t, UNSIGNED_INT, 0,         UINT32_MAX, int32_t,  SIGNED_INT,   INT32_MIN, INT32_MAX)

2. Generate cast functions:

    IC_CASTING_FUNCTIONS(MY_MATRIX)

3. Use generated functions:

    int32_t a = cast_uint32_t_to_int32_t(123u);
    uint32_t b = cast_int32_t_to_uint32_t(-5);

    float f = 123.45f;
    int32_t i = cast_float_to_int32_t(f);

-------------------------------------------------------------------------------
NOTES

- Behavior is deterministic provided IC_CAST_ASSERT_FUNC enforces runtime checks.
- Defining IC_CAST_ASSERT_FUNC disables clamping (unnecessary after an assertion failure).
- IC_CAST_ASSERT_FUNC(expr) can be defined as (void)0 which removes runtime checks completely,
  which can be desirable to for removing all checks and internal casts in performance-critical code after testing. 
- NaN floats get clamped to the minimum value of the target type, and infinities get clamped to the respective min/max.
- Since one of the constraints is clamp Nan and -infinity to the min value, and +infinity to the max value, using 
  cast_float_to_float will actually filter the value, but also means all casts from float lack a compiletime quick path.
===============================================================================
*/

#ifndef IC_NUM_CAST_H
#define IC_NUM_CAST_H

#include "ic_glue_macro.h"
#include "ic_static_assert.h"
#include "ic_inline.h"
#include "ic_disable_warnings.h"

/*
===============================================================================
USER CONFIGURABLE BITS PER BYTE 
===============================================================================
*/

#ifndef IC_NUM_BITS_PER_BYTE
#define IC_NUM_BITS_PER_BYTE 8
#endif

#ifdef CHAR_BIT
IC_STATIC_ASSERT(IC_NUM_BITS_PER_BYTE == CHAR_BIT, "Bits per byte mismatch");
#endif

/*
===============================================================================
TYPE MOLDS
===============================================================================
*/

#define IC_MOLD_SIGNED_INT     SIGNED_INT
#define IC_MOLD_UNSIGNED_INT   UNSIGNED_INT
#define IC_MOLD_FLOAT          FLOAT

/*
===============================================================================
CONVERSION SAFETY CHECKS
===============================================================================
*/

// ---------------------- SIGNED INT -> SIGNED INT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_SIGNED_INT_TO_SIGNED_INT(from_min, from_max, to_min, to_max) \
    ((to_min <= from_min) && (to_max >= from_max))

// Assert at runtime condition
#define IC_INNER_SAFE_SIGNED_INT_TO_SIGNED_INT(v, lo, hi) \
    ((v) >= (lo) && (v) <= (hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_SIGNED_INT_TO_SIGNED_INT(to_t, v, lo, hi) \
    ((to_t)((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v))))

// ---------------------- SIGNED INT -> UNSIGNED INT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_SIGNED_INT_TO_UNSIGNED_INT(from_min, from_max, to_min, to_max) (0)

// Assert at runtime condition
#define IC_INNER_SAFE_SIGNED_INT_TO_UNSIGNED_INT(v, lo, hi) \
    ((v) >= 0 && (unsigned long long)(v) <= (unsigned long long)(hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_SIGNED_INT_TO_UNSIGNED_INT(to_t, v, lo, hi) \
    ((to_t)( \
        ((v) < 0) ? 0ULL : \
        ((unsigned long long)(v) > (unsigned long long)(hi)) ? \
            (unsigned long long)(hi) : \
            (unsigned long long)(v) \
    ))

// ---------------------- SIGNED INT -> FLOAT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_SIGNED_INT_TO_FLOAT(from_min, from_max, to_min, to_max) \
    ((from_min) >= (to_min) && (from_max) <= (to_max))

// Assert at runtime condition
#define IC_INNER_SAFE_SIGNED_INT_TO_FLOAT(v, lo, hi) \
    ((double)(v) >= (double)(lo) && (double)(v) <= (double)(hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_SIGNED_INT_TO_FLOAT(to_t, v, lo, hi) \
    ((to_t)(((double)(v) < (double)(lo)) ? (lo) : \
    ((double)(v) > (double)(hi) ? (hi) : (v))))

// ---------------------- UNSIGNED INT -> SIGNED INT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_UNSIGNED_INT_TO_SIGNED_INT(from_min, from_max, to_min, to_max) \
    ((from_min >= 0) && (from_max <= (unsigned long long)to_max))

// Assert at runtime condition
#define IC_INNER_SAFE_UNSIGNED_INT_TO_SIGNED_INT(v, lo, hi) \
    ((unsigned long long)(v) <= (unsigned long long)(hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_UNSIGNED_INT_TO_SIGNED_INT(to_t, v, lo, hi) \
    ((to_t)((unsigned long long)(v) > (unsigned long long)(hi) ? (hi) : (v)))

// ---------------------- UNSIGNED INT -> UNSIGNED INT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_UNSIGNED_INT_TO_UNSIGNED_INT(from_min, from_max, to_min, to_max) \
    ((to_max >= from_max))

// Assert at runtime condition
#define IC_INNER_SAFE_UNSIGNED_INT_TO_UNSIGNED_INT(v, lo, hi) \
    ((v) <= (hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_UNSIGNED_INT_TO_UNSIGNED_INT(to_t, v, lo, hi) \
    ((to_t)((v) > (hi) ? (hi) : (v)))

// ---------------------- UNSIGNED INT -> FLOAT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_UNSIGNED_INT_TO_FLOAT(from_min, from_max, to_min, to_max) \
    ((from_max) <= (to_max))

// Assert at runtime condition
#define IC_INNER_SAFE_UNSIGNED_INT_TO_FLOAT(v, lo, hi) \
    ((double)(v) <= (double)(hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_UNSIGNED_INT_TO_FLOAT(to_t, v, lo, hi) \
    ((to_t)((v) > (unsigned long long)(hi) ? (hi) : (v)))

// ---------------------- FLOAT -> SIGNED INT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_FLOAT_TO_SIGNED_INT(from_min, from_max, to_min, to_max) (0)

// Assert at runtime condition
#define IC_INNER_SAFE_FLOAT_TO_SIGNED_INT(v, lo, hi) \
    ((v) == (v) && \
    (double)(v) >= (double)(lo) && \
    (double)(v) <= (double)(hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_FLOAT_TO_SIGNED_INT(to_t, v, lo, hi) \
    ((to_t)((v) != (v) ? (lo) : \
    ((double)(v) < (double)(lo) ? (lo) : \
    ((double)(v) > (double)(hi) ? (hi) : (v)))))


// ---------------------- FLOAT -> UNSIGNED INT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_FLOAT_TO_UNSIGNED_INT(from_min, from_max, to_min, to_max) (0)

// Assert at runtime condition
#define IC_INNER_SAFE_FLOAT_TO_UNSIGNED_INT(v, lo, hi) \
    ((v) == (v) && \
    (double)(v) >= 0.0 && \
    (double)(v) <= (double)(hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_FLOAT_TO_UNSIGNED_INT(to_t, v, lo, hi) \
    ((to_t)((v) != (v) ? 0 : \
    ((double)(v) < 0.0 ? 0 : \
    ((double)(v) > (double)(hi) ? (hi) : (v)))))


// ---------------------- FLOAT -> FLOAT ---------------------- 

// Clamp at compile-time condition
#define IC_COMP_CLAMPABLE_FLOAT_TO_FLOAT(from_min, from_max, to_min, to_max) \
    ((to_min <= from_min) && (to_max >= from_max))

// Assert at runtime condition
#define IC_INNER_SAFE_FLOAT_TO_FLOAT(v, lo, hi) \
    ((v) == (v) && (v) >= (lo) && (v) <= (hi))

// Clamp at runtime (no UB)
#define IC_INNER_CLAMP_FLOAT_TO_FLOAT(to_t, v, lo, hi) \
    ((to_t)((v) != (v) ? (lo) : \
    ((v) < (lo) ? (lo) : \
    ((v) > (hi) ? (hi) : (v)))))

/*
===============================================================================
CONVERSION DISPATCHERS
===============================================================================
*/

#define IC_INNER_POLICY(from_mold, to_mold) \
    IC_GLUE4(IC_INNER_SAFE_, from_mold, _TO_, to_mold)

#define IC_COMP_CLAMPABLE(from_mold, to_mold) \
    IC_GLUE4(IC_COMP_CLAMPABLE_, from_mold, _TO_, to_mold)

#define IC_INNER_CLAMP_DISPATCH(from_mold, to_mold) \
    IC_GLUE4(IC_INNER_CLAMP_, from_mold, _TO_, to_mold)

/*
===============================================================================
CAST NAME
===============================================================================
*/

#define IC_INNER_CAST_NAME(from, to) \
    IC_GLUE4(cast_, from, _to_, to)

/*
===============================================================================
CAST GENERATOR
===============================================================================
*/

#ifndef IC_CAST_ASSERT_FUNC // If not using assertions, generate clamp cast functions

#define IC_INNER_GEN_CAST(from_t, from_mold, from_min, from_max, \
                          to_t, to_mold, to_min, to_max) \
IC_HEADER_FUNC to_t \
IC_INNER_CAST_NAME(from_t, to_t)(const from_t v) \
{ \
    IC_STATIC_ASSERT(sizeof(from_t) * IC_NUM_BITS_PER_BYTE <= 64, "From type " #from_t " is bigger than 64 bits"); \
    IC_STATIC_ASSERT(sizeof(to_t) * IC_NUM_BITS_PER_BYTE   <= 64, "To type " #to_t " is bigger than 64 bits"); \
    \
    if (IC_COMP_CLAMPABLE(from_mold, to_mold)(from_min, from_max, to_min, to_max)) { \
        return (to_t)v; \
    } \
    return IC_INNER_CLAMP_DISPATCH(from_mold, to_mold)(to_t, v, to_min, to_max); \
}

#elif defined(IC_CAST_ASSERT_FUNC) // If assertion function is defined, generate assert-based casts instead of clamping

#define IC_INNER_GEN_CAST(from_t, from_mold, from_min, from_max, \
                          to_t, to_mold, to_min, to_max) \
IC_HEADER_FUNC to_t \
IC_INNER_CAST_NAME(from_t, to_t)(const from_t v) \
{ \
    IC_STATIC_ASSERT(sizeof(from_t) * IC_NUM_BITS_PER_BYTE <= 64, "From type " #from_t " is bigger than 64 bits"); \
    IC_STATIC_ASSERT(sizeof(to_t) * IC_NUM_BITS_PER_BYTE   <= 64, "To type " #to_t " is bigger than 64 bits"); \
    \
    if (IC_COMP_CLAMPABLE(from_mold, to_mold)(from_min, from_max, to_min, to_max)) { \
        return (to_t)v; \
    } \
    IC_CAST_ASSERT_FUNC(IC_INNER_POLICY(from_mold, to_mold)(v, to_min, to_max)); \
    return (to_t)(v); \
}

#endif


/*
===============================================================================
PUBLIC API
===============================================================================
*/

#define IC_CASTING_FUNCTIONS(TYPE_MATRIX) \
    IC_DISABLE_WARNINGS \
    TYPE_MATRIX(IC_INNER_GEN_CAST) \
    IC_ENABLE_WARNINGS

#endif // IC_NUM_CAST_H