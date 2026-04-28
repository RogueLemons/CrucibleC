#ifndef IRON_HAMMER_C_TESTS_NUM_CAST_TEST_H
#define IRON_HAMMER_C_TESTS_NUM_CAST_TEST_H

#include "ic_hammer.h"
#include "docs/premade/numbers.h"
#include <math.h>

IHC_TEST(verify_simple_same_bit_cast_gives_expected_value)
{
    const i32 integer = 42;
    const f32 floating = 42.1337f;
    const i32 int_from_float = cast_f32_to_i32(floating);
    IHC_CHECK(integer == int_from_float);
}

IHC_TEST(verify_floating_to_unsigned_stays_within_bounds)
{
    const f64 too_big = 5555555.5555;
    const u8 u8_max = UINT8_MAX;
    const u8 big_to_u8 = cast_f64_to_u8(too_big);
    IHC_CHECK(u8_max == big_to_u8);

    const f64 negative = (-1.0) * too_big;
    const u8 neg_to_u8 = cast_f64_to_u8(negative);
    IHC_CHECK(0u == neg_to_u8);
}

IHC_TEST(verify_floating_infinity_and_nan_becomes_bounded)
{
    // 32-bit floating point
    const f32 f32_max = FLT_MAX;
    const f32 f32_min = -FLT_MAX;
    const f32 pos_inf_to_f32 = cast_f32_to_f32(INFINITY);
    IHC_CHECK(f32_max == pos_inf_to_f32);
    const f32 neg_inf_to_f32 = cast_f32_to_f32(-INFINITY);
    IHC_CHECK(f32_min == neg_inf_to_f32);
    const f32 NaN_to_f32 = cast_f32_to_f32(NAN);
    IHC_CHECK(f32_min == NaN_to_f32);

    // 64-bit floating point
    const f64 f64_max = DBL_MAX;
    const f64 f64_min = -DBL_MAX;
    const f64 pos_inf_to_f64 = cast_f64_to_f64(INFINITY);
    IHC_CHECK(f64_max == pos_inf_to_f64);
    const f64 neg_inf_to_f64 = cast_f64_to_f64(-INFINITY);
    IHC_CHECK(f64_min == neg_inf_to_f64);
    const f64 NaN_to_f64 = cast_f64_to_f64(NAN);
    IHC_CHECK(f64_min == NaN_to_f64);
}

IHC_TEST(verify_signed_integer_conversions_for_large_type_to_small_type)
{
    const i64 i64_max = INT64_MAX;
    const i64 i64_min = INT64_MIN;

    // i64 to i16
    const i16 i16_max = INT16_MAX;
    const i16 i16_min = INT16_MIN;
    const i16 arb_to_i16 = cast_i64_to_i16(789);
    IHC_CHECK(arb_to_i16 == 789);
    const i16 big_to_i16 = cast_i64_to_i16(i64_max);
    IHC_CHECK(i16_max == big_to_i16);
    const i16 small_to_i16 = cast_i64_to_i16(i64_min);
    IHC_CHECK(i16_min == small_to_i16);

    // i64 to u16
    const u16 u16_max = UINT16_MAX;
    const u16 arb_to_u16 = cast_i64_to_u16(789);
    IHC_CHECK(arb_to_u16 == 789);
    const u16 big_to_u16 = cast_i64_to_u16(i64_max);
    IHC_CHECK(u16_max == big_to_u16);
    const u16 small_to_u16 = cast_i64_to_u16(i64_min);
    IHC_CHECK(0u == small_to_u16);

    // i64 to f32
    const f32 arb_to_f32 = cast_i64_to_f32(789);
    IHC_CHECK(arb_to_f32 == 789.0f);
    const f32 big_to_f32 = cast_i64_to_f32(i64_max);
    const f32 small_to_f32 = cast_i64_to_f32(i64_min);
    // note f32 max is bigger than i64 max
    IHC_CHECK(big_to_f32 == i64_max); 
    IHC_CHECK(small_to_f32 == i64_min);
}

IHC_TEST(verify_unsigned_integer_conversions_for_large_type_to_small_type)
{
    const u64 u64_max = UINT64_MAX;

    // u64 to i16
    const i16 i16_max = INT16_MAX;
    const i16 arb_to_i16 = cast_u64_to_i16(789);
    IHC_CHECK(arb_to_i16 == 789);
    const i16 big_to_i16 = cast_u64_to_i16(u64_max);
    IHC_CHECK(i16_max == big_to_i16);

    // u64 to u16
    const u16 u16_max = UINT16_MAX;
    const u16 arb_to_u16 = cast_u64_to_u16(789);
    IHC_CHECK(arb_to_u16 == 789);
    const u16 big_to_u16 = cast_u64_to_u16(u64_max);
    IHC_CHECK(u16_max == big_to_u16);

    // u64 to f32
    const f32 arb_to_f32 = cast_u64_to_f32(789);
    IHC_CHECK(arb_to_f32 == 789.0f);
    const f32 big_to_f32 = cast_u64_to_f32(u64_max);
    // cast functions forbid undefined behavior, underflow, and overflow, but allow precision loss
    IHC_CHECK((0.999f * u64_max) <= big_to_f32 || big_to_f32 <= (1.001f * u64_max));
}

IHC_TEST(verify_floating_point_conversions_for_large_type_to_small_type)
{
    const f64 f64_max = DBL_MAX;
    const f64 f64_min = -DBL_MAX;

    // f64 to i16
    const i16 i16_max = INT16_MAX;
    const i16 i16_min = INT16_MIN;
    const i16 arb_to_i16 = cast_f64_to_i16(789.0);
    IHC_CHECK(arb_to_i16 == 789);
    const i16 big_to_i16 = cast_f64_to_i16(f64_max);
    IHC_CHECK(i16_max == big_to_i16);
    const i16 small_to_i16 = cast_f64_to_i16(f64_min);
    IHC_CHECK(i16_min == small_to_i16);

    // f64 to u16
    const u16 u16_max = UINT16_MAX;
    const u16 arb_to_u16 = cast_f64_to_u16(789.0);
    IHC_CHECK(arb_to_u16 == 789);
    const u16 big_to_u16 = cast_f64_to_u16(f64_max);
    IHC_CHECK(u16_max == big_to_u16);
    const u16 small_to_u16 = cast_f64_to_u16(f64_min);
    IHC_CHECK(0u == small_to_u16);

    // f64 to f32
    const f32 f32_max = FLT_MAX;
    const f32 f32_min = -FLT_MAX;
    const f32 arb_to_f32 = cast_f64_to_f32(789.0);
    IHC_CHECK(arb_to_f32 == 789.0f);
    const f32 big_to_f32 = cast_f64_to_f32(f64_max);
    IHC_CHECK(big_to_f32 == f32_max);
    const f32 small_to_f32 = cast_f64_to_f32(f64_min);
    IHC_CHECK(small_to_f32 == f32_min);
}

#endif // IRON_HAMMER_C_TESTS_NUM_CAST_TEST_H