#ifndef CRUCIBLEC_WORKSHOPC_MOVE_TAGS_H
#define CRUCIBLEC_WORKSHOPC_MOVE_TAGS_H

#ifdef WORKSHOPC_PARSING

static inline void* workshopc_out(void* arg) { return arg; }
static inline void* workshopc_move(void* arg) { return arg; }
static inline void* workshopc_modify(void* arg) { return arg; }

#define out(expr)  ((__typeof__(expr))workshopc_out((void*)(expr)))
#define move(expr) ((__typeof__(expr))workshopc_move((void*)(expr)))
#define mut(expr)  ((__typeof__(expr))workshopc_modify((void*)(expr)))

#define moved __attribute__((annotate("workshopc_move")))
#define output  __attribute__((annotate("workshopc_out")))
#define mutable  __attribute__((annotate("workshopc_modify")))

#else

#define out(expr)  (expr)
#define move(expr) (expr)
#define mut(expr)  (expr)

#define moved
#define output
#define mutable

#endif

#endif // CRUCIBLEC_WORKSHOPC_MOVE_TAGS_H