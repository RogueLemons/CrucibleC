#define RETURN_IF_NULL(ptr, value) if (!(ptr)) return (value)

void log_error(const char* message);
int count_items(const int* values);

// Good: early returns directly in the top-level block of the function

int guard_clause(int* value)
{
    if (!value)
        return -1; // good: early return

    return *value; // good
}

int guard_clause_block(int* value)
{
    if (!value)
    {
        log_error("value is null");
        return -1; // good: early return, the last statement of its block
    }

    return *value; // good
}

int guard_clause_macro(int* value)
{
    RETURN_IF_NULL(value, -1); // good: early return from a macro

    return *value; // good
}

int guard_clause_after_other_code(const int* values)
{
    int count = count_items(values);

    if (count == 0)
        return 0; // good: early returns do not have to come first

    return count * 2; // good
}

// Good: void functions do not need a final return in relaxed mode

void void_without_return(int* value)
{
    *value = 0; // good
}

void void_early_return(int* value)
{
    if (!value)
        return; // good: early return

    *value = 0;
}

// Bad: returns that are not early returns

int return_in_loop(const int* values, int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (values[i] < 0)
            return i; // bad: return inside a loop
    }

    return -1; // good
}

int return_in_else(int* value)
{
    if (value)
        *value = 0;
    else
        return -1; // bad: an if with an else is not a guard clause

    return 0; // good
}

int return_in_nested_if(int* value, int flag)
{
    if (flag)
    {
        if (!value)
            return -1; // bad: not directly in the top-level block

        return *value; // good: the last statement of a top-level if block
    }

    return 0; // good
}

int return_in_both_branches(int flag)
{
    if (flag)
        return 1; // bad: an if with an else is not a guard clause
    else
        return 2; // bad: return inside an else
} // bad: non-void functions must still end with a return

void void_return_in_loop(int* values, int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (values[i] < 0)
            return; // bad: return inside a loop
    }
}
