#include "headers/managed_structs.h"
#include "external/some_struct.h"

void int_vector_foo(int_vector_t* self);

void double_destroy_is_allowed(void)
{
    int_vector_t vec = int_vector_make(4);

    int_vector_destroy(&vec);
    int_vector_destroy(&vec); // good: raii_use_after_destroy is true
}

void field_access_after_destroy_is_allowed(void)
{
    int_vector_t vec = int_vector_make(4);

    int_vector_destroy(&vec);

    int size = vec.size;      // good: raii_use_after_destroy is true
    int_vector_foo(&vec);     // good: raii_use_after_destroy is true
}

void double_destroy_of_argument_is_allowed(int_vector_t vec)
{
    int_vector_destroy(&vec);
    int_vector_destroy(&vec); // good: raii_use_after_destroy is true
}

void missing_cleanup_still_detected(void)
{
    int_vector_t vec = int_vector_make(4); // bad: missing destroy is unrelated to raii_use_after_destroy, kept here to verify errors are still detected in this file
}
