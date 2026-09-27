#include <stddef.h>

#include "headers/move_tags.h"
#include "headers/ref_tag.h"

// The reference tag and the argument pointer movement tags exist side by
// side on the same parameter, in any order

void scale(mod REF float* value, float factor); // good: both tags
void reset(REF mod float* value); // good: in any order
void read_only(REF const float* value); // good: const needs no movement tag
void missing_movement_tag(REF float* value); // bad: REF is not a movement tag, so a mutable pointer still needs one
void create_float(REF out float** created); // good
void take_ownership(move float* owned);

void compatible_calls(void)
{
    float value = 1.0f;

    scale(mod_cast(&value), 2.0f); // good: both the operator and the reference are satisfied
    reset(mod_cast(&value)); // good
    read_only(&value); // good

    float* pointer = NULL;
    create_float(out_cast(&pointer)); // good
    scale(mod_cast(pointer), 2.0f); // bad: a pointer variable is not a reference
    scale(&value, 2.0f); // bad: the mod operator is missing

    take_ownership(move_cast(pointer)); // good
    create_float(out_cast(&pointer)); // good: out gives the pointer a new value, also with REF written first
    read_only(pointer); // bad: not a reference, but not a use after move either
}
