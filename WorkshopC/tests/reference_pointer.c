#include <stddef.h>

#include "headers/ref_tag.h"

struct point
{
    int x;
    int y;
};
typedef struct point point_t;

struct shape
{
    point_t center;
    point_t corners[4];
};
typedef struct shape shape_t;

int read_value(REF const int* value);
void set_value(REF int* value, int new_value);
int read_point_x(REF const point_t* point);
size_t text_length(REF const char* text);
int* find_value(void);

// -------------------------------------------------------------
// A reference can never be null, so it needs no null check
// -------------------------------------------------------------

int dereference_reference(REF const int* value)
{
    return *value; // good: disable_null_check_rule_for_reference_pointers is true
}

int dereference_normal_pointer(const int* value)
{
    return *value; // bad: a normal pointer still needs a null check
}

// -------------------------------------------------------------
// The argument for a reference must be the address of an object
// or another reference
// -------------------------------------------------------------

void good_arguments(REF int* reference, REF shape_t* shape_reference)
{
    int local = 1;
    point_t point = {1, 2};
    int numbers[3] = {1, 2, 3};
    char buffer[8] = "text";
    shape_t shape;

    set_value(&local, 2); // good: the address of a variable
    read_point_x(&point); // good
    read_value(&point.x); // good: the address of a field of a variable
    read_value(&numbers[1]); // good: the address of an array element
    read_point_x(&shape.corners[2]); // good
    text_length(buffer); // good: an array decays to a pointer to its first element
    text_length("literal"); // good: a string literal
    set_value(reference, 3); // good: a reference passed on
    read_value(&*reference); // good
    read_point_x(&shape_reference->center); // good: a field reached through a reference
    *reference = 4; // good: writing through a reference is fine
}

void bad_arguments(int* pointer, point_t* point_pointer)
{
    if (!pointer || !point_pointer)
        return;

    int* local_pointer = pointer;

    set_value(pointer, 1); // bad: a normal pointer may be null
    set_value(local_pointer, 1); // bad: a copy of a normal pointer may be null
    set_value(NULL, 1); // bad: NULL is never a valid reference
    set_value(find_value(), 1); // bad: a returned pointer may be null
    read_value(&point_pointer->x); // bad: reached through a normal pointer
    read_value(&pointer[0]); // bad: indexing a pointer, not an array
}

// -------------------------------------------------------------
// A reference may not be reassigned
// -------------------------------------------------------------

void reassign_reference(REF int* reference, REF int* other)
{
    reference = other; // bad: reference reassigned
    reference++; // bad: reference reassigned
    reference += 1; // bad: reference reassigned
    --reference; // bad: reference reassigned
}

// -------------------------------------------------------------
// The reference tag only belongs on pointers
// -------------------------------------------------------------

void tag_on_value(REF int value); // bad: not a pointer

void suppressed(int* pointer)
{
    if (!pointer)
        return;

    // WorkshopC off
    set_value(pointer, 1); // good: suppressed, no diagnostic expected
    // WorkshopC on
}

// -------------------------------------------------------------
// The reference tag may only be used on function parameters
// -------------------------------------------------------------

REF int* tagged_global = NULL; // bad: a global variable
REF int* tagged_function(void); // bad: the tag applies to the function itself

struct tagged_field_holder
{
    REF int* field; // bad: a struct field
};

void tagged_local(void)
{
    int value = 0;
    REF int* local = &value; // bad: a local variable
}

// Calls through function pointers are not checked yet, so the tag is not
// allowed on the parameters of function pointer types either

typedef void (*reference_callback)(REF int* value); // bad: a parameter of a function pointer type
void register_reference_callback(void (*callback)(REF int* value)); // bad: a parameter of a function pointer parameter

void reference_local_callback(void)
{
    void (*callback)(REF int* value) = NULL; // bad: a function pointer variable inside a function
}

typedef void (*plain_reference_callback)(int* value); // good
