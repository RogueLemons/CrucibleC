#include <stddef.h>
#include <string.h>

#include "external/array_library.h"

// -------------------------------------------------------------
// With only_allow_span_data_passing_in_one_line_static_functions,
// the data of a span is only handed on inside small wrappers: static
// functions with a single statement, which take spans. All other
// code passes the spans themselves.
// -------------------------------------------------------------

struct int_span
{
    int *data;
    size_t size;
};
typedef struct int_span int_span_t;
int int_span_init(int_span_t *self);

int *int_span_data(const int_span_t *self);
void process_ints(int *data, size_t size);

// Wrappers

static void int_span_copy(int_span_t destination, int_span_t source)
{
    memcpy(destination.data, source.data, source.size * sizeof(int)); // good: a static wrapper with a single statement
}

static void *int_span_fill(int_span_t span, int value)
{
    return memset(span.data, value, span.size * sizeof(int)); // good: a single return counts too
}

static inline void int_span_zero(const int_span_t *span)
{
    memset(span->data, 0, span->size * sizeof(int)); // good: static inline, through a pointer to the span
}

static void int_span_use_external(int_span_t span)
{
    external_array_library_use(span.data, span.size); // good: a wrapper around a third-party function
}

static void int_span_copy_from_getter(int_span_t destination, const int_span_t *source)
{
    memcpy(int_span_data(&destination), int_span_data(source), 3 * sizeof(int)); // good: getters inside a wrapper
}

// Not wrappers

void int_span_copy_exported(int_span_t destination, int_span_t source)
{
    memcpy(destination.data, source.data, source.size * sizeof(int)); // bad: not static
}

static void int_span_copy_in_two_steps(int_span_t destination, int_span_t source)
{
    size_t bytes = source.size * sizeof(int);
    memcpy(destination.data, source.data, bytes); // bad: more than a single statement
}

static int *int_span_clear_and_get(int_span_t span)
{
    return (memset(span.data, 0, span.size * sizeof(int)), span.data); // bad: a comma expression is two statements in one
}

static void int_span_process(int_span_t span)
{
    process_ints(span.data, span.size); // bad: a wrapper may still only hand the data to library functions
}

void test_wrappers(int_span_t a, int_span_t b)
{
    int_span_copy(a, b); // good: spans all the way
    int_span_fill(a, 7); // good
    int_span_zero(&a); // good
    int_span_use_external(b); // good

    memcpy(a.data, b.data, 3 * sizeof(int)); // bad: raw data outside of a wrapper
    external_array_library_use(a.data, a.size); // bad: raw data outside of a wrapper
    memset(int_span_data(&a), 0, sizeof(int)); // bad: a getter's data outside of a wrapper
}
