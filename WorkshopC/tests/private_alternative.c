#include "headers/private_tag.h"

struct Container
{
    int public_field;
    PRIVATE int count;
    PRIVATE int total;
};
typedef struct Container Container_t;

// -------------------------------------------------------------
// Good: the pod creator function reaches the tagged fields
// through a plain (non-pointer) local value of its own struct
// type, which is allowed even though it takes no pointer 'self'
// parameter at all.
// -------------------------------------------------------------
static Container_t Container_pod(int count, int total)
{
    Container_t self = {0};
    self.public_field = 0;
    self.count = count;
    self.total = total;
    return self;
}

struct Wrapper
{
    Container_t public_inner;
    PRIVATE Container_t inner;
};
typedef struct Wrapper Wrapper_t;

// -------------------------------------------------------------
// Wrapper is a free struct and requires an init function
// -------------------------------------------------------------
static int Wrapper_init(Wrapper_t* self)
{
    if (!self)
        return 0;

    self->public_inner = Container_pod(0, 0);
    self->inner = Container_pod(0, 0);
    return 1;
}

struct PointerWrapper
{
    int tag;
    PRIVATE Container_t* container;
};
typedef struct PointerWrapper PointerWrapper_t;

// -------------------------------------------------------------
// PointerWrapper is a raii struct: the struct resource management
// rule requires these lifecycle functions to exist, but this file
// is not meant to exercise that rule. Each one reaches the tagged
// field through its own pointer 'self' parameter, which this rule
// already allows.
// -------------------------------------------------------------
PointerWrapper_t PointerWrapper_move(PointerWrapper_t* self)
{
    PointerWrapper_t moved = {0};
    moved.container = self->container;
    self->container = (void*)0;
    return moved;
}

void PointerWrapper_destroy(PointerWrapper_t* self)
{
    if (!self || !self->container)
        return;

    // free(self->container);
    self->container = (void*)0;
}

PointerWrapper_t PointerWrapper_return(PointerWrapper_t* self)
{
    return *self;
}

_Bool PointerWrapper_valid(const PointerWrapper_t* self)
{
    if (!self)
        return 0;

    if (!self->container)
        return 0;

    return 1;
}

// -------------------------------------------------------------
// Good and bad: a raii creator function may set its own tagged
// field through a plain local value, but that does not extend to
// the tagged fields of a struct it merely points to.
// -------------------------------------------------------------
Container_t* create_wrapper();
PointerWrapper_t PointerWrapper_make(void)
{
    PointerWrapper_t self = {0};
    self.container = create_wrapper(); // good

    if (!self.container) // good
    {
        self.container->public_field = 0; // good
        self.container->count = 0; // bad
        self.container->total = 0; // bad
    }

    return self;
}

// -------------------------------------------------------------
// Good: the tagged field may also be carried over between two
// plain local values of the owning struct's type.
// -------------------------------------------------------------
PointerWrapper_t PointerWrapper_copy(const PointerWrapper_t* self)
{
    PointerWrapper_t copy = {0};

    // A copy function should normally perform a deep copy, but
    // this is just a test case verifying that the tagged field
    // can be reached through a plain local value.
    copy.container = self->container;
    return copy;
}

// -------------------------------------------------------------
// Good: function name starts with the owning struct's actual
// (tag) name, first parameter is named 'self' and points
// (const) at the owning struct via its typedef.
// -------------------------------------------------------------
int Container_get_count(const Container_t* self)
{
    return self->count;
}

// -------------------------------------------------------------
// Good: same as above but non-const, proving 'self' may or may
// not be const-qualified.
// -------------------------------------------------------------
void Container_set_count(Container_t* self, int value)
{
    self->count = value;
}

// -------------------------------------------------------------
// Good: 'self' is written against the raw struct tag instead of
// the typedef, proving either spelling is accepted.
// -------------------------------------------------------------
int Container_get_count_raw(const struct Container* self)
{
    return self->count;
}

// -------------------------------------------------------------
// Good: accessor for the second tagged field in the same struct.
// -------------------------------------------------------------
int Container_get_total(const Container_t* self)
{
    return self->total;
}

// -------------------------------------------------------------
// Good: setter for the second tagged field, used by Container_pod
// below so that the pod creator never touches the tagged fields
// directly itself.
// -------------------------------------------------------------
void Container_set_total(Container_t* self, int value)
{
    self->total = value;
}

// -------------------------------------------------------------
// Bad: function name does not start with the owning struct's
// name.
// -------------------------------------------------------------
int get_count(const Container_t* self)
{
    return self->count; // bad
}

