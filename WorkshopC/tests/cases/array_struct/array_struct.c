#include <stddef.h>

struct sample_array_holder
{
    int values[4];
};
typedef struct sample_array_holder sample_array_holder_t;

int global_values[2] = { 1, 2 }; // bad: arrays may not be declared outside structs

void test_local_array_declaration(void)
{
    int local_values[3] = { 1, 2, 3 }; // bad: arrays may not be declared outside structs
    (void)local_values;
}

void use_struct_array(sample_array_holder_t *holder)
{
    (void)holder;
}
