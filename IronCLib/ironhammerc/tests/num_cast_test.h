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

#endif // IRON_HAMMER_C_TESTS_NUM_CAST_TEST_H