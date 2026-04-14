#ifndef IC_TYPENUM_H
#define IC_TYPENUM_H

/*
===============================================================================
IC Strong Integer Enum Wrapper (X-Macro Based, Header Safe, Enum-Free)

C Compatibility:
- C89+ core support
- C99+ inline where available via IC_HEADER_FUNC
- Fully header-safe (no linker multiple-definition issues)
- No use of enum types (ABI-stable, deterministic)

===============================================================================

========================
MINIMAL USER EXAMPLE
========================

1) Define X-macro list:

#define STATUS_LIST(X, Type) \
    X(Type, Ok, 0, "Everything is fine") \
    X(Type, MechanicalFailure, 1, "Mechanical failure detected") \
    X(Type, ElectricalFailure, 2, "Electrical failure detected") \
    X(Type, SoftwareFailure, 3, "Software failure detected")

2) Declare everything:

IC_TYPENUM_FULL(Status, int, STATUS_LIST)

===============================================================================

========================
GENERATED CODE (conceptual)
========================

typedef struct {
    int Status_value;
} Status;

IC_HEADER_FUNC int Status_get(const Status v);
IC_HEADER_FUNC int Status_eq(const Status a, const Status b);
IC_HEADER_FUNC const char* Status_to_string(const Status v);

static const Status Status_Ok = {0};
static const Status Status_MechanicalFailure = {1};
static const Status Status_ElectricalFailure = {2};
static const Status Status_SoftwareFailure = {3};

===============================================================================
*/

#include "ic_inline.h"


/*
===============================================================================
Core type + helpers
===============================================================================
*/
#define IC_INNER_ENUM_TYPE_MAX_SIZE (2 * sizeof(void *))

#define IC_TYPENUM(name, type, LIST) \
    IC_STATIC_ASSERT(sizeof(type) <= IC_INNER_ENUM_TYPE_MAX_SIZE, "Inner enum type is too large, define typenum and related functions manually") \
    \
    typedef struct { \
        type name##_value; \
    } name; \
    \
    IC_HEADER_FUNC type name##_get(const name v) { \
        return v.name##_value; \
    } \
    \
    IC_HEADER_FUNC int name##_eq(const name a, const name b) { \
        return a.name##_value == b.name##_value; \
    }


/*
===============================================================================
To-string support (switch-based, user-defined strings)
===============================================================================
*/

#define IC_TYPENUM_TO_STRING_CASE(Type, name, value, str) \
    case value: return str;


#define IC_TYPENUM_TO_STRING(Type, LIST) \
    IC_HEADER_FUNC const char* Type##_to_string(const Type v) \
    { \
        switch (Type##_get(v)) { \
            LIST(IC_TYPENUM_TO_STRING_CASE, Type) \
            default: return "Unknown " #Type; \
        } \
    }


/*
===============================================================================
Constant generation (typed named values)
===============================================================================
*/

#define IC_TYPENUM_CONST(Type, name, value, str) \
    static const Type Type##_##name = { value };


#define IC_TYPENUM_GENERATE_CONSTS(Type, LIST) \
    LIST(IC_TYPENUM_CONST, Type)


/*
===============================================================================
FULL COMPOSITE MACRO (recommended entry point)
===============================================================================
*/

#define IC_TYPENUM_FULL(Type, type, LIST) \
    IC_TYPENUM(Type, type, LIST) \
    IC_TYPENUM_TO_STRING(Type, LIST) \
    IC_TYPENUM_GENERATE_CONSTS(Type, LIST)


#endif