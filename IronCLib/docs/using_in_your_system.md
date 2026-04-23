# Using in your system
The following sections include topics for using IronCLib in a code base. This is a continuation of [the header library documentation](header_library.md), where this document describes design patterns built on top of IronCLib (not the library itself).

## Table of Contents
* [Using in your system](#using-in-your-system)
* [Create a standardized, type-safe error system](#create-a-standardized-type-safe-error-system)
* [Create one entrypoint for memory allocation](#create-one-entrypoint-for-memory-allocation)
* [Write with rules for structs and opaque storage](#write-with-rules-for-structs-and-opaque-storage)
* [Make it your own](#make-it-your-own)

## Create a standardized, type-safe error system
The following will provide an example, showing how to use this library to create a standardized error system, working like an exception-as-return-value system, with one standardized error type for the whole application. 

### Why use this?
C has no consistent error handling model, which leads to mixed return codes, null checks, and implicit failure states that are easy to miss and hard to maintain. This system replaces that with a single, type-safe error model where all functions return explicit result types and all errors come from one centralized typenum. This makes failures visible in the type system, consistent across the codebase, and easy to extend without breaking existing code.

It turns error handling into explicit program structure, improving readability, reducing hidden bugs, and making control flow predictable and uniform.

### Setup for you
A header file full of global standard errors is [provided here](setup_for_you/global_error.h), and a header for creating the standard results with the global errors is [provided here](setup_for_you/global_result.h).

### my_app_result.h
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
#include "ic_typenum.h"

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

#### Usage
```c
int* int_list = allocate_memory(sizeof(int), 5, AllocMode_Standard, 0);
```

### Expand it
With a centralized memory allocator in place, let's begin expanding it. 

#### my_app_memory.h
Ease-of-use macros can be added. *Note: Adding these macros can make it harder to scan the code for all memory allocations.*

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

Self-implemented memory allocators can also be used instead, such as an allocator using a memory pool on the stack, and they can be added or removed at any time in the project for project-wide improvement. *Note: A stack based allocator can make it worthwhile to use "simple" struct forward declarations instead of using opaque storage.*

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
#include "ic_opaque_storage.h"

#define STRING_SIZE   (24)
#define STRING_ALIGN  (8)
IC_OPAQUE_STORAGE(String, STRING_ALIGN, STRING_SIZE)

void string_init(String* const out_string);
void string_init_with(String* const out_string, const char* const c_str);
void string_cleanup(String* const out_string);

void string_set(String* const out_string, const char* const c_str); 
void string_add(String* const out_string, const String* const other_string); 
void string_add_chars(String* const out_string, const char* const c_str) 
const char* string_get_data(const String* const s); 
size_t string_get_size(const String* const s); 
size_t string_get_capacity(const String* const s);
```

#### Usage
```c
void foo()
{
    string str;
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
#include "ic_opaque_storage.h"
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
(TODO)
