#include "headers/move_tags.h"
#include "external/missing_arg_ptr_tag.h"
#include <string.h>

// Good
float no_pointer_function(float a, float b);
void func_that_takes_move(move int* moved_int_ptr);
void func_that_gives_out(out float* out_float_ptr);
void func_that_modifies(mod char* mutable_char_ptr);
void func_that_reads_const(const double* const_double_ptr);

// Usage testing

void foo(void)
{
    // Mod test
    char c = 'c';
    func_that_modifies(mod_cast(&c)); // good
    func_that_modifies(out_cast(&c)); // bad: wrong operator, the parameter is mod
    func_that_modifies(&c); // bad: missing mod_cast

    // Move test
    int* i_ptr = NULL;
    func_that_takes_move(move_cast(i_ptr)); // good
    func_that_takes_move(mod_cast(i_ptr)); // bad: wrong operator, and i_ptr was already moved
    func_that_takes_move(i_ptr); // bad: missing operator, and i_ptr was already moved

    // Out test
    float f;
    func_that_gives_out(out_cast(&f)); // good
    func_that_gives_out(move_cast(&f)); // bad: wrong operator, the parameter is out
    func_that_gives_out(&f); // bad: missing out_cast

    // Functions without movement tags should not use operators
    int i = 42;
    declared_in_external_header(&i); // good
    declared_in_external_header(move_cast(&i)); // bad: operator on an untagged parameter
    declared_in_external_header(out_cast(&i)); // bad: operator on an untagged parameter
    declared_in_external_header(mod_cast(&i)); // bad: operator on an untagged parameter

    // System header
    const char* str = "Hello, world!";
    const char* str_2 = "Hello, world!";
    int result = strcmp(str, str_2);
    char buf[100];
    strcpy(buf, "hello");
    strcat(buf, " world");
}
