# EasyC Lib
A small C library for writing safer, more consistent C code.

Includes a header-only utility library and a planned static analysis parser for additional warnings and safety checks.

## Table of Contents
* [Overview](#easyc-lib)
* [Header Library](#header-library)
  * [ec.h](#ech)
  * [ec_static_assert.h](#ec_static_asserth)
  * [ec_inline.h](#ec_inlineh)
  * [ec_typenum.h](#ec_typenumh)
  * [ec_opaque_storage.h](#ec_opaque_storageh)
  * [ec_result.h](#ec_resulth)
* [TODO](#todo)
  * [Library](#todo)
  * [Parser](#todo)

## Header Library
A drop-in, header-only library designed for easy integration into any C project. Simply add it to your include path and start using it immediately—no build steps or external dependencies required.

It provides a set of portable utilities that abstract away common inconsistencies across compilers and C standards. The goal is to improve safety, portability, and clarity in low-level C code while keeping the API minimal and predictable.

This library does not aim to build a macro language inside C. Instead, it focuses on small, necessary, and practical abstractions that make writing safe, consistent C code easier without hiding the language itself.

### ec.h
A simple header that includes all other headers.

### ec_static_assert.h
A tiny, portable compile-time assertion macro for C.

It provides:
- `EC_STATIC_ASSERT(condition, message)` (*Note: `message` must be a valid identifier*)
- Support for C11 `_Static_assert`
- MSVC `static_assert` compatibility
- Fallback for older C standards

#### Why use this?
It exists because C does not provide a consistent, cross-compiler mechanism for enforcing compile-time assertions. This abstraction makes it possible to validate assumptions about types, sizes, and configuration during compilation across different compilers and language standards. This results in earlier detection of platform-specific issues and more reliable portable code.

#### Example

##### Usage
```c
#include "ec_static_assert.h"

EC_STATIC_ASSERT(sizeof(int) == 4, int_must_be_4_bytes);
```

##### Expansion
```c
// e.g. this for modern compilers
static_assert(sizeof(int) == 4, int_must_be_4_bytes);
// or this for older (or barebone) compilers
typedef char static_assert_failed_int_must_be_4_bytes[(sizeof(int) == 4) ? 1 : -1];
```

### ec_inline.h
A tiny, portable inline abstraction layer for C.

It provides:
- `EC_INLINE` for inline intent hints
- `EC_HEADER_SAFE` for safe header-defined functions
- Supports C89+, C99 inline, and compiler-specific inline extensions (GCC, Clang, MSVC), with safe fallbacks where inline is unavailable

#### Why use this?
It exists because inline behavior and header function definitions are not consistently defined across C standards and compilers. This abstraction makes it possible to express inline intent and safely define header-level functions across different toolchains. This results in predictable header behavior and portable performance-oriented code. *Note: Do not confuse inline in C for inline in C++.*

#### Example
```c
#include "ec_inline.h"

EC_HEADER_SAFE int square(int x) {
    return x * x;
}
```

### ec_typenum.h
A tiny, header-safe, type-safe enum-like system for C using X-macros.

It provides:
- Strongly-typed enum-like values
- Named constants generated from a single X-macro list
- Helper functions (`*_get`, `*_eq`, `*_to_string`)
- Controlled, explicit underlying type representation

It expects a basic underlying type (e.g. `int`, `char`) that works with `switch` statements and `==` and the value list must be defined as an X-macro (`LIST(X, Type)` pattern). Use `EC_TYPENUM_FULL(Type, underlying_type, LIST)` as the main entry point. Lower-level macros (`EC_TYPENUM`, `EC_TYPENUM_TO_STRING`, `EC_TYPENUM_GENERATE_CONSTS`) can be used individually to include only the parts you need.

#### Why use this?
It exists because C enums do not guarantee a fixed underlying type and are compiler-defined, which reduces portability and ABI stability. This abstraction makes it possible to define enum-like types with explicit underlying representation while keeping values, strings, and helpers synchronized from a single source. This results in safer, more predictable enum-like behavior with reduced duplication and fewer mismatch errors.

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
A tiny, portable opaque-struct system for C that enables encapsulation while still allowing stack allocation.

It provides:
- Hidden struct implementation in headers
- Stack-allocatable opaque storage types
- Compile-time size and alignment validation
- Separation of interface and implementation

Expects you to define a fixed `size` and `alignment` for the type. You create it with `EC_OPAQUE_STORAGE(Type, ALIGNMENT, SIZE)` in the header and `EC_OPAQUE_DEFINE(Type, ALIGNMENT, SIZE)` in the source.

#### Why use this?
It exists because C struct layouts are normally exposed in headers, tightly coupling users to internal representation and preventing safe evolution of implementation. This abstraction makes it possible to hide internal structure while still allowing stack allocation and enforcing size and alignment constraints. This results in true encapsulation, ABI-safe design, and fully controlled internal state.

#### Example

##### Header
```c
#include "ec_opaque_storage.h"

#define COLOR_SIZE   (sizeof(int) * 3)
#define COLOR_ALIGN  (EC_ALIGNOF(int))

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
A tiny, portable `Result<T, E>` style type for C with explicit error handling and controlled propagation.

It provides:
- `Result<T, E>` style struct
- Automatic `ok` / `err` constructors
- Safe access macros
- Early-return error propagation (`TRY`-style flow)

Result types as created with `EC_RESULT_TYPE`. `EC_RESULT_IS_OK`, `EC_RESULT_VALUE`, and `EC_RESULT_ERROR` provide safe access to the result state and its data. The design avoids exceptions by making error handling explicit, while keeping the API ergonomic and easy to use. `EC_TRY_RETURN_ERR_AS` enables early-return error propagation when chaining operations.

#### Why use this?
It exists because C lacks built-in error handling, forcing either error codes or implicit control flow patterns that are easy to misuse. This abstraction makes it possible to represent success and failure explicitly as data while enabling controlled propagation of errors through function chains. This results in more predictable control flow and fewer ignored or hidden failure states.

#### Example

##### Header
```c
#include <stdio.h>
#include "ec_result.h"

EC_RESULT_TYPE(CharResult, char, int)
```

##### Usage
```c
#include "char_result.h"

CharResult get_letter(int ok) {
    CharResult r = ok ? CharResult_ok('A') : CharResult_err(-1);
    // Return early for error
    EC_TRY_RETURN_AS(CharResult, r);
    // Do stuff for r being valid
    return r;
}

int main() {
    CharResult r = get_letter(1);
    if (EC_RESULT_IS_OK(r)) {
        printf("%c\n", EC_RESULT_VALUE(r));
    } else {
        printf("Error: %d\n", EC_RESULT_ERROR(r));
    }
    return 0;
}
```

##### Expansion
`EC_RESULT_TYPE(CharResult, char, int)` expands to:

```c
typedef struct {
    int ok;
    union {
        char value;
        int error;
    } data;
} CharResult;

EC_HEADER_SAFE CharResult CharResult_ok(char v) {
    return (CharResult){ .ok = 1, .data.value = v };
}

EC_HEADER_SAFE CharResult CharResult_err(int e) {
    return (CharResult){ .ok = 0, .data.error = e };
}
```

# TODO

## Lib
- Add EC_TYPENUM_FULL_HEADER and EC_TYPENUM_FULL_SOURCE macros that allow users to static const and static inline in their header
- Add example and guidance for creating a strong system linking together EasyC result types, typenums, and structs. 

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