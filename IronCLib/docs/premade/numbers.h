#ifndef PREMADE_NUMBERS_H
#define PREMADE_NUMBERS_H

/*
USAGE:
    - Example: i32 x = cast_i64_to_i32(some_i64);
    - Cast functions are named as cast_<from>_to_<to>
    - Cast functions remove UB and clamps to min/max of target type on underflow/overflow
    - Define IC_CAST_ASSERT_FUNC(expr) assert(expr) to replace clamps with asserts
    - Define IC_CAST_ASSERT_FUNC(expr) (void)0 to remove all runtime checks and clamps (use with caution)

INCLUDED TYPES:
    - i8, i16, i32, i64
    - u8, u16, u32, u64
    - f32, f64
    - size_t

MUTALLY EXCLUSIVE CAST SUPPORT FOR:
    - int, unsigned int, float, double
*/

#include "ironclib/ic_num_cast.h"

#include <stdint.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>

// Create typedefs for shorter function names and DATA macros for easier writing of matrix
// signed integers
typedef int8_t   i8;
#define I8_DATA   i8,  IC_MOLD_SIGNED_INT,   INT8_MIN,  INT8_MAX
typedef int16_t  i16;
#define I16_DATA  i16, IC_MOLD_SIGNED_INT,   INT16_MIN, INT16_MAX
typedef int32_t  i32;
#define I32_DATA  i32, IC_MOLD_SIGNED_INT,   INT32_MIN, INT32_MAX
typedef int64_t  i64;
#define I64_DATA  i64, IC_MOLD_SIGNED_INT,   INT64_MIN, INT64_MAX
// unsigned integers
typedef uint8_t  u8;
#define U8_DATA   u8,  IC_MOLD_UNSIGNED_INT, 0,          UINT8_MAX
typedef uint16_t u16;
#define U16_DATA  u16, IC_MOLD_UNSIGNED_INT, 0,          UINT16_MAX
typedef uint32_t u32;
#define U32_DATA  u32, IC_MOLD_UNSIGNED_INT, 0,          UINT32_MAX
typedef uint64_t u64;
#define U64_DATA  u64, IC_MOLD_UNSIGNED_INT, 0,          UINT64_MAX
// floating point
typedef float  f32;
#define F32_DATA f32, IC_MOLD_FLOAT,        -FLT_MAX,   FLT_MAX
typedef double f64;
#define F64_DATA f64, IC_MOLD_FLOAT,        -DBL_MAX,   DBL_MAX
// size
#define SIZE_DATA size_t, IC_MOLD_UNSIGNED_INT, 0, SIZE_MAX

// Expansion helper macros
#define DATA_PAIR_IMPL(X, a1,a2,a3,a4,b1,b2,b3,b4) X(a1,a2,a3,a4,b1,b2,b3,b4)
#define DATA_PAIR(X, A, B) DATA_PAIR_IMPL(X, A, B)

