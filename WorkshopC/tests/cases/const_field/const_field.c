#include <stddef.h>

#include "external/const_fields.h"
#include "system/system_const_fields.h"

// -------------------------------------------------------------
// A struct field may not be const itself, e.g. 'const int' or
// 'int* const'. What a pointer field points to may be const, e.g.
// 'const int*', since the field itself can still be assigned.
// -------------------------------------------------------------

typedef const int const_int_t;
typedef int *const int_const_pointer_t;
typedef const int *int_pointer_to_const_t;

struct values
{
    int plain; // good
    const int *pointer_to_const; // good: only the data it points to is const
    int const *pointer_to_const_east; // good: the same, with const on the right
    const char *name; // good
    const int **pointer_to_pointer_to_const; // good
    int_pointer_to_const_t typedef_pointer_to_const; // good: a typedef of a pointer to const
    const int *pointers_to_const[2]; // good: an array of pointers to const
    const int (*pointer_to_const_row)[3]; // good: a pointer to an array of const

    const int constant; // bad: const itself
    int const constant_east; // bad: const itself, with const on the right
    int *const const_pointer; // bad: a const pointer
    const int *const const_pointer_to_const; // bad: a const pointer, even to const
    int *const *pointer_to_const_pointer; // good: the field itself is not const
    int **const const_pointer_to_pointer; // bad: a const pointer
    const_int_t typedef_constant; // bad: const through a typedef
    int_const_pointer_t typedef_const_pointer; // bad: a const pointer through a typedef
    const int constants[3]; // bad: an array of const elements
    int *const const_pointers[2]; // bad: an array of const pointers
    volatile const int volatile_constant; // bad: const and volatile
};

struct nested
{
    struct
    {
        const int inner; // bad: a field of an anonymous inner struct too
    } inner_struct;

    const struct values *values; // good: a pointer to a const struct
    const struct values copy; // bad: a const struct
};

union number
{
    const int integer; // bad: a union field too
    float real;
};
