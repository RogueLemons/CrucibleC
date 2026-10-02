#include <string.h>

void log_message(const char* message);
void log_const_pointer(const char* const message);
void log_bytes(const unsigned char* bytes);
void overwrite_message(char* message);
void log_count(int count, ...);

typedef void (*logger_function)(const char* message);
typedef void (*writer_function)(char* message);

void test_string_literals_forbidden(logger_function logger, writer_function writer)
{
    log_message("hello");                   // bad: the option is off
    log_message(("parenthesized"));         // bad: the option is off
    log_const_pointer("hello");             // bad: the option is off
    logger("through a function pointer");   // bad: the option is off
    size_t length = strlen("hello");        // good: standard library

    overwrite_message("hello");             // bad: the literal could be written to
    writer("through a function pointer");   // bad: the literal could be written to
    log_count(1, "variadic");               // bad: no parameter type to check

    char copy[] = "hello";
    log_message(copy);                      // bad: an array, not a literal
}
