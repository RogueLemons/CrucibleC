#include <stddef.h>

#include "headers/managed_structs.h"

// Arrays of raii structs are allowed when the struct provides an array
// destroy function, every element is initialized from a function return
// value, and the array is destroyed with that function before scope exit.

void int_vector_destroy_array(int_vector_t* self, size_t n);

// A raii struct without an array destroy function, which is fine until an
// array of it is declared
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

struct holder
{
    int_vector_t vecs[2];
};
typedef struct holder holder_t;
int holder_init(holder_t* self);

// -------------------------------------------------------------
// Good
// -------------------------------------------------------------

void destroyed_with_sizeof(void)
{
    int_vector_t vectors[3] = { int_vector_make(1), int_vector_make(2), int_vector_make(3) }; // good
    int_vector_destroy_array(vectors, sizeof(vectors) / sizeof(vectors[0])); // good
}

void destroyed_with_literal_size(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good
    int_vector_destroy_array(vectors, 2); // good: any expression with the right value
}

void inferred_size(void)
{
    int_vector_t vectors[] = { int_vector_make(1), int_vector_make(2) }; // good: the size comes from the list
    int_vector_destroy_array(vectors, 2); // good
}

void destroyed_on_every_path(int flag)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) }; // good

    if (flag)
    {
        int_vector_destroy_array(vectors, 2); // good
        return;
    }

    int_vector_destroy_array(vectors, 2); // good
}

void clear_field_array(holder_t* self)
{
    int_vector_destroy_array(self->vecs, 2); // good: an array field is the array itself too
}

// -------------------------------------------------------------
// Bad
// -------------------------------------------------------------

void never_destroyed(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) };
} // bad: the array is never destroyed

void destroyed_element_by_element(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) };
    int_vector_destroy(&vectors[0]);
    int_vector_destroy(&vectors[1]);
} // bad: only the array destroy function destroys the array

void missing_on_one_path(int flag)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) };

    if (flag)
        return; // bad: not destroyed before this return

    int_vector_destroy_array(vectors, 2);
}

void wrong_count(void)
{
    int_vector_t vectors[3] = { int_vector_make(1), int_vector_make(2), int_vector_make(3) };
    int_vector_destroy_array(vectors, 2); // bad: the size is 3
}

void wrong_field_count(holder_t* self)
{
    int_vector_destroy_array(self->vecs, 3); // bad: the size is 2
}

void pointer_argument(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) };
    int_vector_t* pointer = vectors;
    int_vector_destroy_array(pointer, 2); // bad: a pointer, not the array itself
} // bad: so the array is never destroyed

void uninitialized(void)
{
    int_vector_t vectors[2]; // bad: not initialized
    int_vector_destroy_array(vectors, 2);
}

void element_from_variable(void)
{
    int_vector_t first = int_vector_make(1);
    int_vector_t vectors[1] = { first }; // bad: raii elements must come from a function return value
    int_vector_destroy_array(vectors, 1);
    int_vector_destroy(&first);
}

void missing_element(void)
{
    int_vector_t vectors[3] = { int_vector_make(1), int_vector_make(2) }; // bad: the third element is missing
    int_vector_destroy_array(vectors, 3);
}

void two_dimensional(void)
{
    int_vector_t grid[2][1] = { { int_vector_make(1) }, { int_vector_make(2) } }; // bad: only one dimension is allowed
}

void used_after_array_destroy(void)
{
    int_vector_t vectors[2] = { int_vector_make(1), int_vector_make(2) };
    int_vector_destroy_array(vectors, 2);
    int size = vectors[0].size; // bad: used after being destroyed
}

void no_array_destroy_function(void)
{
    buffer_t buffers[1] = { buffer_make() }; // bad: buffer has no buffer_destroy_array
}

// -------------------------------------------------------------
// A pod struct may not contain raii structs, neither as an array
// nor as a matrix
// -------------------------------------------------------------

struct pod_with_raii_array
{
    int count;
    int_vector_t vecs[2]; // bad: an array of raii structs inside a pod struct
};
typedef struct pod_with_raii_array pod_with_raii_array_t;
pod_with_raii_array_t pod_with_raii_array_pod(void);

struct pod_with_raii_matrix
{
    int_vector_t grid[2][3]; // bad: a matrix of raii structs inside a pod struct
};
typedef struct pod_with_raii_matrix pod_with_raii_matrix_t;
pod_with_raii_matrix_t pod_with_raii_matrix_pod(void);

struct pod_with_pod_arrays
{
    pos_t points[2]; // good: pod structs may contain arrays of pod structs
    pos_t grid[2][3]; // good
};
typedef struct pod_with_pod_arrays pod_with_pod_arrays_t;
pod_with_pod_arrays_t pod_with_pod_arrays_pod(void);
