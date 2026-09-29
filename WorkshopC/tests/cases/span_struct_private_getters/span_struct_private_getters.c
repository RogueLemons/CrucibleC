#include <stddef.h>
#include <string.h>

#include "external/array_library.h"
#include "headers/private_tag.h"

// -------------------------------------------------------------
// The fields of a span can be private, so that only the span's own
// functions reach them. A getter returning the data pointer then
// counts like 'span.data': outside the standard library and third
// party functions, its result may not be passed on.
// -------------------------------------------------------------

struct int_span
{
    PRIVATE int *data;
    PRIVATE size_t size;
};
typedef struct int_span int_span_t;

struct int_const_span
{
    PRIVATE const int *data;
    PRIVATE size_t size;
};
typedef struct int_const_span int_const_span_t;

// The span's own functions

int *int_span_data(const int_span_t *self)
{
    return self->data; // good: a getter of the span
}

size_t int_span_size(const int_span_t *self)
{
    return self->size; // good: a getter of the span
}

int int_span_at(const int_span_t *self, size_t index)
{
    return self->data[index]; // good: an element of the span
}

int_span_t *int_span_self(int_span_t *self)
{
    return self; // good: a pointer to the span itself
}

int_span_t int_span_drop_first(const int_span_t *self, size_t count)
{
    if (count > self->size)
        count = self->size;

    int_span_t rest = { self->data + count, self->size - count }; // good: the span's own function makes a smaller span
    return rest;
}

const int *int_const_span_data(const int_const_span_t *self)
{
    return self->data; // good: a getter of the const span
}

// Everything else

void process_ints(int *data, size_t size);
void process_const_ints(const int *data, size_t size);
void process_span(const int_span_t *span);
int *global_buffer(void);

void test_span_getters(void)
{
    int values[3] = { 1, 2, 3 };
    int_span_t span = { values, 3 }; // good
    int_const_span_t view = { values, 3 }; // good

    process_span(&span); // good: the span itself
    process_span(int_span_self(&span)); // good: a pointer to the span is the span, not its data
    int_span_t rest = int_span_drop_first(&span, 1); // good: a smaller span from the span's own function
    int first = int_span_at(&span, 0); // good: a single element
    process_ints(values, int_span_size(&span)); // bad: the array itself may not be passed either
    memcpy(int_span_data(&span), values, sizeof(values)); // good: standard library
    external_array_library_use(int_span_data(&span), int_span_size(&span)); // good: third-party function
    process_ints(global_buffer(), 3); // good: not named after a span, so not its data

    process_ints(int_span_data(&span), int_span_size(&span)); // bad: the getter hands over the span's data
    process_ints(int_span_data(&span) + 1, 2); // bad: also with pointer arithmetic
    process_const_ints(int_const_span_data(&view), 3); // bad: also for a const span
    process_ints(span.data, 3); // bad: the private field, and the span's data

    process_span(&rest);
    (void)first;
}
