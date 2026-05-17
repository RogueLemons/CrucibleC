# Using in your system
The following sections include topics for using IronCLib in a code base. This is a continuation of [the header library documentation](header_library.md), where this document describes design patterns built on top of IronCLib (not the library itself).

> *Note: Remember that all the following sections are optional and exist for inspiration, not a hard manifesto, just like how all headers are optional.*

## Table of Contents
* [Using in your system](#using-in-your-system)
* [Create a standardized, type-safe error system](#create-a-standardized-type-safe-error-system)
* [Create one entrypoint for memory allocation](#create-one-entrypoint-for-memory-allocation)
* [Write with rules for structs and opaque storage](#write-with-rules-for-structs-and-opaque-storage)
* [Make it your own](#make-it-your-own)
* [Decouple from library and make a thread pool](#decouple-from-library-and-make-a-thread-pool)
* [Good practices for co-jobs](#good-practices-for-co-jobs)
* [What NOT to do](#what-not-to-do)

## Create a standardized, type-safe error system
The following will provide an example, showing how to use this library to create a standardized error system, working like an exception-as-return-value system, with one standardized error type for the whole application. 

### Why use this?
C has no consistent error handling model, which leads to mixed return codes, null checks, and implicit failure states that are easy to miss and hard to maintain. This system replaces that with a single, type-safe error model where all functions return explicit result types and all errors come from one centralized typenum. This makes failures visible in the type system, consistent across the codebase, and easy to extend without breaking existing code.

It turns error handling into explicit program structure, improving readability, reducing hidden bugs, and making control flow predictable and uniform.

### Premade
A header file full of global standard errors is [provided here](premade/global_error.h), and a header for creating the standard results with the global errors is [provided here](premade/global_result.h).

### my_app_result.h
Create your header where all errors shall be defined. Whenever you want to add more errors (or your Java mind wants more exceptions) then this is the file you edit. With uint16_t you can define 65536 different errors or with uint8_t 256. 

```c
#include "ironclib/ic_typenum.h"
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
#include "ironclib/ic_result.h"

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

### example.h
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

### example.c
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

### other_file_example.c
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

## Create one entrypoint for memory allocation
The library has given a safer memory allocator with the wrapper `IC_MALLOC_ARRAY`, and by simply using this the risk of bugs will decrease, yet it can be taken a step further. The following demonstrates how to centralize all dynamic memory operations behind a single, type-safe interface. This creates a consistent allocation model across the entire codebase, reducing misuse and making memory behavior explicit and enforceable.

### Why use this?

In raw C, memory allocation is, implicit (malloc vs calloc vs realloc), inconsistent (different argument rules), and easy to misuse (wrong sizes, missing overflow checks, invalid realloc usage). This structure makes allocation validated, consistent by centralization, and extensible.

### my_app_memory
This section shows how to set up a centralized dynamic memory allocator with argument safety. The purpose is to never use malloc, calloc, or realloc directly after this, giving the system full control of memory usage.

#### my_app_memory.h
```c
#include "ironclib/ic_typenum.h"

#define ALLOC_MODE_LIST(X, Type) \
    X(Type, Standard, 0, "Standard allocation") \
    X(Type, Zeroed, 1, "Zero-initialized allocation") \
    X(Type, Realloc, 2, "Reallocate existing memory")

IC_TYPENUM_FULL(AllocMode, int, ALLOC_MODE_LIST)

void* allocate_memory(const long element_size, const long element_count, const AllocMode mode, void* old_ptr);
void free_memory(void* const ptr);
```

#### my_app_memory.c
```c
#include "ironclib/ic_static_assert.h"
#include "ironclib/ic_glue_macro.h"
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

> *Note: The source file creates real enums in order to switch-case the input. Use only when performance is critical since it removes part of the type-safety of the typenum.*

#### Usage
```c
int* int_list = allocate_memory(sizeof(int), 5, AllocMode_Standard, 0);
```

### Expand it
With a centralized memory allocator in place, let's begin expanding it. 

#### my_app_memory.h
Ease-of-use macros can be added. 

> *Note: Adding these macros can make it harder to scan the code for all memory allocations.*

```c
#define alloc_mem(count, type) allocate_memory(sizeof(type), count, AllocMode_Standard, 0)
#define alloc_zeroed_mem(count, type) allocate_memory(sizeof(type), count, AllocMode_Zeroed, 0)
#define realloc_mem(old_ptr, count, type) allocate_memory(sizeof(type), count, AllocMode_Realloc, old_ptr)
```

#### Macro Usage
```c
int* int_list = alloc_mem(5, int);
```

#### my_app_memory.c
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

Self-implemented memory allocators can also be used instead, such as an allocator using a memory pool on the stack, and they can be added or removed at any time in the project for project-wide improvement. 

> *Note: A stack based allocator can make it worthwhile to use "simple" struct forward declarations instead of using opaque storage.*

```c
void* allocate_memory(const long element_size, const long element_count, const AllocMode mode, void* old_ptr)
{
    // <function code>
    void* res = try_stack_pool_memory_allocate_first(element_size, element_count, mode);
    // <function code>
}
```

#### my_app_result.h
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

#### my_app_memory.h
```c
#include "my_app_result.h"

VoidPtrResult allocate_memory(const long element_size, const long element_count, const AllocMode mode, void* old_ptr);
VoidResult free_memory(void* const ptr); // If using own allocator, implementation could give fail result
```

## Write with rules for structs and opaque storage
> “With great power comes great responsibility.”
>
> — Uncle Ben, original quote written by Stan Lee in Spider-Man

Computers do exactly what they are told, not what is wanted. Why do some coders say to always initialize variables? In this example, unitialized data gets sent into the function and the result is not deterministic.

```c
typedef struct {
    int a;
    int b;
    int c;
} AdvancedFunctionArguments;
int AdvancedFunction(AdvancedFunctionArguments* args);

void foo()
{
    AdvancedFunctionArguments args;
    // Easier to debug with AdvancedFunctionArguments args = {0}; 
    args.a = 3;
    args.b = 7;
    // args.c not initialized
    const int res = AdvancedFunction(&args);
}
```

For a function with arguments like this, the coders must make sure no mistakes are made any time the function is used instead of putting more work on the function definition in one place. `int AdvFunc(int a, int b, int c)` will not compile for `AdvFunc(3, 7)`. This does not mean coders should be forbidden from using structs for arguments, just to be mindful of what is being done.

This section will focus on making headers easier to read with opaque storage, and provide a simple ruleset that can be followed to give clear direction in the code base. 

### Why use this?
With clear naming conventions and with headers that do not give complete access to data being passed by default, it becomes easier to avoid user errors. 

### Standardize project's initializers
Introduce project policies: a) all structs shall be immediately initialized, and b) they shall be cleaned up at all scope exits. Decide on the naming convention `struct_init` and `struct_cleanup`. Formalize that all getters and setters shall include this in the name. Make sure to properly apply const correctness within functions and begin non-const arguments with `out_`. This creates a solid base to provide a lot more clarity in the project.

The following provides an example of what this might look like.

#### my_string.h
```c
#include "ironclib/ic_opaque_storage.h"

#define STRING_SIZE   (24)
#define STRING_ALIGN  (8)
IC_OPAQUE_STORAGE(String, STRING_ALIGN, STRING_SIZE)

void string_init(String* const out_string);
void string_init_with(String* const out_string, const char* const c_str);
void string_cleanup(String* const out_string);

void string_set(String* const out_string, const char* const c_str); 
void string_add(String* const out_string, const String* const other_string); 
void string_add_chars(String* const out_string, const char* const c_str); 
const char* string_get_data(const String* const s); 
size_t string_get_size(const String* const s); 
size_t string_get_capacity(const String* const s);
```

#### Usage
```c
void foo()
{
    String str;
    string_init_with(&str, "Hello there!");

    printf("String data: %s\n", string_get_data(&str));
    printf("String size: %zu\n", string_get_size(&str));
    printf("String capacity: %zu\n", string_get_capacity(&str));

    string_cleanup(&str);
}
```

### Naming conventions to consider
This section does not exist as some divine source of wisdom. It only provides suggestions to consider. As such, the naming convention can be expanded.

- `struct_populate` can be used side-by-side with `struct_init` as a way of saying the struct does not require cleanup and is only containing simple data.
- `struct_cleanup` can be an empty macro definition, and a rule to always have it accompanied, can be implemented so that if a cleanup function is later implemented there is no need to edit code anywhere.
- The argument prefix `out_` can be specified to only apply to initialization, and the prefixes `mut_` (or `mod_`) and `mov_` can be implemented to show clear intent about modifiying or transfering ownership. This way the function signatures provide expectations.
- `new` and `make` can also be used for initializers with `delete` as the cleanup function, or `construct` and `destruct`, where these initializers could return the struct instead of taking one as a reference.

### Merge with result types
The above example does not return success values or failure results. It could be done by returning integers, or the result system can be expanded. This builds on top of the previously discussed project wide result system. Anytime a new result type based on a struct is added it should be next to the struct definition in the header. 

If a combined system is desired then a common solution is to use goto statements for cleanup, and single exit of scope. Although it is possible to build macros that will manage cleanup and early return on error, there is a strong risk it leads to macro-hell and effectively becomes a macro-language instead of intentional C code.

#### my_result_string.h 
```c
#include "ironclib/ic_opaque_storage.h"
#include "my_app_result.h"

#define STRING_SIZE   (24)
#define STRING_ALIGN  (8)
IC_OPAQUE_STORAGE(String, STRING_ALIGN, STRING_SIZE)
MY_APP_RESULT_TYPE(String)                              // Create string result

StringResult construct_string(const char* const c_str); // Return string result
void destruct_string(String* const mut_str);            // Destructor does not return anything

VoidResult string_set(String* const out_string, const char* const c_str); 
VoidResult string_add(String* const out_string, const String* const other_string); 
VoidResult string_add_chars(String* const out_string, const char* const c_str);
// Assuming following result types exist in my_app_result.h
StringViewResult string_get_data(const String* const s);
SizeResult string_get_size(const String* const s); 
SizeResult string_get_capacity(const String* const s);
```

#### Usage
```c
// Macro shall exist at high level file, e.g. the generic result types file.
// This macro introduces three rules:
//      1) The result and expression r must have same error type
//      2) To use this the function must create a result variable at start
//      3) To use this the function must end with a cleanup goto tag
// Additionally, when implementing this macro, it is recommended to allow no other use of goto
// in the project. It is used exclusively for controlled cleanup paths in error handling.
#define TRY(type, r) do {                               \
    type _ic_res_tmp = (r);                             \
    if (!(_ic_res_tmp).ok) {                            \
        result.ok = 0;                                  \
        result.data.error = (_ic_res_tmp).data.error;   \
        goto cleanup;                                   \
    }                                                   \
} while (0)

VoidResult foo()
{
    VoidResult result = Result_err(Error_Runtime);

    StringResult str = construct_string("Hello again");
    TRY(StringResult, str);

    const StringViewResult c_str = string_get_data(&str);
    TRY(StringViewResult, c_str);
    printf("String data: %s\n", valueof(c_str));

    const SizeResult size = string_get_size(&str);
    TRY(SizeResult, size);
    printf("String size: %zu\n", valueof(size));

    const SizeResult cap = string_get_capacity(&str);
    TRY(SizeResult, cap);
    printf("String capacity: %zu\n", valueof(cap));

    result = Result_ok;
cleanup:
    destruct_string(&str);
    return result;
}
```

## Make it your own
IronCLib can be wrapped, renamed, or extended to match the needs of a specific codebase. Projects might not use IronCLib directly, but instead build a thin abstraction layer over selected components to enforce consistent naming, behavior, and debugging rules.

### Example
In this example the user wraps the memory allocator in their own macro and adds extra features to their own memory system.

```c
// my_memory.h
#include "ic_memory.h"
#include <string.h>
#include <stddef.h>

#define allocate_memory(size, type) IC_MALLOC_ARRAY(type, size)

#define bit_copy(dst_ptr, src_ptr) do {                              \
    const size_t _dst_size = sizeof(*(dst_ptr));                     \
    const size_t _src_size = sizeof(*(src_ptr));                     \
                                                                     \
    const size_t _min = (_dst_size < _src_size)                      \
        ? _dst_size                                                  \
        : _src_size;                                                 \
                                                                     \
    memcpy((dst_ptr), (src_ptr), _min);                              \
                                                                     \
    if (_dst_size > _min) {                                          \
        memset((char*)(dst_ptr) + _min, 0, _dst_size - _min);        \
    }                                                                \
} while (0)
```

### Be careful about making a macro language
IronCLib macros are designed around a simple principle: they should either be explicit in what they do, or generate code that remains easy to reason about; ideally both. The goal is not to hide behavior behind abstraction, but to reduce repetition without reducing clarity.

In many cases, C macros work best when they stay close to the language itself, rather than introducing new control-flow semantics. They are often most useful as constrained utilities that improve readability or reduce boilerplate.

For simple cases, macros can act as lightweight accessors that reduce verbosity without hiding intent:

```c
// my_result.h
#include "ic_result.h"
#include "global_error.h"

#define val(res) (res.data.value)
#define err(res) ((const Error)(res.data.err))

// Usage
IntResult res = foo();
if (res.ok) {
    bar(val(res));
} else {
    printf(Error_to_string(err(res)));
}
```

However, it is easy to extend this pattern into full control-flow abstraction, which can reduce readability and make debugging more difficult, especially in larger codebases or long-lived projects.

Standardizing result handling with macros is acceptable, but more complex constructs should be carefully evaluated before being introduced.

```c
#define reterr(expr) do {                                \
    const __typeof__(expr) res_tmp = expr;               \
    if (!(res_tmp).ok) {                                 \
        RESULT.ok = 0;                                   \
        RESULT.data.error = err(res_tmp);                \
        goto CLEANUP;                                    \
    }                                                    \
} while(0)

#define FUNCTION_START(res_type, func_name, __VA_ARGS__) \
    res_type func_name(...)                              \
    {                                                    \
        res_type RESULT = res_type##_err(Error_Runtime); \
        do                                               

#define FUNCTION_END(cleanup_expr)                       \ 
    while(0);                                            \
    CLEANUP:                                             \
        cleanup_expr;                                    \
        return RESULT;                                   \
    }

// Usage
FUNCTION_START(BoolResult, foo, int some_int)
{
    Object obj;
    Object_init(&obj);
    CharResult some_char = int_to_char(some_int);
    reterr(some_char);
    RESULT = Object_use_char(&obj, val(some_char));
}
FUNCTION_END(Object_cleanup(&obj))
```

> *Note: Congratulations on noticing that the reterr macro is not MSVC-compatible in its current form. Consider the rewrite necessary for a port that does not have __typeof__.*

It may be tempting to build increasingly powerful macro abstractions, but doing so often shifts complexity from runtime behavior into compile-time structure, which can make debugging and onboarding significantly harder. As with all abstractions, the trade-off should be explicit and deliberate. Every rule, abstraction, or non-standard principle introduces a form of debt that must be accounted for later, so care should be taken to avoid introducing too many competing concepts.

### Performance, inline, and namespace pollution
It is important to remember that IronCLib is a header library, and even the premade files are header only. Efforts are made to give good performance and help compilers inline and optimize. However, it is the end user that is responsible for wrapping the functions of IronCLib if necessary, be it for performance reasons, decoupling from the library, or even avoiding namespace pollution. With that said, most inline functions in this library are very small, and any code unused by a code-generation macro is very likely to be thrown away by the compiler.

> *Note: The future project SteelCLib will provide more flexibility for managing these concerns.*

## Decouple from library and make a thread pool
This section discusses how to decouple from the IronC implementation of concurrency (which is of course an outlandish idea!), incorporate the previously mentioned global error system, and build a simple thread pool system. 

### Why use this?
By wrapping `ic_concurrency` behind an application-specific API and global error system, the codebase is decoupled from the underlying concurrency implementation, allowing it to be replaced or modified without affecting higher-level systems. A consistent error model is enforced across the application and can be included in this wrapping. 

Thread pools primarily improve performance by reusing a fixed set of threads instead of repeatedly creating and destroying them, which reduces overhead and improves throughput under load. They also provide a controlled way to schedule and execute work, limiting concurrency and making system behavior more predictable and easier to manage at scale.

### Premade
A header is [provided here](premade/parallel_work.h) that has already wrapped the concurrency implementation. It also contains a thread pool implementation.

### Convert to Error
Begin by writing a function that translates integer results into the global errors.

```c
#include "ironclib/ic_inline.h"
#include "global_error.h"

IC_HEADER_FUNC Error concurrency_result_to_error(const int concurrency_result)
{
    switch (concurrency_result)
    {
        case IC_CONCURRENCY_OK: return Error_NoError;
        case IC_CONCURRENCY_NULLREF: return Error_NullRef;
        case IC_CONCURRENCY_FAILURE: return Error_Runtime;
        case IC_CONCURRENCY_ALREADY_JOINED: return Error_InvalidState;
        case IC_CONCURRENCY_ALREADY_LOCKED: return Error_InvalidState;
        default: return Error_Unknown;
    }
}
```

### Wrap concurrency parts
Add typedefs for the IronC concurrency types (or even wrap them in structs), then wrap the underlying library functions.

```c
#include "ironclib/ic_concurrency.h"

// MUTEX API
typedef ic_mutex Mutex;
IC_HEADER_FUNC Error mutex_init(Mutex* const out_mutex) { return concurrency_result_to_error(ic_mutex_init(out_mutex)); }
IC_HEADER_FUNC Error mutex_trylock(Mutex* const mutex) { return concurrency_result_to_error(ic_mutex_trylock(mutex)); }
```

If the code-base uses result values then replacing `Error` with `VoidResult` is perfectly valid and can even work better in making the system feel more uniform (and will support the same try-propagation of errors).

> *Note: Do not wrap concurrency types in result types or opaque structs, as this will require disciplined knowledge of concurrency and the implementation of e.g. atomics, mutexes, and threads for all supported, and future supported, platforms.*

### Implement thread pool
A thread pool can be built on top of the wrapped concurrency API by combining a fixed set of worker threads, a shared work queue, a mutex, and a signaling primitive such as `Gate`. The important idea is that threads are created once during initialization and then continuously wait for work instead of repeatedly being created and destroyed.

The provided implementation `TaskPool` uses a bounded ring-buffer queue protected by a mutex:

```c
typedef struct TaskPool
{
    Task PRIVATE_workers[TASKPOOL_MAX_THREADS];
    uint32_t PRIVATE_worker_count;

    Mutex PRIVATE_mutex;
    Gate PRIVATE_work_gate;

    AtomicI32 PRIVATE_state;
    AtomicI32 PRIVATE_active_jobs;

    TaskPoolJob PRIVATE_jobs[TASKPOOL_MAX_PENDING_TASKS];

    uint32_t PRIVATE_head;
    uint32_t PRIVATE_tail;
    uint32_t PRIVATE_count;

} TaskPool;

// API
typedef struct TaskPool TaskPool;
typedef void (*TaskPoolFunction)(void* arg);
#define TASKPOOL_MAX_THREADS
#define TASKPOOL_MAX_PENDING_TASKS
Error task_pool_init(TaskPool* const out_pool, const uint32_t worker_count);
Error task_pool_submit(TaskPool* const pool, const TaskPoolFunction func, void* const arg, TaskCompletion* const out_completion);
Error task_pool_destroy(TaskPool* const pool);

typedef struct TaskCompletion TaskCompletion;
Error task_completion_wait(const TaskCompletion* const completion, const int32_t retry_period_ms, const int64_t timeout_ms);
```

> *Note: The only safe way to make an opaque struct containing concurrency objects is with forward declaration, heap memory allocation, and implementation in source file. If heap usage is of no concern then this is a good way to hide implementation details. Here however the prefix `PRIVATE_` is used so that if a user attempts to access the fields it will look like the wrong way to use the struct (which it is).*

Tasks are submitted into the queue while holding the mutex:

```c
Error task_pool_submit(TaskPool* const pool, const TaskPoolFunction func, void* const arg, TaskCompletion* const out_completion)
{
    mutex_lock(&pool->PRIVATE_mutex);

    pool->PRIVATE_jobs[pool->PRIVATE_tail] = (TaskPoolJob)
    {
        .PRIVATE_func = func,
        .PRIVATE_arg = arg,
        .PRIVATE_completion = out_completion
    };

    pool->PRIVATE_tail = (pool->PRIVATE_tail + 1) % TASKPOOL_MAX_PENDING_TASKS;
    pool->PRIVATE_count++;

    mutex_unlock(&pool->PRIVATE_mutex);

    return gate_signal_one(&pool->PRIVATE_work_gate);
}
```

Worker threads spend most of their lifetime sleeping on the gate until work becomes available:

```c
while (1)
{
    if (!Error_eq(gate_wait(&p->PRIVATE_work_gate), Error_NoError))
    {
        continue;
    }

    mutex_lock(&p->PRIVATE_mutex);

    if (p->PRIVATE_count == 0)
    {
        mutex_unlock(&p->PRIVATE_mutex);
        continue;
    }

    TaskPoolJob job = p->PRIVATE_jobs[p->PRIVATE_head];

    p->PRIVATE_head = (p->PRIVATE_head + 1) % TASKPOOL_MAX_PENDING_TASKS;
    p->PRIVATE_count--;

    mutex_unlock(&p->PRIVATE_mutex);

    job.PRIVATE_func(job.PRIVATE_arg);
}
```

The implementation also demonstrates useful higher-level behavior that applications often need, such as graceful draining, abort-style shutdown, bounded queues, completion tracking, timeout handling, and explicit lifecycle states. While the exact design will vary between projects, these ideas tend to form the foundation of practical thread pool systems.

Why is this not part of the library? Because the library focuses on making C a safer language to use, not provide every possible utility. Everything in the `premade` is outside the scope of the project and are examples of what IronCLib can offer. There are also many trade-offs to consider for a thread pool. Using condition variables or atomics or sleeping the thread will all have their own pros and cons for CPU, memory, and complexity, depending on if the pool expects long or short lasting tasks, or how many. Each decision creates branching paths of more decisions and complexity. For example, how should memory ownership of task arguments work? If the user should be allowed to know when the work is finished, how should the user be alerted? If via a boolean atomic, what is best method to not waste CPU and time when polling it? If via condition variable, should one be used to signal all task results or one condition variable per thread? 

Managing multiple threads is not the easiest aspect of coding, yet this library has made an attempt to make it easier to do safely with clear APIs, with portability, and documentation. The `TaskPool` being discussed is also tested in [concurrency_signal_test.h](../ironhammerc/tests/concurrency_signal_test.h) in the test suite. 

### Merging gates and broadcasts
For the future, it can be worthwhile to create wrapper of `ic_condition_variable` that merges the behavior of `ic_gate` and `ic_broadcast`. These two structs and their functions where made with safety and ease of use in mind, as well as being as lightweight as possible for this *header* library, but an event type that can both signal_one AND signal_all could definitely have use.

What is important to understand is that the gate and broadcast interfaces exist because condition variables do not have any state nor can they remember the past, so the gate and broadcast help manage this. But merging them requires a clear understanding of concurrency and a clear definition of desired behavior. What needs does the application have (e.g. ordered or unordered queue?) and how would the implementation differ from what professionals expect of event-type APIs? With that said, here is an idea for an API.

```c
#include "ironclib/ic_typenum.h"
#include "ironclib/ic_concurrency.h"
#include "ironclib/ic_concurrency_signal.h"

#define PASSPOINT_MODE_LIST(X, Type) \
    X(Type, Gated, 0, "One-at-a-time passing, counting permits") \
    X(Type, Open, 1, "All automatic immediate pass, no permit counting")
IC_TYPENUM_FULL(PasspointMode, uint8_t, USE_MODE_LIST)

typedef struct Passpoint {
    ConditionVariable cv;
    Mutex mtx;
    PasspointMode mode;
    uint32_t permits;   // Only counted in GATED mode, neither increase nor decrease in OPEN mode
} Passpoint;

Error passpoint_init(Passpoint* out_p);
Error passpoint_destroy(Passpoint* p);
Error passpoint_get_mode(Passpoint* p, PasspointMode* out_mode);

Error passpoint_wait(Passpoint* p);
Error passpoint_signal_one(Passpoint* p);           // GATED: increments permit count, wakes one waiter; OPEN: no-op)
Error passpoint_open_and_signal_all(Passpoint* p);  // GATED: wakes all and switches to OPEN (sticky); OPEN: no-op
Error passpoint_set_gated(Passpoint* p);            // OPEN: Switch back to GATED mode (sticky), permits = 0; GATED: no-op
```

## Good practices for co-jobs
A good first step when using co-jobs is to centralize configuration macros in a single header that is included everywhere the scheduler is used. This ensures consistent limits and scoring behavior across the entire codebase, and prevents subtle mismatches between translation units.

```c
// my_app_ic_co_job.h

#define IC_CO_MAX_STEPS 20
#define IC_CO_MAX_JOBS 5

#ifdef IC_CO_SCORE
#error IC_CO_SCORE is already defined (conflicting configuration)
#endif

#define IC_CO_SCORE(priority, credits, state) ((priority) + (credits) + (state))

#include "ironclib/ic_co_job.h"
```

A single global scoring function is often enough for most systems, and is the simplest and most predictable option. However, it is also valid to define different scoring strategies in different source files if the system benefits from local scheduling behavior. In that case, each override must be explicit and must fail loudly if it does not take effect (as shown above), ensuring that no silent fallback behavior is introduced.

### Creating co-jobs
There are three supported ways, each intended for a different usage pattern.

> *Note: Once constructed, a job is considered immutable and should not be modified after being added to a scheduler.*

#### Static const job using IC_MAKE_CO_JOB

For small fixed jobs, the simplest approach is to define a static const job directly with the macro:

```c
// job.h
#include "my_app_ic_co_job.h"

struct ProcessingJobData
{
    int i;
};
typedef struct ProcessingJobData ProcessingJobData;

const ic_co_job* get_processing_job(void);
// job.c
#include "job.h"
#include <stdio.h>

static void processing_job_step_1(void* data);
static void processing_job_step_2(void* data);
static void processing_job_step_3(void* data);

static const ic_co_job g_processing_job = IC_MAKE_CO_JOB(
    processing_job_step_1,
    processing_job_step_2,
    processing_job_step_3
);

const ic_co_job* get_processing_job(void)
{
    return &g_processing_job;
}
```

This approach is compact, efficient, and works well when the job structure is known entirely at compile time.

> *Note: IC_MAKE_CO_JOB supports a maximum of 10 steps. For larger or dynamically assembled jobs, use runtime construction instead.*

#### Static job with explicit initializer
A job can also be stored as static storage and initialized in a dedicated setup function:

```c
// job.c
#include "job.h"

static void processing_job_step_1(void* data);
static void processing_job_step_2(void* data);
static void processing_job_step_3(void* data);

static ic_co_job g_processing_job;

void init_processing_job(void)
{
    g_processing_job = ic_co_job_create();

    ic_co_job_add_step(&g_processing_job, processing_job_step_1);
    ic_co_job_add_step(&g_processing_job, processing_job_step_2);
    ic_co_job_add_step(&g_processing_job, processing_job_step_3);
}

const ic_co_job* get_processing_job(void)
{
    return &g_processing_job;
}
```

This is useful when the job must be assembled conditionally or configured during startup while still remaining globally reusable.

#### Dynamic job creation
The most portable and explicit way to construct jobs is through runtime construction. This allows jobs to be created whenever they are needed, without relying on preprocessor generation. It is the most flexible model and works well when jobs are data-driven or instantiated multiple times.

```c
ic_co_job make_processing_job(void)
{
    ic_co_job job = ic_co_job_create();

    ic_co_job_add_step(&job, processing_job_step_1);
    ic_co_job_add_step(&job, processing_job_step_2);
    ic_co_job_add_step(&job, processing_job_step_3);

    return job;
}
```

### Data ownership rule
The scheduler never allocates, frees, or assumes ownership of user-provided data in any model. A consistent ownership model for job data should be chosen per system. The scheduler itself never owns job data; it only stores and forwards a `void*` context pointer during execution. This means the lifetime and responsibility of the data must always be defined externally by the user. Here are two good options:

#### Job-managed data model
The job conceptually owns and manages its own state, even if the actual memory storage is provided externally by the caller. Initialization, updates, and cleanup logic are handled entirely by the job steps themselves. This keeps the job self-contained and makes future edits easier, since new internal state and setup behavior can be added without changing how the rest of the system interacts with the job.

#### Externally managed data model
The data is created and destroyed outside the job using explicit constructor/destructor-style functions:

```c
ProcessingJobData make_processing_job_data(void);
void destroy_processing_job_data(ProcessingJobData*);
```

This makes ownership and lifetime fully explicit, and most importantly allows fully configured data to be passed into the job before scheduling. This is useful when jobs depend on external configuration, shared state, or runtime-generated input. The tradeoff is that the caller must now coordinate setup and cleanup correctly. Even if destruction is currently trivial, defining a destroy function (or an empty macro with same signature) from the beginning keeps the ownership model stable and allows cleanup behavior to evolve later without requiring changes to calling code.

### Running scheduler to completion and cooperative early exit
Jobs are designed to run to completion, and normally execute inside a scheduler loop such as:

```c
while (!ic_co_scheduler_is_done(&sched))
{
    ic_co_scheduler_tick(&sched);

    if (ic_co_scheduler_credits(&sched) == 0)
    {
        // End of scheduler cycle
    }
}
```

The scheduler itself does not provide forced cancellation or emergency abort behavior. Supporting hard cancellation would require either additional cleanup callbacks or more complex step function signatures, imposing assumptions and overhead on every job regardless of whether cancellation is needed.

Instead, early termination is expected to be handled cooperatively through shared job state; it is not a cancellation mechanism. Since each step already receives user-controlled data through the `void*` context pointer, jobs can observe external state between steps and simply perform no further work when an abort condition has been signaled.

This model treats a co-job as a single larger operation divided into resumable steps, rather than as a collection of unrelated functions. The scheduler controls when execution advances, while the job itself controls whether meaningful work should continue. A practical pattern is to store shared execution state in a single struct that is visible to all participating jobs:

```c
typedef struct SharedData
{
    int x, y, z;
    bool abort;
    bool failure_detected;
} SharedData;

bool run_processing_sequence(int x, int y, int z)
{
    SharedData shared_data = {
        .x = x,
        .y = y,
        .z = z,
        .abort = false,
        .failure_detected = false
    };

    ic_co_scheduler sched = ic_make_co_scheduler();
    ic_co_scheduler_add_job(&sched, get_job_a(), &shared_data, 1, 1);
    ic_co_scheduler_add_job(&sched, get_job_b(), &shared_data, 1, 1);

    while (!ic_co_scheduler_is_done(&sched))
    {
        ic_co_scheduler_tick(&sched);

        if (shared_data.abort)
        {
            break;
        }
    }

    if (shared_data.abort)
    {
        // Allow remaining jobs to observe abort state
        const int steps_per_cycle = 2; // weight of all jobs combined in THIS example
        for (int i = 0; i < steps_per_cycle; i++)
        {
            ic_co_scheduler_tick(&sched);
        }
    }

    return !(shared_data.failure_detected || shared_data.abort);
}
```

The scheduler is not stopped on abort so that remaining steps can observe and react to the updated state. In this model, jobs remain structurally complete while still supporting coordinated shutdown behavior. Abort handling stays explicit, local to the shared state model, and fully under application control rather than being embedded into the scheduler itself.

> *Note: Schedulers do not have a reset. If a scheduling function is run multiple times in quick succession and adding jobs to it causes a performance impact, then the expected method to handle it is to create a `make_X_scheduler_context` function that initializes a scheduler once, AND the data for the jobs, and then returns copies from it. The returned context could e.g. be `struct XSchedulerContext { XSharedData data; ic_co_scheduler sched; }`.*

## What NOT to do
- Do not mix raw C implementation patterns (malloc, enums, structs) with IronCLib abstractions within the same architectural layer; each layer should follow a single, consistent paradigm. Once IronCLib is used within a module, it should be applied consistently. Ownership, data flow, and error handling should follow a unified convention throughout that module.
- Do not bypass result types for convenience, as this can weaken the explicit error model.
- Do not introduce multiple competing memory management or error handling systems within the same codebase.
- Do not use IronCLib macros to build control-flow systems; structured patterns like TRY are allowed, but macros should not replace normal C control flow or introduce hidden execution logic (macros are appropriate when used for structure and repetition reduction, but should not be avoided purely out of caution or preference for verbosity).