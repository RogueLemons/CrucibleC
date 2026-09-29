#include <stddef.h>
#include <string.h>

#include "external/array_library.h"

struct int_span
{
    int *data;
    size_t size;
};
typedef struct int_span int_span_t;

struct bad_span
{
    int *data;
    size_t size;
    int extra;
};
typedef struct bad_span bad_span_t;

void test_span_struct_basics(void)
{
    int values[3] = { 1, 2, 3 };
    int_span_t uninitialized; // bad: span values must be initialized at declaration
    int_span_t wrong_count = { values, 4 }; // bad: the count is not the array size
    int_span_t fewer_elements = { values, 2 }; // bad: without allow_spans_to_be_given_fewer_elements_than_their_size, the count must be the array size
    int_span_t valid = { values, 3 }; // good

    int_span_t with_sizeof = { values, sizeof(values) / sizeof(values[0]) }; // good
    int_span_t with_designators = { .data = values, .size = 3 }; // good
    int_span_t with_conditional = { values, 1 ? 3 : 3 }; // good
}

// -------------------------------------------------------------
// A const span holds a pointer to const data and is named with
// const_span_struct_suffix, otherwise it works like a span
// -------------------------------------------------------------

struct int_const_span // good: a pointer to const data
{
    const int *data;
    size_t size;
};
typedef struct int_const_span int_const_span_t;

struct const_data_span // bad: a span may not hold a pointer to const data, it must be a const span
{
    const int *data;
    size_t size;
};
typedef struct const_data_span const_data_span_t;

struct mutable_data_const_span // bad: a const span must hold a pointer to const data
{
    int *data;
    size_t size;
};
typedef struct mutable_data_const_span mutable_data_const_span_t;

struct extra_field_const_span // bad: the same shape as a span is required
{
    const int *data;
    size_t size;
    int extra;
};
typedef struct extra_field_const_span extra_field_const_span_t;

struct const_pointer_span // good: the pointer itself is const, the data it points to is not
{
    int *const data;
    size_t size;
};
typedef struct const_pointer_span const_pointer_span_t;

void test_const_span_struct(void)
{
    int values[3] = { 1, 2, 3 };
    const int constants[2] = { 4, 5 };

    int_const_span_t uninitialized_const; // bad: const span values must be initialized at declaration too
    int_const_span_t wrong_const_count = { constants, 3 }; // bad: the count is not the array size
    int_const_span_t valid_const = { constants, 2 }; // good
    int_const_span_t from_mutable = { values, 3 }; // good: a const span may view data that can change
    int_const_span_t with_designators_const = { .data = constants, .size = sizeof(constants) / sizeof(constants[0]) }; // good
}

// -------------------------------------------------------------
// Without only_allow_array_passing_to_library_functions_and_spans,
// an array may be passed to any function
// -------------------------------------------------------------

void project_array_use(int *data, size_t size);
void project_const_array_use(const int *data, size_t size);

void test_array_passing_without_restriction(void)
{
    int values[3] = { 1, 2, 3 };
    const int constants[2] = { 4, 5 };

    memcpy(values, constants, sizeof(constants)); // good: standard library
    external_array_library_use(values, 3); // good: third-party function
    project_array_use(values, 3); // good: project function, not restricted
    project_const_array_use(constants, 2); // good: a const array to a project function, not restricted

    int_span_t span = { values, 3 }; // good
    project_array_use(span.data, span.size); // good: the data of a span is not restricted either
    int_span_t* span_ptr = &span;
    project_array_use(span_ptr->data, span_ptr->size); // good: the data of a span is not restricted even when accessed through a pointer to the span
}
