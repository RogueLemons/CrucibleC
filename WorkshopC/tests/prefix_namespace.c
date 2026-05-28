#include "headers/a/b/c/prefix_testing_bad.h"
#include "headers/a/b/c/prefix_testing_good.h"

struct struct_in_c_file
{
    int x, y, z;
};

typedef struct struct_in_c_file typedef_in_c_file;

static void static_function_in_c_file(int* out)
{
    *out = 5;
}

void function_in_c_file(int* out)
{
    *out = 5;
}
