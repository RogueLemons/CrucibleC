#include <stddef.h>
#include <string.h>

#include "external/array_library.h"

struct image_span
{
    int *data;
    size_t size;
};
typedef struct image_span image_span_t;
image_span_t image_span_pod(int *data, size_t size)
{
    image_span_t span = { data, size };
    return span;
}

struct wrong_order_span
{
    size_t size;
    int *data;
};
typedef struct wrong_order_span wrong_order_span_t;
wrong_order_span_t wrong_order_span_pod(int *data, size_t size);

struct wrong_names_span
{
    int *ptr;
    size_t size;
};
typedef struct wrong_names_span wrong_names_span_t;
wrong_names_span_t wrong_names_span_pod(int *data, size_t size);

struct wrong_types_span
{
    float data;
    int size;
};
typedef struct wrong_types_span wrong_types_span_t;
wrong_types_span_t wrong_types_span_pod(float *data, int size);

struct image_const_span
{
    const int *data;
    size_t size;
};
typedef struct image_const_span image_const_span_t;
image_const_span_t image_const_span_pod(const int *data, size_t size)
{
    image_const_span_t span = { data, size };
    return span;
}

void project_array_use(int *data, size_t size);

// Look-alikes of span pod creators: only '<span name> <span name><pod suffix>(...)'
// for a span struct that exists counts as one
int helper_pod(int *data, size_t size); // ends with the pod suffix, but there is no span 'helper'
image_span_t unknown_span_pod(int *data, size_t size); // there is no span struct 'unknown_span'
image_const_span_t unknown_const_span_pod(const int *data, size_t size); // there is no const span struct 'unknown_const_span'

struct score_span // bad: its pod creator does not return it, so it has no constructor
{
    int *data;
    size_t size;
};
typedef struct score_span score_span_t;
int score_span_pod(int *data, size_t size); // the right name, but it does not return score_span

struct score_const_span // bad: its pod creator does not return it, so it has no constructor
{
    const int *data;
    size_t size;
};
typedef struct score_const_span score_const_span_t;
score_span_t score_const_span_pod(const int *data, size_t size); // the right name, but it returns another span
void project_const_array_use(const int *data, size_t size);

void internal_array_use(int *data, size_t size)
{
    (void)data;
    (void)size;
}

image_span_t make_span_from_arguments(int *data, size_t size)
{
    return image_span_pod(data, size); // good: data is already a pointer
}

void test_span_array_destinations(void)
{
    int values[3] = { 1, 2, 3 };

    image_span_pod(values, 3); // good: span pod creator
    image_span_pod(values, 4); // bad: pod creator has the wrong count
    image_const_span_pod(values, 3); // good: const span pod creator
    image_const_span_pod(values, 5); // bad: const span pod creator has the wrong count
    memcpy(values, values, sizeof(values)); // good: standard library
    memmove(values, values, sizeof(values)); // good: standard library
    external_array_library_use(values, 3); // good: third-party function
    internal_array_use(values, 3); // bad: internal project function
    project_array_use(values, 3); // bad: project function

    const int constants[2] = { 4, 5 };

    image_const_span_pod(constants, 2); // good: a const array to a const span pod creator
    image_const_span_pod(constants, 3); // bad: const span pod creator has the wrong count
    memcpy(values, constants, sizeof(constants)); // good: standard library, also for a const array
    project_const_array_use(constants, 2); // bad: a const array to a project function

    helper_pod(values, 3); // bad: only ends with the pod suffix
    unknown_span_pod(values, 3); // bad: ends with span and pod suffix, but names no span struct
    unknown_const_span_pod(constants, 2); // bad: ends with const span and pod suffix, but names no const span struct
    score_span_pod(values, 3); // bad: named after its span, but does not return it
    score_const_span_pod(constants, 2); // bad: named after its const span, but returns another span
}
