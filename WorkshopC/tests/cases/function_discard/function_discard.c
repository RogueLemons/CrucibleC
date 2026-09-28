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

void test_discards(void)
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

    sizeof(returns_int());                  // good: unevaluated context
    (void)value;
}
