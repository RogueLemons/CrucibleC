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
    declared_function_3(&c); // good
    declared_function_3(mod_cast(&c)); // bad

    int* i_ptr = NULL;
    declared_function_1(i_ptr); // good
    declared_function_1(move_cast(i_ptr)); // bad: operator used while disabled, and i_ptr was already moved

    float f;
    declared_function_2(&f); // good
    declared_function_2(out_cast(&f)); // bad
}

// -------------------------------------------------------------
// A pointer may not be used after it has been moved, until it
// has been reassigned
// -------------------------------------------------------------

void take_ownership(move int* owned);
void create_int(out int** created);
void read_int(const int* value);
void modify_int(mod int* value);

void use_after_move(void)
{
    int* value = NULL;
    create_int(&value);
    take_ownership(value);  // good: moved
    read_int(value);        // bad: used after it was moved
}

void double_move(void)
{
    int* value = NULL;
    create_int(&value);
    take_ownership(value); // good
    take_ownership(value); // bad: moved a second time
}

void compared_after_move(void)
{
    int* value = NULL;
    create_int(&value);
    take_ownership(value);

    if (value != NULL) // bad: comparing is a use too
        modify_int(value); // bad
}

void reassigned_after_move(void)
{
    int* value = NULL;
    create_int(&value);
    take_ownership(value);
    create_int(&value); // good: out writes a new value into the pointer
    read_int(value); // good

    take_ownership(value);
    value = NULL; // good: plain reassignment
    read_int(value); // good
}

void address_and_sizeof_after_move(void)
{
    int* value = NULL;
    create_int(&value);
    take_ownership(value);

    unsigned long size = sizeof(value); // good: sizeof does not use the value
    int** address = &value; // good: taking the address does not read the value
}

void moved_parameter(move int* owned)
{
    take_ownership(owned); // good
    read_int(owned); // bad: parameters can be moved too
}

void moved_in_one_branch(int flag)
{
    int* value = NULL;
    create_int(&value);

    if (flag)
        take_ownership(value);

    read_int(value); // bad: may have been moved
}

void moved_in_branch_that_returns(int flag)
{
    int* value = NULL;
    create_int(&value);

    if (flag)
    {
        take_ownership(value);
        return;
    }

    read_int(value); // good: the branch that moved it returned
}

void moved_in_other_branch(int flag)
{
    int* value = NULL;
    create_int(&value);

    if (flag)
        take_ownership(value); // good
    else
        read_int(value); // good: the move happened in the other branch
}

void moved_in_loop(int count)
{
    int* value = NULL;
    create_int(&value);

    for (int i = 0; i < count; ++i)
        take_ownership(value); // bad: moved again in the next iteration
}

void moved_in_loop_then_break(int count)
{
    int* value = NULL;
    create_int(&value);

    for (int i = 0; i < count; ++i)
    {
        if (i == 3)
        {
            take_ownership(value);
            break;
        }
    }

    read_int(value); // bad: may have been moved before the break
}

void recreated_every_iteration(int count)
{
    for (int i = 0; i < count; ++i)
    {
        int* value = NULL;
        create_int(&value);
        take_ownership(value); // good: a new value in every iteration
    }
}

void moved_in_switch_case(int choice)
{
    int* value = NULL;
    create_int(&value);

    switch (choice)
    {
        case 1:
            take_ownership(value);
            break;
        default:
            read_int(value); // good: case 1 did not fall through
            break;
    }

    read_int(value); // bad: may have been moved in case 1
}

// -------------------------------------------------------------
// A mod or out parameter is only borrowed by the function, so it
// may not be moved away
// -------------------------------------------------------------

void move_modified_parameter(mod int* borrowed)
{
    take_ownership(borrowed); // bad: a mod parameter is not owned by the function
}

void move_out_parameter(out int** result)
{
    create_int(result); // good
    take_ownership(*result); // bad: what an out parameter points to is not owned by the function
}

void modify_borrowed_parameter(mod int* borrowed)
{
    modify_int(borrowed); // good: passing it on to be modified is fine
    read_int(borrowed); // good
}
