
#define IHC_PRINT(ctx, test, msg, value, has_value) \
do { \
    printf("%-12s %-20s %-20s", ctx, test, msg); \
    if (has_value) printf(" %u", value); \
    printf("\n"); \
} while (0)

#include <stdio.h>
#include "ic_hammer.h"

IHC_TEST(test_math) {
    IHC_ASSERT(1 + 1 == 2);
    IHC_CHECK(2 * 2 == 4);
}

IHC_TEST(test_fail) {
    IHC_CHECK(1 == 0);
    IHC_ASSERT(2 == 3);
}

int main(void) {

    const ihc_test_case my_tests[] = {
        IHC_TEST_ENTRY(test_math),
        IHC_TEST_ENTRY(test_fail)
    };

    IHC_RUN(my_tests);
    IHC_REPORT();

    return ihc_failures();
}