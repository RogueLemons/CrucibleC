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

/*
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

    // Cleanup missing for local1
    int_vector_destroy(&local2);
    int_vector_destroy(&arg);
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

void test_if_both_clean(int i)
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
            int_vector_destroy(&inner);
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
*/