#include <stddef.h>
#include <string.h>

#include "external/array_library.h"
#include "headers/move_tags.h"

// -------------------------------------------------------------
// The data pointer of a span is the array it views, so with
// only_allow_array_passing_to_library_functions_and_spans it may
// only be handed to the standard library or a third-party function.
// Project functions take the span itself, and not even a pod
// creator may get it, since the size given with it can not be
// checked.
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

void process_ints(int *data, size_t size);
void process_const_ints(const int *data, size_t size);
void process_span(int_span_t span);
void edit_value(int *value);
int read_value(const int *value);

void test_span_data_passing(void)
{
    int values[3] = { 1, 2, 3 };
    int_span_t span = int_span_pod(values, 3);
    int_span_t *span_ptr = &span;
    int_const_span_t view = int_const_span_pod(values, 3);

    process_span(span); // good: the span keeps its data and size together
    memcpy(span.data, view.data, 3 * sizeof(int)); // good: standard library
    external_array_library_use(span.data, span.size); // good: third-party function
    int_span_t rest = int_span_pod(span.data + 1, span.size - 1); // bad: the size given to the pod creator can not be checked
    edit_value(&span.data[0]); // good: a pointer to a single element
    span.data[1] = read_value(&view.data[2]); // good: element access, and a single element

    process_ints(span.data, span.size); // bad: splits the data from its size again
    process_ints(span.data + 1, span.size - 1); // bad: the rest of the array without its span
    process_ints(span_ptr->data, span_ptr->size); // bad: also through a pointer to the span
    process_const_ints(view.data, view.size); // bad: also for a const span
    process_ints(mod_cast(span.data), span.size); // bad: a callsite operator does not hide it
    process_const_ints((const int *)span.data, span.size); // bad: neither does a cast

    int* span_data = span.data; // good
    int span_size = span.size;  // good
    span_data = span_ptr->data; // good
    span_size = span_ptr->size; // good

    process_span(rest);
}
