#include "headers/private_tag.h"

// -------------------------------------------------------------
// An interface struct holds exactly two private fields: a 'void*'
// named object, the object the functions work on, and a pointer to a
// const vtable struct named vtable. A const interface holds a
// 'const void* object' instead.
// -------------------------------------------------------------

struct shape_vtable
{
    double (*area)(const void *self);
    void (*scale)(void *self, double factor);
};
typedef struct shape_vtable shape_vtable_t;

struct circle
{
    double radius;
};

// Good

struct shape_interface
{
    PRIVATE void *object; // good
    PRIVATE const shape_vtable_t *vtable; // good
};

struct shape_const_interface
{
    PRIVATE const void *object; // good: a const interface holds a const void*
    PRIVATE const struct shape_vtable *vtable; // good: the struct name works too
};

struct shape_holder // good: not an interface, no requirements
{
    void *object;
    int count;
};

// Bad: the number of fields

struct extra_interface // bad: three fields
{
    PRIVATE void *object; // good
    PRIVATE const shape_vtable_t *vtable; // good
    PRIVATE int count;
};

struct lonely_interface // bad: only one field
{
    PRIVATE void *object; // good
};

// Bad: the object field

struct self_interface
{
    PRIVATE void *self; // bad: must be named object
    PRIVATE const shape_vtable_t *vtable; // good
};

struct typed_interface
{
    PRIVATE struct circle *object; // bad: must be a void*
    PRIVATE const shape_vtable_t *vtable; // good
};

struct mutable_const_interface
{
    PRIVATE void *object; // bad: a const interface must hold a const void*
    PRIVATE const shape_vtable_t *vtable; // good
};

struct readonly_interface
{
    PRIVATE const void *object; // bad: an interface must hold a void*, a const interface a const void*
    PRIVATE const shape_vtable_t *vtable; // good
};

// Bad: the vtable field

struct mutable_vtable_interface
{
    PRIVATE void *object; // good
    PRIVATE shape_vtable_t *vtable; // bad: the vtable must be const
};

struct named_interface
{
    PRIVATE void *object; // good
    PRIVATE const shape_vtable_t *functions; // bad: must be named vtable
};

struct not_vtable_interface
{
    PRIVATE void *object; // good
    PRIVATE const struct circle *vtable; // bad: not a vtable struct
};

struct swapped_interface
{
    PRIVATE const shape_vtable_t *vtable; // bad: the object comes first
    PRIVATE void *object; // bad: the vtable comes second
};

// Bad: not private

struct public_interface
{
    void *object; // bad: not marked private
    const shape_vtable_t *vtable; // bad: not marked private
};

// WorkshopC off
// Reason: Testing that PRIVATE is not needed
struct supressed_public_interface
{
    void *object;
    const shape_vtable_t *vtable; 
};
// WorkshopC on
