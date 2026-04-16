#ifndef IC_BOUNDED_LOOP_H
#define IC_BOUNDED_LOOP_H

/*
===============================================================================
IC Bounded Loop Abstractions

C Compatibility:
- C89-compatible in practice (GCC/Clang/MSVC), uses compiler-supported extensions for for-loop declarations
- C89+ supported (size_t requirement applies externally)
- MSVC / GCC / Clang supported

Provides:
    IC_BOUNDED_WHILE      -> bounded while loop with safety limit
    IC_BOUNDED_DO_WHILE   -> bounded do-while loop with safety limit
    IC89_BOUNDED_WHILE    -> C89-compatible bounded while loop with user-defined iterator

Purpose:
- Prevent accidental infinite loops
- Provide deterministic maximum iteration limits
- Preserve natural C loop semantics
- Make loop safety explicit and readable in source code
- Treat invalid (<= 0) loop bounds as zero iterations

===============================================================================
*/

#include <stddef.h>

/*
-------------------------------------------------------------------------------
IC_BOUNDED_WHILE

Purpose:
- Executes a loop while:
    1. condition is true
    2. iteration count < max_loops

Safety guarantees:
- Hard upper bound on iterations (prevents infinite loops)
- max_loops is evaluated exactly once
- max_loops <= 0 is treated as 0 (loop does not execute)
- Uses unsigned size_t after validation to avoid overflow
- Loop counter is scoped locally via macro expansion

Implementation notes:
- Expands to a single for-loop
- Uses a per-line unique counter variable
- No hidden state shared between macros

Warnings:
- condition is evaluated every iteration and may have side effects
- __LINE__-based naming may collide if multiple macros are used on same line
- Avoid user variables starting with `_ic_`

Example:
    IC_BOUNDED_WHILE(x != NULL, 1000) {
        x = x->next;
    }
-------------------------------------------------------------------------------
*/
#define IC_CONCAT_IMPL(a, b) a##b
#define IC_CONCAT(a, b) IC_CONCAT_IMPL(a, b)

#define IC_BOUNDED_WHILE(condition, max_loops)                              \
    for (size_t IC_CONCAT(_ic_loop_, __LINE__) = 0,                         \
                IC_CONCAT(_ic_max_, __LINE__) =                             \
                    ((max_loops) <= 0) ? 0u : (size_t)(max_loops);          \
         (IC_CONCAT(_ic_loop_, __LINE__) < IC_CONCAT(_ic_max_, __LINE__))   \
         && (condition);                                                    \
         ++IC_CONCAT(_ic_loop_, __LINE__))

/*
-------------------------------------------------------------------------------
IC_BOUNDED_DO_WHILE

Purpose:
- Executes a loop at least once, then continues while:
    1. condition is true
    2. iteration count < max_loops

Safety guarantees:
- Guaranteed single evaluation of max_loops
- Hard upper bound on iterations
- max_loops <= 0 results in zero iterations
- First iteration is always executed unless max_loops is 0
- Fully predictable termination behavior

Implementation notes:
- Uses a first-iteration flag (_ic_first)
- Expands into a single for-loop construct
- No hidden runtime system or shared state

Warnings:
- condition is evaluated after each iteration
- side effects in condition will be executed repeatedly
- avoid macro name collisions with `_ic_` prefix

Example:
    IC_BOUNDED_DO_WHILE(!success, 5) {
        success = try_connect();
        attempts++;
    }
-------------------------------------------------------------------------------
*/
#define IC_BOUNDED_DO_WHILE(condition, max_loops)                           \
    for (size_t IC_CONCAT(_ic_loop_, __LINE__) = 0,                         \
                IC_CONCAT(_ic_max_, __LINE__) =                             \
                    ((max_loops) <= 0) ? 0u : (size_t)(max_loops),          \
                IC_CONCAT(_ic_first_, __LINE__) = 1;                        \
         IC_CONCAT(_ic_first_, __LINE__) ||                                 \
         ((IC_CONCAT(_ic_loop_, __LINE__) < IC_CONCAT(_ic_max_, __LINE__))  \
          && (condition));                                                  \
         IC_CONCAT(_ic_first_, __LINE__) = 0,                               \
         ++IC_CONCAT(_ic_loop_, __LINE__))

         
/*
-------------------------------------------------------------------------------
IC89_BOUNDED_WHILE

Purpose:
- Strict C89-compatible bounded loop abstraction
- Executes a loop while:
    1. condition is true
    2. iteration count < limit

Design goals:
- Maximum portability (strict C89)
- No hidden state or internal variables
- Explicit loop counter provided by the user
- Deterministic iteration bound behavior

Safety guarantees:
- If limit <= 0, loop executes 0 times
- Works with all C89-compliant compilers
- Uses only standard for-loop semantics
- break and continue behave exactly as in a normal for-loop

Important behavioral notes:
- The iterator variable is fully controlled by the caller and WILL be overwritten:
    it is always initialized to 0 at loop start
- Both `limit` and `condition` are evaluated repeatedly:
    - `limit` may be evaluated multiple times (once per iteration in condition)
    - `condition` is evaluated every iteration as part of the loop condition
- Side effects in `limit` or `condition` expressions may result in unintended behavior

Warnings:
- Do NOT pass expressions with side effects to `limit`
  (e.g. function calls, increments, assignments)
- Do NOT assume `limit` is evaluated only once
- The iterator variable is always reset to 0 by the macro
- Avoid relying on evaluation order of complex expressions in `condition`

Implementation notes:
- Expands directly into a standard C for-loop
- No hidden temporaries or internal state variables
- Fully transparent control flow (no macro magic beyond syntactic sugar)

Example:
    int it;
    int i = 0;
    IC89_BOUNDED_WHILE(i < 10, 100, it) {
        i++;
        printf("%d\n", i);
    }
-------------------------------------------------------------------------------
*/         
         
#define IC89_BOUNDED_WHILE(cond, limit, it)         \
    for ((it) = 0;                                  \
    (it) < ((limit) <= 0 ? 0 : (limit)) && (cond);  \
    ++(it))

#endif // IC_BOUNDED_LOOP_H