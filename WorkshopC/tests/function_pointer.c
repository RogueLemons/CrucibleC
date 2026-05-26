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