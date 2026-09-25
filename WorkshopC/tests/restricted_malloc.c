// #include <stddef.h>
// #include <stdio.h>
// #include <string.h>
// #include <sys/types.h>

// Declared here instead of including <stdlib.h>, and some of them are not
// declared by every C library (e.g. MinGW)
typedef unsigned long size_t;
typedef long ssize_t;
typedef int FILE;
#define stdin ((FILE *)0)
#define NULL ((void *)0)
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *memory, size_t size);
void free(void *memory);
char *strdup(const char *s);
char *strndup(const char *s, size_t n);
int asprintf(char **strp, const char *fmt, ...);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
char *realpath(const char *path, char *resolved_path);

void register_deleter(void (*deleter)(void *));

#define ALLOCATE(size) malloc(size)

// The functions from list_of_allowed_malloc_functions may use all of them
void *memory_alloc(size_t size)
{
    void *memory = malloc(size);                        // good
    char *copy = strdup("text");                        // good
    char *prefix = strndup("text", 2);                  // good
    char *formatted = NULL;
    asprintf(&formatted, "%d", 5);                      // good
    char *line = NULL;
    size_t capacity = 0;
    getline(&line, &capacity, stdin);                   // good
    char *path = realpath(".", NULL);                   // good
    void *from_macro = ALLOCATE(size);                  // good: macro used in an allowed function

    (void)copy; (void)prefix; (void)path; (void)from_macro;
    return memory;
}

void *memory_alloc_array(size_t count, size_t size, void *old)
{
    void *resized = realloc(old, count * size);         // good
    if (resized)
        return resized;
    return calloc(count, size);                         // good
}

void memory_free(void *memory)
{
    free(memory);                                       // good
    register_deleter(free);                             // good: passed as a function pointer
}

// Any other function may not use them, however the function looks
void use_memory_directly(void *old, FILE *stream)
{
    void *a = malloc(8);                                // bad
    void *b = calloc(2, 4);                             // bad
    void *c = realloc(old, 16);                         // bad
    free(a);                                            // bad
    char *d = strdup("text");                           // bad
    char *e = strndup("text", 2);                       // bad
    char *f = NULL;
    asprintf(&f, "%d", 5);                              // bad
    char *g = NULL;
    size_t capacity = 0;
    getline(&g, &capacity, stream);                     // bad
    char *h = realpath(".", NULL);                      // bad

    (void)b; (void)c; (void)d; (void)e; (void)h;
}

void use_memory_through_pointers(void)
{
    void *(*allocator)(size_t) = malloc;                // bad: taking the address is a use too
    register_deleter(free);                             // bad: passed as a function pointer
    void *from_macro = ALLOCATE(4);                     // bad: macro hides the malloc
    memory_free(from_macro);                            // good: calls the allowed wrapper
    (void)allocator;
}

// Names must match exactly
void *memory_alloc_extra(size_t size)
{
    return malloc(size);                                // bad: not exactly 'memory_alloc'
}

void *Memory_alloc(size_t size)
{
    return malloc(size);                                // bad: names are case sensitive
}

// Outside of any function
static void *(*global_allocator)(size_t) = malloc;      // bad

// Something else that happens to be called 'free' is not affected
struct allocator
{
    void (*free)(void *);
};

void use_custom_allocator(struct allocator *custom, void *memory)
{
    custom->free(memory);                               // good: a struct field, not the library free
    custom->free = memory_free;                         // good
}

void suppressed_use(void)
{
    // WorkshopC off
    void *memory = malloc(8);                           // good: suppressed, no diagnostic expected
    // WorkshopC on
    memory_free(memory);                                // good
}
