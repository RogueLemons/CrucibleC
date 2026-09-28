#ifndef CRUCIBLEC_WORKSHOPC_MOVE_TAGS_LOWER_H
#define CRUCIBLEC_WORKSHOPC_MOVE_TAGS_LOWER_H

// Each side of a call says what it does with the pointer:
//
//   void consume(receives item* value);       consume(give(value));
//   void create(initializes item** result);   create(overwrite(&value));
//   void edit(borrows item* value);           edit(lend(value));
//
// A macro replaces its name everywhere after this header, so include it
// after system and third party headers.

#ifdef WORKSHOPC_PARSING

static inline void* workshopc_out(void* arg) { return arg; }
static inline void* workshopc_move(void* arg) { return arg; }
static inline void* workshopc_modify(void* arg) { return arg; }

#define give(expr)      ((__typeof__(expr))workshopc_move((void*)(expr)))
#define overwrite(expr) ((__typeof__(expr))workshopc_out((void*)(expr)))
#define lend(expr)      ((__typeof__(expr))workshopc_modify((void*)(expr)))

#define receives    __attribute__((annotate("workshopc_move")))
#define initializes __attribute__((annotate("workshopc_out")))
#define borrows     __attribute__((annotate("workshopc_modify")))

#else

#define give(expr)      (expr)
#define overwrite(expr) (expr)
#define lend(expr)      (expr)

#define receives
#define initializes
#define borrows

#endif

#endif // CRUCIBLEC_WORKSHOPC_MOVE_TAGS_LOWER_H
