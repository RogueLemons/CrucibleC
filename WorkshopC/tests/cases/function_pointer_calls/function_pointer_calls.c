#include <stddef.h>

#include "headers/move_tags.h"
#include "headers/ref_tag.h"

struct point
{
    int x;
    int y;
};
typedef struct point point;

typedef void (*mover_t)(move point* value);
typedef void (*filler_t)(out point** value);
typedef void (*scaler_t)(mod point* value, int factor);
typedef int (*measurer_t)(REF const point* value);

struct operations
{
    mover_t move_point;
    filler_t fill_point;
    scaler_t scale_point;
    measurer_t measure_point;
};
typedef struct operations operations;

// -------------------------------------------------------------
// A call through a plain function pointer follows the tags of
// its type, like a call to a function
// -------------------------------------------------------------

void plain_calls(mover_t mover, filler_t filler, scaler_t scaler, measurer_t measurer)
{
    point local = { 1, 2 };
    point* value = NULL;

    filler(out_cast(&value)); // good: the operator of an out parameter
    filler(&value); // bad: missing out_cast
    scaler(mod_cast(value), 2); // good: the operator of a modify parameter
    scaler(value, 2); // bad: missing mod_cast
    scaler(move_cast(value), 2); // bad: the wrong operator for a modify parameter
    measurer(&local); // good: the address of an object for a reference
    measurer(value); // bad: a normal pointer may be null
    measurer(NULL); // bad: a reference can never be null
    mover(move_cast(value)); // good: ownership moves through the function pointer
    scaler(mod_cast(value), 3); // bad: value was moved to mover
}

void local_function_pointer(void)
{
    point* value = NULL;
    filler_t filler = NULL;

    filler(&value); // bad: missing out_cast, also for a local function pointer
}

int pass_on_reference(REF const point* value, measurer_t measurer)
{
    return measurer(value); // good: a reference passed on
}

void pass_on_borrowed(mod point* value, mover_t mover)
{
    mover(move_cast(value)); // bad: a modify parameter is only borrowed, it can not be moved
}

// -------------------------------------------------------------
// A call through a struct field follows the tags of the field's
// type, through a pointer or a value alike
// -------------------------------------------------------------

void field_calls(const operations* ops, operations copy)
{
    point local = { 1, 2 };
    point* value = NULL;

    if (!ops)
        return;

    ops->fill_point(out_cast(&value)); // good
    ops->fill_point(&value); // bad: missing out_cast
    ops->scale_point(value, 2); // bad: missing mod_cast
    copy.scale_point(mod_cast(value), 2); // good: through a struct value
    copy.scale_point(move_cast(value), 2); // bad: the wrong operator for a modify parameter
    ops->measure_point(&local); // good
    ops->measure_point(NULL); // bad: a reference can never be null
    copy.measure_point(value); // bad: a normal pointer may be null
    ops->move_point(move_cast(value)); // good: ownership moves through the field
    copy.scale_point(mod_cast(value), 3); // bad: value was moved to move_point
}

int field_pass_on_reference(REF const point* value, const operations* ops)
{
    return ops ? ops->measure_point(value) : 0; // good: a reference passed on
}

void field_pass_on_borrowed(mod point* value, const operations* ops)
{
    if (ops)
        ops->move_point(move_cast(value)); // bad: a modify parameter is only borrowed, it can not be moved
}
