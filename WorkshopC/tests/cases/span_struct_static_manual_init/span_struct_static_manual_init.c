#include <stddef.h>

#include "headers/managed_structs.h"

// -------------------------------------------------------------
// With allow_pod_span_to_be_initialized_manually_if_static, a pod span
// with static storage duration may be initialized with braces, since a
// static initializer can not call the span's pod function. The span
// rule still checks the braces, e.g. the count of the array.
// -------------------------------------------------------------

struct int_span
{
    int *data;
    size_t size;
};
typedef struct int_span int_span_t;
int_span_t int_span_pod(int *data, size_t size);

struct int_const_span
{
    const int *data;
    size_t size;
};
typedef struct int_const_span int_const_span_t;
int_const_span_t int_const_span_pod(const int *data, size_t size);

void use_ints(int_span_t span);

int g_values[4] = { 1, 2, 3, 4 };
const int g_constants[2] = { 5, 6 };

int_span_t g_span = { g_values, 4 }; // good: a global has static storage duration
static int_span_t s_span = { g_values, 4 }; // good: a static global
static int_const_span_t s_constants = { g_constants, 2 }; // good: a static const span
static int_span_t s_too_long = { g_values, 5 }; // bad: the span rule still checks the count
static int_span_t s_empty; // bad: a span must be initialized
static pos_t s_origin = { 0, 0 }; // bad: not a span, only spans may be initialized with braces

void test_static_locals(void)
{
    static int local_values[2] = { 0 };
    static int_span_t local_span = { local_values, 2 }; // good: a static local

    int values[3] = { 1, 2, 3 };
    int_span_t span = { values, 3 }; // bad: not static, use int_span_pod
    int_span_t from_pod = int_span_pod(values, 3); // good

    use_ints(local_span); // good
    use_ints(span); // good
    use_ints(from_pod); // good
    use_ints((int_span_t){ values, 3 }); // bad: a brace-built span argument, not a static declaration
}
