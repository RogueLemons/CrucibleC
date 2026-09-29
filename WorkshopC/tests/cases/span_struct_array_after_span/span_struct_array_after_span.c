#include <stddef.h>
#include <string.h>

// -------------------------------------------------------------
// With require_span_immediately_after_array, an array is only used
// through its span: the span is the one way to reach the array.
// Only the span's initializer, the array's own declaration and code
// that is never evaluated (sizeof, typeof) may name the array.
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

struct int_row_span
{
    int (*data)[3];
    size_t size;
};
typedef struct int_row_span int_row_span_t;
int int_row_span_init(int_row_span_t *self);

struct holder
{
    int items[3];
};
typedef struct holder holder_t;
int holder_init(holder_t *self);

void edit_value(int *value);

int global_values[4] = { 0 }; // good
int_span_t global_values_span = { global_values, sizeof(global_values) / sizeof(global_values[0]) }; // good: the span's initializer
size_t global_count = sizeof(global_values) / sizeof(global_values[0]); // good: sizeof is never evaluated

void test_array_use(holder_t *holder)
{
    int values[3] = { 1, 2, 3 }; // good
    int_span_t values_span = { values, sizeof(values) / sizeof(values[0]) }; // good: the span's initializer

    values_span.data[0] = 4; // good: through the span
    edit_value(&values_span.data[1]); // good: an element through the span
    size_t bytes = sizeof(values); // good: sizeof is never evaluated
    int values_bytes = sizeof(values); // good: sizeof is never evaluated, also into an int
    // __typeof__(values[0]) element = 0; // good: typeof is never evaluated
    // holder->items[0] = element; // good: an array in a struct has no span

    int first = values[0]; // bad: an element of the array itself
    values[1] = 5; // bad: an element of the array itself
    int *pointer = values; // bad: a pointer to the array itself
    int *second = &values[1]; // bad: the address of an element of the array itself
    int (*whole)[3] = &values; // bad: the address of the array itself
    memset(values, 0, sizeof(values)); // bad: the array itself, even to the standard library
    global_values[0] = first; // bad: a global array too
    global_values_span.data[0] = first; // good: through the global's span

    const int constants[2] = { 4, 5 }; // good: an array of const objects gets a const span
    int_const_span_t constants_span = { constants, sizeof(constants) / sizeof(constants[0]) };
    int read = constants_span.data[1]; // good: read through the const span
    size_t constants_bytes = sizeof(constants); // good: sizeof is never evaluated
    int direct = constants[0]; // bad: an element of the const array itself
    const int *constant_pointer = constants; // bad: a pointer to the const array itself

    int matrix[2][3] = { { 0 } }; // good
    int_row_span_t matrix_span = { matrix, 2 };
    matrix_span.data[1][2] = 1; // good: through the matrix's span
    matrix[1][2] = 1; // bad: the matrix itself

    int lonely[2] = { 0 }; // bad: no span follows
    int count = 0;
    lonely[0] = count; // only the missing span above is reported

    const float lonely_constants[5] = { 0 }; // bad: no const span follows
}
