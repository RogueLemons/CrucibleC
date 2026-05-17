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
  * [ic_concurrency.h and ic_concurrency_signal.h](#ic_concurrencyh)
  * [ic_co_job.h](#ic_co_jobh)
* [Using in your system](#using-in-your-system)

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
- C99 inline, and compiler-specific inline extensions (GCC, Clang, MSVC), with safe fallbacks where inline is unavailable

### Why use this?
It exists because inline behavior and header function definitions are not consistently defined across C standards and compilers. This abstraction makes it possible to express inline intent and safely define header-level functions across different toolchains. This results in predictable header behavior and portable performance-oriented code. 

> *Note: Do not confuse inline in C for inline in C++.*

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

> *Note: User is in charge of making sure no duplicate values.*

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

### Why this design?
Enum-like types in C can also be represented using singleton pointer values, such as:

```c
typedef const struct ColorTag* Color;

extern const struct ColorTag g_color_red_obj;
extern const struct ColorTag g_color_blue_obj;

#define COLOR_RED  (&g_color_red_obj)
#define COLOR_BLUE (&g_color_blue_obj)
```

This gives strong identity via pointer comparison (c == COLOR_BLUE) and allows extensible, metadata-rich values. However, it introduces pointer semantics, requires strict separation between header declarations and source definitions, and depends on correct linkage across translation units. If the implementation uses static objects in the source file, that directly conflicts with the extern declarations in the header, since static gives internal linkage and prevents the intended cross-file identity model (this solution would instead require `Color get_color_red(void)` in header, wrapped in the macro for enum-like syntax).

In contrast, ic_typenum.h wraps the underlying value in a small typed struct:

```c
typedef struct {
    int Status_value;
} Status;

static const Status Status_Ok   = {0};
static const Status Status_Error = {1};
static const Status Status_Blue  = {2};
```

This preserves value semantics while enforcing a distinct type per enum and keeping everything self-contained in a single header, avoiding cross-translation-unit coordination and linker dependency issues. The tradeoff is an additional abstraction layer: values are wrapped in structs and must be accessed through generated helpers rather than used as raw integers.

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

> *Note: Although IC_OPAQUE_STORAGE aligns data internally, strict aliasing rules do not promise to work for pointer casts (even if some compilers behave as if they do). Furthermore, the internal bytes of the opaque struct is aligned yet C makes no promise that a one-field struct shares the alignment of its single field (even if in practice it often does). The safest option is to use memcpy internally (even if it might be slower for large structs).*

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
    _Alignas(COLOR_ALIGN) ic_byte data[COLOR_SIZE];
} Color;
```

`IC_OPAQUE_IMPL_ASSERT` is required in the `.c` file because it performs compile-time validation of the real `struct ColorImpl` definition. It ensures that the actual struct’s size and alignment match the declared `COLOR_SIZE` and `COLOR_ALIGN`. Without this check, there is no guarantee that the internal implementation fits the opaque storage, which can lead to ABI mismatches or undefined behavior.

> *Note: In the case that aligning data is unsupported then IC_ALIGNAS_IS_BLANK will be defined, allowing for checks and different compile-time behavior from user (e.g. using memcpy instead of pointer cast, or manually aligning with uintptr_t).*

### What NOT to do
- Do not cast directly between opaque type pointers and internal structs unless inside the implementation file.
- Do not use IC_OPAQUE_STORAGE in header without IC_OPAQUE_IMPL_ASSERT in the source.
- Do not access internal data fields directly from user code.

### Why this design?
When implementing opaque-like types in C, there are several common approaches, each with different tradeoffs.

One approach is using a forward-declared struct with heap allocation:

```c
typedef struct Color Color;

Color* color_create(void);
void   color_destroy(Color*);
```

This provides true encapsulation and ABI stability since users never see the struct layout. It also makes future evolution easier because cleanup logic can later be added inside destroy without changing the API. The downside is mandatory heap allocation, pointer indirection, and explicit ownership management. The benefit is simplicity: this pattern is already easy to implement without special tooling.

Another approach is exposing a public struct with an internal _private member:

```c
typedef struct {
    struct {
        int r, g, b;
    } _private;
} Color;
```

This preserves stack allocation, value semantics, and debugger visibility while remaining simple enough to manage manually. In practice, this style works best with getters and setters implemented in source files (no macros!) so normal code never directly references `_private`. Seeing `_private` in code should act as a warning that internal state is being bypassed intentionally. The downside is that encapsulation is convention-based rather than enforced.

`ic_opaque_storage.h` exists for cases where both stack allocation and hidden implementation are desired simultaneously. It combines the main advantages of both approaches, but at the cost of additional complexity through explicit size/alignment declarations and validation macros.

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

> *Note: The `_ok` and `_err` functions (e.g. `CharResult_ok` and `CharResult_err`) are recommended to use, but the macros for getting values are purely optional (and the recomendation is to decide a style for the project as a whole). There is value in the explicitness of writing e.g. `if (r.ok) { printf("%c\n", r.data.value); }`.*

### What NOT to do
- Do not ignore `.ok` and directly access `.data` without checking success.
- Do not use Result types as a replacement for validation logic (they are for propagation, not input checking).

### Why this design?
A union is used internally because the generated result type supports arbitrary user-defined value and error types, meaning the library cannot make assumptions about their size or layout. Since only one side of the result is valid at a time (`value` or `error`), a union allows both to share the same storage and reduces the overall size of the result struct.

Another possible design is storing both the value and error separately without a union and determining success entirely from the error state, e.g. by requiring a dedicated Error_NoError value. This avoids inactive-union-member semantics and allows both fields to always contain valid dummy data, but increases the size of the result type when the error type is large since both values must exist simultaneously, and also requires the user’s error system to define a dedicated no-error state.

## ic_memory.h
A tiny, portable memory and alignment abstraction layer for C.

It provides:
- `ic_byte` as a simple byte type
- `IC_ALIGNAS` for explicit alignment and `IC_ALIGNAS_TYPE` for alignment of a type
- `IC_ALIGNOF` for querying alignment
- `IC_MALLOC_ARRAY` for array allocation with early bad-argument catching

It supports C99+ with fallbacks, while taking advantage of C11 features when available, and works across MSVC, GCC, and Clang. 

>*Note: Alignment is not the easiest problem to manage and different compilers might handle exact usage differently (a portability limitation that must be accepted if using this). You can write your own macro to adjust for this.*

`IC_MALLOC_ARRAY` catches negative arguments and integer overflow early and returns null. In the worst fallback `IC_ALIGNAS` expands to nothing and `IC_ALIGNOF` uses `offsetof`. `ic_byte` is just an `unsigned char` but it helps with code clarity.

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

These macros enforce a maximum iteration limit, preventing accidental infinite loops while preserving natural C loop semantics. 

> *Note: The macros build on for loops with internal variables starting with _ic_ to avoid name collisions. Since they are macros, the "arguments" given to them cannot contain commas (e.g. foo(a,b)).*

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

> *Note: Code generated with IC_CASTING_FUNCTIONS temporarily disables compiler warnings on GCC, Clang, and MSVC using compiler-specific pragmas. This is done under the assumption that compilers will optimize away dead code paths in the generated code that would otherwise trigger warnings. Normal warning behavior is restored immediately after the generated section.*

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

### Why this design?
Numeric casting can be implemented using more modular function-pointer based systems where conversions are selected and composed at runtime. While flexible, this shifts responsibility for correct casting semantics to the user, since safety and consistency are no longer enforced by a fixed structure. At that point, much of the benefit of a generated cast system is lost, because correctness depends on manual wiring rather than guaranteed rules.

`ic_num_cast.h` instead generates explicit conversion functions from a user-defined type matrix, ensuring all casts are known at compile time and behave deterministically. It avoids error-return APIs because they introduce branching and decision points exactly where the programmer is already reasoning about numeric edge cases. The goal is to reduce cognitive load: casting should be a predictable operation, not another layer of logic to manage.

## ic_concurrency.h
A small, portable concurrency abstraction layer for C, providing atomics, threads (tasks), mutexes, and sleep functionality.

It provides:
- `ic_atomic_i32` for safe atomic 32-bit integer operations
- `ic_task` for portable thread creation and management
- `ic_mutex` for mutual exclusion
- `ic_thread_sleep` for cross-platform thread sleeping

`ic_concurrency_signal.h` provides:
- `ic_condition_variable` for thread coordination via wait/notify semantics, blocking without CPU usage while waiting
- `ic_gate` for signalling one waiter at a time
- `ic_broadcast` for signalling all waiters

The API is designed to be minimal and predictable while hiding platform-specific threading details (C11, pthreads, or Windows).

### Why use this?
It exists because concurrency in C is highly platform-dependent and inconsistent across compilers and operating systems. This abstraction makes it possible to write multi-threaded code using a single, unified API while preserving explicit control over behavior, with light safety features such as null checks and controlled state handling. This results in more portable, easier-to-reason-about concurrency code without introducing heavy frameworks or runtime dependencies.

### Example

#### Run parallel work
This example shows how to run a worker function twice in parallel while incrementing a shared atomic counter, which ensures the work is done ten times across both threads. 

```c
#include "ironclib/ic_concurrency.h"

static int worker(void* arg)
{
    ic_atomic_i32* counter = (ic_atomic_i32*)arg;

    int32_t i = ic_atomic_fetch_add(counter, 1);
    while (i < 10)
    {
        // Do work for this "slot"

        i = ic_atomic_fetch_add(counter, 1);
    }

    return 0;
}

int run_two_workers_in_parallel()
{
    ic_atomic_i32 counter = ic_make_atomic(0);

    ic_task task;
    if (ic_task_init(&task, worker, &counter) != IC_CONCURRENCY_OK)
    {
        return 1;
    }

    (void)worker(&counter); 
    // this thread and task's thread run this in parallel
    // while safely incrementing counter

    if (ic_task_join(&task) != IC_CONCURRENCY_OK)
    {
        return 2;
    }

    const int32_t result = ic_atomic_load(&counter);
    // result == 10

    return 0;
}
```

> *Note: Work distribution between threads is not guaranteed. One thread may perform more iterations than the other, but the total work will still be 10 for this example.*

#### Set up module mutex for critical work
This example shows how to set up a mutex private to a module, and use it for a critical section of work.

```c
// critical_worker.h
int set_up_critical_worker(void);
int clean_up_critical_worker(void);
int perform_critical_work(void);

// critical_worker.c
#include "critical_worker.h"
#include "ironclib/ic_concurrency.h"

static ic_mutex critical_mutex;
static int critical_module_is_initialized = 0;

int set_up_critical_worker(void)
{
    if (critical_module_is_initialized)
    {
        return 2;
    }

    if (ic_mutex_init(&critical_mutex) != IC_CONCURRENCY_OK)
    {
        return 1;
    }

    critical_module_is_initialized = 1;
    return 0;
}

int clean_up_critical_worker(void)
{
    if (!critical_module_is_initialized)
    {
        return 2;
    }

    if (ic_mutex_destroy(&critical_mutex) != IC_CONCURRENCY_OK)
    {
        return 1;
    }

    critical_module_is_initialized = 0;
    return 0;
}

int perform_critical_work(void)
{
    if (!critical_module_is_initialized)
    {
        return 2;
    }

    int result = 0;

    ic_mutex_lock(&critical_mutex);

    // Critical section:
    // Only one thread executes this at a time.

    ic_mutex_unlock(&critical_mutex);

    return result;
}
```

In this example, the idea is for **a single thread** to perform the setup and cleanup each once, at the start and end of the program, respectively. After that the `perform_critical_work` function can be called as many times as desired for the duration of the program.

> *Note: `ic_mutex` is non-recursive. Locking a non-recursive mutex twice from the same thread will deadlock in most underlying implementations.*

#### Sleep
A simple sleep function. It makes no guarantee for exact of sleep, but it is portable and always takes an int32_t argument as milliseconds to ensure same behavior across platforms and check for less-than-zero.

```c
#include "ironclib/ic_concurrency.h"

const int32_t five_seconds = 5 * 1000;
ic_thread_sleep(five_seconds);
```

#### Trigger and react to signals
The library provides two layers for signal-style synchronization:

At the lowest level, `ic_condition_variable` is a direct wrapper over platform condition variables. It is powerful but requires **manual coordination with a mutex and careful signaling discipline**, which makes it easy to misuse in real applications.

```c
#include "ironclib/ic_concurrency_signal.h"

ic_mutex m;
ic_condition_variable cv;
int ready = 0;

ic_mutex_lock(&m);
while (!ready)
{
    ic_condition_variable_wait(&cv, &m);
}
ic_mutex_unlock(&m);
```

For higher-level usage, the library provides lightweight `ic_gate` and `ic_broadcast`, which wraps a mutex + condition variable + internal state into a safe signal abstraction designed specifically for signaling patterns. They are used for signaling one waiting thread and many waiting threads, respectively.

```c
#include <stdio.h>
#include "ironclib/ic_concurrency.h"
#include "ironclib/ic_concurrency_signal.h"

static ic_gate gate;

int worker(void* arg)
{
    int id = *(int*)arg;

    printf("Worker %d waiting...\n", id);

    // Wait until gate opens
    ic_gate_wait(&gate);

    printf("Worker %d passed through gate\n", id);

    return 0;
}

int main(void)
{
    if (ic_gate_init(&gate) != IC_CONCURRENCY_OK)
    {
        return 1;
    }

    int ids[3] = { 1, 2, 3 };

    ic_task tasks[3];

    // Start 3 waiting workers
    for (int i = 0; i < 3; i++)
    {
        if (ic_task_init(&tasks[i], worker, &ids[i]) != IC_CONCURRENCY_OK)
        {
            return 2;
        }
    }

    ic_thread_sleep(500);

    printf("Signal #1\n");
    ic_gate_signal_one(&gate);

    ic_thread_sleep(500);

    printf("Signal #2\n");
    ic_gate_signal_one(&gate);

    ic_thread_sleep(500);

    printf("Signal #3\n");
    ic_gate_signal_one(&gate);

    // Join all workers
    for (int i = 0; i < 3; i++)
    {
        ic_task_join(&tasks[i]);
    }

    ic_gate_destroy(&gate);

    return 0;
}
```

The order of woken up tasks waiting on an `ic_gate` is random. Worker 2 can be woken by Signal 1 for instance. The gate is also lossless, meaning that signals sent ahead of time are stored as permits for future waiting threads to immediately continue.

```c
#include <stdio.h>
#include "ironclib/ic_concurrency.h"
#include "ironclib/ic_concurrency_signal.h"

static ic_broadcast broadcast;

int worker(void* arg)
{
    int id = *(int*)arg;

    printf("Worker %d waiting for broadcast...\n", id);

    // Wait until broadcast is signaled
    ic_broadcast_wait(&broadcast);

    printf("Worker %d received broadcast\n", id);

    return 0;
}

int main(void)
{
    // Initialize broadcast
    if (ic_broadcast_init(&broadcast) != IC_CONCURRENCY_OK)
    {
        return 1;
    }

    int ids[3] = { 1, 2, 3 };

    ic_task tasks[3];

    // Start waiting workers
    for (int i = 0; i < 3; i++)
    {
        if (ic_task_init(&tasks[i], worker, &ids[i]) != IC_CONCURRENCY_OK)
        {
            return 2;
        }
    }

    ic_thread_sleep(1000);

    printf("Broadcasting signal...\n");

    // Wake ALL waiting workers
    if (ic_broadcast_signal_all(&broadcast) != IC_CONCURRENCY_OK)
    {
        return 3;
    }

    // Join all workers
    for (int i = 0; i < 3; i++)
    {
        if (ic_task_join(&tasks[i]) != IC_CONCURRENCY_OK)
        {
            return 4;
        }
    }

    // Cleanup
    if (ic_broadcast_destroy(&broadcast) != IC_CONCURRENCY_OK)
    {
        return 5;
    }

    return 0;
}
```

The broadcast signals all waiting threads and will let all future waiting threads pass immediately until the broadcast is manually reset with `ic_broadcast_reset`. It does not guarantee any ordering.

#### More documentation in header itself
The [ic_concurrency.h file](../ironclib/ic_concurrency.h) and [ic_concurrency_signal.h files](../ironclib/ic_concurrency.h) contain a larger API than most other headers in this library. All headers included commented documentation directly in the library headers but this one is extra worthwhile to have a look at.

#### Why this design (for signals)?
All signal types are ultimately built on condition variables, and ic_condition_variable exists to expose that low-level primitive with minimal abstraction for cases where full control is needed. However, correct usage requires careful mutex coordination and signaling discipline, which is easy to get wrong in practice.

To reduce misuse, `ic_gate` and `ic_broadcast` provide safer, higher-level patterns. A gate wakes one waiting thread per signal, while a broadcast releases all waiters and keeps the signal active for future waiters until reset. They are kept as separate types because they represent fundamentally different behaviors: a gate is a one-time "pass one thread" mechanism, while a broadcast is a persistent "allow all through" state. Merging them into one abstraction would make the rules harder to reason about and easier to misuse. 

If both behaviors are needed together, the rules for building a combined `Passpoint` are defined in the [using_in_your_system.h](using_in_your_system.md#merging-gates-and-broadcasts). It combines multiple synchronization behaviors and is intended for cases where these semantics are already well understood. If this model is not clear, it is recommended to use `ic_gate` or ic_broadcast directly.

### Conceptual Expansion

`ic_atomic_i32` wraps:
- C11 `_Atomic` when available  
- GCC/Clang `__atomic` builtins  
- MSVC `Interlocked` operations  

`ic_task` wraps:
- `thrd_t` (C11)  
- `pthread_t` (POSIX)  
- `HANDLE` (Windows)  

`ic_mutex` wraps:
- `mtx_t` (C11)  
- `pthread_mutex_t` (POSIX)  
- `SRWLOCK` (Windows)  

`ic_thread_sleep` wraps:
- `thrd_sleep` (C11)  
- `nanosleep` (POSIX)  
- `Sleep` (Windows)  

`ic_condition_variable` wraps:
- `cnd_t` (C11)
- `pthread_cond_t` (POSIX)
- `CONDITION_VARIABLE` (Windows)

`ic_gate` and `ic_broadcast` wrap:
- `ic_mutex` and
- `ic_condition_variable`

This ensures consistent behavior across platforms while keeping the implementation header-only.

### What NOT to do
- Do not lock mutex twice in same thread (will in practice almost always lead to deadlock).
- Do not access UNSAFE_PRIVATE_ACCESS_* fields directly; they are internal and may change.
- Do not call ic_task_join multiple times on the same task.
- Do not use atomics as a replacement for mutexes when coordinating complex shared state.
- Do not ignore return codes from initialization and threading functions.
- Do not assume thread scheduling or execution order; always design for concurrency correctness.

## ic_co_job.h
A lightweight cooperative multitasking system for single-threaded execution.

It provides:
- `ic_co_job` as a sequence of ordered steps (function pointers)
- `ic_co_scheduler` for scheduling and interleaving multiple jobs
- Incremental execution via `ic_co_scheduler_tick`
- Priority-based selection of the next step to execute
- Weighted execution budgets (“credits”) per scheduling cycle

Jobs are defined as simple ordered lists of functions (void (*)(void*)), and are executed step-by-step rather than all at once. The scheduler distributes execution across jobs using a scoring system that balances priority, remaining credits, and progress.

### Why use this?
It exists because many systems need asynchronous or staged execution without the overhead or complexity of OS threads. This abstraction makes it possible to split work into small deterministic steps and interleave execution across multiple jobs in a controlled way. This results in smoother frame-based execution, predictable scheduling, and reduced blocking in single-threaded environments.

Unlike thread-based concurrency, execution is fully deterministic and runs in the calling thread, making it suitable for game loops, embedded systems, and real-time update pipelines.

### Example

```c
#include "ironclib/ic_co_job.h"

/* ===== Asset loading job ===== */

typedef struct AssetLoadData
{
    int texture_id;
    int mesh_id;
} AssetLoadData;

static void load_textures(void* data)
{
    AssetLoadData* d = (AssetLoadData*)data;
    d->texture_id += 1; // simulate loading step
}

static void load_meshes(void* data)
{
    AssetLoadData* d = (AssetLoadData*)data;
    d->mesh_id += 1; // simulate loading step
}

static ic_co_job make_asset_loading_job(void)
{
    return IC_MAKE_CO_JOB(load_textures, load_meshes);
}

/* ===== Simulation update job ===== */

typedef struct SimulationData
{
    int physics_ticks;
    int entity_updates;
} SimulationData;

static void physics_step(void* data)
{
    SimulationData* d = (SimulationData*)data;
    d->physics_ticks++;
}

static void entity_update_step(void* data)
{
    SimulationData* d = (SimulationData*)data;
    d->entity_updates++;
}

static void make_simulation_job(void)
{
    return IC_MAKE_CO_JOB(physics_step, entity_update_step);
}

int main(void)
{
    ic_co_scheduler sched = ic_make_co_scheduler();

    // data not owned by job itself
    AssetLoadData asset_data = { .texture_id = 0, .mesh_id = 0 };
    ic_co_job asset_loading_job = make_asset_loading_job();
    ic_co_scheduler_add_job(&sched, &asset_loading_job, &asset_data, 2, 1); // priority is 2, 1 step per cycle

    // data not owned by job itself
    SimulationData sim_data  = { .physics_ticks = 0, .entity_updates = 0 };
    ic_co_job simulation_job = make_simulation_job();
    ic_co_scheduler_add_job(&sched, &simulation_job, &sim_data, 1, 1);  // lower priority

    // run scheduled jobs to completion
    while (!ic_co_scheduler_is_done(&sched))
    {
        ic_co_scheduler_tick(&sched);
    }

    return 0;
}
```

> *Note: Scoring can be overridden, yet default is `score = priority * credits - steps` which causes interleaving (desired) and can cause a lower priority job with high weight to go first.*

### Conceptual Model
Each job progresses independently through its own ordered sequence of steps:

```
Job A: step 0 → step 1 → step 2 → done
Job B: step 0 → step 1 → done
Job C: step 0 → step 1 → step 2 → step 3 → done
```

The scheduler does **not** run one entire job at a time. Instead, each call to ic_co_scheduler_tick executes exactly one step from the currently selected job.

A possible execution order could look like:

```
tick 1  → Job A step 0
tick 2  → Job B step 0
tick 3  → Job A step 1
tick 4  → Job C step 0
tick 5  → Job A step 2
tick 6  → Job B step 1
tick 7  → Job C step 1
```

The exact order depends on:

- job priorities
- remaining credits (weights)
- current progress (state)
- the configured `IC_CO_SCORE` macro

The scheduler repeatedly:

1. Rebuilds runnable jobs when credits reset
2. Refills credits from job weights
3. Picks the best next job using the score function
4. Executes one step from that job

This creates a deterministic cooperative scheduling model without preemption or OS threads.

### What NOT to do
- Do not assume real parallel execution; everything runs on a single thread.
- Do not rely on exact scheduling order unless you control all priorities and weights.
- Do not use long-running steps; jobs are intended to be small and incremental.
- Do not modify job state externally while it is being scheduled unless explicitly designed for it.

### Why this design?
`ic_co_job` consists of multiple `ic_co_step` functions using the signature `void step_func(void* data)`. Other coroutine-style designs were considered, such as a single linear function using a signature such as `void step_func(int step, void* data, int* is_done)` or a dynamic state-machine approach using something such as `void step_func(void* data, step_func* next_step)`. These are valid designs with their own benefits, but IronCLib prioritizes safety and predictability over flexibility.

The alternative approaches require more care from the user. What happens if `is_done` is never set? Will users remember that code before and after an internal `switch(step)` executes on every call? If `is_done` is removed entirely, will users always provide the correct expected step count separately? Dynamic state-machine designs are even more flexible, but they also introduce branching execution paths that can loop forever or become difficult to reason about. Systems built this way often need additional cancellation, escape, or watchdog mechanisms to guarantee termination and maintain control over execution. The issue is not that these designs are inherently wrong, because they are not, but that they rely more heavily on disciplined usage, while this library tries to reduce the dangers of using disciplined code *instead of* safe code.

With `ic_co_job`, many of these concerns are handled directly by the API. Each step executes exactly once, in a fixed order, making execution easier to reason about. When the steps are written sequentially in source code, the result resembles one large function divided into explicit stages while still allowing scheduling between jobs. Jobs also have a fixed maximum number of steps and always execute linearly from start to finish, intentionally trading flexibility for a model that is easier to analyze, debug, and trust.

> *Note: For the upcoming SteelCLib, some version of the linear `void step_func(int step, void* data, int* is_done)` and dynamic `void step_func(void* data, step_func* next_step)` are likely to be included, with safety features of their own.*

## Using in your system
For more reading on how to use this library in your application, [go here](using_in_your_system.md).

> *Note: All headers contain documentation as comments. For more reading, look directly into the files.*
