#ifndef TESTS_EXTERNAL_ASSIGNMENT_ARGUMENT_REASSIGNMENT_H
#define TESTS_EXTERNAL_ASSIGNMENT_ARGUMENT_REASSIGNMENT_H

static inline int external_reassign_argument(int value)
{
    value = value + 1;
    return value;
}

#endif