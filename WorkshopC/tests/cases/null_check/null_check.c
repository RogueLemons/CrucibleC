#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#include "external/function_without_nullcheck.h"
#include "headers/function_without_nullcheck.h"

#define IS_NULL(ptr) ((ptr) == ((void*)0))
#define ABORT_IF_NULL(ptr) do { int is_null_ = (ptr) == NULL; assert(is_null_ && "ptr may not be null"); } while(0)

float good_null_check(float* f_ptr_good)
{
    if (NULL == f_ptr_good)
    {
        return 0.0f;
    }
    return *f_ptr_good;
}

float good_check_null(float* f_ptr)
{
    if (f_ptr == NULL)
    {
        return 0.0f;
    }
    return *f_ptr;
}

static int is_null(void* ptr_1)
{
    int is_null_ = ptr_1 == NULL;
    return is_null_;
}

static int null_is(void* ptr_2)
{
    int null_is_ = NULL == ptr_2;
    return null_is_;
}

static char no_null_check(char* c_ptr)
{
    return *c_ptr;
}

static int function_with_nullcheck_macro(int* i_ptr)
{
    if (IS_NULL(i_ptr))
    {
        return 0;
    }
    return *i_ptr;
}

static double function_with_abort_macro(double* d_ptr)
{
    ABORT_IF_NULL(d_ptr);
    return *d_ptr;
}

static int access_before_nullcheck(int* arg_ptr)
{
    int i = *arg_ptr;
    if (arg_ptr == NULL)
    {
        return 0;
    }
    return i;
}

static float implicit_bool_cast_1(float* f_1)
{
    if (f_1)
    {
        return *f_1;
    }
    return 0.0f;
}

static float implicit_bool_cast_2(float* f_2)
{
    if (!f_2)
    {
        return 0.0f;
    }
    return *f_2;
}

static float implicit_bool_cast_3(float* f_3)
{
    return f_3 ? *f_3 : 0.0f;
}

static float implicit_bool_cast_4(float* f_4)
{
    return !f_4 ? 0.0f : *f_4;
}

#define THIS_IS_NULL ((void*)0)
static char self_made_null_macro(char* some_char)
{
    if (some_char == THIS_IS_NULL)
    {
        return '\0';
    }
    return *some_char;
}

typedef struct int_wrapper
{
    int value;
} int_wrapper;

static int field_access_without_null_check(const int_wrapper* wrap_1)
{
    return wrap_1->value;
}

static int field_access_without_null_check_2(const int_wrapper* wrap_2)
{
    int value = wrap_2->value;
    return value;
}

static int field_access_without_null_check_3(const int_wrapper* wrap_3)
{
    int_wrapper copy = (*wrap_3);
    return copy.value;
}

static int field_access_with_null_check(const int_wrapper* good_use_wrap)
{
    if (!good_use_wrap)
    {
        return 0;
    }
    return good_use_wrap->value;
}

// -------------------------------------------------------------
// A null check only counts in the scope it was made in, and in
// the scopes nested inside it.
// -------------------------------------------------------------

void takes_pointer(const int* p);

static int checked_before_nested_block(int* checked_outer, int flag)
{
    if (checked_outer == NULL)
        return 0;

    if (flag)
    {
        return *checked_outer; // good: checked in an enclosing scope
    }

    return 0;
}

static int checked_in_condition(int* checked_in_if)
{
    if (checked_in_if != NULL)
    {
        return *checked_in_if; // good: inside the checking if
    }

    return 0;
}

static int checked_else_branch(int* checked_else)
{
    if (checked_else == NULL)
    {
        return 0;
    }
    else
    {
        return *checked_else; // good
    }
}

static int checked_in_loop_condition(int* checked_loop)
{
    int sum = 0;

    while (checked_loop != NULL && *checked_loop > 0) // good: && checks the left side first
    {
        sum += *checked_loop; // good
        checked_loop = 0;
    }

    return sum;
}

static int checked_with_assert(int* asserted)
{
    assert(asserted != NULL);
    return *asserted; // good
}

static int checked_with_bool_and(int* bool_and)
{
    if (bool_and && *bool_and > 0) // good
        return 1;

    return 0;
}

static int checked_only_in_closed_scope(int* closed_scope, int flag)
{
    if (flag)
    {
        if (closed_scope == NULL)
            return 0;

        return *closed_scope; // good: checked in this scope
    }

    return *closed_scope; // bad: the check above was in a scope that has ended
}

static int checked_only_in_sibling_branch(int* sibling_branch, int flag)
{
    if (flag)
    {
        if (!sibling_branch)
            return 0;
    }
    else
    {
        return *sibling_branch; // bad: checked in the other branch only
    }

    return 0;
}

static int checked_only_inside_loop(int* inside_loop, int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (inside_loop == NULL)
            return 0;
    }

    return *inside_loop; // bad: the loop may not run at all
}

static int passing_is_not_checking(int* passed_on)
{
    takes_pointer(passed_on);
    return *passed_on; // bad: passing the pointer on is not a null check
}

static int subscript_is_a_dereference(int* subscripted)
{
    return subscripted[0]; // bad: subscript dereferences the pointer
}

static int every_parameter_is_reported(int* first_unchecked, int* second_unchecked)
{
    return *first_unchecked + *second_unchecked; // bad: both are reported
}


static int only_one_parameter_checked(int* checked_one, int* unchecked_other)
{
    if (checked_one == NULL)
        return 0;

    return *checked_one + *unchecked_other; // bad: only unchecked_other is reported, checking one pointer does not check the other
}

static int checked_as_bool_variables(int* checked_as_bool)
{
    bool is_not_null = checked_as_bool != NULL;

    if (is_not_null)
        return *checked_as_bool; // good: checked_as_bool is true only if checked_as_bool is not null

    return 0;
}