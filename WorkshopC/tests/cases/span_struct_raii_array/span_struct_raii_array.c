#include <stddef.h>

#include "headers/managed_structs.h"

// -------------------------------------------------------------
// With require_span_immediately_after_array, an array may only be
// used through its span. An array of raii structs is still destroyed
// through the array itself, with its array destroy function, since
// that is what the struct resource management rule tracks.
// -------------------------------------------------------------

void int_vector_destroy_array(int_vector_t* self, size_t n)
{
    for (int i = n + 1; i >= 0; i--)
    {
        int_vector_destroy(&self[i]);
    }
}

struct int_vector_span
{
    int_vector_t* data;
    size_t size;
};
typedef struct int_vector_span int_vector_span_t;
int int_vector_span_init(int_vector_span_t* self);
size_t int_vector_span_size(const int_vector_span_t* self);

int_vector_t* int_vector_span_data(int_vector_span_t* self)
{
    return self->data;
}

// A raii struct whose array destroyer does not have the array destroyer
// signature, so it is not trusted with the array
struct buffer
{
    int size;
};
typedef struct buffer buffer_t;
buffer_t buffer_make(void);
buffer_t buffer_copy(const buffer_t* self);
buffer_t buffer_move(buffer_t* self);
void buffer_destroy(buffer_t* self);
buffer_t buffer_return(buffer_t* self);
_Bool buffer_valid(const buffer_t* self);
int buffer_destroy_array(buffer_t* self, size_t n);

struct buffer_span
{
    buffer_t* data;
    size_t size;
};
typedef struct buffer_span buffer_span_t;
int buffer_span_init(buffer_span_t* self);

// A pod struct with a function shaped like an array destroyer, which is
// not trusted with the array since the struct has no raii creator
void pos_destroy_array(pos_t* self, size_t n);

struct pos_span
{
    pos_t* data;
    size_t size;
};
typedef struct pos_span pos_span_t;
int pos_span_init(pos_span_t* self);

void takes_vectors(int_vector_t* vectors);
void takes_vector_span(int_vector_span_t vectors);
void process_vectors(int_vector_t* vectors, size_t count);

// A one-statement static wrapper may hand the span's data on, but with
// only_allow_array_passing_to_library_functions_and_spans only to a
// standard-library or third-party function
static void process_vector_span(int_vector_span_t* span)
{
    process_vectors(int_vector_span_data(span), int_vector_span_size(span)); // bad: process_vectors is a project function
}

// -------------------------------------------------------------
// Good
// -------------------------------------------------------------

void destroyed_with_sizeof(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    int_vector_destroy_array(vectors, sizeof(vectors) / sizeof(vectors[0])); // good: the array destroy function takes the array
}

void destroyed_with_literal_size(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    int_vector_destroy_array(vectors, 2); // good
}

void used_through_span(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    _Bool valid = int_vector_valid(&vectors_span.data[0]); // good: an element through the span
    takes_vector_span(vectors_span); // good: the span itself
    
    valid = int_vector_valid(&int_vector_span_data(&vectors_span)[0]); // good

    int_vector_destroy_array(vectors, 2); // good
}

void span_data_through_wrapper(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    process_vector_span(&vectors_span); // good: the wrapper takes the span, and only its body is reported

    int_vector_destroy_array(vectors, 2); // good
}

void span_used_on_path_where_array_is_alive(int flag)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    if (flag)
    {
        int_vector_destroy_array(vectors, 2); // good
        return;
    }

    takes_vector_span(vectors_span); // good: only destroyed on the path that returned

    int_vector_destroy_array(vectors, 2); // good
}

// -------------------------------------------------------------
// Bad
// -------------------------------------------------------------

void span_used_after_destroy(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    int_vector_destroy_array(vectors, 2); // good

    _Bool valid = int_vector_valid(&vectors_span.data[0]); // bad: the span views destroyed elements
    takes_vector_span(vectors_span); // bad: same
    valid = int_vector_valid(&int_vector_span_data(&vectors_span)[0]); // bad
}

void span_data_through_getter(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    process_vectors(int_vector_span_data(&vectors_span), int_vector_span_size(&vectors_span)); // bad: the getter hands over the span's data
    process_vectors(vectors_span.data, vectors_span.size); // bad: same, through the field

    int_vector_destroy_array(vectors, 2); // good
}

void span_getter_after_destroy(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    int_vector_destroy_array(vectors, 2); // good

    size_t count = int_vector_span_size(&vectors_span); // bad: the span views destroyed elements
}

void destroyer_with_wrong_signature(void)
{
    buffer_t buffers[2] = { buffer_make(), buffer_make() }; // bad: buffer_destroy_array does not return void
    buffer_span_t buffers_span = { buffers, 2 }; // good

    int result = buffer_destroy_array(buffers, 2); // bad: not an array destroyer, so the array is not reachable here
}

void element_through_array(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    _Bool valid = int_vector_valid(&vectors[0]); // bad: not through the span

    int_vector_destroy_array(vectors, 2); // good
}

void passed_to_project_function(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_span_t vectors_span = { vectors, 2 }; // good

    takes_vectors(vectors); // bad: not through the span, and not a library, span or pod function

    int_vector_destroy_array(vectors, 2); // good
}

void no_span(void)
{
    int_vector_t lonely[2] = { int_vector_make(1), int_vector_make(2) }; // bad: no span follows
    int count = 0;

    int_vector_destroy_array(lonely, 2); // good: only the missing span is reported
}

void pod_struct_array_destroyer(void)
{
    pos_t positions[2] = { pos_pod(0, 0), pos_pod(1, 1) }; // good
    pos_span_t positions_span = { positions, 2 }; // good

    pos_destroy_array(positions, 2); // bad: pos is not a raii struct, so the array is not reachable here
}
