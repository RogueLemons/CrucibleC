#include "headers/managed_structs.h"

void take_raii_struct_by_value(int_vector_t raii_value_arg)
{
    int_vector_destroy(&raii_value_arg);
}

int_vector_t produce_vector(void)
{
    return int_vector_make(3);                              // good: returned to the caller
}

int_vector_t* get_vector_pointer(void);

struct holder
{
    int value;
    int_vector_t vec;
};
typedef struct holder holder_t;

int holder_init(holder_t* self, int value)
{
    self->value = value;
    self->vec = int_vector_make(value);                     // good: assigned
    return 0;
}

void owned_uses(void)
{
    int_vector_t a = int_vector_make(4);                    // good: initializes a variable
    int_vector_t b = int_vector_copy(&a);                   // good
    int_vector_t c = produce_vector();                      // good
    take_raii_struct_by_value(int_vector_make(2));          // good: ownership passed to the function
    take_raii_struct_by_value(produce_vector());            // good

    get_vector_pointer();                                   // good: returns a pointer, not a raii struct
    pos_pod(1, 2);                                          // good: pod structs may be discarded
    int x = pos_pod(1, 2).x;                                // good: pod structs may be accessed directly
    unsigned long size = sizeof(int_vector_make(1));        // good: sizeof does not call the function

    int_vector_destroy(&a);
    int_vector_destroy(&b);
    int_vector_destroy(&c);
}

void discarded_uses(int flag)
{
    int_vector_t a = int_vector_make(4);

    int_vector_make(4);                                     // bad: discarded
    (void)int_vector_make(4);                               // bad: not even with a void cast
    int_vector_copy(&a);                                    // bad: discarded
    produce_vector();                                       // bad: discarded
    (void)produce_vector();                                 // bad: discarded
    (int_vector_make(1), 0);                                // bad: left side of a comma is discarded
    flag ? int_vector_make(1) : int_vector_make(2);         // bad: both results are discarded

    for (int_vector_make(1); flag < 1; ++flag)              // bad: discarded
    {
    }

    int_vector_t (*maker)(int) = int_vector_make;
    maker(3);                                               // bad: also through a function pointer

    int_vector_destroy(&a);
}

void member_access_uses(void)
{
    int_vector_t a = int_vector_make(4);

    int size = int_vector_make(4).size;                     // bad: can never be destroyed
    int capacity = int_vector_copy(&a).capacity;            // bad
    int* data = produce_vector().data;                      // bad
    int first = int_vector_make(4).data[0];                 // bad

    int_vector_destroy(&a);
}

void suppressed_use(void)
{
    // WorkshopC off
    int_vector_make(4);                                     // good: suppressed, no diagnostic expected
    // WorkshopC on
}
