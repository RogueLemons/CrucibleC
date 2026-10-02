int returns_int(void)
{
    return 1;
}

int returns_other(void)
{
    return 2;
}

void returns_void(void)
{
}

typedef int (*value_function)(void);

typedef struct calls_vtable
{
    value_function get;
} calls_vtable;

typedef struct calls
{
    const calls_vtable* vtable;
    value_function direct;
} calls;

void test_discards(calls* object, calls_vtable table, value_function list[2])
{
    returns_int();                         // bad: non-void return is discarded
    (returns_int(), returns_other());      // bad: left and right values are discarded
    (void)returns_int();                   // good: explicit discard
    returns_void();                        // good: void function

    int value = returns_int();             // good: initialized from result
    value = returns_other();               // good: assigned result
    if (returns_int())                     // good: used as a condition
        value = 3;

    value_function function = returns_int;
    function();                            // bad: function pointer result is discarded
    (void)function();                      // good: explicit discard
    (*function)();                         // bad: dereferenced function pointer result is discarded
    object->direct();                      // bad: struct field function pointer result is discarded
    object->vtable->get();                 // bad: vtable function pointer result is discarded
    table.get();                           // bad: struct value field function pointer result is discarded
    list[0]();                             // bad: array element function pointer result is discarded
    int (*untyped)(void) = returns_int;
    untyped();                             // bad: function pointer without typedef result is discarded
    (void)object->vtable->get();           // good: explicit discard

    sizeof(returns_int());                  // good: unevaluated context
    (void)value;
}
