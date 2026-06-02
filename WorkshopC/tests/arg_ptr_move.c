#include "headers/move_tags.h"
#include "headers/missing_arg_ptr_tag.h"
#include "external/missing_arg_ptr_tag.h"
#include <string.h>

// Good
float no_pointer_function(float a, float b);
void declared_function_1(move int* moved_int_ptr);
void declared_function_2(out float* out_float_ptr);
void declared_function_3(mod char* mutable_char_ptr);
void declared_function_4(const double* const_double_ptr);

void defined_function_1(mod int* mod_int_ptr)
{
    *mod_int_ptr += 5;
}

int multiple_good_arguments(out float* out_float_ptr, mod char* mutable_char_ptr, const double* const_double_ptr);


// Ignored
// WorkshopC off
void ignored_function(double* double_ptr);
// WorkshopC on

// Bad
void declared_function_5(int* int_ptr_without_tag);

void defined_function_2(float* float_ptr_without_tag)
{
    *float_ptr_without_tag += 5.0f;
}

void mismatch_in_declaration_and_definition(move int* some_ptr);

void mismatch_in_declaration_and_definition(mod int* some_ptr)
{
    if (some_ptr)
    {
        *some_ptr *= 2;
    }
}

void header_and_source_tag_mismatch(mod float* f_ptr)
{
    *f_ptr = 5;
}

int multiple_bad_arguments(float* out_float_ptr, char* mutable_char_ptr, const double* const_double_ptr);

// Usage testing for callsite off
void foo(void)
{
    char c = 'c';
    // Good
    declared_function_3(&c);
    // Bad
    declared_function_3(mod_cast(&c));

    int* i_ptr = NULL;
    // Good
    declared_function_1(i_ptr);
    // Bad
    declared_function_1(move_cast(i_ptr));

    float f;
    // Good
    declared_function_2(&f);
    // Bad
    declared_function_2(out_cast(&f));
}