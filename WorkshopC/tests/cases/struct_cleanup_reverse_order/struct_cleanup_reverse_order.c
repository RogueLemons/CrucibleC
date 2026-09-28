#include <stddef.h>

#include "headers/managed_structs.h"

void int_vector_destroy_array(int_vector_t* self, size_t n);

void destroys_in_reverse_order(void)
{
    int_vector_t a = int_vector_make(1);
    int_vector_t b = int_vector_make(2);
    int_vector_t c = int_vector_make(3);

    int_vector_destroy(&c); // good
    int_vector_destroy(&b); // good
    int_vector_destroy(&a); // good
}

void destroys_in_declaration_order(void)
{
    int_vector_t a = int_vector_make(1);
    int_vector_t b = int_vector_make(2);
    int_vector_t c = int_vector_make(3);

    int_vector_destroy(&a); // bad: b and c were declared later
    int_vector_destroy(&b); // bad: c was declared later
    int_vector_destroy(&c); // good
}

void destroys_nested_scopes_in_reverse_order(void)
{
    int_vector_t outer = int_vector_make(1);

    {
        int_vector_t inner = int_vector_make(2);
        int_vector_destroy(&inner); // good
    }

    int_vector_destroy(&outer); // good
}

void destroys_through_a_deep_scope_maze(void)
{
    int_vector_t root = int_vector_make(1);

    {
        int_vector_t first = int_vector_make(2);

        if (first.size > 0)
        {
            int_vector_t second = int_vector_make(3);

            {
                int_vector_t third = int_vector_make(4);
                int_vector_t fourth = int_vector_make(5);

                int_vector_destroy(&fourth); // good
                int_vector_destroy(&third);  // good
            }

            if (second.size > 0)
            {
                int_vector_t fifth = int_vector_make(6);
                int_vector_destroy(&fifth); // good
            }
            else
            {
                int_vector_t sixth = int_vector_make(7);
                int_vector_destroy(&sixth); // good
            }

            int_vector_destroy(&second); // good
        }

        while (0)
        {
            int_vector_t loop_value = int_vector_make(8);
            int_vector_destroy(&loop_value); // good
        }

        int_vector_destroy(&first); // good
    }

    int_vector_destroy(&root); // good
}

void destroys_outer_before_deep_inner(void)
{
    int_vector_t outer = int_vector_make(1);

    {
        int_vector_t middle = int_vector_make(2);

        {
            int_vector_t inner = int_vector_make(3);
            int_vector_destroy(&middle); // bad: inner is still live
            int_vector_destroy(&inner);  // good
        }
    }

    int_vector_destroy(&outer); // good
}

void destroys_mixed_scalar_and_array_in_reverse_order(void)
{
    int_vector_t first = int_vector_make(1);
    int_vector_t values[2] = {
        int_vector_make(2),
        int_vector_make(3)
    };
    int_vector_t last = int_vector_make(4);

    int_vector_destroy(&last);       // good
    int_vector_destroy_array(values, 2); // good
    int_vector_destroy(&first);       // good
}

void destroys_mixed_scalar_and_array_in_wrong_order(void)
{
    int_vector_t first = int_vector_make(1);
    int_vector_t values[2] = {
        int_vector_make(2),
        int_vector_make(3)
    };
    int_vector_t last = int_vector_make(4);

    int_vector_destroy(&first);       // bad: values and last were declared later
    int_vector_destroy_array(values, 2); // bad: last was declared later
    int_vector_destroy(&last);        
}

void pod_and_free_structs_do_not_affect_reverse_order(void)
{
    int_vector_t first = int_vector_make(1);
    pos_t pod_value = pos_pod(2, 3);
    free_struct_t free_value;
    free_struct_init(&free_value, 4, 5);
    int_vector_t last = int_vector_make(6);

    int_vector_destroy(&last);  // good
    pos_t pod_in_the_middle = pos_pod(7, 8);
    int_vector_destroy(&first); // good: POD and free values are not RAII values


}

void pod_and_free_structs_do_not_hide_order_errors(void)
{
    int_vector_t first = int_vector_make(1);
    pos_t pod_value = pos_pod(2, 3);
    int_vector_t last = int_vector_make(4);
    free_struct_t free_value;
    free_struct_init(&free_value, 5, 6);

    int_vector_destroy(&first); // bad: last is a later RAII declaration
    pos_t pod_in_the_middle = pos_pod(7, 8);
    int_vector_destroy(&last);  // good
}

void destroys_raii_by_value_argument_after_local(int_vector_t argument)
{
    int_vector_t local = int_vector_make(1);

    int_vector_destroy(&local);    // good: local was declared after argument
    int_vector_destroy(&argument); // good
}

void destroys_raii_by_value_argument_before_local(int_vector_t argument)
{
    int_vector_t local = int_vector_make(1);

    int_vector_destroy(&argument); // bad: local was declared later
    int_vector_destroy(&local);    // good
}

int_vector_t return_function_counts_as_destroy(int_vector_t argument)
{
    int_vector_t local = int_vector_make(1);

    int_vector_destroy(&local);    // good: local is destroyed before argument
    return int_vector_return(&argument); // good: return transfers argument ownership
}

int_vector_t return_function_destroys_too_late(int_vector_t argument)
{
    int_vector_t local = int_vector_make(1);

    int_vector_destroy(&argument); // bad: local is declared later
    return int_vector_return(&local); // good: return transfers local ownership
}
