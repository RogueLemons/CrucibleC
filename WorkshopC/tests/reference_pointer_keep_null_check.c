#include "headers/ref_tag.h"

// With disable_null_check_rule_for_reference_pointers set to false, the
// null check rule treats reference pointers like any other pointer

int unchecked_reference(REF const int* value)
{
    return *value; // bad: the null check is still required
}

int checked_reference(REF const int* value)
{
    if (!value)
        return 0;

    return *value; // good
}
