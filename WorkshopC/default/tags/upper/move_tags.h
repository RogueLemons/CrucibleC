#ifndef CRUCIBLEC_WORKSHOPC_MOVE_TAGS_H
#define CRUCIBLEC_WORKSHOPC_MOVE_TAGS_H

#ifdef WORKSHOPC_PARSING

static inline void* workshopc_out(void* arg) { return arg; }
static inline void* workshopc_move(void* arg) { return arg; }
static inline void* workshopc_modify(void* arg) { return arg; }

#define MOVE(expr) ((__typeof__(expr))workshopc_move((void*)(expr)))
#define OUT(expr)  ((__typeof__(expr))workshopc_out((void*)(expr)))
#define MUT(expr)  ((__typeof__(expr))workshopc_modify((void*)(expr)))

#define MOVED __attribute__((annotate("workshopc_move")))
#define OUTPUT __attribute__((annotate("workshopc_out")))
#define MUTABLE  __attribute__((annotate("workshopc_modify")))

#else

#define MOVE(expr) (expr)
#define OUT(expr)  (expr)
#define MUT(expr)  (expr)

#define MOVED
#define OUTPUT
#define MUTABLE

#endif

#endif // CRUCIBLEC_WORKSHOPC_MOVE_TAGS_H