#include <stddef.h>

// -------------------------------------------------------------
// A span can start inside an array, at '&array[index]' or
// 'array + index'. Its count is then checked against the elements
// left from that index, like a whole array against its size.
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

void edit_value(int *value);
void process_ints(int *data, size_t size);

void test_initializers(size_t index)
{
    int values[10] = { 0 };
    const int constants[4] = { 1, 2, 3, 4 };

    int_span_t tail = { &values[3], 7 }; // good: the 7 elements left from index 3
    int_span_t added = { values + 3, 7 }; // good: the same with pointer arithmetic
    int_span_t added_first = { 3 + values, 7 }; // good: in either order
    int_span_t from_start = { &values[0], 10 }; // good: the whole array
    int_span_t at_end = { &values[10], 0 }; // good: an empty span at the end
    int_span_t last_two = { &values[sizeof(values) / sizeof(values[0]) - 2], 2 }; // good: a constant expression as the index
    int_const_span_t const_tail = { &constants[1], 3 }; // good: a const span too

    int_span_t too_many = { &values[3], 10 }; // bad: only 7 elements are left from index 3
    int_span_t too_few = { &values[3], 6 }; // bad: without allow_spans_to_be_given_fewer_elements_than_their_size, all 7 must be used
    int_span_t added_too_many = { values + 3, 100 }; // bad: only 7 elements are left from index 3
    int_span_t outside = { &values[12], 1 }; // bad: the index is past the end of the array
    int_span_t unknown_index = { &values[index], 7 }; // bad: the index must be a constant to be checked
    int_const_span_t const_too_many = { &constants[1], 4 }; // bad: only 3 elements are left from index 1

    int* unknown_source = NULL;
    int unknown_source_size = 111;
    int_span_t unknown_source_span = { unknown_source, unknown_source_size }; // good: the user must be trusted
}

void test_pod_creator(size_t index)
{
    int values[10] = { 0 };

    index_span_pod(&values[4], 6); // good: the 6 elements left from index 4
    index_span_pod(values + 9, 1); // good: the last element
    index_span_pod(&values[4], 7); // bad: only 6 elements are left from index 4
    index_span_pod(values + index, 3); // bad: the index must be a constant to be checked

    edit_value(&values[3]); // good: one element passed to another function
    process_ints(values, 10); // bad: the array itself may not be passed to a project function

    int* unknown_source = NULL;
    int unknown_source_size = 111;
    index_span_t unknown_source_span_from_pod = index_span_pod(unknown_source, unknown_source_size); // good: the user must be trusted
}
