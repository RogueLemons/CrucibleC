// Globals use the 'G_' prefix and must be capitalized, static and const.
// Static locals use only their dedicated 's_' naming policies.

// Good

static const int G_LIMIT = 5;                           // good
static const char* const G_NAME = "name";               // good

int use_locals(int argument)
{
    static int s_calls = 0;                             // good: lowercase and not const are fine
    static char* s_cache = 0;                           // good: a pointer that is not const
    static int s_Mixed_Case = 0;                        // good: capital letters are not required
    int local = argument;                               // good: local variable, not static
    s_calls++;
    return s_calls + s_Mixed_Case + local + G_LIMIT + (s_cache ? 1 : 0) + (G_NAME ? 1 : 0);
}

int use_bad_locals(void)
{
    static int calls = 0;                               // bad: missing the 's_' prefix
    static int G_LIKE_GLOBAL = 0;                       // bad: static locals use 's_', not the global prefix
    calls++;
    return calls + G_LIKE_GLOBAL;
}

// Bad, to show that the rule itself is active for globals

static int limit = 5;                                   // bad: missing prefix, not capital letters, and not const
int G_COUNTER = 0;                                      // bad: not static, and not const
