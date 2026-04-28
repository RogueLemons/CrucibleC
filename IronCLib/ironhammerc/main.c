
// Define printing macro for test results
#define IHC_PRINT(ctx, test, msg, value, has_value) \
do { \
    printf("%-12s %-20s %-20s", ctx, test, msg); \
    if (has_value) printf(" %u", value); \
    printf("\n"); \
} while (0)

// Includes
#include <stdio.h>
#include "ic_hammer.h"
#include "ic_static_assert.h"

// Test cases
#include "tests/inline_test.h"
#include "tests/memory_test.h"
#include "tests/bounded_loop_test.h"
#include "tests/typenum_test.h"
#include "tests/num_cast_test.h"

IC_STATIC_ASSERT(8 == 2 * 4, "A simple verification that the static assert macro works correctly");

IHC_TEST(verify_static_assert_works_inside_functions) 
{
    IC_STATIC_ASSERT(2 == 1 + 1, "Simple nested static assert test");
}

int main(void) {

    const ihc_test_case iron_c_lib_tests[] = 
    {
        // ic_static_assert.h
        IHC_TEST_ENTRY(verify_static_assert_works_inside_functions),
        // ic_inline.h
        IHC_TEST_ENTRY(verify_header_functions_can_be_used_in_multiple_source_files),
        // ic_memory.h
        IHC_TEST_ENTRY(verify_memory_protects_against_negative_size),
        IHC_TEST_ENTRY(verify_memory_protects_against_zero_size),
        IHC_TEST_ENTRY(verify_memory_protects_against_overflow),
        IHC_TEST_ENTRY(verify_memory_allocates_valid_array),
        // ic_bounded_loop.h
        IHC_TEST_ENTRY(verify_bounded_while_prevents_infinite_loops),
        IHC_TEST_ENTRY(verify_bounded_while_works_as_normal_within_bounds),
        IHC_TEST_ENTRY(verify_bounded_while_treats_non_positive_bounds_as_zero),
        IHC_TEST_ENTRY(verify_bounded_do_while_prevents_infinite_loops),
        IHC_TEST_ENTRY(verify_bounded_do_while_works_as_normal_within_bounds),
        IHC_TEST_ENTRY(verify_bounded_do_while_treats_non_positive_bounds_as_zero),
        // ic_typenum.h
        IHC_TEST_ENTRY(verify_typenum_internal_values_are_correct),
        IHC_TEST_ENTRY(verify_typenum_equality_function_works),
        IHC_TEST_ENTRY(verify_typenum_provides_correct_strings),
        // ic_num_cast.h
        IHC_TEST_ENTRY(verify_simple_same_bit_cast_gives_expected_value),
        IHC_TEST_ENTRY(verify_floating_to_unsigned_stays_within_bounds),
        IHC_TEST_ENTRY(verify_floating_infinity_and_nan_becomes_bounded),
    };

    IHC_RUN(iron_c_lib_tests);
    IHC_REPORT();

    return ihc_failures();
}