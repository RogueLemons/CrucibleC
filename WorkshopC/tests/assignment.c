#include <stddef.h>

static int global_unassigned_int; // bad: not initialized
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
    int unassigned; // bad: not initialized
    int* unassigned_ptr; // bad: not initialized

    int x = 1; // good
    int y = 2; // good
    int *ptr = &x; // good
    struct Data data; // good

    ptr = 0; // bad: pointer assigned NULL
    ptr = NULL; // bad: pointer assigned NULL
    ptr = (void*)0; // bad: pointer assigned NULL
    ptr = (int*)0; // bad: pointer assigned NULL
    int ptr_is_null = ptr == NULL; // good

    data.ptr = 0; // bad: pointer field assigned NULL
    data.ptr = NULL; // bad: pointer field assigned NULL
    data.ptr = (void*)0; // bad: pointer field assigned NULL
    data.ptr = (int*)0; // bad: pointer field assigned NULL
    int data_ptr_is_null = data.ptr == NULL; // good

    takes_mut_int(0); // bad: NULL passed as argument
    takes_mut_int(NULL); // bad: NULL passed as argument
    takes_mut_int((void*)0); // bad: NULL passed as argument
    takes_mut_int((int*)0); // bad: NULL passed as argument

    arg = 10; // bad: argument reassigned
    ptrArg = &x; // bad: argument reassigned
    dataArg.value = 123; // bad: field of a by-value argument modified
    int* ptr_to_arg = &arg; // bad: pointer to non-const argument
    const int* const_ptr_to_arg = (const int*)&arg; // good: pointer to const

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

    float* zero_init_ptr = {0}; // bad: pointer initialized with {0}
    struct Data struct_with_ptr = {0}; // bad: object with pointers initialized with {0}
    struct DataNoPtrs simple_data_obj = {0}; // good

    struct Data init_data_with_null = { .ptr = NULL, .value = 5}; // bad: NULL in a pointer field initializer
}

void argument_address_test(
    int arg,
    const int const_arg,
    int *ptrArg,
    struct Data dataArg
)
{
    int x = 1; // good

    int* mut_ptr = &arg; // bad: pointer to non-const
    const int* const_ptr = &arg; // good: pointer to const
    int* cast_away_const = (int*)&const_arg; // bad: cast to pointer to non-const
    int* reassigned_ptr = &x; // good
    reassigned_ptr = &arg; // bad: pointer to non-const, in an assignment
    takes_mut_int(&arg); // bad: pointer to non-const, as a call argument
    takes_const_int(&arg); // good: pointer to const, as a call argument
    int* field_ptr = &dataArg.value; // bad: field of a by-value argument
    const int* const_field_ptr = &dataArg.value; // good: pointer to const
    int** ptr_arg_address = &ptrArg; // bad: the pointer argument itself could be changed
    int* const* const_ptr_arg_address = &ptrArg; // good: pointer to const pointer
    int* pointee_ptr = &*ptrArg; // good: points at the pointee, not the argument
    int* field_behind_ptr = &dataArg.ptr[0]; // good: points behind the argument
    int copy = arg; // good: a copy, no address taken
    int* ptr_copy = ptrArg; // good: a copy of the pointer value
    size_t address_size = sizeof(&arg); // good: never evaluated
    (void)&arg; // good: discarded
}

int* return_argument_address(int arg)
{
    return &arg; // bad: returned as pointer to non-const
}

static int global_unassigned_array[4]; // bad: array not initialized
static int global_assigned_array[4] = {1, 2, 3, 4}; // good

void array_test(void)
{
    int a = 1; // good
    int b = 2; // good

    int unassigned_int_array[3]; // bad: array not initialized
    char unassigned_char_array[16]; // bad: array not initialized
    float unassigned_float_array[8]; // bad: array not initialized
    int* unassigned_ptr_array[4]; // bad: array not initialized
    void* unassigned_void_ptr_array[2]; // bad: array not initialized
    float unassigned_matrix[2][2]; // bad: array not initialized
    int unassigned_3d_array[2][3][4]; // bad: array not initialized

    int assigned_int_array[3] = {1, 2, 3}; // good
    int zero_int_array[3] = {0}; // good
    char assigned_char_array[16] = "hello"; // good
    float assigned_float_array[2] = {1.0f, 2.0f}; // good
    int* assigned_ptr_array[2] = {&a, &b}; // good
    float assigned_matrix[2][2] = {{1.0f, 2.0f}, {3.0f, 4.0f}}; // good
    int inferred_size_array[] = {1, 2, 3, 4}; // good

    struct Data struct_array[2]; // good
    struct DataNoPtrs simple_struct_array[2]; // good

    struct Data* unassigned_struct_ptr_array[2]; // bad: array not initialized
    struct Data* assigned_struct_ptr_array[2] = {&struct_array[0], &struct_array[1]}; // good
    struct Data* assigned_struct_ptr_3_array[3] = {&struct_array[0], &struct_array[1]}; // bad because the third element is a pointer made null
    struct Data* assigned_struct_ptr_array_with_null[11] = { 0 }; // bad because the config forbids null assignment to pointers
    int* assigned_pointer_matrix[2][2] = {{&a, &b}, {&b, &a}}; // good
    int* partial_pointer_matrix[2][2] = {{&a, &b}, {&a}}; // bad: last element of the second row is missing
    int* missing_row_pointer_matrix[2][2] = {{&a, &b}}; // bad: the second row is missing
    int* explicit_null_pointer_array[2] = {&a, NULL}; // bad: explicit null element
    int* all_null_pointer_array[2] = {NULL, NULL}; // bad: both elements are null
    int* designated_pointer_array[3] = {[0] = &a, [2] = &b}; // bad: element 1 is missing
    int partial_int_array[5] = {1, 2}; // good: only arrays of pointers must be fully initialized
}