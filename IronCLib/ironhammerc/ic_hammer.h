#ifndef IC_IRON_HAMMER_H
#define IC_IRON_HAMMER_H

/*
===========================================================
 IC IRON HAMMER - minimal header-only test framework

 Event API:
   (context, test_name, message, value, has_value)

 ===========================================================
 IC IRON HAMMER - QUICK USAGE GUIDE

 1. PRINT HOOK (USER DEFINED)

 You must define this macro before including the header:

    IHC_PRINT(ctx, test, msg, value, has_value)

 -----------------------------------------------------------

 2. DEFINE TESTS

    IHC_TEST(test_name) {
        IHC_ASSERT(condition);   // stops test on failure
        IHC_CHECK(condition);    // logs failure, continues
    }

 -----------------------------------------------------------

 3. REGISTER TESTS

    ihc_test_case tests[] = {
        IHC_TEST_ENTRY(test_a),
        IHC_TEST_ENTRY(test_b)
    };

 -----------------------------------------------------------

 4. RUN TESTS

    IHC_RUN(tests);

 -----------------------------------------------------------

 5. REPORT RESULTS

    IHC_REPORT();

 -----------------------------------------------------------

 6. EXIT CODE (OPTIONAL)

    return ihc_failures();

 -----------------------------------------------------------

 TOOL SUMMARY

 - IHC_TEST        : declare a test function
 - IHC_ASSERT      : fatal check (returns from test)
 - IHC_CHECK       : non-fatal check (continues test)
 - ihc_test_case   : test case struct
 - IHC_TEST_ENTRY  : register test in list
 - IHC_RUN         : execute test list
 - IHC_REPORT      : print summary
 - ihc_failures()  : number of failed tests

 ===========================================================

EXAMPLE USAGE:

#include <stdio.h>

#define IHC_PRINT(ctx, test, msg, value, has_value) \
    do { \
        printf("%-12s %-20s %-20s", ctx, test, msg); \
        if (has_value) printf(" %u", value); \
        printf("\n"); \
    } while (0)

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

    ihc_test_case my_tests[] = {
        IHC_TEST_ENTRY(test_math),
        IHC_TEST_ENTRY(test_fail)
    };

    IHC_RUN(my_tests);
    IHC_REPORT();

    return ihc_failures();
}

===========================================================
*/
 
// =========================
//   Global counters
// =========================
static unsigned int ihc_passed_tests = 0;
static unsigned int ihc_failed_tests = 0;

static unsigned int ihc_passed_checks = 0;
static unsigned int ihc_failed_checks = 0;

// =========================
//   Current test context
// =========================
static const char *ihc_current_test = (const char*)0;

// =========================
//   Print hook
// =========================
#ifndef IHC_PRINT
#define IHC_PRINT(ctx, test, msg, value, has_value) ((void)0)
#endif

// =========================
//   Failure accessor
// =========================
static unsigned int ihc_failures(void) {
    return ihc_failed_tests;
}

// =========================
//   Test type
// =========================
typedef void (*ihc_test_fn)(void);

typedef struct {
    const char *name;
    ihc_test_fn fn;
} ihc_test_case;

// =========================
//   Define test
// =========================
#define IHC_TEST(name) \
    static void name(void)

// =========================
//   Register test
// =========================
#define IHC_TEST_ENTRY(test) { #test, test }

// =========================
//   Assertions (fatal)
// =========================
#define IHC_ASSERT(expr) do {                               \
    if (expr) {                                                   \
        ihc_passed_checks++;                                \
    } else {                                                      \
        ihc_failed_checks++;                                \
        IHC_PRINT("ASSERT FAIL", ihc_current_test,    \
                         #expr, 0, 0);                            \
        return;                                                   \
    }                                                             \
} while (0)

// =========================
//   Checks (non-fatal)
// =========================
#define IHC_CHECK(expr) do {                                \
    if (expr) {                                                   \
        ihc_passed_checks++;                                \
    } else {                                                      \
        ihc_failed_checks++;                                \
        IHC_PRINT("CHECK FAIL", ihc_current_test,     \
                         #expr, 0, 0);                            \
    }                                                             \
} while (0)
 
// =========================
//   Run tests
// =========================
#define IHC_RUN(list) do {                                  \
    IHC_PRINT("\n================= IRON HAMMER TEST RUN =================", "", "", 0, 0); \
                                                                   \
    unsigned int count = (sizeof(list) / sizeof((list)[0]));             \
                                                                   \
    for (unsigned int i = 0; i < count; ++i) {                   \
        ihc_current_test = (list)[i].name;                 \
        unsigned int before = ihc_failed_checks;           \
                                                                   \
        IHC_PRINT("RUN", ihc_current_test, "", 0, 0); \
                                                                   \
        (list)[i].fn();                                           \
                                                                   \
        if (ihc_failed_checks == before) {                 \
            ihc_passed_tests++;                            \
            IHC_PRINT("PASS", ihc_current_test, "", 0, 0); \
        } else {                                                  \
            ihc_failed_tests++;                            \
            IHC_PRINT("FAIL", ihc_current_test,      \
                             "test failed count", ihc_failed_tests, 1);               \
        }                                                         \
    }                                                             \
} while (0)

 
// =========================
//   Report / summary
// =========================
#define IHC_REPORT() do {                                   \
    IHC_PRINT("\n================= IRON HAMMER C REPORT =================", "", "", 0, 0); \
                                                                   \
    unsigned int total_tests =                                    \
        ihc_passed_tests + ihc_failed_tests;          \
                                                                   \
    unsigned int total_checks =                                   \
        ihc_passed_checks + ihc_failed_checks;        \
                                                                   \
    IHC_PRINT("REPORT", "tests_passed", "",               \
                    ihc_passed_tests, 1);                  \
                                                                   \
    IHC_PRINT("REPORT", "tests_total", "",                \
                    total_tests, 1);                             \
                                                                   \
    IHC_PRINT("REPORT", "checks_total", "",               \
                    total_checks, 1);                            \
} while (0)

#endif // IC_IRON_HAMMER_H