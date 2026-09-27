static int add(int a, int b)
{
    return a + b;
}

static int multiply(int a, int b)
{
    return a * b;
}

typedef int (*math_function)(int, int);

void perform_math(math_function math_func, int a, int b, int* out_res)
{
    *out_res = math_func(a, b);
}

void bad_perform_math(int (*math_func)(int, int), int a, int b, int* out_res)
{
    *out_res = math_func(a, b);
}

void perform_add(int a, int b, int* out_res)
{
    math_function add_func = add;
    *out_res = add_func(a, b);
}

void bad_perform_add(int a, int b, int* out_res)
{
    int (*add_func)(int, int) = add;
    *out_res = add_func(a, b);
}

typedef unsigned long size_type;
typedef size_type (*size_function)(int);

struct callbacks
{
    math_function typed_field; // good
    int (*raw_field)(int, int); // bad: struct field without a typedef
    math_function typed_array_field[2]; // good
};
typedef struct callbacks callbacks;

math_function typed_functions[2] = {add, multiply}; // good
int (*raw_functions[2])(int, int) = {add, multiply}; // bad: array of function pointers without a typedef
math_function* typed_pointer = 0; // good: pointer to a typedef function pointer
int (**raw_pointer)(int, int) = 0; // bad: pointer to a function pointer without a typedef
size_type (*raw_with_typedef_return)(int) = 0; // bad: only the return type is a typedef
size_function typed_size_function = 0; // good

math_function get_typed_function(void) // good
{
    return add;
}

int (*get_raw_function(void))(int, int) // bad: returns a function pointer without a typedef
{
    return add;
}
