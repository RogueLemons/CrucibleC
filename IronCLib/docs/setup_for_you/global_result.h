#ifndef SETUP_FOR_YOU_GLOBAL_RESULT_H
#define SETUP_FOR_YOU_GLOBAL_RESULT_H

#include "ic_result.h"
#include "global_error.h"
#include <stddef.h>
#include <stdbool.h>
// #include "numbers.h" // Uncomment if you want to use the predefined number types in your result values

/*
USAGE:
    - Include this header in your application to define a global Result type using the global error type.
    - Use GLOBAL_RESULT_TYPE(Type) to define Result types for your specific value types.
    - Define result types next to struct definitions for better organization, unless a common type, then extend this file.
    - Example: if (result.ok) { foo(result.data.value); } else { bar(result.data.error); }

INCLUDED RESULT TYPES (all using global Error type):
      RESULT TYPE       VALUE TYPE      OK CONSTRUCTOR EXAMPLE                  ERR CONSTRUCTOR EXAMPLE
      ---------------------------------------------------------------------------------------------------------------------
    - CharResult:       char,           CharResult_ok('A'),                     CharResult_err(Error_Runtime)
    - IntResult:        int,            IntResult_ok(42),                       IntResult_err(Error_Runtime)
    - FloatResult:      float,          FloatResult_ok(3.14f),                  FloatResult_err(Error_Runtime)
    - DoubleResult:     double,         DoubleResult_ok(2.718),                 DoubleResult_err(Error_Runtime)
    - UIntResult:       unsigned int,   UIntResult_ok(100),                     UIntResult_err(Error_Runtime)
    - StringViewResult: const char*,    StringViewResult_ok("Hello, World!"),   StringViewResult_err(Error_Runtime)
    - VoidPtrResult:    void*,          VoidPtrResult_ok(some_memory),          VoidPtrResult_err(Error_Runtime)
    - SizeResult:       size_t,         SizeResult_ok(1024),                    SizeResult_err(Error_Runtime)
    - BoolResult:       bool,           BoolResult_ok(true),                    BoolResult_err(Error_Runtime)
    - VoidResult:       VoidType,       VoidResult_ok(),                        VoidResult_err(Error_Runtime)

IF numbers.h IS INCLUDED:
    - I8Result (i8), I16Result (i16), I32Result (i32), I64Result (i64)
    - U8Result (u8), U16Result (u16), U32Result (u32), U64Result (u64)
    - F32Result (f32), F64Result (f64)
*/

// Define a macro to generate standard Result types for the application using the global error type
#define INNER_RESULT_NAME_IMPL(Type) Type##Result
#define INNER_RESULT_NAME(Type) INNER_RESULT_NAME_IMPL(Type)
#define INNER_GLOBAL_RESULT_TYPE_IMPL(Name, Type) IC_RESULT_TYPE(Name, Type, Error)

// API
#define GLOBAL_RESULT_TYPE(Type) INNER_GLOBAL_RESULT_TYPE_IMPL(INNER_RESULT_NAME(Type), Type)

// Common result types
IC_RESULT_TYPE(CharResult, char, Error)
IC_RESULT_TYPE(IntResult, int, Error)
IC_RESULT_TYPE(FloatResult, float, Error)
IC_RESULT_TYPE(DoubleResult, double, Error)
IC_RESULT_TYPE(UIntResult, unsigned int, Error)
typedef const char* StringView;
GLOBAL_RESULT_TYPE(StringView)
typedef void* VoidPtr;
GLOBAL_RESULT_TYPE(VoidPtr)
IC_RESULT_TYPE(SizeResult, size_t, Error)
IC_RESULT_TYPE(BoolResult, bool, Error)

// VoidResult
typedef struct { char _; } VoidType;
GLOBAL_RESULT_TYPE(VoidType)
typedef VoidTypeResult VoidResult;
#define VoidResult_ok() VoidTypeResult_ok((VoidType){0})
#define VoidResult_err(x) VoidTypeResult_err(x)

// If numbers.h is included, number result types are also generated
#ifdef SETUP_FOR_YOU_NUMBERS_H
IC_RESULT_TYPE(I8Result, i8, Error)
IC_RESULT_TYPE(I16Result, i16, Error)
IC_RESULT_TYPE(I32Result, i32, Error)
IC_RESULT_TYPE(I64Result, i64, Error)
IC_RESULT_TYPE(U8Result, u8, Error)
IC_RESULT_TYPE(U16Result, u16, Error)
IC_RESULT_TYPE(U32Result, u32, Error)
IC_RESULT_TYPE(U64Result, u64, Error)
IC_RESULT_TYPE(F32Result, f32, Error)
IC_RESULT_TYPE(F64Result, f64, Error)
#endif

#endif // SETUP_FOR_YOU_GLOBAL_RESULT_H