// Define all cast conversion functions in x list format
#define CAST_CONVERSION_MATRIX(X)       \
    DATA_PAIR(X, I8_DATA,   I8_DATA)    \
    DATA_PAIR(X, I8_DATA,   I16_DATA)   \
    DATA_PAIR(X, I8_DATA,   I32_DATA)   \
    DATA_PAIR(X, I8_DATA,   I64_DATA)   \
    DATA_PAIR(X, I8_DATA,   U8_DATA)    \
    DATA_PAIR(X, I8_DATA,   U16_DATA)   \
    DATA_PAIR(X, I8_DATA,   U32_DATA)   \
    DATA_PAIR(X, I8_DATA,   U64_DATA)   \
    DATA_PAIR(X, I8_DATA,   SIZE_DATA)  \
    DATA_PAIR(X, I8_DATA,   F32_DATA)   \
    DATA_PAIR(X, I8_DATA,   F64_DATA)   \
    DATA_PAIR(X, I16_DATA,  I8_DATA)    \
    DATA_PAIR(X, I16_DATA,  I16_DATA)   \
    DATA_PAIR(X, I16_DATA,  I32_DATA)   \
    DATA_PAIR(X, I16_DATA,  I64_DATA)   \
    DATA_PAIR(X, I16_DATA,  U8_DATA)    \
    DATA_PAIR(X, I16_DATA,  U16_DATA)   \
    DATA_PAIR(X, I16_DATA,  U32_DATA)   \
    DATA_PAIR(X, I16_DATA,  U64_DATA)   \
    DATA_PAIR(X, I16_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, I16_DATA,  F32_DATA)   \
    DATA_PAIR(X, I16_DATA,  F64_DATA)   \
    DATA_PAIR(X, I32_DATA,  I8_DATA)    \
    DATA_PAIR(X, I32_DATA,  I16_DATA)   \
    DATA_PAIR(X, I32_DATA,  I32_DATA)   \
    DATA_PAIR(X, I32_DATA,  I64_DATA)   \
    DATA_PAIR(X, I32_DATA,  U8_DATA)    \
    DATA_PAIR(X, I32_DATA,  U16_DATA)   \
    DATA_PAIR(X, I32_DATA,  U32_DATA)   \
    DATA_PAIR(X, I32_DATA,  U64_DATA)   \
    DATA_PAIR(X, I32_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, I32_DATA,  F32_DATA)   \
    DATA_PAIR(X, I32_DATA,  F64_DATA)   \
    DATA_PAIR(X, I64_DATA,  I8_DATA)    \
    DATA_PAIR(X, I64_DATA,  I16_DATA)   \
    DATA_PAIR(X, I64_DATA,  I32_DATA)   \
    DATA_PAIR(X, I64_DATA,  I64_DATA)   \
    DATA_PAIR(X, I64_DATA,  U8_DATA)    \
    DATA_PAIR(X, I64_DATA,  U16_DATA)   \
    DATA_PAIR(X, I64_DATA,  U32_DATA)   \
    DATA_PAIR(X, I64_DATA,  U64_DATA)   \
    DATA_PAIR(X, I64_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, I64_DATA,  F32_DATA)   \
    DATA_PAIR(X, I64_DATA,  F64_DATA)   \
    DATA_PAIR(X, U8_DATA,   I8_DATA)    \
    DATA_PAIR(X, U8_DATA,   I16_DATA)   \
    DATA_PAIR(X, U8_DATA,   I32_DATA)   \
    DATA_PAIR(X, U8_DATA,   I64_DATA)   \
    DATA_PAIR(X, U8_DATA,   U8_DATA)    \
    DATA_PAIR(X, U8_DATA,   U16_DATA)   \
    DATA_PAIR(X, U8_DATA,   U32_DATA)   \
    DATA_PAIR(X, U8_DATA,   U64_DATA)   \
    DATA_PAIR(X, U8_DATA,   SIZE_DATA)  \
    DATA_PAIR(X, U8_DATA,   F32_DATA)   \
    DATA_PAIR(X, U8_DATA,   F64_DATA)   \
    DATA_PAIR(X, U16_DATA,  I8_DATA)    \
    DATA_PAIR(X, U16_DATA,  I16_DATA)   \
    DATA_PAIR(X, U16_DATA,  I32_DATA)   \
    DATA_PAIR(X, U16_DATA,  I64_DATA)   \
    DATA_PAIR(X, U16_DATA,  U8_DATA)    \
    DATA_PAIR(X, U16_DATA,  U16_DATA)   \
    DATA_PAIR(X, U16_DATA,  U32_DATA)   \
    DATA_PAIR(X, U16_DATA,  U64_DATA)   \
    DATA_PAIR(X, U16_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, U16_DATA,  F32_DATA)   \
    DATA_PAIR(X, U16_DATA,  F64_DATA)   \
    DATA_PAIR(X, U32_DATA,  I8_DATA)    \
    DATA_PAIR(X, U32_DATA,  I16_DATA)   \
    DATA_PAIR(X, U32_DATA,  I32_DATA)   \
    DATA_PAIR(X, U32_DATA,  I64_DATA)   \
    DATA_PAIR(X, U32_DATA,  U8_DATA)    \
    DATA_PAIR(X, U32_DATA,  U16_DATA)   \
    DATA_PAIR(X, U32_DATA,  U32_DATA)   \
    DATA_PAIR(X, U32_DATA,  U64_DATA)   \
    DATA_PAIR(X, U32_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, U32_DATA,  F32_DATA)   \
    DATA_PAIR(X, U32_DATA,  F64_DATA)   \
    DATA_PAIR(X, U64_DATA,  I8_DATA)    \
    DATA_PAIR(X, U64_DATA,  I16_DATA)   \
    DATA_PAIR(X, U64_DATA,  I32_DATA)   \
    DATA_PAIR(X, U64_DATA,  I64_DATA)   \
    DATA_PAIR(X, U64_DATA,  U8_DATA)    \
    DATA_PAIR(X, U64_DATA,  U16_DATA)   \
    DATA_PAIR(X, U64_DATA,  U32_DATA)   \
    DATA_PAIR(X, U64_DATA,  U64_DATA)   \
    DATA_PAIR(X, U64_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, U64_DATA,  F32_DATA)   \
    DATA_PAIR(X, U64_DATA,  F64_DATA)   \
    DATA_PAIR(X, SIZE_DATA, I8_DATA)    \
    DATA_PAIR(X, SIZE_DATA, I16_DATA)   \
    DATA_PAIR(X, SIZE_DATA, I32_DATA)   \
    DATA_PAIR(X, SIZE_DATA, I64_DATA)   \
    DATA_PAIR(X, SIZE_DATA, U8_DATA)    \
    DATA_PAIR(X, SIZE_DATA, U16_DATA)   \
    DATA_PAIR(X, SIZE_DATA, U32_DATA)   \
    DATA_PAIR(X, SIZE_DATA, U64_DATA)   \
    DATA_PAIR(X, SIZE_DATA, SIZE_DATA)  \
    DATA_PAIR(X, SIZE_DATA, F32_DATA)   \
    DATA_PAIR(X, SIZE_DATA, F64_DATA)   \
    DATA_PAIR(X, F32_DATA,  I8_DATA)    \
    DATA_PAIR(X, F32_DATA,  I16_DATA)   \
    DATA_PAIR(X, F32_DATA,  I32_DATA)   \
    DATA_PAIR(X, F32_DATA,  I64_DATA)   \
    DATA_PAIR(X, F32_DATA,  U8_DATA)    \
    DATA_PAIR(X, F32_DATA,  U16_DATA)   \
    DATA_PAIR(X, F32_DATA,  U32_DATA)   \
    DATA_PAIR(X, F32_DATA,  U64_DATA)   \
    DATA_PAIR(X, F32_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, F32_DATA,  F32_DATA)   \
    DATA_PAIR(X, F32_DATA,  F64_DATA)   \
    DATA_PAIR(X, F64_DATA,  I8_DATA)    \
    DATA_PAIR(X, F64_DATA,  I16_DATA)   \
    DATA_PAIR(X, F64_DATA,  I32_DATA)   \
    DATA_PAIR(X, F64_DATA,  I64_DATA)   \
    DATA_PAIR(X, F64_DATA,  U8_DATA)    \
    DATA_PAIR(X, F64_DATA,  U16_DATA)   \
    DATA_PAIR(X, F64_DATA,  U32_DATA)   \
    DATA_PAIR(X, F64_DATA,  U64_DATA)   \
    DATA_PAIR(X, F64_DATA,  SIZE_DATA)  \
    DATA_PAIR(X, F64_DATA,  F32_DATA)   \
    DATA_PAIR(X, F64_DATA,  F64_DATA)

