#include <stddef.h>
#include <string.h>

#include "external/array_library.h"

struct library_array_holder
{
    int values[4];
};
typedef struct library_array_holder library_array_holder_t;

void project_array_use(int *values, size_t count);

void test_library_only_array_passing(library_array_holder_t *holder)
{
    int local_values[2] = { 1, 2 }; // bad: ad hoc arrays are never allowed
    project_array_use(holder->values, 4); // bad: project functions are not library functions
    memcpy(holder->values, local_values, sizeof(holder->values)); // good: standard library
    external_array_library_use(holder->values, 4); // good: third-party function
}
