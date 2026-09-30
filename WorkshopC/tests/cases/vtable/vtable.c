#include <stddef.h>

// -------------------------------------------------------------
// A vtable struct holds only function pointers that take a void* or
// const void* first, the object the function works on. A vtable
// variable is static const and initialized at declaration with braces,
// with a function for every function pointer. With struct resource
// management on, a vtable without a creator function is a free struct.
// -------------------------------------------------------------

typedef void (*shape_print_fn)(const void *self);

struct shape_vtable // good: a free struct without a creator function
{
    double (*area)(const void *self); // good
    void (*scale)(void *self, double factor); // good
    shape_print_fn print; // good: through a typedef
    void (*release)(void *const self); // good: a const parameter is still a void*
};
typedef struct shape_vtable shape_vtable_t;

struct circle;

struct bad_vtable
{
    double (*area)(const void *self); // good
    int count; // bad: not a function pointer
    void *object; // bad: a data pointer, not a function pointer
    void (*draw)(struct circle *self); // bad: the first parameter is not a void*
    void (*reset)(void); // bad: no parameters
    void (*tick)(); // bad: no prototype
    void (*poke)(volatile void *self); // bad: a volatile void*
};

struct lonely // bad: not a vtable, so it still needs a creator function
{
    int value;
};

static double circle_area(const void *self);
static void circle_scale(void *self, double factor);
static void circle_print(const void *self);
static void circle_release(void *const self);

static const shape_vtable_t circle_vtable = { circle_area, circle_scale, circle_print, circle_release }; // good

static const shape_vtable_t designated_vtable = // good: designated elements and '&function'
{
    .area = circle_area,
    .scale = &circle_scale,
    .print = circle_print,
    .release = circle_release,
};

const shape_vtable_t not_static_vtable = { circle_area, circle_scale, circle_print, circle_release }; // bad: not static
static shape_vtable_t not_const_vtable = { circle_area, circle_scale, circle_print, circle_release }; // bad: not const
extern const shape_vtable_t extern_vtable; // bad: not static
static const shape_vtable_t uninitialized_vtable; // bad: not initialized
static const shape_vtable_t missing_vtable = { circle_area, circle_scale }; // bad: missing 'print' and 'release'

static const shape_vtable_t null_vtable =
{
    circle_area,
    NULL, // bad: NULL is not a function
    circle_print,
    circle_release,
};

static const shape_vtable_t zero_vtable =
{
    circle_area,
    circle_scale,
    0, // bad: 0 is not a function
    circle_release,
};

void use_vtable(shape_vtable_t vtable); // bad: a vtable passed by value

void use_vtable_pointer(const shape_vtable_t *vtable); // good: a pointer to a vtable

void test_vtable_variables(void)
{
    static const shape_vtable_t local_vtable = { circle_area, circle_scale, circle_print, circle_release }; // good: a static local
    const shape_vtable_t *pointer = &circle_vtable; // good: a pointer to a vtable

    shape_vtable_t copy = circle_vtable; // bad: not static const, and not initialized with braces

    use_vtable_pointer(pointer);
    use_vtable_pointer(&local_vtable);
    use_vtable_pointer(&copy);
    use_vtable_pointer(&designated_vtable);
    use_vtable_pointer(&not_static_vtable);
    use_vtable_pointer(&not_const_vtable);
    use_vtable_pointer(&uninitialized_vtable);
    use_vtable_pointer(&missing_vtable);
    use_vtable_pointer(&null_vtable);
    use_vtable_pointer(&zero_vtable);
}