// Generate casting functions
IC_CASTING_FUNCTIONS(CAST_CONVERSION_MATRIX)

// Make cast functions for standard types
typedef unsigned int unsigned_int;
#define UINT_DATA       unsigned_int, IC_MOLD_UNSIGNED_INT, 0,         UINT_MAX
#define INT_DATA        int,          IC_MOLD_SIGNED_INT,   INT_MIN,   INT_MAX
#define FLOAT_DATA      float,        IC_MOLD_FLOAT,        -FLT_MAX,  FLT_MAX
#define DOUBLE_DATA     double,       IC_MOLD_FLOAT,        -DBL_MAX,  DBL_MAX

#define STANDARD_CAST_CONVERSION_MATRIX(X)  \
    DATA_PAIR(X, INT_DATA,   INT_DATA)      \
    DATA_PAIR(X, INT_DATA,   UINT_DATA)     \
    DATA_PAIR(X, INT_DATA,   FLOAT_DATA)    \
    DATA_PAIR(X, INT_DATA,   DOUBLE_DATA)   \
    DATA_PAIR(X, UINT_DATA,  INT_DATA)      \
    DATA_PAIR(X, UINT_DATA,  UINT_DATA)     \
    DATA_PAIR(X, UINT_DATA,  FLOAT_DATA)    \
    DATA_PAIR(X, UINT_DATA,  DOUBLE_DATA)   \
    DATA_PAIR(X, FLOAT_DATA, INT_DATA)      \
    DATA_PAIR(X, FLOAT_DATA, UINT_DATA)     \
    DATA_PAIR(X, FLOAT_DATA, FLOAT_DATA)    \
    DATA_PAIR(X, FLOAT_DATA, DOUBLE_DATA)   \
    DATA_PAIR(X, DOUBLE_DATA, INT_DATA)     \
    DATA_PAIR(X, DOUBLE_DATA, UINT_DATA)    \
    DATA_PAIR(X, DOUBLE_DATA, FLOAT_DATA)   \
    DATA_PAIR(X, DOUBLE_DATA, DOUBLE_DATA)

// Generate casting functions
IC_CASTING_FUNCTIONS(STANDARD_CAST_CONVERSION_MATRIX)

#endif // PREMADE_NUMBERS_H