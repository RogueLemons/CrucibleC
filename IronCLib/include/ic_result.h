#ifndef IC_RESULT_H
#define IC_RESULT_H

/*
===============================================================================
IC_RESULT.H

C Compatibility:
- C99: fully supported
- C11+: recommended
- GCC / Clang / MSVC supported

Dependencies:
- ic_inline.h        -> IC_HEADER_FUNC
- ic_static_assert.h -> optional compile-time checks

Design:
- Result<T,E> type
- automatic constructor generation
- safe TRY propagation with compile-time type check

-------------------------------------------------------------------------------

Example Usage:

    #include <stdio.h>
    #include "ic_result.h"

    // Define a Result type: char value, int error
    IC_RESULT_TYPE(CharResult, char, int)

    CharResult get_letter(int ok) {
        if (ok) {
            return CharResult_ok('A');
        } else {
            return CharResult_err(-1);
        }
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

===============================================================================
*/

#include "ic_inline.h"

/*==============================================================================
  RESULT TYPE + AUTOMATIC CONSTRUCTORS

This macro generates:

1. Result struct type
2. ok constructor
3. error constructor

==============================================================================*/

#define IC_RESULT_TYPE(name, value_type, error_type) \
    typedef struct { \
        int ok; \
        union { \
            value_type value; \
            error_type error; \
        } data; \
    } name; \
    \
    IC_HEADER_FUNC name name##_ok(const value_type v) { \
        return (name){ .ok = 1, .data.value = v }; \
    } \
    \
    IC_HEADER_FUNC name name##_err(const error_type e) { \
        return (name){ .ok = 0, .data.error = e }; \
    }


/*==============================================================================
  ACCESS HELPERS
==============================================================================*/

#define IC_RESULT_IS_OK(r) ((r).ok)
#define IC_RESULT_VALUE(r) ((r).data.value)
#define IC_RESULT_ERROR(r) ((r).data.error)


/*==============================================================================
  TRY PROPAGATION WITH TYPE SAFETY CHECK

- early return on error
- compile-time compatibility check via struct initialization trick
- zero runtime overhead in optimized builds

==============================================================================*/

#define IC_TRY_RETURN_ERR_AS(type, result) \
    do { \
        if (!(result).ok) { \
            (void)((type){ .data.error = (result).data.error }); \
            return type##_err((result).data.error); \
        } \
    } while (0)


#endif