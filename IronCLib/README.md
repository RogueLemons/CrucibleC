# IronCLib
A small C library for writing safer, more consistent C code.

It is a header-only utility library and a code analysis parser is planned for additional warnings and safety checks.

Includes:
- Compile-time assertions (`ic_static_assert.h`)
- Inline and header safety abstractions (`ic_inline.h`)
- Type-safe enum replacement (`ic_typenum.h`)
- Opaque struct storage for encapsulation (`ic_opaque_storage.h`)
- Result-based error handling (`ic_result.h`)
- Memory allocation abstraction layer (`ic_memory.h`)
- Bounded loop safety utilities (`ic_bounded_loop.h`)

## Table of Contents
* [Overview](#ironclib)
* [Quick Intro](#quick-intro)
  * [Example 1](#example-1)
  * [Example 2](#example-2)
  * [Example 3](#example-3)
* [Header Library](#header-library)
  * [ic.h](#ich)
  * [ic_static_assert.h](#ic_static_asserth)
  * [ic_inline.h](#ic_inlineh)
  * [ic_typenum.h](#ic_typenumh)
  * [ic_opaque_storage.h](#ic_opaque_storageh)
  * [ic_result.h](#ic_resulth)
  * [ic_memory.h](#ic_memoryh)
  * [ic_bounded_loop.h](#ic_bounded_looph)
* [Using in your system](#using-in-your-system)
  * [Create a standardized, type-safe error system](#create-a-standardized-type-safe-error-system)
  * [Create one entrypoint for memory allocation](#create-one-entrypoint-for-memory-allocation)
  * [Write with rules for structs and opaque storage](#write-with-rules-for-structs-and-opaque-storage)
* [TODO](#todo)
  * [Library](#todo)
  * [Parser](#todo)

## Quick Intro
A fast showcase of how the library is used, before more in-depth descriptions and examples. 

### Example 1
This example shows how you can combine a type-safe enum replacement with result types static assert in a header implemented function.

```c
#include "ic.h"
#include <stdint.h>

#define MATH_ERROR_LIST(X, Type) \
    X(Type, Generic, 0, "An error occurred") \
    X(Type, DivideByZero, 1, "Division by zero") 

IC_TYPENUM_FULL(MathError, uint8_t, MATH_ERROR_LIST)
IC_RESULT_TYPE(IntResult, int, MathError)

IC_HEADER_FUNC IntResult safe_divide(int a, int b) {
    IC_STATIC_ASSERT(sizeof(int) >= 4, "int too small");

    if (b == 0) {
        return IntResult_err(Error_DivideByZero);
    }

    return IntResult_ok(a / b);
}
```

### Example 2
This example shows how to create opaque structs allocated on the stack, where users can only access struct implementation members via functions. *Note: This example does not conform with strict aliasing.*

```c
// ------- Matrix4x4.h -------
#include "ic.h"

#define MAT4_SIZE  (sizeof(float) * 16)
#define MAT4_ALIGN (IC_ALIGNOF(float))
IC_OPAQUE_STORAGE(Matrix4x4, MAT4_ALIGN, MAT4_SIZE)

// API
void mat4_set_identity(Matrix4x4* const m);
float mat4_get(const Matrix4x4* const m, const int row, const int col);

// ------- Matrix4x4.c -------
#include "Matrix4x4.h"

struct Matrix4x4Impl {
    float data[16];
};
typedef struct Matrix4x4Impl Matrix4x4Impl;
IC_OPAQUE_IMPL_ASSERT(Matrix4x4, MAT4_ALIGN, MAT4_SIZE)

void mat4_set_identity(Matrix4x4* const m) {
    Matrix4x4Impl* const real = (Matrix4x4Impl*)m;

    for (int i = 0; i < 16; ++i)
    {
        real->data[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

float mat4_get(const Matrix4x4* const m, const int row, const int col) {
    const Matrix4x4Impl* const real = (const Matrix4x4Impl*)m;
    return real->data[row * 4 + col];
}
```

### Example 3
This examples shows using a slightly safer memory allocator and bounded while loop. 

```c
#include "ic.h"

int main(void)
{
    int count = 10;
    int* values = IC_MALLOC_ARRAY(int, count);
    if (!values) return 1;

    int i = 0;
    IC_BOUNDED_WHILE(i < count, 1000) {
        values[i] = 3;
        i++;
    }

    free(values);
    return 0;
}
```

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

##### Conceptual Expansion
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

#### Conceptual Expansion
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

##### Conceptual Expansion
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

##### Conceptual Expansion
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

### ic_memory.h
A tiny, portable memory and alignment abstraction layer for C.

It provides:
- `ic_byte` as a simple byte type
- `IC_ALIGNAS` for explicit alignment and `IC_ALIGNAS_TYPE` for alignment of a type
- `IC_ALIGNOF` for querying alignment
- `IC_MALLOC_ARRAY` for array allocation with early bad-argument catching

It supports C89+ with fallbacks, while taking advantage of C11 features when available, and works across MSVC, GCC, and Clang. *Note: Alignment is not the easiest problem to manage and different compilers might handle exact usage differently (a portability limitation that must be accepted if using this). You can write your own macro to adjust for this.*

`IC_MALLOC_ARRAY` catches negative arguments and integer overflow early and returns null. In worst fallback `IC_ALIGNAS` expands to nothing and `IC_ALIGNOF` uses `offsetof`. `ic_byte` is just an `unsigned char` but it helps with code clarity.

#### Why use this?
It exists because memory allocation and alignment handling in C are error-prone and inconsistent across compilers and standards. This abstraction makes it possible to safely allocate arrays without risking integer overflow and to write portable alignment-aware code. This results in fewer memory-related bugs, safer allocations, and improved cross-platform reliability.

#### Example

##### Usage
```c
#include "ic_memory.h" 
#include <stddef.h>
#include <stdlib.h>

void foo()
{
   const size_t n = 10;
   ic_byte* const buffer = IC_MALLOC_ARRAY(ic_byte, n)
   if (buffer == NULL)
   {
        return;
   }

   // Do stuff
   free(buffer);
}

typedef struct {
    float e[8];
} Matrix2x4;

typedef struct {
    IC_ALIGNAS_TYPE(Matrix2x4) float e[4];
} Vec4;

static const size_t Vec4Align = IC_ALIGNOF(Vec4);
```

##### Conceptual Expansion
```c
static inline void* ic_inner_malloc_array_impl(size_t count, size_t elem_size)
{
    if (count == 0 || elem_size == 0) return NULL;
    if (count > ((size_t)-1) / elem_size) return NULL;
    return malloc(count * elem_size);
}

void foo()
{
    const size_t n = 10;
    ic_byte* const buffer = (ic_byte*)ic_inner_malloc_array_impl(
        (n <= 0) ? 0u : (size_t)n,
        sizeof(ic_byte)
    );
    if (buffer == NULL) return;

    // Do stuff
    free(buffer);
}

typedef struct {
    float e[8];
} Matrix2x4;

typedef struct {
    // with alignment
    _Alignas(_Alignof(Matrix2x4)) float x, y, z, w;
    //or no alignment for fallback
    float x, y, z, w;
} Vec4;
typedef struct Vec4 Vec4;
static const size_t Vec4Align = _Alignof(Vec4);
```

### ic_bounded_loop.h
A tiny, portable bounded loop abstraction layer for C.

It provides:
- `IC_BOUNDED_WHILE` for limited while-loops
- `IC_BOUNDED_DO_WHILE` for limited do-while loops
- `IC89_BOUNDED_FOR` for limit while-loops on old compilers

These macros enforce a maximum iteration limit, preventing accidental infinite loops while preserving natural C loop semantics. *Note: The macros build on for loops with internal variables starting with _ic_ to avoid name collitions. Since they are macros, the "arguments" given to them cannot contain commas (e.g. foo(a,b)).*

#### Why use this?
It exists because C provides no built-in protection against infinite loops. A single missing condition or incorrect update can lead to non-terminating behavior. This abstraction makes it possible to enforce deterministic upper bounds on loop execution while keeping the syntax familiar. This results in safer control flow, easier debugging, and more predictable runtime behavior.

#### Example

##### Usage
```c
#include "ic_bounded_loop.h"

IC_BOUNDED_WHILE(x != NULL, 1000) {
    x = x->next;
}
```

##### Conceptual Expansion
```c
for (size_t loop = 0, max = 1000; (loop < max) && (x != NULL); ++loop)
{
   x = x->next;
}
```

## Using in your system
The following sections include topics for using IronCLib in a code base.

### Create a standardized, type-safe error system
The following will provide an example, showing how to use this library to create a standardized error system, working like an exception-as-return-value system, with one standardized error type for the whole application. 

#### Why use this?
C has no consistent error handling model, which leads to mixed return codes, null checks, and implicit failure states that are easy to miss and hard to maintain. This system replaces that with a single, type-safe error model where all functions return explicit result types and all errors come from one centralized typenum. This makes failures visible in the type system, consistent across the codebase, and easy to extend without breaking existing code.

It turns error handling into explicit program structure, improving readability, reducing hidden bugs, and making control flow predictable and uniform.

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

You can also add a void result type to make error handling even more uniform for void functions.

```c
typedef struct { char _; } VoidType;
IC_RESULT_TYPE(VoidResult, VoidType, Error)
#define Result_ok VoidResult_ok((VoidType){0})
#define Result_err(x) VoidResult_err(x)
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

### Create one entrypoint for memory allocation
The library has given a safer memory allocator with the wrapper `IC_MALLOC_ARRAY`, and by simply using this the risk of bugs will decrease, yet it can be taken a step further. The following demonstrates how to centralize all dynamic memory operations behind a single, type-safe interface. This creates a consistent allocation model across the entire codebase, reducing misuse and making memory behavior explicit and enforceable.

#### Why use this?

In raw C, memory allocation is, implicit (malloc vs calloc vs realloc), inconsistent (different argument rules), and easy to misuse (wrong sizes, missing overflow checks, invalid realloc usage). This structure makes allocation validated, consistent by centralization, and extensible.

#### my_app_memory
This section shows how to set up a centralized dynamic memory allocator with argument safety. The purpose is to never use malloc, calloc, or realloc directly after this, giving the system full control of memory usage.

##### my_app_memory.h
```c
#include "ic_typenum.h"

#define ALLOC_MODE_LIST(X, Type) \
    X(Type, Standard, 0, "Standard allocation") \
    X(Type, Zeroed, 1, "Zero-initialized allocation") \
    X(Type, Realloc, 2, "Reallocate existing memory")

IC_TYPENUM_FULL(AllocMode, int, ALLOC_MODE_LIST)

void* allocate_memory(const long element_size, const long element_count, const AllocMode mode, void* old_ptr);
void free_memory(void* const ptr);
```

##### my_app_memory.c
```c
#include "ic_static_assert.h"
#include "ic_glue_macro.h"
#include <stdlib.h>

#define ALLOC_MODE_INTERNAL_VALUES(Type, Name, Value, Str) IC_GLUE3(Type, _, Name) = Value,
enum {
    ALLOC_MODE_LIST(ALLOC_MODE_INTERNAL_VALUES, AllocModeVal)
};

void* allocate_memory(const long element_size, const long element_count, const AllocMode mode, void* old_ptr)
{
    if (element_size <= 0 || element_count <= 0) // validate input
    {
        return NULL;
    }
    const size_t size  = (size_t)element_size;
    const size_t count = (size_t)element_count;
    if (count > ((size_t)-1) / size) // overflow guard
    {
        return NULL;
    }
    const size_t total_size = size * count;

    switch (AllocMode_get(mode))
    {
        case AllocModeVal_Standard:
            if (old_ptr != NULL) { return NULL; }
            return malloc(total_size);
        case AllocModeVal_Zeroed:
            if (old_ptr != NULL) { return NULL; }
            return calloc(count, size);
        case AllocModeVal_Realloc:
            if (old_ptr == NULL) { return NULL; }
            return realloc(old_ptr, total_size);
        default:
            return NULL;
    }

    return NULL;
}

void free_memory(void* const ptr)
{
    if (ptr == NULL) { return; }
    free(ptr);
}
```

*Note: The source file creates real enums in order to switch-case the input. Use only when performance is critical since it removes part of the type-safety of the typenum.*

##### Usage
```c
int* int_list = allocate_memory(sizeof(int), 5, AllocMode_Standard, 0);
```

#### Expand it
With a centralized memory allocator in place, let's begin expanding it. 

##### my_app_memory.h
Ease-of-use macros can be added. *Note: Adding these macros can make it harder to scan the code for all memory allocations.*

```c
#define alloc_mem(count, type) allocate_memory(sizeof(type), count, AllocMode_Standard, 0)
#define alloc_zeroed_mem(count, type) allocate_memory(sizeof(type), count, AllocMode_Zeroed, 0)
#define realloc_mem(old_ptr, count, type) allocate_memory(sizeof(type), count, AllocMode_Realloc, old_ptr)
```

##### Macro Usage
```c
int* int_list = alloc_mem(5, int);
```

##### my_app_memory.c
After this the core function can be expanded to include debugging features, such as logging and memory leak tracking. 

```c
// <function code>
    switch (AllocMode_get(mode))
    {
        case AllocModeVal_Standard:
            if (old_ptr != NULL) 
            {
                LOG("[MemAlloc][Standard] Illegal old_ptr value"); 
                return NULL; 
            }
            void* res = malloc(total_size);
            if (res == NULL)
            {
                LOG("[MemAlloc][Standard] Failed to allocate memory");
            }
            else 
            {
                track(res, total_size);
            }
            return res;
    
    // <other cases>
    }
// <function code>

void free_memory(void* const ptr)
{
    if (ptr == NULL) 
    {
        LOG("[FreeMem] NULL pointer, no op");
        return; 
    }
    free(ptr);
    untrack(ptr);
}
```

Self-implemented memory allocators can also be used instead, such as an allocator using a memory pool on the stack, and they can be added or removed at any time in the project for project-wide improvement. *Note: A stack based allocator can make it worthwhile to use "simple" struct forward declarations instead of using opaque storage.*

```c
void* allocate_memory(const long element_size, const long element_count, const AllocMode mode, void* old_ptr)
{
    // <function code>
    void* res = try_stack_pool_memory_allocate_first(element_size, element_count, mode);
    // <function code>
}
```

##### my_app_result.h
Go further and return a result type. Building off of the previous section, [Create a standardized, type-safe error system](#create-a-standardized-type-safe-error-system), it becomes as easy as adding a few extra error types and editing the result type of the memory allocator function.

```c
#define MY_APP_ERROR_LIST(X, Type) \
    X(Type, NoError, 0, "Everything is fine") \
    X(Type, Runtime, 1, "Something went wrong") \
    X(Type, Argument, 2, "Illegal argument was provided") \
    X(Type, NullRef, 3, "Illegal null pointer was provided") \
    X(Type, Permission, 4, "Failure due to lacking permissions") \
    X(Type, BadAlloc, 5, "Failed to allocate memory")

// Later in file
typedef void* VoidPtr;
MY_APP_RESULT_TYPE(VoidPtr)
```

##### my_app_memory.h
```c
#include "my_app_result.h"

VoidPtrResult allocate_memory(const long element_size, const long element_count, const AllocMode mode, void* old_ptr);
VoidResult free_memory(void* const ptr); // If using own allocator, implementation could give fail result
```

### Write with rules for structs and opaque storage
(in progress)

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
- For SteelCLib, only care about C99 and forward, and add bounded for loop
- Add memory alloc and span helpers? Add easy and safe zero init?
- Make typenum generated functions use pointers (only if starting to allow non-integer internal types, maybe for SteelC)?
- Add tests that can be verified on multiple compilers
- Rename project to IronC (because it is rigid and not using it can cause code to break) with SteelC as name of expanded version (more flexible), and then call the parser WorkshopC because it helps create strong-like-metal C 
- Add IronHammerC testing system

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
