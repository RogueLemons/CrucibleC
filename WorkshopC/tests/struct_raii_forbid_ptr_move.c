#include "headers/managed_structs.h"

struct holder
{
    int_vector_t vec;
};
typedef struct holder holder_t;
int holder_init(holder_t* self);

// -------------------------------------------------------------
// Good: the move function is given the address of a variable
// -------------------------------------------------------------

void move_local_variable(void)
{
    int_vector_t original = int_vector_make(4);
    int_vector_t moved = int_vector_move(&original);        // good: address of a local variable

    int_vector_destroy(&original);
    int_vector_destroy(&moved);
}

void move_value_parameter(int_vector_t owned)
{
    int_vector_t moved = int_vector_move(&owned);           // good: a by-value parameter is owned by this scope

    int_vector_destroy(&owned);
    int_vector_destroy(&moved);
}

void move_in_parentheses(void)
{
    int_vector_t original = int_vector_make(4);
    int_vector_t moved = int_vector_move((&original));       // good

    int_vector_destroy(&original);
    int_vector_destroy(&moved);
}

// -------------------------------------------------------------
// Bad: the move function is given a pointer, or something that
// is not a plain variable
// -------------------------------------------------------------

void move_pointer_parameter(int_vector_t* vec)
{
    int_vector_t moved = int_vector_move(vec);              // bad: a pointer parameter, the object is not owned here

    int_vector_destroy(&moved);
}

void move_pointer_variable(void)
{
    int_vector_t original = int_vector_make(4);
    int_vector_t* pointer = &original;
    int_vector_t moved = int_vector_move(pointer);          // bad: a pointer variable

    int_vector_destroy(&original);
    int_vector_destroy(&moved);
}

void move_field_through_pointer(holder_t* holder)
{
    int_vector_t moved = int_vector_move(&holder->vec);     // bad: a field reached through a pointer

    int_vector_destroy(&moved);
}

void move_field_of_local(void)
{
    holder_t holder;
    holder_init(&holder);

    int_vector_t moved = int_vector_move(&holder.vec);      // bad: a struct field, not a variable

    int_vector_destroy(&moved);
}

void move_dereferenced_pointer(int_vector_t* vec)
{
    int_vector_t moved = int_vector_move(&*vec);            // bad: still the object behind a pointer

    int_vector_destroy(&moved);
}

void suppressed_move(int_vector_t* vec)
{
    // WorkshopC off
    int_vector_t moved = int_vector_move(vec);              // good: suppressed, no diagnostic expected
    // WorkshopC on

    int_vector_destroy(&moved);
}
