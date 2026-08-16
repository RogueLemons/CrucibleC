#include <stddef.h>
#include <assert.h>
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