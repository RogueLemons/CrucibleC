[← Previous: Rules](rules.md)

# Tag headers

Some rules rely on **tags**: short macros written in the code that say what a pointer parameter or a struct field is for, e.g. that a function takes ownership of a pointer, or that a field is private. WorkshopC comes with premade headers that define every tag in two naming styles, and any other names can be used instead.

## Contents

- [How tags work](#how-tags-work)
- [The premade headers](#the-premade-headers)
- [Reading the lowercase names](#reading-the-lowercase-names)
- [Using the headers](#using-the-headers)
- [Writing your own tags](#writing-your-own-tags)
  - [What WorkshopC recognizes](#what-workshopc-recognizes)
  - [An example header](#an-example-header)
  - [Choosing names](#choosing-names)

## How tags work

A tag is an ordinary macro. While WorkshopC analyzes the code, the macro `WORKSHOPC_PARSING` is defined, and the tag expands to an annotation that WorkshopC reads. In a normal build, the tag expands to nothing:

```c
#ifdef WORKSHOPC_PARSING
#define confined __attribute__((annotate("workshopc_private_field")))
#else
#define confined
#endif
```

The call site operators of the [argument pointer movement rule](rules.md#argument-pointer-movement-rule), such as `give(p)`, work the same way: they expand to their argument in a normal build. Tags therefore cost nothing. The compiled program is exactly the same as without them, and any C compiler can build the code.

## The premade headers

The headers come in two naming styles, in [`default/tags/upper`](../default/tags/upper) and [`default/tags/lower`](../default/tags/lower), and the build copies both to `release/tags`. Both folders hold the same file names, so the style is chosen by the include path alone, e.g. `-I workshopc/tags/lower`.

| Header | Used by | Contents |
|--------|---------|----------|
| `move_tags.h` | [Argument pointer movement rule](rules.md#argument-pointer-movement-rule) | The three movement tags and their call site operators |
| `private_tag.h` | [Private alternative rule](rules.md#private-alternative-rule) | The private field tag |
| `ref_tag.h` | [Reference pointer rule](rules.md#reference-pointer-rule) | The reference pointer tag |
| `workshopc_tags.h` | All of the above | Includes the three headers above |

The names in each style:

| Meaning | Uppercase | Lowercase |
|---------|-----------|-----------|
| The function takes ownership of the pointer (move tag) | `MOVED` | `receives` |
| The call site gives ownership away (move operator) | `MOVE(p)` | `give(p)` |
| The function writes a result through the pointer (out tag) | `OUTPUT` | `initializes` |
| The call site lets its pointer be written (out operator) | `OUT(p)` | `overwrite(p)` |
| The function changes what the pointer points to (modify tag) | `MUTABLE` | `borrows` |
| The call site lets its data be changed (modify operator) | `MUT(p)` | `lend(p)` |
| Only the struct's own functions may use the field (private tag) | `PRIVATE` | `confined` |
| The pointer always points to a real object (reference tag) | `REF` | `massive` |

The [private rule](rules.md#private-rule) needs no tag, since it finds private fields by their name.

## Reading the lowercase names

The lowercase names describe what each side of a call does with a pointer, so a declaration and its call read like sentences:

```c
void consume(receives item* value);         consume(give(value));         // ownership moves to consume
void create(initializes item** result);      create(overwrite(&value));    // create writes value, the old one is replaced
void edit(borrows item* value);              edit(lend(value));            // edit changes value, the caller keeps it
int read(massive const item* value);         read(&local);                 // value is never null
struct item { confined int secret; };                                      // only item's own functions touch secret
```

A pointer is like a shell around what it points to: a `massive` pointer is never an empty shell, there is always a real object inside it, taken directly from a variable, a field or an array element and never through another pointer. A `confined` field is confined to the functions of its own struct.

## Using the headers

1. Add the folder of the chosen style to the include path of your build, so that it also ends up in `compile_commands.json`. With CMake, for example:

   ```cmake
   target_include_directories(my_app PRIVATE path/to/workshopc/tags/lower)
   ```

   The headers can also be copied into the project.
2. Include `workshopc_tags.h`, or only the headers you need.
3. Include them **after** system and third-party headers. A macro replaces its name everywhere after it is defined, also in the headers included after it.
4. Do not use the tag names for anything else. The lowercase names are chosen to be rare, but the uppercase ones are the safest.

Tags combine freely, in any order:

```c
void scale(borrows massive float* value, float factor);     // changes value, which is never null
scale(&local, 2.0f);
```

## Writing your own tags

The premade headers are only examples. Any names can be used, in capital letters or not, as long as they expand to what WorkshopC recognizes.

### What WorkshopC recognizes

While `WORKSHOPC_PARSING` is defined:

| Kind | Must expand to |
|------|----------------|
| Move tag | `__attribute__((annotate("workshopc_move")))` |
| Out tag | `__attribute__((annotate("workshopc_out")))` |
| Modify tag | `__attribute__((annotate("workshopc_modify")))` |
| Private tag | `__attribute__((annotate("workshopc_private_field")))` |
| Reference tag | `__attribute__((annotate("workshopc_reference_pointer")))` |
| Move, out and modify operators | A call to a function named `workshopc_move`, `workshopc_out` or `workshopc_modify` that takes the argument and returns it unchanged |

When `WORKSHOPC_PARSING` is not defined, the tags must expand to nothing and the operators to their argument, so that the real build is unaffected.

### An example header

A header with the tags `moved`, `output` and `mutable` and the operators `move()`, `out()` and `mut()`:

```c
#ifdef WORKSHOPC_PARSING

static inline void* workshopc_move(void* arg) { return arg; }
static inline void* workshopc_out(void* arg) { return arg; }
static inline void* workshopc_modify(void* arg) { return arg; }

#define move(expr) ((__typeof__(expr))workshopc_move((void*)(expr)))
#define out(expr)  ((__typeof__(expr))workshopc_out((void*)(expr)))
#define mut(expr)  ((__typeof__(expr))workshopc_modify((void*)(expr)))

#define moved   __attribute__((annotate("workshopc_move")))
#define output  __attribute__((annotate("workshopc_out")))
#define mutable __attribute__((annotate("workshopc_modify")))

#else

#define move(expr) (expr)
#define out(expr)  (expr)
#define mut(expr)  (expr)

#define moved
#define output
#define mutable

#endif
```

With it, declarations and calls look like this:

```c
void give_data_to_other_section(moved Data* data);      give_data_to_other_section(move(data));
void initialize_data(output Data** data);                initialize_data(out(&data));
void edit_data(mutable Data* data, int i, int j, int k); edit_data(mut(data), 1, 2, 3);
```

### Choosing names

Names like the ones above read well but easily clash with other code: every variable called `output` breaks once the header is included, and `mutable` is a C++ keyword. Pick names that are rare in your code, and keep them the same across the project so that every call site reads the same way.

---

[Next: Preset rationale →](preset-rationale.md) · [Back to README](../README.md)
