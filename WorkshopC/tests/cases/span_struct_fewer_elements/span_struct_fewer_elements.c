#include <stddef.h>

// -------------------------------------------------------------
// With allow_spans_to_be_given_fewer_elements_than_their_size, a
// span may cover only the start of an array: its count may be less
// than the array's element count, but never more
// -------------------------------------------------------------

struct int_span
{
    int *data;
    size_t size;
};
typedef struct int_span int_span_t;
int int_span_init(int_span_t *self);

struct int_const_span
{
    const int *data;
    size_t size;
};
typedef struct int_const_span int_const_span_t;
int int_const_span_init(int_const_span_t *self);

struct index_span
{
    int *data;
    size_t size;
};
typedef struct index_span index_span_t;
index_span_t index_span_pod(int *data, size_t size);

void test_initializers(size_t count)
{
    int values[3] = { 1, 2, 3 };
    const int constants[4] = { 4, 5, 6, 7 };

    int_span_t all = { values, 3 }; // good: every element
    int_span_t first_two = { values, 2 }; // good: fewer elements
    int_span_t empty = { values, 0 }; // good: no elements
    int_span_t all_but_last = { values, sizeof(values) / sizeof(values[0]) - 1 }; // good: a constant expression
    int_span_t designated = { .data = values, .size = 1 }; // good: fewer elements with designators
    int_span_t too_many = { values, 4 }; // bad: more elements than the array has
    int_span_t negative = { values, -1 }; // bad: becomes a huge size_t
    int_span_t unknown = { values, count }; // bad: the count must be a constant to be checked

    int_const_span_t const_first_two = { constants, 2 }; // good: a const span too
    int_const_span_t const_too_many = { constants, 5 }; // bad: more elements than the array has

    int_span_t tail_part = { &values[1], 1 }; // good: fewer of the 2 elements left from index 1
    int_span_t tail_too_many = { values + 1, 3 }; // bad: only 2 elements are left from index 1
}

void test_pod_creator(void)
{
    int values[3] = { 1, 2, 3 };

    index_span_pod(values, 3); // good: every element
    index_span_pod(values, 1); // good: fewer elements
    index_span_pod(values, 5); // bad: more elements than the array has
}
