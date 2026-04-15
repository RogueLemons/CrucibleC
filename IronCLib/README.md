# IronCLib
A small C library for writing safer, more consistent C code.

It is a header-only utility library and a code analysis parser is planned for additional warnings and safety checks.

Includes:
- Compile-time assertions (`ic_static_assert.h`)
- Inline and header safety abstractions (`ic_inline.h`)
- Type-safe enum replacement (`ic_typenum.h`)
- Opaque struct storage for encapsulation (`ic_opaque_storage.h`)
- Result-based error handling (`ic_result.h`)

## Table of Contents
* [Overview](#ironclib)
* [Header Library](#header-library)
  * [ic.h](#ich)
  * [ic_static_assert.h](#ic_static_asserth)
  * [ic_inline.h](#ic_inlineh)
  * [ic_typenum.h](#ic_typenumh)
  * [ic_opaque_storage.h](#ic_opaque_storageh)
  * [ic_result.h](#ic_resulth)
* [TODO](#todo)
  * [Library](#todo)
  * [Parser](#todo)

## Header Library
A drop-in, header-only library designed for easy integration into any C project. Simply add it to your include path and start using it immediately—no build steps or external dependencies required.

It provides a set of portable utilities that abstract away common inconsistencies across compilers and C standards. The goal is to improve safety, portability, and clarity in low-level C code while keeping the API minimal and predictable.

This library does not aim to build a macro language inside C. Instead, it focuses on small, necessary, and practical abstractions that make writing safe, consistent C code easier without hiding the language itself.

### ic.h
A simple header that includes all other headers.

### ic_static_assert.h
A tiny, portable compile-time assertion macro for C.

It provides:
- `IC_STATIC_ASSERT(condition, message)`
- Support for C11 `_Static_assert`
- MSVC `static_assert` compatibility
- Fallback for older C standards

#### Why use this?
It exists because C does not provide a consistent, cross-compiler mechanism for enforcing compile-time assertions. This abstraction makes it possible to validate assumptions about types, sizes, and configuration during compilation across different compilers and language standards. This results in earlier detection of platform-specific issues and more reliable portable code.

#### Example

##### Usage
```c
#include "ic_static_assert.h"

IC_STATIC_ASSERT(sizeof(int) == 4, "int must be 4 bytes");
```

##### Expansion
```c
// e.g. this for modern compilers
static_assert(sizeof(int) == 4, "int must be 4 bytes");
// or this for older (or barebone) compilers, __LINE__ = 3 in example
typedef char static_assert_failed_at_line_3[(sizeof(int) == 4) ? 1 : -1];
```

### ic_inline.h
A tiny, portable inline abstraction layer for C.

It provides:
- `IC_INLINE` for inline intent hints
- `IC_HEADER_FUNC` for safe header-defined functions
- Supports C89+, C99 inline, and compiler-specific inline extensions (GCC, Clang, MSVC), with safe fallbacks where inline is unavailable

#### Why use this?
It exists because inline behavior and header function definitions are not consistently defined across C standards and compilers. This abstraction makes it possible to express inline intent and safely define header-level functions across different toolchains. This results in predictable header behavior and portable performance-oriented code. *Note: Do not confuse inline in C for inline in C++.*

#### Example
```c
#include "ic_inline.h"

IC_HEADER_FUNC int square(int x) {
    return x * x;
}
```

### ic_typenum.h
A tiny, header-safe, type-safe enum-like system for C using X-macros.

It provides:
- Strongly-typed enum-like values
- Named constants generated from a single X-macro list
- Helper functions (`*_get`, `*_eq`, `*_to_string`)
- Controlled, explicit underlying type representation

It expects a basic underlying type (e.g. `int`, `char`) that works with `switch` statements and `==` and the value list must be defined as an X-macro (`LIST(X, Type)` pattern). Use `IC_TYPENUM_FULL(Type, underlying_type, LIST)` as the main entry point. Lower-level macros (`IC_TYPENUM`, `IC_TYPENUM_TO_STRING`, `IC_TYPENUM_GENERATE_CONSTS`) can be used individually to include only the parts you need.

*Note: User is in charge of making sure no duplicate values.*

#### Why use this?
It exists because C enums do not guarantee a fixed underlying type and are compiler-defined, which reduces portability and ABI stability. This abstraction makes it possible to define enum-like types with explicit underlying representation while keeping values, strings, and helpers synchronized from a single source. This results in safer, more predictable enum-like behavior with reduced duplication and fewer mismatch errors.

#### Example
```c
#include "ic_typenum.h"

#define STATUS_LIST(X, Type) \
    X(Type, Ok, 0, "Everything is fine") \
    X(Type, Error, 1, "Something went wrong")

IC_TYPENUM_FULL(Status, int, STATUS_LIST)

// Usage
Status s = Status_Ok;

if (Status_eq(s, Status_Error)) {
    // Do stuff
}

const char* msg = Status_to_string(s);
```

#### Expansion
```c
typedef struct {
    int Status_value;
} Status;

IC_HEADER_FUNC int Status_get(const Status v) {
    return v.Status_value;
}

IC_HEADER_FUNC int Status_eq(const Status a, const Status b) {
    return a.Status_value == b.Status_value;
}

IC_HEADER_FUNC const char* Status_to_string(const Status v) {
    switch (Status_get(v)) {
        case 0: return "Everything is fine";
        case 1: return "Something went wrong";
        default: return "Unknown Status";
    }
}

static const Status Status_Ok = {0};
static const Status Status_Error = {1};
```

### ic_opaque_storage.h
A tiny, portable opaque-struct system for C that enables encapsulation while still allowing stack allocation.

It provides:
- Hidden struct implementation in headers
- Stack-allocatable opaque storage types
- Compile-time size and alignment validation
- Separation of interface and implementation

Expects you to define a fixed `size` and `alignment` for the type. You create it with `IC_OPAQUE_STORAGE(Type, ALIGNMENT, SIZE)` in the header and `IC_OPAQUE_IMPL_ASSERT(Type, ALIGNMENT, SIZE)` in the source.

#### Why use this?
It exists because C struct layouts are normally exposed in headers, tightly coupling users to internal representation and preventing safe evolution of implementation. This abstraction makes it possible to hide internal structure while still allowing stack allocation and enforcing size and alignment constraints. This results in true encapsulation, ABI-safe design, and fully controlled internal state.

#### Example

##### Header
```c
#include "ic_opaque_storage.h"

#define COLOR_SIZE   (sizeof(int) * 3)
#define COLOR_ALIGN  (IC_ALIGNOF(int))

IC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)

void color_init(Color* c, int r, int g, int b);
int  color_get_red(const Color* c);
```

##### Source
```c
#include "color.h"

struct ColorImpl {
    int r, g, b;
};
typedef struct ColorImpl ColorImpl;

IC_OPAQUE_IMPL_ASSERT(Color, COLOR_ALIGN, COLOR_SIZE)

void color_init(Color* c, int r, int g, int b) {
    ColorImpl* real = (ColorImpl*)c;
    real->r = r;
    real->g = g;
    real->b = b;
}

int color_get_red(const Color* c) {
    const ColorImpl* real = (const ColorImpl*)c;
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
`IC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)` expands to:

```c
typedef unsigned char ic_byte;
typedef struct {
    /*align as COLOR_ALIGN*/ ic_byte data[COLOR_SIZE];
} Color;
```

`IC_OPAQUE_IMPL_ASSERT` is required in the `.c` file because it performs compile-time validation of the real `struct ColorImpl` definition. It ensures that the actual struct’s size and alignment match the declared `COLOR_SIZE` and `COLOR_ALIGN`. Without this check, there is no guarantee that the internal implementation fits the opaque storage, which can lead to ABI mismatches or undefined behavior.

### ic_result.h
A tiny, portable `Result<T, E>` style type for C with explicit error handling and controlled propagation.

It provides:
- `Result<T, E>` style struct
- Automatic `ok` / `err` constructors
- Safe access macros
- Early-return error propagation (`TRY`-style flow)

Result types as created with `IC_RESULT_TYPE`. `IC_RESULT_IS_OK`, `IC_RESULT_VALUE`, and `IC_RESULT_ERROR` provide safe access to the result state and its data. The design avoids exceptions by making error handling explicit, while keeping the API ergonomic and easy to use. `IC_TRY_RETURN_ERR_AS` enables early-return error propagation when chaining operations.

#### Why use this?
It exists because C lacks built-in error handling, forcing either error codes or implicit control flow patterns that are easy to misuse. This abstraction makes it possible to represent success and failure explicitly as data while enabling controlled propagation of errors through function chains. This results in more predictable control flow and fewer ignored or hidden failure states.

#### Example

##### Header
```c
#include "ic_result.h"

IC_RESULT_TYPE(CharResult, char, int)
```

##### Usage
```c
#include "char_result.h"
#include <stdio.h>

CharResult get_letter(int ok) {
    CharResult r = ok ? CharResult_ok('A') : CharResult_err(-1);
    // Return early for error
    IC_TRY_RETURN_ERR_AS(CharResult, r);
    // Do stuff for r being valid
    return r;
}

int main() {
    CharResult r = get_letter(1);
    if (IC_RESULT_IS_OK(r)) {
        printf("%c\n", IC_RESULT_VALUE(r));
    } else {
        printf("Error: %d\n", IC_RESULT_ERROR(r));
    }
    return 0;
}
```

##### Expansion
`IC_RESULT_TYPE(CharResult, char, int)` expands to:

```c
typedef struct {
    int ok;
    union {
        char value;
        int error;
    } data;
} CharResult;

IC_HEADER_FUNC CharResult CharResult_ok(char v) {
    return (CharResult){ .ok = 1, .data.value = v };
}

IC_HEADER_FUNC CharResult CharResult_err(int e) {
    return (CharResult){ .ok = 0, .data.error = e };
}
```

### Create a standardized, type-safe error system
The following will provide an example, showing how to use this library to create a standardized error system, working like an exception-as-return-value system, with one standardized error type for the whole application. 

#### my_app_result.h
Create your header where all errors shall be defined. Whenever you want to add more errors (or your Java mind wants more exceptions) then this is the file you edit. With uint16_t you can define 65536 different errors or with uint8_t 256. 

```c
#include "ic_typenum.h"
#include <stdint.h>

#define MY_APP_ERROR_LIST(X, Type) \
    X(Type, NoError, 0, "Everything is fine") \
    X(Type, Runtime, 1, "Something went wrong") \
    X(Type, Argument, 2, "Illegal argument was provided") \
    X(Type, NullRef, 3, "Illegal null pointer was provided") \
    X(Type, Permission, 4, "Failure due to lacking permissions") \
    X(Type, BadAlloc, 5, "Failed to allocate memory")

IC_TYPENUM_FULL(Error, uint16_t, MY_APP_ERROR_LIST)
```

Then add your own macro that builds upon the IronC result system. `MY_APP_RESULT_TYPE(Type)` is the "real" macro which when used does the same thing as writing `IC_RESULT_TYPE(TypeResult, Type, Error)`. 

```c
#include "ic_result.h"

#define RESULT_NAME_IMPL(Type) Type##Result
#define RESULT_NAME(Type) RESULT_NAME_IMPL(Type)
#define MY_APP_RESULT_TYPE_IMPL(Name, Type) IC_RESULT_TYPE(Name, Type, Error)
#define MY_APP_RESULT_TYPE(Type) MY_APP_RESULT_TYPE_IMPL(RESULT_NAME(Type), Type)
```

You can then in this header add the common types as well if you want. It is good to keep these ultra common types in a single place.

```c
IC_RESULT_TYPE(CharResult, char, Error)
IC_RESULT_TYPE(IntResult, int, Error)
IC_RESULT_TYPE(FloatResult, float, Error)
typedef const char* StringLiteral;
MY_APP_RESULT_TYPE(StringLiteral)
IC_RESULT_TYPE(UI16Result, uint16_t, Error)
```

Put all the parts from this section in the same file, but with the file inclusions at the top of course.

#### example.h
To make new result types based on structs defined in this project, then the best place to put the result type is next to the struct.

```c
#include "my_app_result.h"

struct Block
{
    int height;
    int length;
    int width;
};
typedef struct Block Block;

MY_APP_RESULT_TYPE(Block)

BlockResult create_block(int h, int l, int w); 
```

#### example.c
At this point it is easy to define the declared function.

```c
BlockResult create_block(int h, int l, int w)
{
    if ((h <= 0) || (l <= 0) || (w <= 0))
    {
        return BlockResult_err(Error_Argument);
    }

    Block block = { .height = h, .length = l, .width = w };
    return BlockResult_ok(block);
}
```

#### other_file_example.c
If a global error type is defined and set up in a result system the code can later look like this example. The end user should use these tools to build up their own tools and not just rush in. Setting up tools and macros that suits a certain group's and project's needs before rushing in to start coding is a good approach. IronC is a library to make tools with clear names, but the users should pick what they need and modify.

```c
// These definitions would exist in a top level header
#define valueof(x) IC_RESULT_VALUE(x)
#define return_on_err(type, result) IC_TRY_RETURN_ERR_AS(type, result)

SwordResult make_sword(Resources* resources)
{
    MoltenIronResult molten_iron = smelt_iron(resources);
    return_on_err(SwordResult, molten_iron);

    SteelBlockResult steel_block = forge_steel(resources, valueof(molten_iron));
    return_on_err(SwordResult, steel_block);

    BladeResult blade = make_blade(resources, valueof(steel_block));
    return_on_err(SwordResult, blade);

    HiltResult hilt = make_hilt(resources);
    return_on_err(SwordResult, hilt);

    SwordResult sword = combine_into_sword(valueof(blade), valueof(hilt)); 
    return sword;
}
```


# TODO

## Lib
- Add IC_TYPENUM_FULL_HEADER and IC_TYPENUM_FULL_SOURCE macros that allow users to static const and static inline in their header or function defintions and extern varibles
- Consider adding a macro tag for IC_TYPENUM that converts everything to a simple typedef of the inner type
- Add optional system to opaque storage that can be turned on and off with a macro tag, that includes IC_OPAQUE_LOAD and IC_OPAQUE_STORE that handles aliasing safety through hard-copying internal bytes, but will without the tag just to fast pointer casting
- Add debug mode that uses runtime assert that can be turned off with macro tag (e.g. for accessing Result types)
- Consider removing result accessors (e.g. IC_RESULT_VALUE) and replace with functions for const safety (maybe overkill? Could include asserts, probably do this for SteelCLib instead)
- Expand typenum for SteelCLib to take a manually assigned comparitor for compatability with all inner types
- For SteelCLib add macro tag that performs static assert on size of all result types for users to guarantee size of return values?
- For SteelCLib add VoidResult macros
- Add memory alloc and span helpers? Add easy and safe zero init?
- Make typenum generated functions use pointers?
- Add tests that can be verified on multiple compilers
- Rename project to IronC (because it is rigid and not using it can cause code to break) with SteelC as name of expanded version (more flexible), and then call the parser WorkshopC because it helps create strong-like-metal C
- Add example and guidance for creating a strong system linking together IronC result types, typenums, and structs. 

## Parser
Parser must be implemented to transfer goals of EasyCTranspiler into a warning/suggestion system for pure C code. The name shall be WorkshopC.

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
- Be able to turn off warnings in-file by writing "// WorkshopC off" and turn back on with "// WorkshopC on"
- Look for WorkshopCSettings.csv file and use its settings (default if not exist)
- Number of warnings be return value of script/app main function.
