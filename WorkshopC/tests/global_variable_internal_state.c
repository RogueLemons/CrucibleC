// Globals as internal state: prefixed with 'global_' and static,
// capital letters and const are not required

struct point
{
    int x;
    int y;
};

// Good

static int global_counter = 0;                          // good
static const char* global_name = "state";               // good: const is not required
static int global_values[4] = {1, 2, 3, 4};             // good
static struct point global_origin = {0, 0};             // good
static int global_Mixed_Case = 0;                       // good: capital letters are not required
static int global_redeclared;                           // good
static int global_redeclared = 5;                       // good: redeclarations are only checked once

int use_state(int argument)
{
    static int calls = 0;                               // good: static local, not a global
    int local = argument;                               // good: local variable
    calls++;
    return global_counter + global_values[0] + global_origin.x + local + global_redeclared + global_Mixed_Case + calls;
}

// Bad

static int counter = 0;                                 // bad: missing prefix
int global_exported = 0;                                // bad: not static
int exported = 0;                                       // bad: missing prefix, and not static
int global_tentative;                                   // bad: not static
extern int global_defined_elsewhere;                    // bad: extern is not static

extern int shared_value;                                // declaration of the variable below
int shared_value = 3;                                   // bad: missing prefix, and not static, reported once

// WorkshopC off
int suppressed_value = 0;                               // good: suppressed, no diagnostic expected
// WorkshopC on
