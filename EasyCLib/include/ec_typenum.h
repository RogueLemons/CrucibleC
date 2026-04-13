#ifndef EC_TYPENUM_H
#define EC_TYPENUM_H

/*
===============================================================================
EC Typenum (Full System - X-Macro Based, Header Safe, Enum-Free)

C Compatibility:
- C89+ core support
- C99+ inline where available via EC_HEADER_SAFE
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

EC_TYPENUM_FULL(Status, int, STATUS_LIST)

===============================================================================

========================
GENERATED CODE (conceptual)
========================

typedef struct {
    int Status_value;
} Status;

EC_HEADER_SAFE int Status_get(const Status v);
EC_HEADER_SAFE int Status_eq(const Status a, const Status b);
EC_HEADER_SAFE const char* Status_to_string(const Status v);

static const Status Status_Ok = {0};
static const Status Status_MechanicalFailure = {1};
static const Status Status_ElectricalFailure = {2};
static const Status Status_SoftwareFailure = {3};

===============================================================================
*/

#include "ec_inline.h"


/*
===============================================================================
Core type + helpers
===============================================================================
*/

#define EC_TYPENUM(name, type, LIST) \
    typedef struct { \
        type name##_value; \
    } name; \
    \
    EC_HEADER_SAFE type name##_get(const name v) { \
        return v.name##_value; \
    } \
    \
    EC_HEADER_SAFE int name##_eq(const name a, const name b) { \
        return a.name##_value == b.name##_value; \
    }


/*
===============================================================================
To-string support (switch-based, user-defined strings)
===============================================================================
*/

#define EC_TYPENUM_TO_STRING_CASE(Type, name, value, str) \
    case value: return str;


#define EC_TYPENUM_TO_STRING(Type, LIST) \
    EC_HEADER_SAFE const char* Type##_to_string(const Type v) \
    { \
        switch (Type##_get(v)) { \
            LIST(EC_TYPENUM_TO_STRING_CASE, Type) \
            default: return "Invalid"; \
        } \
    }


/*
===============================================================================
Constant generation (typed named values)
===============================================================================
*/

#define EC_TYPENUM_CONST(Type, name, value, str) \
    static const Type Type##_##name = { value };


#define EC_TYPENUM_GENERATE_CONSTS(Type, LIST) \
    LIST(EC_TYPENUM_CONST, Type)


/*
===============================================================================
FULL COMPOSITE MACRO (recommended entry point)
===============================================================================
*/

#define EC_TYPENUM_FULL(Type, type, LIST) \
    EC_TYPENUM(Type, type, LIST) \
    EC_TYPENUM_TO_STRING(Type, LIST) \
    EC_TYPENUM_GENERATE_CONSTS(Type, LIST)


#endif