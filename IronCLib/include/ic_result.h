#ifndef IC_RESULT_H
#define IC_RESULT_H

/*
===============================================================================
IC RESULT

C Compatibility:
- C89: supported
- C11+: recommended
- GCC / Clang / MSVC supported

Dependencies:
- ic_inline.h        -> IC_HEADER_FUNC
- ic_static_assert.h -> optional compile-time checks

Design:
- Result<T,E> type
- automatic constructor generation
- TRY propagation that is type-safe via assignment rules

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
    IC_HEADER_FUNC name name##_ok(value_type v) { \
        name r; \
        r.ok = 1; \
        r.data.value = v; \
        return r; \
    } \
    \
    IC_HEADER_FUNC name name##_err(error_type e) { \
        name r; \
        r.ok = 0; \
        r.data.error = e; \
        return r; \
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
- zero runtime overhead in optimized builds

==============================================================================*/

#define IC_TRY_RETURN_ERR_AS(type, result) \
    do { \
        type const ic_try_res = (result); \
        if (!ic_try_res.ok) { \
            return type##_err(ic_try_res.data.error); \
        } \
    } while (0)


#endif