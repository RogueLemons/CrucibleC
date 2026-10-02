[← Previous: README](../README.md)

# A tour of WorkshopC

This page shows what code written for WorkshopC looks like, one idea at a time. Each section starts with a problem that plain C leaves open, shows how WorkshopC closes it, and links to the full rule. It takes about ten minutes to read.

The examples of WorkshopC code on this page were checked with the [`default` preset](configuration.md#built-in-presets): the lines marked `// Reported` are exactly the lines it reports. They use the [lowercase tag names](tag-headers.md) from `workshopc_tags.h`. The short plain C snippets only show the problem that each section solves.

## Contents

1. [Ownership you can see](#1-ownership-you-can-see)
2. [Structs with lifetimes](#2-structs-with-lifetimes)
3. [Pointers that are never null](#3-pointers-that-are-never-null)
4. [Buffers that know their size](#4-buffers-that-know-their-size)
5. [Private fields](#5-private-fields)
6. [Small rules, steady code](#6-small-rules-steady-code)
7. [Exceptions that explain themselves](#7-exceptions-that-explain-themselves)
8. [Where to go next](#where-to-go-next)

## 1. Ownership you can see

In plain C, a pointer parameter says nothing about ownership:

```c
void queue_push(queue* q, item* value);   // Does the queue own value now? Who frees it?
```

The answer lives in documentation, if anywhere, and nothing checks it. With WorkshopC, every non-const pointer parameter is tagged with what the function does with the pointer, and the call site says the same thing with a matching operator:

| Tag | Call site | Meaning |
|-----|-----------|---------|
| `receives` | `give(p)` | The function takes ownership. The caller may not use `p` again until it is reassigned. |
| `initializes` | `overwrite(&p)` | The function writes a result through the pointer. |
| `borrows` | `lend(p)`, not used in `default` | The function changes what `p` points to, and the caller keeps ownership. |

```c
void queue_push(borrows queue* q, receives item* value);
void item_create(initializes item** result);
void item_update(borrows item* value);
int item_weight(const item* value);

void example(borrows queue* q)
{
    item* value = NULL;
    item_create(overwrite(&value));     // item_create writes value
    item_update(value);                 // item_update changes value, the caller keeps it
    int weight = item_weight(value);    // const: nothing to mark
    queue_push(q, give(value));         // ownership moves to the queue

    item_update(value);                 // Reported: value was given away
}
```

The call site has to agree with the declaration, so a forgotten operator is caught too:

```c
void forgotten(borrows queue* q)
{
    item* other = NULL;
    item_create(&other);                // Reported: missing overwrite()
    queue_push(q, other);               // Reported: missing give()
}
```

The tags and operators are macros that disappear in a normal build, so they cost nothing at runtime. And when a function changes later, e.g. a parameter starts taking ownership, every call site that was not updated is reported.

→ [Argument pointer movement rule](rules.md#argument-pointer-movement-rule)

## 2. Structs with lifetimes

Plain C leaves cleanup to discipline. Every early return, every error path and every copy of a struct is a chance to leak a resource or to free it twice.

WorkshopC gives every struct a category, decided by its creator function. A **raii struct**, created by a `_make` function, owns resources and comes with a fixed set of lifecycle functions:

```c
typedef struct buffer
{
    unsigned char* data;
    size_t size;
    size_t capacity;
} buffer;

buffer buffer_make(size_t capacity);              // constructor
buffer buffer_copy(const buffer* self);           // copy constructor
buffer buffer_move(borrows buffer* self);         // move constructor
void buffer_destroy(borrows buffer* self);        // destructor
buffer buffer_return(borrows buffer* self);       // moves the value out of a function
bool buffer_valid(const buffer* self);            // tells whether the buffer is usable
void buffer_append(borrows buffer* self, unsigned char byte);
```

WorkshopC then follows every `buffer` variable: it has exactly one owner, it must be destroyed or returned before every scope exit, and a returned buffer can not be dropped on the floor.

```c
buffer read_header(void)
{
    buffer header = buffer_make(64);
    buffer_append(&header, 0x7f);
    return buffer_return(&header);      // ownership moves to the caller
}

void print_header(bool verbose)
{
    buffer header = read_header();
    if (verbose)
        return;                         // Reported: header is never destroyed on this path

    buffer_destroy(&header);
}

void misuse(void)
{
    buffer a = read_header();
    buffer b = a;                       // Reported: two owners of the same memory
    read_header();                      // Reported: the result can never be destroyed
    buffer_destroy(&b);
    buffer_destroy(&a);
}
```

In the `default` preset, raii structs are also destroyed in reverse declaration order, like C++ destructors. Plain values use the lighter **pod struct** category, created by a `_pod` function, which only guarantees that every value is initialized from a function or another value. Code that needs full control can use a **free struct**, created by an `_init` function, which has no rules at all.

→ [Struct resource management rule](rules.md#struct-resource-management-rule-raii)

## 3. Pointers that are never null

Any pointer parameter in C may be null, so either every function checks, or some function eventually crashes. WorkshopC makes the choice explicit. A normal pointer parameter must be checked before it is dereferenced, and the check is followed through every path of the function:

```c
int item_weight(const item* value)
{
    return value->weight;               // Reported: value may be null
}

int item_weight_checked(const item* value)
{
    if (value == NULL)
        return 0;

    return value->weight;               // OK: checked first
}
```

When a parameter must always be a real object, it is tagged as a reference instead. The lowercase name is `massive`: there is always something solid inside. The function needs no check, and every call site must pass the address of a real object:

```c
int item_weight_of(massive const item* value)
{
    return value->weight;               // OK: never null
}

void example(const item* maybe)
{
    item local = item_pod(3);
    int a = item_weight_of(&local);     // OK: the address of a real object
    int b = item_weight_of(maybe);      // Reported: a normal pointer may be null
}
```

The null check moves out of the function and into the one place that knows the answer: the call site.

→ [Null check rule](rules.md#null-check-rule) · [Reference pointer rule](rules.md#reference-pointer-rule)

## 4. Buffers that know their size

When an array is passed to a function, it decays to a pointer and its size is lost. The function has to trust a separate count, which may be wrong:

```c
void fill(int* values, size_t count, int value);

int values[8] = {0};
fill(values, 10, 1);                    // Compiles fine, and writes past the end
```

WorkshopC standardizes **spans**: structs that carry a pointer and its size together. In the `default` preset, every array is followed right away by a span holding all of it, and from then on the span is the only way to reach the array:

```c
typedef struct int_span
{
    int* data;
    size_t size;
} int_span;

int_span int_span_pod(borrows int* data, size_t size);
void int_span_fill(int_span span, int value);

void example(void)
{
    int values[8] = {0};
    int_span values_span = int_span_pod(values, 8);

    int_span_fill(values_span, 1);      // OK: the size travels with the data
    values[0] = 2;                      // Reported: use values_span instead
}
```

The count given to a span is checked against the array, so `int_span_pod(values, 10)` is reported, and so is an array without a span. Raw pointers to the data are only handed to the standard library and third-party functions, which keeps every unchecked pointer-and-size pair at the edge of the project.

→ [Span struct rule](rules.md#span-struct-rule) · [Array struct rule](rules.md#array-struct-rule)

## 5. Private fields

C has no access control: any code can reach into any struct and break its invariants. WorkshopC adds it. A field tagged `confined` may only be used by the struct's own functions, those whose names start with the struct's name and that take it as `self`:

```c
typedef struct counter
{
    confined int count;
} counter;

counter counter_pod(int start);
void counter_increment(borrows counter* self);

int counter_value(const counter* self)
{
    if (self == NULL)
        return 0;

    return self->count;                 // OK: one of counter's own functions
}

void example(void)
{
    counter clicks = counter_pod(0);
    counter_increment(&clicks);         // OK
    int total = clicks.count;           // Reported: count is confined to counter
}
```

The same idea extends to polymorphism: the interfaces rule gives vtables and interfaces one standard layout, with private fields that only the interface's own functions can reach.

→ [Private alternative rule](rules.md#private-alternative-rule) · [Private rule](rules.md#private-rule) · [Interfaces rule](rules.md#interfaces-rule)

## 6. Small rules, steady code

Besides the big ideas, a set of smaller rules keeps everyday code predictable:

```c
typedef enum state { STATE_IDLE, STATE_RUNNING } state;

int save_settings(void);

const char* state_name(state value)
{
    const char* name = "unknown";

    switch (value)
    {
        case STATE_IDLE:
            name = "idle";
            break;
        case STATE_RUNNING:
            name = "running";
            break;
        default:
            break;
    }

    return name;
}

void example(void)
{
    state current = 1;                  // Reported: not a member of state
    save_settings();                    // Reported: the result is discarded
    (void)save_settings();              // OK: discarded on purpose
}
```

In the `default` preset:
- Enums have a typedef and only take their own members. → [Enum](rules.md#enum-rule)
- Every switch has a default, and no case falls through. → [Strict switch](rules.md#strict-switch-rule)
- Return values are used, or discarded on purpose with `(void)`. → [Function discard](rules.md#function-discard-rule)
- Variables are initialized when they are declared, and arguments are never reassigned. → [Assignment](rules.md#assignment-rule)
- Functions end with a single return, with guard clauses allowed before it. → [Single return](rules.md#single-return-rule)
- Globals are `static`, and their names show whether they can change. → [Global variable](rules.md#global-variable-rule)
- Names in headers carry a prefix from their folder, like a namespace. → [Prefix namespace](rules.md#prefix-namespace-rule)
- `malloc` and `free` only appear inside a few wrapper functions. → [Restricted malloc](rules.md#restricted-malloc-rule)
- Structs and function pointers have typedefs. → [Typedef struct](rules.md#typedef-struct-rule) · [Function pointer](rules.md#function-pointer-rule)

## 7. Exceptions that explain themselves

Every rule can give way where it has to, but never silently. A section of a file is turned off with `// WorkshopC off`, and the `default` preset requires a reason on the very next line:

```c
// WorkshopC off
// Reason: the values are fixed by the wire protocol
enum legacy_code { LEGACY_OK = 0, LEGACY_FAIL = 7 };
// WorkshopC on
```

The reasons show up in code review, and a forgotten `// WorkshopC on` is always reported, so a suppression can not quietly spread over the rest of a file.

→ [Suppressing rules](rules.md#suppressing-rules)

## Where to go next

- [Run WorkshopC](usage.md) on your own code.
- [Choose a preset](configuration.md#choosing-a-preset): `default` for new code, or the [adoption path](configuration.md#the-adoption-path) for existing code.
- Read the [rules reference](rules.md) for every rule and option.

---

[Next: Running WorkshopC →](usage.md) · [Back to README](../README.md)
