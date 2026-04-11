#include <stdio.h>
#include <stdlib.h>
#include "example/easyc/file.eh"
#include "normal/c/file.h"

// ========= prefix =========

static int add(int a, int b)
{
    return a + b;
}

prefix int_math 
{
    int add(int addend_a, int addend_b)
    {
        return ::add(addend_a, addend_b);
    }

    int subtract(int minuend, int subtrahend)
    {
        return add(minuend, -subtrahend);
    }

    prefix detail 
    {
        static int divide_and_round(int numerator, int denominator)
        {
            unsigned int sign_diff = ((unsigned int)(numerator ^ denominator)) >> 31;
            int sign = 1 - 2*sign_diff;   // +1 or -1

            int abs_den = denominator < 0 ? -denominator : denominator;
            int bias = abs_den / 2;

            return (numerator + sign*bias) / denominator;
        }

        static void abort_for_illegal_denominator(int denominator)
        {
            if (denominator == 0) 
            {
                ::printf("Error: division by zero.\n");
                ::exit(1);
            }
        }
    }

    int divide(int numerator, int denominator)
    {
        detail::abort_for_illegal_denominator(denominator);
        return detail::divide_and_round(numerator, denominator);
    }

    int multiply(int factor_a, int factor_b)
    {
        return factor_a*factor_b;
    }
}

// ========= mut =========

int normalize_to_range(int value, int low_boundary, int high_boundary)
{
    mut int result = value;

    if (result < low_boundary)
    {
        result = low_boundary;
    }
    if (result > high_boundary)
    {
        result = high_boundary;
    }

    return result;
}

void set_to_zero_if_negative(mut int* i_ptr)
{
    if (i_ptr != NULL && (*i_ptr) < 0)
    {
        (*i_ptr) = 0;
    }
}

// ========= check =========

float dereference_float(check float* f_ptr)
{
    return (*f_ptr);
}

static mut float ratio = 0.4;

check float* get_ratio()
{
    check float* result = &ratio;
    return result;
}

// ========= typestruct =========

typestruct Color
{
    int r;
    int g;
    int b;
};

// ========= indef =========

void Color::set::white(check mut Color* col)
{
    indef int MAX = 255;
    indef Color WHITE = { .r = MAX, .g = MAX, .b = MAX };
    (*col) = WHITE;
}

// ========= typenum =========

typenum LogOption
{
    info = 0,
    warning = 1,
    error = 2
};

void Log(char* log, LogOption logopt)
{
    if (LogOption::equals(logopt, LogOption::info))
    {
        LogInfo(log);
    }
    else if (LogOption::equals(logopt, LogOption::warning))
    {
        LogWarning(log);
    }
    else if (LogOption::equals(logopt, LogOption::error))
    {
        LogError(log);
    }
}

// ========= typenum with own internal type =========

typenum(char) GraphicMode
{
    performance = 'p',
    quality = 'q'
};

// ========= typenum based on c-strings =========

typenum(const char*) ErrorType
{
    mechanical = "Mechanical failure",
    electrical = "Electrical failure",
    software = "Software failure"
};

void PrintErrorMessage(ErrorType err)
{
    char* err_msg = ErrorType::get(err);
    printf("%s\n", err_msg);
}

// ========= cleanpop =========

typestruct String
{
    char* data;
    unsigned int size;
    unsigned int capacity;
};
void String::populate(check mut String* str)
{
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}
void String::cleanup(check mut String* str)
{
    free(str->data);
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}
void String::set(check mut String* target, check char* c_string);
void String::add(check mut String* target, check char* addition);
int String::equals(check String* str1, check String* str2);

void foo()
{
    cleanpop mut String str;
    String::set(&str, "Hello there!");
    String* str_view = &str;
    printf("The greeting: %s\n", str_view->data);
}

void bar()
{
    cleanpop mut String greeting_1;
    String::set(&greeting_1, "Hello there!");
    String* greeting_1_view = &greeting_1;
    
    cleanpop mut String greeting_2;
    String* greeting_2_view = &greeting_2;

    if (String::equals(greeting_1_view, greeting_2_view))
    {
        cleanpop mut String doppelganger;
        String::set(&doppelganger, "Wow, they are doppelgangers!");
        printf("%s\n", (&doppelganger)->data);
        return;
    }

    if (greeting_1_view->data == NULL || greeting_2_view->data == NULL)
    {
        printf("Something went wrong");
        return;
    }

    printf("Greeting 1: %s\n", greeting_1_view->data);
    printf("Greeting 2: %s\n", greeting_2_view->data);

}

// ========= cleanpop with arguments =========

void String::populate_with_1(check mut String* str, check char* c_string)
{
    String::populate(str);
    String::set(str, c_string);
}

void baz()
{
    cleanpop("Initial string!") mut String str;
    printf("Data: %s\n", (&str)->data);
}

void String::populate_with_2(check mut String* str, char c, int repeat_char_count);
int some_number();

void foofoo()
{
    cleanpop('A', some_number()) mut String str;
    if ((&str)->size > 5)
        return;
    
    // do stuff
}

// ========= cleanpop with move semantics =========

String::move(check mut String* from, check mut String* to)
{
    if (to == from)
    {
        return;
    }

    String::cleanup(to);
    *to = *from;
    from->data = NULL;
    from->size = 0;
    from->capacity = 0;
}

void foobar()
{
    cleanpop("Start value") mut String str_1;
    // do stuff
    cleanpop mut String str_2 = move(&str_1);
    // do more stuff
}

// ========= cleanpop macros and standard types =========

#define int::populate(i) (*i) = 0
#define int::cleanup(i) 

void foobaz()
{
    cleanpop mut int i;
    // Do stuff
}
