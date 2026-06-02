#ifndef TESTS_HEADERS_MOVE_TAGS_H
#define TESTS_HEADERS_MOVE_TAGS_H

#ifdef WORKSHOPC_PARSING

static inline void* workshopc_out(void* arg) { return arg; }
static inline void* workshopc_move(void* arg) { return arg; }
static inline void* workshopc_modify(void* arg) { return arg; }

#define out_cast(expr)  ((__typeof__(expr))workshopc_out((void*)(expr)))
#define move_cast(expr) ((__typeof__(expr))workshopc_move((void*)(expr)))
#define mod_cast(expr)  ((__typeof__(expr))workshopc_modify((void*)(expr)))

#define move __attribute__((annotate("workshopc_move")))
#define out  __attribute__((annotate("workshopc_out")))
#define mod  __attribute__((annotate("workshopc_modify")))

#else

#define out_cast(expr)  (expr)
#define move_cast(expr) (expr)
#define mod_cast(expr)  (expr)

#define move
#define out
#define mod

#endif

#endif // TESTS_HEADERS_MOVE_TAGS_H