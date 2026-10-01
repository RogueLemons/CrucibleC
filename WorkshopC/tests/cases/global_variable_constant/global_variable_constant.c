// Globals as constants: mutable and const names use separate
// capitalization settings. Static locals use their own naming settings,
// and static globals are allowed in headers.

#include "headers/global_variable_constants.h"

typedef void (*callback_t)(void);

// Good

const int MAX_SIZE = 10;                                // good
static const float PI_VALUE = 3.14f;                    // good: static is allowed, not required
const int VERSION_2 = 2;                                // good: digits and underscores are fine
const char* const APP_NAME = "app";                     // good: const pointer to const
const int* const* const DEEP_POINTER = 0;               // good: const on every level
const int LOOKUP_TABLE[3] = {1, 2, 3};                  // good: array of const
const char* const NAMES[2] = {"a", "b"};                // good: array of const pointers to const
void (* const CALLBACK)(void) = 0;                      // good: a function can not be const itself
const callback_t CALLBACK_TYPEDEF = 0;                  // good
extern const int EXTERNAL_LIMIT;                        // good

int use_constants(int argument)
{
    static const int CALL_LIMIT = 5;                    // good: static local in capital letters and const
    static const char* const DEFAULT_NAME = "name";     // good
    int local = argument;                               // good: local variable, not static
    return MAX_SIZE + LOOKUP_TABLE[0] + local + CALL_LIMIT + HEADER_LIMIT + (DEFAULT_NAME ? 1 : 0);
}

// Bad

int counter = 0;                                        // bad: not capital letters, and not const
int MUTABLE_VALUE = 0;                                  // bad: not const
const int lower_case = 1;                               // bad: not capital letters
const int g_PREFIXED = 1;                               // bad: the prefix is not required, so 'g_' must be capital too
const char* NAME_POINTER = "name";                      // bad: the pointer itself is not const
char* const DATA_POINTER = 0;                           // bad: what it points to is not const
const char** const NESTED_POINTER = 0;                  // bad: the middle pointer is not const
int VALUES[3] = {1, 2, 3};                              // bad: array elements are not const
const int* POINTER_ARRAY[2] = {0, 0};                   // bad: the pointers in the array are not const

int use_bad_constants(void)
{
    static int calls = 0;                               // bad: not capital letters, and not const
    static const int lower_limit = 3;                   // bad: not capital letters
    static int MUTABLE_CALLS = 0;                       // bad: not const
    calls++;
    return calls + lower_limit + MUTABLE_CALLS;
}
