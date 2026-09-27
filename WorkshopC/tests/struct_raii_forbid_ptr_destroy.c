#include "headers/managed_structs.h"

// With raii_may_only_destroy_value_ref, a raii destroy function may only be
// given the address of a variable, so only an object owned by the calling
// scope can be destroyed

// A free struct: its raii fields may be destroyed from anywhere
struct holder
{
    int_vector_t vec;
    int_vector_t vecs[2];
};
typedef struct holder holder_t;
int holder_init(holder_t* self);

// A raii struct with a raii field
struct pair
{
    int_vector_t first;
};
typedef struct pair pair_t;
pair_t pair_make(void);
pair_t pair_copy(const pair_t* self);
pair_t pair_move(pair_t* self);
pair_t pair_return(pair_t* self);
_Bool pair_valid(const pair_t* self);

// -------------------------------------------------------------
// Good: the destroy function is given the address of a variable
// -------------------------------------------------------------

void destroy_local_variable(void)
{
    int_vector_t vec = int_vector_make(4);
    int_vector_destroy(&vec); // good: address of a local variable
}

void destroy_value_parameter(int_vector_t owned)
{
    int_vector_destroy(&owned); // good: a by-value parameter is owned by this scope
}

void destroy_in_parentheses(void)
{
    int_vector_t vec = int_vector_make(4);
    int_vector_destroy((&vec)); // good
}

int_vector_t int_vector_return(int_vector_t* self)
{
    int_vector_t returned = int_vector_copy(self);
    int_vector_destroy(self); // good: the struct's own helper functions may use 'self'
    return returned;
}

void pair_destroy(pair_t* self)
{
    int_vector_destroy(&self->first); // good: a struct destroys its raii fields in its own destroy function
}

void destroy_free_struct_field_through_pointer(holder_t* holder)
{
    int_vector_destroy(&holder->vec); // good: a field of a free struct
}

void destroy_free_struct_field_of_local(void)
{
    holder_t holder;
    holder_init(&holder);

    int_vector_destroy(&holder.vec); // good: a field of a free struct
    int_vector_destroy(&holder.vecs[1]); // good: an element of an array field of a free struct
}

// -------------------------------------------------------------
// Bad: the destroy function is given a pointer, or something that
// is not a plain variable
// -------------------------------------------------------------

void destroy_pointer_parameter(int_vector_t* vec)
{
    int_vector_destroy(vec); // bad: a pointer parameter, the object is not owned here
}

void destroy_raii_field_through_pointer(pair_t* pair)
{
    int_vector_destroy(&pair->first); // bad: a field of a raii struct, only pair's own functions may destroy it
}

void destroy_raii_field_of_local(void)
{
    pair_t pair = pair_make();

    int_vector_destroy(&pair.first); // bad: a field of a raii struct, not a variable
    pair_destroy(&pair);
}

void destroy_dereferenced_pointer(int_vector_t* vec)
{
    int_vector_destroy(&*vec); // bad: still the object behind a pointer
}

void suppressed_destroy(int_vector_t* vec)
{
    // WorkshopC off
    int_vector_destroy(vec); // good: suppressed, no diagnostic expected
    // WorkshopC on
}
