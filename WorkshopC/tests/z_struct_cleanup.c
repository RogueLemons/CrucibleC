#include "headers/managed_structs.h"
#include "external/some_struct.h"

void int_vector_foo(int_vector_t* self);

void detect_no_cleanup()
{
    int_vector_t no_cleanup_var = int_vector_make(10);
    int_vector_foo(&no_cleanup_var);
    // Cleanup is missing here, should be:
    // int_vector_destroy(&no_cleanup_var);
}


void detect_no_cleanup_of_arg(int_vector_t no_cleanup_arg)
{
    int_vector_foo(&no_cleanup_arg);
    // Cleanup is missing here, should be:
    // int_vector_destroy(&no_cleanup_arg);
}

int detect_many_missing_cleanups(int_vector_t arg1, int_vector_t arg2)
{
    int_vector_t local1 = int_vector_make(10);
    int_vector_t local2 = int_vector_copy(&arg1);
    int_vector_t local3 = int_vector_move(&arg2);

    // Cleanup is missing for arg1, arg2, local1, local2, local3
    return 0;
}


void complex_missing_cleanup(int i, int_vector_t arg)
{
    int_vector_t local1 = int_vector_make(10);
    int_vector_t local2 = int_vector_copy(&arg);

    if (i > 0) {
        int_vector_t local3 = int_vector_move(&local1);
        int_vector_t local4 = int_vector_copy(&local3);

        // Cleanup is missing for local1, local2, local3
        int_vector_destroy(&arg);
        int_vector_destroy(&local4);
        return;
    }

    // Cleanup missing for arg, local1
    int_vector_destroy(&local2);
}


void complex_missing_cleanup_2(int i)
{
    int_vector_t local_vec_1 = int_vector_make(10);

    for (int j = 0; j < i; ++j) {
        int_vector_t local_vec_2 = int_vector_copy(&local_vec_1);

        if (j % 2 == 0) {
            int_vector_t local_vec_3 = int_vector_move(&local_vec_2);
            // Cleanup missing for local_vec_3
            int_vector_destroy(&local_vec_1);
            int_vector_destroy(&local_vec_2);
            continue;
        }

        if (i > 10)
        {
            int_vector_destroy(&local_vec_2);
            break;
        }
        // Cleanup missing for local_vec_2
    }
    int_vector_destroy(&local_vec_1);
}

void nested_cleanup_test_with_all_cleanups(int i, int j)
{
    int_vector_t outer = int_vector_make(10);

    if (i > 0) {
        int_vector_t middle = int_vector_make(20);

        if (j > 0) {
            int_vector_t inner = int_vector_make(30);

            int_vector_destroy(&inner);
            int_vector_destroy(&middle);
            int_vector_destroy(&outer);
            return;
        }

        int_vector_destroy(&middle);
    }

    int_vector_destroy(&outer);
}

void test_that_both_clean(int i)
{
    int_vector_t local = int_vector_make(10);

    if (i > 0) {
        int_vector_destroy(&local);
    }
    else {
        int_vector_destroy(&local);
    }
}

void test_loop_multiple_paths_bad(int i)
{
    int_vector_t outer = int_vector_make(10);

    for (int j = 0; j < i; ++j) {
        int_vector_t inner = int_vector_make(5);

        if (j == 0) {
            // int_vector_destroy(&inner);
            continue;
        }

        if (j == 1) {
            // int_vector_destroy(&inner);
            break;
        }

        int_vector_destroy(&inner);
    }

    int_vector_destroy(&outer);
}

void test_loop_multiple_paths_good(int i)
{
    int_vector_t outer = int_vector_make(10);

    for (int j = 0; j < i; ++j) {
        int_vector_t inner = int_vector_make(5);

        if (j == 0) {
            int_vector_destroy(&inner);
            continue;
        }

        if (j == 1) {
            int_vector_destroy(&inner);
            break;
        }

        int_vector_destroy(&inner);
    }

    int_vector_destroy(&outer);
}

int_vector_t return_an_int_vector()
{
    return int_vector_make(7);
}

void forgotten_arg_cleanup(int i, int_vector_t forgot_to_clean_before_return)
{
    if (i == 1)
        return;
    else if (i == 2)
        return;
    else if (i == 3)
        return;

    return;
}

void nested_if_handling(int i)
{
    int_vector_t int_vec = int_vector_make(80);
    if (i > 0)
    {
        if (i < 10)
        {
            int_vector_destroy(&int_vec);
        }
    }
}

void nested_if_handling_2(int i, int_vector_t arg_vec)
{
    if (i > 0)
    {
        if (i < 10)
            int_vector_destroy(&arg_vec);
        else
            int_vector_destroy(&arg_vec);
    }
    else
    {
        int_vector_destroy(&arg_vec);
    }
}

