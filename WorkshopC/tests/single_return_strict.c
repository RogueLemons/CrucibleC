#define RETURN_IF_NULL(ptr, value) if (!(ptr)) return (value)

void log_error(const char* message);

// Good: a single return as the last statement

int single_final_return(int* value)
{
    int result = 0;

    if (value)
        result = *value;

    for (int i = 0; i < 3; ++i)
        result += i;

    return result; // good
}

void void_with_final_return(int* value)
{
    if (value)
        *value = 0;

    return; // good: void functions must end with a return
}

// Bad: early returns are not allowed in strict mode

int guard_clause(int* value)
{
    if (!value)
        return -1; // bad: early return

    return *value; // good
}

int guard_clause_block(int* value)
{
    if (!value)
    {
        log_error("value is null");
        return -1; // bad: early return
    }

    return *value; // good
}

int guard_clause_macro(int* value)
{
    RETURN_IF_NULL(value, -1); // bad: early return from a macro

    return *value; // good
}

void void_early_return(int* value)
{
    if (!value)
        return; // bad: early return

    *value = 0;
    return; // good
}

// Bad: returns anywhere else

int return_in_loop(const int* values, int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (values[i] < 0)
            return i; // bad: return inside a loop
    }

    return -1; // good
}

int return_in_both_branches(int flag)
{
    if (flag)
        return 1; // bad
    else
        return 2; // bad
} // bad: does not end with a return

void void_without_return(int* value)
{
    *value = 0;
} // bad: void functions must end with a return

void suppressed(int* value)
{
    // WorkshopC off
    if (!value)
        return; // good: suppressed, no diagnostic expected
    // WorkshopC on

    *value = 0;
    return; // good
}
