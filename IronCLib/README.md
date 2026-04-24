# IronCLib
IronCLib is a small, header-only C library for writing safer, more consistent C code.

It provides independent, composable utilities that address common pitfalls like unsafe casts, fragile headers, and inconsistent error handling. Each header works on its own, with no external setup required.

IronCLib is not a new programming model, just focused tools that make C code more predictable, portable, and easier to reason about. Best-effort C89 compatibility, where reasonably practical.

## Table of Contents
* [IronCLib](#ironclib)
* [Quick Intro](#quick-intro)
* [Library Overview](#library-overview)
* [Read more?](#read-more)
* [License](#license)
* [TODO](#todo)

## Quick Intro
A fast showcase of how the library is used, before more in-depth descriptions and examples.

### Ready-to-use utilities
The following provides quick examples of the IronCLib parts that require no setup. 

#### Compile-time assert
Verify boolean expressions at compile-time.

```c
#include "ironclib/ic_static_assert.h"

IC_STATIC_ASSERT(sizeof(int) >= 4, "int must be 4 bytes or more");
```

#### Inline and safe header functions
Write functions safely in headers.

```c
#include "ironclib/ic_inline.h"

IC_HEADER_FUNC int add(const int a, const int b) { return a+b; }
```

#### Memory allocation with safety checks
Allocate memory on heap or return NULL for unsafe arguments.

```c
#include "ironclib/ic_memory.h"

int* ten_ints = IC_MALLOC_ARRAY(int, 10);
```

#### Bounded loops
Write loops with a maximum number of iterations.

```c
#include "ironclib/ic_bounded_loop.h"

int res = 0;
IC_BOUNDED_WHILE(0 == res, 1000) // max 1000 iterations
{
    res = foo();
}
```

### Code generation utilities
The following provides quick examples of how to generate code or use it.

- Ad-hoc: defined where used
- Monolithic: generated once per project

#### Type-safe enums
Ad-hoc: Quickly create a type-safe alternative to enums.

```c
#include "ironclib/ic_typenum.h"

#define USE_MODE_LIST(X, Type) \
    X(Type, Quality, 0, "Usage optimized for quality") \
    X(Type, Performance, 1, "Usage optimized for performance") \
    X(Type, Battery, 2, "Usage optimized for battery duration") 
IC_TYPENUM_FULL(UseMode, uint8_t, USE_MODE_LIST)

void run_process(UseMode mode);
```

#### Numeric casting functions
Monolithic: Avoid undefined behavior when casting by generating cast functions from a once-defined list. Pre-made header provided.

```c
#include "premade/numbers.h"

i32 i = cast_f64_to_i32(9876.543);
```

#### Opaque storage
Ad-hoc: Create opaque storage types quickly and with standardized method.

```c
// Header
#include "ironclib/ic_opaque_storage.h"

#define MAT4_SIZE  (sizeof(float) * 16)
#define MAT4_ALIGN (IC_ALIGNOF(float))
IC_OPAQUE_STORAGE(Matrix4x4, MAT4_ALIGN, MAT4_SIZE) // fixed-size opaque type

void mat4_set_identity(Matrix4x4* const m);
float mat4_get(const Matrix4x4* const m, const int row, const int col);

// Source
typedef struct Matrix4x4Impl {
    float data[16];
} Matrix4x4Impl;
IC_OPAQUE_IMPL_ASSERT(Matrix4x4Impl, MAT4_ALIGN, MAT4_SIZE)
```

#### Standardized result types
Ad-hoc and monolithic: Quickly define result types (possibly based on global error type). Pre-made header provided.

```c
// Matrix4x4 header
#include "premade/global_result.h"

GLOBAL_RESULT_TYPE(Matrix4x4)

Matrix4x4Result mat4_inverse(const Matrix4x4* const m);

// Usage
Matrix4x4Result res = mat4_inverse(&m);
if (!res.ok) { /* handle error */ }
```

## Library Overview
IronCLib is split into small, independent headers. You can use any part on its own - but they are designed to be adopted gradually based on your needs.

The lower sections introduce more opinionated patterns. These are most effective when applied consistently, rather than mixed with raw C equivalents within the same layer of a codebase.

### Start here (instant, low-risk improvements)
Drop-in utilities that improve safety and portability without changing how you write C.

- `ic_static_assert.h`
- `ic_inline.h`
- `ic_memory.h`

### Optional utilities (use as needed)
Helpful tools that solve specific problems but don’t require architectural changes.

- `ic_bounded_loop.h`
- `ic_typenum.h`
- `ic_num_cast.h`

### Architectural patterns (adopt deliberately)
These introduce stronger design patterns and are most effective when used consistently across a module or project.

- `ic_opaque_storage.h`
- `ic_result.h`

> *Note: IronCLib works best when adopted consistently within a module. Avoid mixing its patterns with raw C equivalents, and don’t adopt everything at once - start small and expand only where it adds value.*

## Read more?
This document is only a quick intro.

- For for more in-depth explanations of the headers, [go here](docs/header_library.md).
- For ideas on how to use the library in your project, [go here](docs/using_in_your_system.md).
- For premade monolithic files, [go here](docs/premade). These are referenced in documentation.

## License

IronCLib is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

# TODO
- Make typenum generated functions use pointers (only if starting to allow non-integer internal types, maybe for SteelC)?
- Add tests that can be verified on multiple compilers
- Add IronHammerC testing system (and guarantee for tested compilers)
- Implement Make it your own section in using_in_your_system