void double_nested_scope()
{

    {
        int_vector_t double_nested_vec_1 = int_vector_make(10);
        // missing destroy
    }

    {
        int_vector_t double_nested_vec_2 = int_vector_make(10);
        int_vector_destroy(&double_nested_vec_2);
    }

}

void missing_cleanup_on_break()
{
    int i = 10;
    while (i < 100)
    {
        int_vector_t vec_in_while = int_vector_make(i);
        if (i > 50)
        {
            break;
        }
        int_vector_destroy(&vec_in_while);
        i++;
    }
}

void missing_cleanup_on_continue()
{
    int i = 10;
    while (i < 100)
    {
        int_vector_t vec_in_while_2 = int_vector_make(i);
        if (i > 50)
        {
            continue;
        }
        int_vector_destroy(&vec_in_while_2);
        i++;
    }
}

void missing_cleanup_on_switch_break()
{
    int i = 10;
    while (i < 100)
    {
        switch (i)
        {
            case 50:
            {
                int_vector_t vec_in_switch = int_vector_make(i);
                break;
            }

            default:
                break;
        }

        i++;
    }
}

void switch_break_should_not_break_loop()
{
    int i = 10;
    while (i < 100)
    {
        switch (i)
        {
            case 50:
                break;

            default:
                break;
        }

        int_vector_t vec_after_switch = int_vector_make(i);
        int_vector_destroy(&vec_after_switch);

        i++;
    }
}

void missing_cleanup_after_while()
{
    int i = 10;
    int_vector_t vec_before_while = int_vector_make(i);

    while (i < 100)
    {
        int_vector_destroy(&vec_before_while);
        i++;
    }
}

void missing_cleanup_after_loop()
{
    int i = 10;
    int_vector_t vec_before_loop = int_vector_make(i);

    while (i < 100)
    {
        int_vector_destroy(&vec_before_loop);
        break;
    }

    i++;
}

void missing_cleanup_on_multiple_returns(int value)
{
    int_vector_t vec = int_vector_make(value);

    if (value < 10)
    {
        return;
    }

    if (value > 50)
    {
        return;
    }

    int_vector_destroy(&vec);
}

void missing_cleanup_on_return(int_vector_t vec)
{
    int_vector_t local_vec = int_vector_make(10);

    if (local_vec.size > 5)
    {
        return;
    }

    int_vector_destroy(&local_vec);
    int_vector_destroy(&vec);
}

void cleanup_before_return(int_vector_t vec)
{
    int_vector_destroy(&vec);

    if (vec.size > 5)
    {
        return;
    }
}

void cleanup_on_all_paths(int_vector_t vec, int value)
{
    if (value > 50)
    {
        int_vector_destroy(&vec);
        return;
    }

    int_vector_destroy(&vec);
}

void missing_cleanup_on_one_branch(int_vector_t vec, int value)
{
    if (value > 50)
    {
        int_vector_destroy(&vec);
    }
}

void duplicate_cleanup(int_vector_t double_destroyed_vec)
{
    int_vector_destroy(&double_destroyed_vec);
    int_vector_destroy(&double_destroyed_vec);
}

void cleanup_on_nested_scope()
{
    int i = 10;

    {
        int_vector_t vec_in_scope = int_vector_make(i);
        int_vector_destroy(&vec_in_scope);
    }
}

void missing_cleanup_on_nested_return(int_vector_t vec_arg_with_inner_scope_return)
{
    {
        int_vector_t local_vec_in_scope = int_vector_make(10);
        return;
    }
}

void missing_cleanup_on_nested_continue()
{
    int i = 10;

    while (i < 100)
    {
        {
            int_vector_t vec_in_continue_scope = int_vector_make(i);
            continue;
        }

        i++;
    }
}

void missing_cleanup_on_nested_break()
{
    int i = 10;

    while (i < 100)
    {
        {
            int_vector_t vec_in_break_scope = int_vector_make(i);
            break;
        }

        i++;
    }
}

void missing_cleanup_complex_flow(int_vector_t vec_arg_complex_flow, int value)
{
    int_vector_t local_vec_complex_flow = int_vector_make(value);

    if (value < 10)
    {
        return;
    }

    while (value < 100)
    {
        if (value > 50)
        {
            break;
        }

        if (value == 25)
        {
            continue;
        }

        int_vector_destroy(&local_vec_complex_flow);
        value++;
    }

    if (value > 75)
    {
        return;
    }

    int_vector_destroy(&vec_arg_complex_flow);
}