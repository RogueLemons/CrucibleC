#include <stddef.h>

#include "headers/ref_tag.h"

typedef int (*math_func_t)(int a, int b);

int add(int a, int b);
math_func_t get_math_func(void);

// -------------------------------------------------------------
// A reference to a function pointer can never be null
// -------------------------------------------------------------

int apply(REF math_func_t func, int a, int b)
{
    return func(a, b); // good: a reference needs no null check
}

int apply_twice(REF math_func_t func, int a)
{
    return apply(func, a, a) + apply(func, a, a); // good: a reference passed on
}

void callers(math_func_t maybe_null)
{
    apply(add, 1, 2); // good: a function is never null
    apply(&add, 1, 2); // good: the address of a function
    apply(NULL, 1, 2); // bad: NULL
    apply((void*)0, 1, 2); // bad: a null pointer
    apply(0, 1, 2); // bad: a null pointer
    apply((math_func_t)0, 1, 2); // bad: a null function pointer
    apply(maybe_null, 1, 2); // bad: a normal function pointer may be null
    apply(get_math_func(), 1, 2); // bad: a returned function pointer may be null
}
