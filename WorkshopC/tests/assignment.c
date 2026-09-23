#include <stddef.h>

static int global_unassigned_int; // bad
static int global_assigned_int = 5; // good

struct Data {
    int value;
    int *ptr;
};

struct DataNoPtrs {
    char c;
    int i;
    float f;
};

void takes_const_int(const int *p)
{
    (void)p;
}

void takes_mut_int(int *p)
{
    *p = 5;
}

void takes_struct(struct Data d)
{
    (void)d;
}

void test(
    int arg,
    int *ptrArg,
    struct Data dataArg,
    struct Data *dataPtrArg
)
{
    int unassigned; // bad
    int* unassigned_ptr; // bad

    int x = 1; // good
    int y = 2; // good
    int *ptr = &x; // good
    struct Data data; // good

    ptr = 0; // bad
    ptr = NULL; // bad
    ptr = (void*)0; // bad
    ptr = (int*)0; // bad
    int ptr_is_null = ptr == NULL; // good

    data.ptr = 0; // bad
    data.ptr = NULL; // bad
    data.ptr = (void*)0; // bad
    data.ptr = (int*)0; // bad
    int data_ptr_is_null = data.ptr == NULL; // good

    takes_mut_int(0); // bad
    takes_mut_int(NULL); // bad
    takes_mut_int((void*)0); // bad
    takes_mut_int((int*)0); // bad

    arg = 10; // bad
    ptrArg = &x; // bad
    dataArg.value = 123; // bad
    int* ptr_to_arg = &arg; // bad
    const int* const_ptr_to_arg = (const int*)&arg; // bad

    x = 100; // good
    ptr = &y; // good
    *ptrArg = 77; // good
    dataPtrArg->value = 999; // good
    data.ptr = &x; // good
    const int ci = 42; // good
    takes_const_int(&ci); // good
    takes_const_int((const int*)&x); // good
    data.value = 55; // good
    takes_mut_int(ptr); // good
    takes_struct(data); // good

    float* zero_init_ptr = {0}; // bad
    struct Data struct_with_ptr = {0}; // bad
    struct DataNoPtrs simple_data_obj = {0}; // good

    struct Data init_data_with_null = { .ptr = NULL, .value = 5}; // bad
}