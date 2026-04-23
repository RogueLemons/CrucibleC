# Header Library
A drop-in, header-only library designed for easy integration into any C project. Simply add it to your include path and start using it immediately - no build steps or external dependencies required.

It provides a set of portable utilities that abstract away common inconsistencies across compilers and C standards. The goal is to improve safety, portability, and clarity in low-level C code while keeping the API minimal and predictable. 

The library uses macros in three ways: 1) provide small, necessary, and practical abstractions that make writing safe, consistent C code easier without hiding the language itself; 2) provide almost necessary macros to simply standardize certain setups in headers; and 3) provide macros that are used to best effect by generating code once in one place and then writing normal C code thereafter. 

## Table of Contents
* [Header Library](#header-library)
  * [ic.h](#ich)
  * [ic_static_assert.h](#ic_static_asserth)
  * [ic_inline.h](#ic_inlineh)
  * [ic_typenum.h](#ic_typenumh)
  * [ic_opaque_storage.h](#ic_opaque_storageh)
  * [ic_result.h](#ic_resulth)
  * [ic_memory.h](#ic_memoryh)
  * [ic_bounded_loop.h](#ic_bounded_looph)
  * [ic_num_cast.h](#ic_num_casth)
* [Using in your system]

## ic.h
A simple header that includes all other headers.

## ic_static_assert.h
A tiny, portable compile-time assertion macro for C.

It provides:
- `IC_STATIC_ASSERT(condition, message)`
- Support for C11 `_Static_assert`
- MSVC `static_assert` compatibility
- Fallback for older C standards

### Why use this?
It exists because C does not provide a consistent, cross-compiler mechanism for enforcing compile-time assertions. This abstraction makes it possible to validate assumptions about types, sizes, and configuration during compilation across different compilers and language standards. This results in earlier detection of platform-specific issues and more reliable portable code.

### Example

#### Usage
```c
#include "ironclib/ic_static_assert.h"

IC_STATIC_ASSERT(sizeof(int) == 4, "int must be 4 bytes");
```

#### Conceptual Expansion
```c
// e.g. this for modern compilers
static_assert(sizeof(int) == 4, "int must be 4 bytes");
// or this for older (or barebone) compilers, __LINE__ = 3 in example
typedef char static_assert_failed_at_line_3[(sizeof(int) == 4) ? 1 : -1];
```

## ic_inline.h
A tiny, portable inline abstraction layer for C.

It provides:
- `IC_INLINE` for inline intent hints
- `IC_HEADER_FUNC` for safe header-defined functions
- Supports C89+, C99 inline, and compiler-specific inline extensions (GCC, Clang, MSVC), with safe fallbacks where inline is unavailable

### Why use this?
It exists because inline behavior and header function definitions are not consistently defined across C standards and compilers. This abstraction makes it possible to express inline intent and safely define header-level functions across different toolchains. This results in predictable header behavior and portable performance-oriented code. *Note: Do not confuse inline in C for inline in C++.*

### Example
```c
#include "ironclib/ic_inline.h"

IC_HEADER_FUNC int square(int x) {
    return x * x;
}
```

## ic_typenum.h
A tiny, header-safe, type-safe enum-like system for C using X-macros.

It provides:
- Strongly-typed enum-like values
- Named constants generated from a single X-macro list
- Helper functions (`*_get`, `*_eq`, `*_to_string`)
- Controlled, explicit underlying type representation

It expects a basic underlying type (e.g. `int`, `char`) that works with `switch` statements and `==` and the value list must be defined as an X-macro (`LIST(X, Type)` pattern). Use `IC_TYPENUM_FULL(Type, underlying_type, LIST)` as the main entry point. Lower-level macros (`IC_TYPENUM`, `IC_TYPENUM_TO_STRING`, `IC_TYPENUM_GENERATE_CONSTS`) can be used individually to include only the parts you need.

*Note: User is in charge of making sure no duplicate values.*

### Why use this?
It exists because C enums do not guarantee a fixed underlying type and are compiler-defined, which reduces portability and ABI stability. This abstraction makes it possible to define enum-like types with explicit underlying representation while keeping values, strings, and helpers synchronized from a single source. This results in safer, more predictable enum-like behavior with reduced duplication and fewer mismatch errors.

### Example
```c
#include "ironclib/ic_typenum.h"

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

### Conceptual Expansion
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

### What NOT to do
- Do not define duplicate numeric values in the X-macro list (this is undefined behavior by user responsibility).
- Do not mix different underlying types for the same typenum across translation units.
- Do not assume values are inherently safe; typenum enforces correct usage through its functions, but the underlying struct can still be modified incorrectly by user code.

## ic_opaque_storage.h
A tiny, portable opaque-struct system for C that enables encapsulation while still allowing stack allocation.

It provides:
- Hidden struct implementation in headers
- Stack-allocatable opaque storage types
- Compile-time size and alignment validation
- Separation of interface and implementation

Expects you to define a fixed `size` and `alignment` for the type. You create it with `IC_OPAQUE_STORAGE(Type, ALIGNMENT, SIZE)` in the header and `IC_OPAQUE_IMPL_ASSERT(TypeImpl, ALIGNMENT, SIZE)` in the source.

### Why use this?
It exists because C struct layouts are normally exposed in headers, tightly coupling users to internal representation and preventing safe evolution of implementation. This abstraction makes it possible to hide internal structure while still allowing stack allocation and enforcing size and alignment constraints. This results in true encapsulation, ABI-safe design, and fully controlled internal state.

### Example

#### Header
```c
#include "ironclib/ic_opaque_storage.h"

#define COLOR_SIZE   (sizeof(int) * 3)
#define COLOR_ALIGN  (IC_ALIGNOF(int))

IC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)

void color_init(Color* c, int r, int g, int b);
int  color_get_red(const Color* c);
```

#### Source
```c
#include "color.h"

struct ColorImpl {
    int r, g, b;
};
typedef struct ColorImpl ColorImpl;

IC_OPAQUE_IMPL_ASSERT(ColorImpl, COLOR_ALIGN, COLOR_SIZE)

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

*Note: Although IC_OPAQUE_STORAGE aligns data internally, strict aliasing rules do not promise to work for pointer casts (even if some compilers behave as if they do). Furthermore, the internal bytes of the opaque struct is aligned yet C makes no promise that a one-field struct shares the alignment of its single field (even if in practice it often does). The safest option is to use memcpy internally (even if it might be slower for large structs).*

#### Usage
```c
Color c;                    // stack allocated, no malloc
color_init(&c, 1, 2, 3);    // safe access via API

if (color_get_red(&c) > 50)
{
    // Do stuff
}
```

#### Conceptual Expansion
`IC_OPAQUE_STORAGE(Color, COLOR_ALIGN, COLOR_SIZE)` expands to:

```c
typedef unsigned char ic_byte;
typedef struct {
    /*align as COLOR_ALIGN*/ ic_byte data[COLOR_SIZE];
} Color;
```

`IC_OPAQUE_IMPL_ASSERT` is required in the `.c` file because it performs compile-time validation of the real `struct ColorImpl` definition. It ensures that the actual struct’s size and alignment match the declared `COLOR_SIZE` and `COLOR_ALIGN`. Without this check, there is no guarantee that the internal implementation fits the opaque storage, which can lead to ABI mismatches or undefined behavior.

### What NOT to do
- Do not cast directly between opaque type pointers and internal structs unless inside the implementation file.
- Do not use IC_OPAQUE_STORAGE in header without IC_OPAQUE_IMPL_ASSERT in the source.
- Do not access internal data fields directly from user code.

## ic_result.h
A tiny, portable `Result<T, E>` style type for C with explicit error handling and controlled propagation.

It provides:
- `Result<T, E>` style struct
- Automatic `ok` / `err` constructors
- Safe access macros
- Early-return error propagation (`TRY`-style flow)

Result types as created with `IC_RESULT_TYPE`. `IC_RESULT_IS_OK`, `IC_RESULT_VALUE`, and `IC_RESULT_ERROR` provide safe access to the result state and its data. The design avoids exceptions by making error handling explicit, while keeping the API ergonomic and easy to use. `IC_TRY_RETURN_ERR_AS` enables early-return error propagation when chaining operations.

### Why use this?
It exists because C lacks built-in error handling, forcing either error codes or implicit control flow patterns that are easy to misuse. This abstraction makes it possible to represent success and failure explicitly as data while enabling controlled propagation of errors through function chains. This results in more predictable control flow and fewer ignored or hidden failure states.

### Example

#### Header
```c
#include "ironclib/ic_result.h"

IC_RESULT_TYPE(CharResult, char, int)
```

#### Usage
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

#### Conceptual Expansion
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

### What NOT to do
- Do not ignore `.ok` and directly access `.data` without checking success.
- Do not use Result types as a replacement for validation logic (they are for propagation, not input checking).

## ic_memory.h
A tiny, portable memory and alignment abstraction layer for C.

It provides:
- `ic_byte` as a simple byte type
- `IC_ALIGNAS` for explicit alignment and `IC_ALIGNAS_TYPE` for alignment of a type
- `IC_ALIGNOF` for querying alignment
- `IC_MALLOC_ARRAY` for array allocation with early bad-argument catching

It supports C89+ with fallbacks, while taking advantage of C11 features when available, and works across MSVC, GCC, and Clang. *Note: Alignment is not the easiest problem to manage and different compilers might handle exact usage differently (a portability limitation that must be accepted if using this). You can write your own macro to adjust for this.*

`IC_MALLOC_ARRAY` catches negative arguments and integer overflow early and returns null. In worst fallback `IC_ALIGNAS` expands to nothing and `IC_ALIGNOF` uses `offsetof`. `ic_byte` is just an `unsigned char` but it helps with code clarity.

### Why use this?
It exists because memory allocation and alignment handling in C are error-prone and inconsistent across compilers and standards. This abstraction makes it possible to safely allocate arrays without risking integer overflow and to write portable alignment-aware code. This results in fewer memory-related bugs, safer allocations, and improved cross-platform reliability.

### Example

#### Usage
```c
#include "ironclib/ic_memory.h" 
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

#### Conceptual Expansion
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

### What NOT to do
- Do not mix IC_MALLOC_ARRAY with raw malloc in the same allocation system unless you fully control ownership boundaries.
- Do not ignore NULL returns; all allocations must be checked.
- Do not use IC_ALIGNAS/IC_ALIGNOF as a substitute for understanding platform alignment requirements.

## ic_bounded_loop.h
A tiny, portable bounded loop abstraction layer for C.

It provides:
- `IC_BOUNDED_WHILE` for limited while-loops
- `IC_BOUNDED_DO_WHILE` for limited do-while loops
- `IC89_BOUNDED_FOR` for limit while-loops on old compilers

These macros enforce a maximum iteration limit, preventing accidental infinite loops while preserving natural C loop semantics. *Note: The macros build on for loops with internal variables starting with _ic_ to avoid name collisions. Since they are macros, the "arguments" given to them cannot contain commas (e.g. foo(a,b)).*

### Why use this?
It exists because C provides no built-in protection against infinite loops. A single missing condition or incorrect update can lead to non-terminating behavior. This abstraction makes it possible to enforce deterministic upper bounds on loop execution while keeping the syntax familiar. This results in safer control flow, easier debugging, and more predictable runtime behavior.

### Example

#### Usage
```c
#include "ironclib/ic_bounded_loop.h"

IC_BOUNDED_WHILE(x != NULL, 1000) {
    x = x->next;
}
```

#### Conceptual Expansion
```c
for (size_t loop = 0, max = 1000; (loop < max) && (x != NULL); ++loop)
{
   x = x->next;
}
```

### What NOT to do
- Do not rely on bounded loops for correctness logic; they are a safety guard, not a program condition.
- Do not set extremely small iteration limits without understanding worst-case behavior.
- Do not use side-effect-heavy conditions that depend on loop ordering unless carefully reviewed.

## ic_num_cast.h
A tiny, portable, overflow-safe numeric casting system for for signed, unsigned, and floating-point conversions.

It provides:
- Macro-generated cast functions that avoid undefined behavior
- Compile-time detection of safe casts (no runtime cost when possible)
- Runtime clamping or assertion-based safety (user configurable)

Cast functions are generated using `IC_CASTING_FUNCTIONS` and a user-defined type matrix, where each row must include the type, mold tag (`IC_MOLD_SIGNED_INT`, `IC_MOLD_UNSIGNED_INT`, `IC_MOLD_FLOAT`), min value, and max value, for both the type converted from and to. The system enforces safe conversions by either clamping values to valid ranges or asserting correctness before casting. For floating point types, when clamping, NaN or negative infinity become lowest value in conversion range and positive infinity becomes highest.

### Why use this?
It exists because numeric casting in C is unsafe by default: overflow, underflow, and undefined behavior can occur silently, especially across signed/unsigned or float/integer boundaries. This abstraction makes it possible to perform conversions in a deterministic and portable way, with explicit guarantees about behavior. This results in safer numeric code, fewer hidden bugs, and consistent handling of edge cases like NaN and infinity.

### Premade
A header file full of generated cast functions is [provided here](premade/numbers.h). 

### Example
Use this system to create a single header file in which all common number types exist.

#### my_numbers.h
```c
// if adding e.g. #define IC_CAST_ASSERT_FUNC(expr) assert(expr)
// before includes the casts will assert instead of clamp
#include "ironclib/ic_num_cast.h"
#include <stdint.h>
#include <limits.h>
#include <float.h>

// Create typedefs for shorter function names
// and DATA macros for easier writing of matrix
typedef int32_t i32;
#define I32_DATA i32, IC_MOLD_SIGNED_INT, INT32_MIN, INT32_MAX
typedef uint32_t u32;
#define U32_DATA u32, IC_MOLD_UNSIGNED_INT, 0, UINT32_MAX
typedef float f32;
#define F32_DATA f32, IC_MOLD_FLOAT, -FLT_MAX, FLT_MAX

// Expansion helper macros
#define DATA_PAIR_IMPL(X, a1,a2,a3,a4,b1,b2,b3,b4) X(a1,a2,a3,a4,b1,b2,b3,b4)
#define DATA_PAIR(X, A, B) DATA_PAIR_IMPL(X, A, B)

// Define all cast conversion functions in x list format
// For i8, i16, i32, i64, u8, u16, u32, u64, f32, f64 
// this becomes 100 entries if including all conversions
#define CAST_CONVERSION_MATRIX(X) \
    DATA_PAIR(X, I32_DATA, I32_DATA) \
    DATA_PAIR(X, I32_DATA, U32_DATA) \
    DATA_PAIR(X, I32_DATA, F32_DATA) \
    DATA_PAIR(X, U32_DATA, I32_DATA) \
    DATA_PAIR(X, U32_DATA, U32_DATA) \
    DATA_PAIR(X, U32_DATA, F32_DATA) \
    DATA_PAIR(X, F32_DATA, I32_DATA) \
    DATA_PAIR(X, F32_DATA, U32_DATA) \
    DATA_PAIR(X, F32_DATA, F32_DATA)

// Generate casting functions
IC_CASTING_FUNCTIONS(CAST_CONVERSION_MATRIX)
```

Alternatively just write everything in the matrix immediately.

```c
// This is what the data pair matrix above expands to
#define CAST_CONVERSION_MATRIX(X) \
    X(int32_t,  IC_MOLD_SIGNED_INT,   INT32_MIN, INT32_MAX, int32_t,  IC_MOLD_SIGNED_INT,   INT32_MIN, INT32_MAX) \
    X(int32_t,  IC_MOLD_SIGNED_INT,   INT32_MIN, INT32_MAX, uint32_t, IC_MOLD_UNSIGNED_INT, 0,         UINT32_MAX) \
    X(int32_t,  IC_MOLD_SIGNED_INT,   INT32_MIN, INT32_MAX, float,    IC_MOLD_FLOAT,        -FLT_MAX,  FLT_MAX)
    // etc
```

### What NOT to do
- Do not assume implicit C casts are removed; unsafe casts still exist if used directly.
- Do not ignore clamping behavior when using floating-point conversions, documented in header (NaN/Inf handling is intentional).

#### Usage
```c
#include "my_casts.h"
#include <stdio.h>

void foo() 
{
    const f32 f = 543.21;
    const i32 i = cast_f32_to_i32(f);
    printf("%d\n", i);
}
```

#### Conceptual Expansion
```c
static inline int32_t cast_uint32_t_to_int32_t(const uint32_t v)
{
    // Compile time evaluated expression
    if (INT32_MAX > UINT32_MAX) {
        return (int32_t)v;
    }
    // otherwise clamp or assert
    return (int32_t)(v > INT32_MAX ? INT32_MAX : v);
}
```

## Using in your system
For more reading on how to use this library in your application, [go here](using_in_your_system.md).