// -------------------------------------------------------------
// Bad: function name starts with the owning struct's name, but
// the first parameter is not named 'self'.
// -------------------------------------------------------------
int Container_get_total_other(const Container_t* other)
{
    return other->total; // bad
}

// -------------------------------------------------------------
// Bad: function name starts with the owning struct's name and
// the parameter is named 'self', but 'self' points at the wrong
// struct (Wrapper, not Container).
// -------------------------------------------------------------
int Container_get_via_public_wrapper(const Wrapper_t* self)
{
    return self->public_inner.count; // bad
}

int Container_get_via_private_wrapper(const Wrapper_t* self)
{
    return self->inner.count; // bad
}

// -------------------------------------------------------------
// Good: same pattern as the Container accessors above, but for a
// raii struct with a tagged pointer field.
// -------------------------------------------------------------
Container_t* PointerWrapper_get_container(const PointerWrapper_t* self)
{
    return self->container;
}

// -------------------------------------------------------------
// Bad: cannot get a private field of a private field.
// -------------------------------------------------------------
int PointerWrapper_get_count(const PointerWrapper_t* self)
{
    return self->container->count;
}

// -------------------------------------------------------------
// Bad: cannot set a private field of a private field.
// -------------------------------------------------------------
void PointerWrapper_set_count(PointerWrapper_t* self, int count)
{
    self->container->count = count;
}

// -------------------------------------------------------------
// Good: setter counterpart, proving 'self' may be non-const too.
// -------------------------------------------------------------
void PointerWrapper_set_container(PointerWrapper_t* self, Container_t* container)
{
    self->container = container;
}

// -------------------------------------------------------------
// Bad: function name does not start with the owning struct's
// name.
// -------------------------------------------------------------
Container_t* get_container(const PointerWrapper_t* self)
{
    return self->container; // bad
}

// -------------------------------------------------------------
// Bad (private) / good (public): a function that takes no
// arguments at all cannot satisfy the pointer-'self' path, and
// its name does not start with 'Container' either, so the
// by-value exemption does not apply: only the public field may
// be reassigned and read back through the local pod value.
// -------------------------------------------------------------
void test_pod_struct_field_access(void)
{
    Container_t value = Container_pod(1, 2);

    value.public_field = 1; // good
    int read_public_field = value.public_field; // good

    value.count = 2; // bad
    int read_count = value.count; // bad

    value.total = 3; // bad
    int read_total = value.total; // bad
}

// -------------------------------------------------------------
// Bad (private) / good (public): same idea for a raii struct
// created locally with no arguments. The private pointer field
// itself is off-limits, and so is the private subfield reached
// through it, while the struct's own public field is fine.
// -------------------------------------------------------------
void test_raii_struct_field_access(void)
{
    PointerWrapper_t value = PointerWrapper_make();

    value.tag = 1; // good
    int read_tag = value.tag; // good

    value.container = create_wrapper(); // bad
    Container_t* read_container = value.container; // bad

    value.container->count = 2; // bad: private field, and a private subfield
    int read_subfield = value.container->count; // bad: private field, and a private subfield

    PointerWrapper_destroy(&value);
}

// -------------------------------------------------------------
// Bad (private) / good (public): same idea for a free struct
// created locally with no arguments. Reaching into the private
// 'inner' subfield is off-limits regardless of whether the field
// found inside it is itself public or private, while the public
// 'public_inner' subfield may be reached freely (its own private
// fields are still protected).
// -------------------------------------------------------------
void test_free_struct_field_access(void)
{
    Wrapper_t value;
    Wrapper_init(&value);

    value.public_inner.public_field = 1; // good
    int read_public_inner_public_field = value.public_inner.public_field; // good

    value.public_inner.count = 2; // bad: private subfield
    int read_public_inner_count = value.public_inner.count; // bad: private subfield

    value.inner.public_field = 3; // bad: private field
    int read_inner_public_field = value.inner.public_field; // bad: private field

    value.inner.count = 4; // bad: private field, and a private subfield
    int read_inner_count = value.inner.count; // bad: private field, and a private subfield
}

// -------------------------------------------------------------
// Verifies that suppression comments silence this rule. Both
// functions below violate it the same way (the function name
// does not start with the owning struct's name), but only the
// second one is wrapped in a suppressed range and must not be
// reported.
// -------------------------------------------------------------
int get_count_not_suppressed(const Container_t* self)
{
    return self->count; // bad, not suppressed
}

// WorkshopC off
int get_count_suppressed(const Container_t* self)
{
    return self->count; // suppressed, no diagnostic expected
}
// WorkshopC on

