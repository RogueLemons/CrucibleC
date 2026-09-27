#include "headers/managed_structs.h"

// Pod struct arrays are allowed outside of structs when every element is
// initialized like a pod struct variable: from a function return value or
// another struct variable. No element may be filled in implicitly.

void good_arrays(void)
{
    pos_t origin = pos_pod(0, 0);
    pos_t other = pos_pod(1, 1);

    pos_t line[2] = { pos_pod(0, 0), pos_pod(1, 1) }; // good
    pos_t inferred[] = { pos_pod(0, 0), pos_pod(1, 1), pos_pod(2, 2) }; // good: the size comes from the list
    pos_t copies[2] = { origin, other }; // good: other struct variables
    pos_t grid[2][2] = { { origin, other }, { other, origin } }; // good: array of arrays
    pos_t rows[][2] = { { origin, other }, { other, origin } }; // good
    pos_t cube[2][1][2] = { { { origin, other } }, { { other, origin } } }; // good: array of array of arrays
    pos_t mixed[3] = { origin, pos_pod(1, 1), other }; // good: mixing struct variables and function return values
}

void bad_arrays(void)
{
    pos_t origin = pos_pod(0, 0);
    pos_t other = pos_pod(1, 1);

    pos_t uninitialized[2]; // bad: not initialized
    pos_t zeroed[2] = { 0 }; // bad: '{0}' on the whole array
    pos_t zeroed_elements[2] = { { 0 }, { 0 } }; // bad: '{0}' on the elements
    pos_t literal_elements[2] = { { 1, 2 }, { 3, 4 } }; // bad: brace literals
    pos_t compound_elements[2] = { (pos_t){ 1, 2 }, (pos_t){ 3, 4 } }; // bad: compound literals
    pos_t missing[3] = { origin, other }; // bad: the third element is filled in implicitly
    pos_t designated[3] = { [0] = origin, [2] = other }; // bad: the second element is filled in implicitly
    pos_t grid_missing_element[2][2] = { { origin, other }, { other } }; // bad: an element of the second row is missing
    pos_t grid_missing_row[2][2] = { { origin, other } }; // bad: the second row is missing
    pos_t cube_zeroed[2][1][2] = { { { origin, other } }, { { 0 } } }; // bad: '{0}' deep inside
}

void raii_arrays(void)
{
    int_vector_t vectors[1] = { int_vector_make(1) }; // bad: raii arrays are still only allowed inside structs
}

void big_matrix(void)
{
    pos_t a = pos_pod(0, 0);
    pos_t b = pos_pod(1, 1);

    // good: a 5 dimensional matrix with every one of its 32 elements initialized
    pos_t matrix[2][2][2][2][2] = {
        {
            {
                { { a, b }, { a, b } },
                { { a, b }, { a, b } }
            },
            {
                { { a, b }, { a, b } },
                { { a, b }, { a, b } }
            }
        },
        {
            {
                { { a, b }, { a, b } },
                { { a, b }, { a, b } }
            },
            {
                { { a, b }, { a, b } },
                { { a, b }, { a, b } }
            }
        }
    };

    // bad: the very last element is missing
    pos_t matrix_missing[2][2][2][2][2] = {
        {
            {
                { { a, b }, { a, b } },
                { { a, b }, { a, b } }
            },
            {
                { { a, b }, { a, b } },
                { { a, b }, { a, b } }
            }
        },
        {
            {
                { { a, b }, { a, b } },
                { { a, b }, { a, b } }
            },
            {
                { { a, b }, { a, b } },
                { { a, b }, { a } }
            }
        }
    };
}
