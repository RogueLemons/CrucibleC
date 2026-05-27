#include <stddef.h>

#include <stddef.h>

// Forbidden
static int global_unassigned_int;
// Allowed
static int global_assigned_int = 5;

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
    // Forbidden
    int unassigned;
    // Forbidden
    int* unassigned_ptr;

    // Allowed
    int x = 1;
    // Allowed
    int y = 2;
    // Allowed
    int *ptr = &x;
    // Allowed
    struct Data data;

    // Forbidden
    ptr = 0;
    // Forbidden
    ptr = NULL;
    // Forbidden
    ptr = (void*)0;
    // Forbidden
    ptr = (int*)0;
    // Allowed
    int ptr_is_null = ptr == NULL;

    // Forbidden
    data.ptr = 0;
    // Forbidden
    data.ptr = NULL;
    // Forbidden
    data.ptr = (void*)0;
    // Forbidden
    data.ptr = (int*)0;
    // Allowed
    int data_ptr_is_null = data.ptr == NULL;

    // Forbidden
    takes_mut_int(0);
    // Forbidden
    takes_mut_int(NULL);
    // Forbidden
    takes_mut_int((void*)0);
    // Forbidden
    takes_mut_int((int*)0);

    // Forbidden
    arg = 10;
    // Forbidden
    ptrArg = &x;
    // Forbidden
    dataArg.value = 123;
    // Forbidden
    int* ptr_to_arg = &arg;
    // Forbidden
    const int* const_ptr_to_arg = (const int*)&arg;
    
    // Allowed
    x = 100;
    // Allowed
    ptr = &y;
    // Allowed
    *ptrArg = 77;
    // Allowed
    dataPtrArg->value = 999;
    // Allowed
    data.ptr = &x;
    // Allowed
    const int ci = 42;
    // Allowed
    takes_const_int(&ci);
    // Allowed
    takes_const_int((const int*)&x);
    // Allowed
    data.value = 55;
    // Allowed
    takes_mut_int(ptr);
    // Allowed
    takes_struct(data);

    // Forbidden
    float* zero_init_ptr = {0};
    // Forbidden
    struct Data struct_with_ptr = {0};
    // Allowed
    struct DataNoPtrs simple_data_obj = {0};
}