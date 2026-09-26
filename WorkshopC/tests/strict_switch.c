#include <stdlib.h>

void do_something(int value);

// -------------------------------------------------------------
// Good: every case ends with a break or return, and there is a
// default case
// -------------------------------------------------------------

int all_cases_break(int value)
{
    int result = 0;

    switch (value) // good
    {
        case 1:
            result = 10;
            break; // good
        case 2:
            result = 20;
            break; // good
        default:
            result = -1;
            break; // good
    }

    return result;
}

int all_cases_return(int value)
{
    switch (value) // good
    {
        case 1:
            return 10; // good
        default:
            return -1; // good
    }
}

int stacked_labels(int value)
{
    int result = 0;

    switch (value) // good
    {
        case 1:
        case 2:
        case 3: // good: stacked labels share one body
            result = 1;
            break;
        default:
            break;
    }

    return result;
}

int other_endings(int value, int count)
{
    int result = 0;

    for (int i = 0; i < count; ++i)
    {
        switch (value) // good
        {
            case 1:
            {
                result += 1;
                break; // good: the last statement of a block
            }
            case 2:
                if (i > 5)
                    return result; // good: both branches end the case
                else
                    break;
            case 3:
                continue; // good: continue also leaves the switch
            case 4:
                abort(); // good: abort never returns (has noreturn tag)
            case 5:
                abort();
                break;  // good: can still break even after noreturn function
            case 6:
                goto done; // good
            default:
                break;
        }
    }

done:
    return result;
}

int default_in_the_middle(int value)
{
    int result = 0;

    switch (value) // good
    {
        case 1:
            result = 1;
            break;
        default: // good: default does not have to be last
            result = -1;
            break;
        case 2:
            result = 2;
            break;
    }

    return result;
}

// -------------------------------------------------------------
// Bad
// -------------------------------------------------------------

int missing_default(int value)
{
    int result = 0;

    switch (value) // bad: no default case
    {
        case 1:
            result = 1;
            break;
    }

    return result;
}

void empty_switch(int value)
{
    switch (value) // bad: no default case
    {
    }
}

int case_falls_through(int value)
{
    int result = 0;

    switch (value)
    {
        case 1: // bad: falls through into case 2
            result = 1;
        case 2:
            result += 2;
            break;
        default:
            break;
    }

    return result;
}

int default_without_break(int value)
{
    int result = 0;

    switch (value)
    {
        case 1:
            result = 1;
            break;
        default: // bad: the last case needs a break too
            result = -1;
    }

    return result;
}

int break_only_in_if(int value, int flag)
{
    int result = 0;

    switch (value)
    {
        case 1: // bad: only breaks when flag is set
            if (flag)
                break;
            result = 1;
        default:
            break;
    }

    return result;
}

int nested_switch_as_last_statement(int value, int other)
{
    int result = 0;

    switch (value)
    {
        case 1: // bad: the inner break only leaves the inner switch
            switch (other) // good
            {
                case 1:
                    result = 1;
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return result;
}

void suppressed(int value)
{
    // WorkshopC off
    switch (value) // good: suppressed, no diagnostic expected
    {
        case 1:
            do_something(1);
    }
    // WorkshopC on
}
