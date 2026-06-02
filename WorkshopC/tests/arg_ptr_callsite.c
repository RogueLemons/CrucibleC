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
    //Good
    func_that_modifies(mod_cast(&c));
    // Bad
    func_that_modifies(out_cast(&c));
    func_that_modifies(&c);

    // Move test
    int* i_ptr = NULL;
    // Good
    func_that_takes_move(move_cast(i_ptr));
    // Bad
    func_that_takes_move(mod_cast(i_ptr));
    func_that_takes_move(i_ptr);

    // Out test
    float f;
    // Good
    func_that_gives_out(out_cast(&f));
    // Bad
    func_that_gives_out(move_cast(&f));
    func_that_gives_out(&f);

    // Functions without movement tags should not use operators
    // Good
    int i = 42;
    declared_in_external_header(&i);
    // Bad
    declared_in_external_header(move_cast(&i));
    declared_in_external_header(out_cast(&i));
    declared_in_external_header(mod_cast(&i));
}
