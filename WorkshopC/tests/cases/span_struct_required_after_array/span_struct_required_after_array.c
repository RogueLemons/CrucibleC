#include <stddef.h>

// -------------------------------------------------------------
// With require_span_immediately_after_array, every array outside of
// a struct must be followed right away by a span or const span
// variable holding the whole array: from its start, with its full
// size
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

struct holder
{
    int items[4]; // good: an array inside a struct needs no span
};
typedef struct holder holder_t;
int holder_init(holder_t *self);

// Globals

int global_values[4] = { 0 }; // good
int_span_t global_values_span = { global_values, 4 };

int global_lonely[2] = { 0 }; // bad: no span follows
int global_other = 0;

int global_partial[3] = { 0 }; // bad: the span holds only part of the array
int_span_t global_partial_span = { global_partial, 3 - 1 }; // bad: the count is not the array size either

extern int global_elsewhere[8]; // good: only a declaration, the definition gets the span

// Locals

void test_local_arrays(void)
{
    int values[3] = { 1, 2, 3 }; // good
    int_span_t values_span = { values, 3 };

    const int constants[2] = { 4, 5 }; // good: a const span
    int_const_span_t constants_span = { constants, 2 };

    int pods[5] = { 0 }; // good: the span's pod creator
    index_span_t pods_span = index_span_pod(pods, 5);

    int first[3] = { 0 }; // good: '&first[0]' is its start too
    int_span_t first_span = { &first[0], 3 };

    int designated[2] = { 0 }; // good: with designators
    int_span_t designated_span = { .data = designated, .size = sizeof(designated) / sizeof(designated[0]) };

    int pair_a[2] = { 0 }, pair_b[3] = { 0 }; // good: both spans follow in one declaration
    int_span_t pair_a_span = { pair_a, 2 }, pair_b_span = { pair_b, 3 };

    static int kept[2] = { 0 }; // good: a static local too
    int_span_t kept_span = { kept, 2 };

    int lonely[3] = { 0 }; // bad: no span follows
    int count = 0;

    int later[3] = { 0 }; // bad: the span is not the next statement
    count = 1;
    int_span_t later_span = { later, 3 };

    int offset[4] = { 0 }; // bad: the span does not start at the beginning of the array
    int_span_t offset_span = { &offset[1], 3 };

    int other[2] = { 0 }; // bad: the span holds another array
    int_span_t other_span = { values_span.data, 3 };

    int two_spans[3] = { 0 }; // bad: the span is only in the statement after the next
    int_span_t not_it = { values_span.data, 3 };
    int_span_t two_spans_span = { two_spans, 3 };
}

// -------------------------------------------------------------
// A matrix is an array of arrays: its whole is held by a span of
// pointers to its rows, and each row, reached through that span, is
// a normal array
// -------------------------------------------------------------

struct int_row_span
{
    int (*data)[3];
    size_t size;
};
typedef struct int_row_span int_row_span_t;
int int_row_span_init(int_row_span_t *self);

void test_matrices(void)
{
    int matrix[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } }; // good: a span of its rows holds the whole matrix
    int_row_span_t matrix_span = { matrix, 2 };

    int_span_t first_row = { matrix_span.data[0], 3 }; // good: one row is a normal array of 3
    int_span_t second_row = { matrix_span.data[1], 3 }; // good
    int_span_t row_tail = { &matrix_span.data[1][1], 2 }; // good: the elements left in the row from index 1
    int_span_t row_too_long = { matrix_span.data[1], 6 }; // bad: a row has 3 elements, not the 6 of the matrix
    int_row_span_t matrix_too_long = { matrix, 3 }; // bad: the matrix has 2 rows, and is used after its span

    int grid[2][3] = { 0 }; // bad: a span of one row does not hold the whole matrix
    int_span_t grid_row = { grid[0], 3 };
}
