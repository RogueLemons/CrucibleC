#ifndef TESTS_HEADERS_GLOBAL_VARIABLE_STATE_H
#define TESTS_HEADERS_GLOBAL_VARIABLE_STATE_H

static int global_header_state = 0;     // bad: static in a header, every file including it gets its own copy

static inline int global_header_function(void)
{
    static int s_header_calls = 0;      // bad: static locals are treated as globals, so they may not be in a header either
    return ++s_header_calls + global_header_state;
}

#endif // TESTS_HEADERS_GLOBAL_VARIABLE_STATE_H
