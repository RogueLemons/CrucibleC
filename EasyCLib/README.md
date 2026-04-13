# EasyC Lib
Write safe C code more easily. 

Contains a header library and a code parser (TODO) to provide warnings.

## Header Library
A plug-and-play header library that must only be added to your include folder in your build system. 

### ec.h
A simple header that includes all other headers.

### ec_static_assert.h

A tiny, portable compile-time assertion macro for C:  
`EC_STATIC_ASSERT(condition, message)`

Supports C11 (`_Static_assert`), MSVC (`static_assert`), and older C via a fallback trick.  
*Note: `message` must be a valid identifier.*

#### Example

```c
#include "ec_static_assert.h"

EC_STATIC_ASSERT(sizeof(int) == 4, int_must_be_4_bytes);
```

### ec_inline.h

A tiny, portable inline abstraction layer for C.

Provides:
- `EC_INLINE` — best-effort inline hint (not header-safe, do not confuse with C++ inline)
- `EC_HEADER_SAFE` — safe for header-defined functions (no linker issues, often static inline, falls back to static in worst case)

Supports C89+, C99 inline, and MSVC/GCC/Clang.

#### Example

```c
#include "ec_inline.h"

EC_HEADER_SAFE int square(int x) {
    return x * x;
}
```

### ec_typenum.h

A tiny, header-safe, and type-safe enum-like system for C using X-macros (no actual `enum`).

Avoids C `enum` to ensure a fixed, explicit underlying type (ABI stability) and predictable behavior across compilers.

Expects a basic underlying type (e.g. `int`, `char`) that works with `switch` statements and `==`.  
The value list must be defined as an X-macro (`LIST(X, Type)` pattern).

Defines strongly-typed values, constants, and helpers like `*_get`, `*_eq`, and `*_to_string`, all without linker issues.

Use `EC_TYPENUM_FULL(Type, underlying_type, LIST)` as the main entry point.  
Lower-level macros (`EC_TYPENUM`, `EC_TYPENUM_TO_STRING`, `EC_TYPENUM_GENERATE_CONSTS`) can also be used individually to only include exactly what you want in your header.

#### Example

```c
#include "ec_typenum.h"

#define STATUS_LIST(X, Type) \
    X(Type, Ok, 0, "Everything is fine") \
    X(Type, Error, 1, "Something went wrong")

EC_TYPENUM_FULL(Status, int, STATUS_LIST)

// Usage
Status s = Status_Ok;

if (Status_eq(s, Status_Error)) {
    // ...
}

const char* msg = Status_to_string(s);
```

#### Expansion
```c
typedef struct {
    int Status_value;
} Status;

EC_HEADER_SAFE int Status_get(const Status v) {
    return v.Status_value;
}

EC_HEADER_SAFE int Status_eq(const Status a, const Status b) {
    return a.Status_value == b.Status_value;
}

EC_HEADER_SAFE const char* Status_to_string(const Status v) {
    switch (Status_get(v)) {
        case 0: return "Everything is fine";
        case 1: return "Something went wrong";
        default: return "Invalid";
    }
}

static const Status Status_Ok = {0};
static const Status Status_Error = {1};
```

### ec_opaque_storage.h

A tiny, portable opaque-struct system for C that enables true encapsulation while still allowing stack allocation (no heap required).

It provides a way to define *storage-only types* in headers while hiding their real implementation in `.c` files.

This is useful because it:
- Enforces encapsulation (users cannot access fields directly)
- Allows stack allocation without exposing internal layout
- Adds compile-time checks for size and alignment (ABI safety)

Expects you to define a fixed `size` and `alignment` for the type.

---

#### Why opaque structs?

C normally exposes struct layout in headers, which means:
- Any change breaks ABI compatibility
- Internal details leak into every user file
- You lose control over invariants

Opaque structs solve this by:
- Hiding the real structure definition
- Forcing all access through functions
- Allowing internal changes without breaking user code

---

#### Example

##### Header
```c
#include "ec_opaque_storage.h"

#define COLOR_SIZE   (sizeof(int) * 3)
#define COLOR_ALIGN  (_Alignof(int))

EC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)

void color_init(Color* c, int r, int g, int b);
int  color_get_red(const Color* c);
```

##### Source
```c
#include "color.h"

struct Color {
    int r, g, b;
};

EC_OPAQUE_DEFINE(Color, COLOR_ALIGN, COLOR_SIZE)

void color_init(Color* c, int r, int g, int b) {
    struct Color* real = (struct Color*)c;
    real->r = r;
    real->g = g;
    real->b = b;
}

int color_get_red(const Color* c) {
    const struct Color* real = (const struct Color*)c;
    return real->r;
}
```

##### Usage
```c
Color c;                    // stack allocated, no malloc
color_init(&c, 1, 2, 3);    // safe access via API

if (color_get_red(&c) > 50)
{
    // Do stuff
}
```

##### Expansion

`EC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)` expands to:

```c
typedef unsigned char ec_byte;
typedef struct {
    /*align as COLOR_ALIGN*/ ec_byte data[COLOR_SIZE];
} Color;
```

`EC_OPAQUE_DEFINE` is required in the `.c` file because it performs compile-time validation of the real `struct Color` definition. It ensures that the actual struct’s size and alignment match the declared `COLOR_SIZE` and `COLOR_ALIGN`. Without this check, there is no guarantee that the internal implementation fits the opaque storage, which can lead to ABI mismatches or undefined behavior.

### ec_result.h

# TODO

## Lib
- Document the headers (what and why) and give examples on how to use
- Add EC_TYPENUM_FULL_HEADER and EC_TYPENUM_FULL_SOURCE macros that allow users to static const and static inline in their header

## Parser
Parser must be implemented to transfer goals of EasyCTranspiler into a warning/suggestion system for pure C code.

It shall
- Tokenize code properly and use no naive regex tricks.
- Verify const correctness of variables (possibly ignoring variables used in functions as first iteration).
- Make all mutable pointer function arguments start their names with out_ for maximum clarity (or in_, own_, or move_, to show a transfer of ownership).
- Warn against using enum.
- Look at file path and if file path is included in names (e.g. function or struct) then suggest splitting by __ (optionally user defined) to mimic namespaces and improve readability, also warn if name contains e.g. 7 underscores.
- Verify all structs immediately contain a typedef statement on next line.
- Verify that #define statements appear right above the functions they are used in if only used once in file (if in header can be used 0 times).
- Forbid null pointers and uninitialized pointers, optionally enforce all pointers to be null-checked
- Verify all structs are initialized with either a _populate or _init function
- Verify if _init is used all exit paths must include _cleanup
- Verify variables are not called with _populate or _init multiple times in same scope
- Be able to turn off warnings in-file by writing "// EasyC off" and turn back on with "// EasyC on"
- Look for EasyCSettings.csv file and use its settings (default if not exist)
- Number of warnings be return value of script/app main function.