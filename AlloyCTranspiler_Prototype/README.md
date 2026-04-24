# AlloyC (Prototype)
AlloyC is a small superset of C that transpiles to plain C, adding a few keywords to improve safety, readability, and consistency.

It focuses on portability and predictable, easy-to-read output, making C safer by default through simple transformations.

## Table of Contents

* [Quick Start](#quick-start)
* [Keywords](#keywords)
* [Demos](#demos)
  * [Demo 1 - const by default, mut, and check](#demo-1--const-by-default-mut-and-check)
  * [Demo 2 - typestruct and cleanpop](#demo-2--typestruct-and-cleanpop)
* [Proof of concept](#proof-of-concept)
* [Features and Examples](#features-and-examples)
  * [File inclusion and generated code warning](#file-inclusion-and-generated-code-warning)
  * [Organize functions with keyword prefix](#organize-functions-with-keyword-prefix)
  * [Const by default and keyword mut](#const-by-default-and-keyword-mut)
  * [Automatic runtime pointer null check with keyword check](#automatic-runtime-pointer-null-check-with-keyword-check)
  * [Automatically typed structs with keyword typestruct](#automatically-typed-structs-with-keyword-typestruct)
  * [Inline definitions with keyword indef](#inline-definitions-with-keyword-indef)
  * [Type-safe enums with keyword typenum](#type-safe-enums-with-keyword-typenum)
  * [Automatic memory management with keyword cleanpop](#automatic-memory-management-with-keyword-cleanpop)
* [TODO](#todo)
* [Bugs](#bugs)

### Quick Start

AlloyC is a **file-by-file transpiler**:

* `.alc` -> `.c`
* `.alh` -> `.h`

Run with:

```bash
python AlloyCPrototype.py input.alc output.c
```

### Keywords
- **mut**: enables mutation (default is const)
- **check**: adds runtime NULL checks for pointers
- **typestruct**: struct + typedef shortcut
- **prefix**: namespace-style grouping (__ conversion)
- **indef**: local compile-time constants (macro scoped)
- **typenum**: type-safe enum wrapper with helpers
- **cleanpop**: auto populate/cleanup (RAII-like)

### Demos

#### Demo 1 - const by default, mut, and check

##### AlloyC

```c
int clamp(int value, int min, int max)
{
    mut int result = value;

    if (result < min)
        result = min;
    if (result > max)
        result = max;

    return result;
}

float dereference(check float* f_ptr)
{
    return (*f_ptr);
}
```

##### Transpiled C

```c
#ifndef ALC__NULL__CHECK
#include <stdio.h>
#include <stdlib.h>
#define ALC__NULL__CHECK(x) \
do { \
    if ((x) == NULL) { \
        fprintf(stderr, "File %s - Line %d had illegal null pointer, now exiting...\n", __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)
#endif

int clamp(const int value, const int min, const int max)
{
    int result = value;

    if (result < min)
        result = min;
    if (result > max)
        result = max;

    return result;
}

float dereference(const float* const f_ptr)
{
    ALC__NULL__CHECK(f_ptr);
    return (*f_ptr);
}
```

**What this shows:**

* Function parameters are **const by default**
* Local variables require `mut` to be reassigned
* Pointer is **const**, not the pointed value
* `check` automatically inserts runtime null checks

#### Demo 2 - typestruct and cleanpop

##### AlloyC

```c
typestruct String
{
    char* data;
    unsigned int size;
    unsigned int capacity;
};

void foo()
{
    cleanpop mut String str;
    String::set(&str, "Hello world!");
    printf("%s\n", (&str)->data);
}
```

##### Transpiled C

```c
struct String
{
    char* data;
    unsigned int size;
    unsigned int capacity;
};
typedef struct String String;

void foo()
{
    String str;
    String__populate(&str);
    String__set(&str, "Hello world!");
    printf("%s\n", (&str)->data);
    String__cleanup(&str);
}
```

**What this shows:**

* `typestruct` removes typedef boilerplate
* `cleanpop` automatically inserts `populate` on creation
* and `cleanup` on all exits


## Proof of concept
As a prototype this mini-project will never be perfect. It is a proof of concept meant to show what C could look like and what can be done with a transpiler that only has to work on a file-by-file basis. A real implementation would require much more rigorous C code parsing and proper pretty-print. 

It started with the idea "What if C variables were const by default?". Since the project is just a proof of concept it serves more as a way to talk about what code safety is and means in C, and what practices can be applied to write safe C code in a standardize style for the whole group. 

"Why use this instead of C++?" I hear you ask. C++ already exists. Nim already exists. C3 already exists. There are better tools and solutions than AlloyC out there already. However, C programmers are often really happy about C specifically so converting them to a new language, even if it was deemed better (by whatever metric), is going to be difficult. But letting C coders continue to write C code but with just a few added keywords is an easier sell. It is also not just a people question; it is about compatability. Not all processors come with compilers for C++ or whatever language you might prefer, and gcc might have amazing added features to the language which might not be supported by other compilers a group moves to, so it becomes a matter of portability. Furthermore, with this transpiler the goal becomes to help write safe and readable code, both in the AlloyC files and their transpiled C files, so it becomes a way to standardize how the code should look like and avoid easy-to-make mistakes. 

## Features and Examples
The following examples are taken directly from the files [example.alc](example.alc) and [example.c](example.c) in in this repo, and showcase how to use these keywords. 

### File inclusion and generated code warning
All generated files come with a generation warning and automatically converts .alh and .alc file includes to .h and .c respectively.

#### Example
##### AlloyC
```c
#include <stdio.h>
#include <stdlib.h>
#include "example/easyc/file.alh"
#include "normal/c/file.h"
```
##### Transpiled C
```c
// =====================================================
// === WARNING! DO NOT EDIT THIS FILE! =================
// === This code was generated by AlloyC (Prototype) ===
// === and transpiled from an input file ===============
// =====================================================

#include <stdio.h>
#include <stdlib.h>
#include "example/alloyc/file.h"
#include "normal/c/file.h"
```

### Organize functions with keyword prefix
The keyword **prefix** works similar to namespaces in C++ but not quite the same. The prefix gets added to everything global and as well as all function calls and non-standard types. Add two colons :: at the start to avoid prefixes. Access **prefix** paths with ::. The transpiler then simply converts all **prefix** paths and all :: to two underscores __. This gives complete clarity where functions come from and becomes easy to read transpiled code. 

#### Example - Using prefix
##### AlloyC
```c
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
```
##### Transpiled C
```c
static int add(const int a, const int b)
{
    return a + b;
}

int int_math__add(const int addend_a, const int addend_b)
{
    return add(addend_a, addend_b);
}

int int_math__subtract(const int minuend, const int subtrahend)
{
    return int_math__add(minuend, -subtrahend);
}

static int int_math__detail__divide_and_round(const int numerator, const int denominator)
{
    unsigned const int sign_diff = ((unsigned const int)(numerator ^ denominator)) >> 31;
    const int sign = 1 - 2*sign_diff; // +1 or -1

    const int abs_den = denominator < 0 ? -denominator : denominator;
    const int bias = abs_den / 2;

    return (numerator + sign*bias) / denominator;
}

static void int_math__detail__abort_for_illegal_denominator(const int denominator)
{
    if (denominator == 0)
    {
        printf("Error: division by zero.\n");
        exit(1);
    }
}

int int_math__divide(const int numerator, const int denominator)
{
    int_math__detail__abort_for_illegal_denominator(denominator);
    return int_math__detail__divide_and_round(numerator, denominator);
}

int int_math__multiply(const int factor_a, const int factor_b)
{
    return factor_a*factor_b;
}
```

### Const by default and keyword mut
One of the more important aspects, makes all variables constant by default and negates it with **mut** (for mutable). Helps coders keep with const correctness, especially for pointers, while keeping the code easy to read. Does not affect struct members because that causes behavior a lot of people will not expect, so there it is better to instead be explicit with const. 

#### Example - Constant arguments and const pointer (but not const pointer target)
##### AlloyC
```c
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
```
##### Transpiled C
```c
int normalize_to_range(const int value, const int low_boundary, const int high_boundary)
{
    int result = value;

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

void set_to_zero_if_negative(int* const i_ptr)
{
    if (i_ptr != NULL && (*i_ptr) < 0)
    {
        (*i_ptr) = 0;
    }
}
```

### Automatic runtime pointer null check with keyword check
Keyword **check** allows users to make and return **check** pointers (pronounced "chic pointers", as in cool pointers). Functions returning **check** pointers must return an actual variable that is **check**. It automatically adds runtime null checks to all uses of pointers with the keyword, where the null checks are done with a macro called ALC__NULL__CHECK which can be overriden by including another definition from a file (the definition gets put at the top of files after all file includes). Allows testing to better catch cases where there was a promise of a pointer not being null, but it was anyway, and can finally be disabled for performance.

#### Example - Simple dereference check and guarantee of a function return
##### AlloyC
```c
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
```
##### Transpiled C
```c
#ifndef ALC__NULL__CHECK
#include <stdio.h>
#include <stdlib.h>
#define ALC__NULL__CHECK(x) \
do { \
    if ((x) == NULL) { \
        fprintf(stderr, "File %s - Line %d had illegal null pointer, now exiting...\n", __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)
#endif

float dereference_float(const float* const f_ptr)
{
    ALC__NULL__CHECK(f_ptr);
    return(*f_ptr);
}

static float ratio = 0.4;

const float* const get_ratio()
{
    const float* const result = &ratio;
    ALC__NULL__CHECK(result);
    return result;
}
```

### Automatically typed structs with keyword typestruct
Syntactic sugar to keep code short and clean with clear intent. 

#### Example - Using typestruct
##### AlloyC
```c
typestruct Color
{
    int r;
    int g;
    int b;
};
```
##### Transpiled C
```c
struct Color
{
    int r;
    int g;
    int b;
};
typedef struct Color Color;
```

### Inline definitions with keyword indef
For anyone experienced with C++, this is a poor man's constexpr. The keyword **indef** stands for "inline definition" and simply lets users write local compile-time values more locally to where they are used, with very easy to read names. This is purely a readability feature, but readable code is also code where bugs are easier to catch. The real definition this gets converted to gets the name of the function it exists in added to it, and provides a little bit of type safety for more clear and debugable behavior.

#### Example Using indef
##### AlloyC
```c
void Color::set::white(check mut Color* col)
{
    indef int MAX = 255;
    indef Color WHITE = { .r = MAX, .g = MAX, .b = MAX };
    (*col) = WHITE;
}
```
##### Transpiled C
```c
#define Color__set__white__MAX ((const int)(255))
#define Color__set__white__WHITE ((const Color){ .r = Color__set__white__MAX, .g = Color__set__white__MAX, .b = Color__set__white__MAX })
void Color__set__white(Color* const col)
{
    ALC__NULL__CHECK(col);
    (*col) = Color__set__white__WHITE;
}
```

### Type-safe enums with keyword typenum
The keyword **typenum** allows users to create type-safe enums, which are basically just define statements which reduces safety of code as functions expecting an enum type can be given any integer or even an enum of the wrong type. With **typenum** you get your values wrapped in a struct and with helper macros. It also supports the internal type being anything, such as a one-byte char.

#### Example - Using typenum
##### AlloyC
```c
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
```
##### Transpiled C
```c
struct LogOption { int LogOption_value; };
typedef struct LogOption LogOption;
#define LogOption__info ((const LogOption){ .LogOption_value = 0 })
#define LogOption__warning ((const LogOption){ .LogOption_value = 1 })
#define LogOption__error ((const LogOption){ .LogOption_value = 2 })
#define LogOption__count 3
#define LogOption__equals(a, b) ((a).LogOption_value == (b).LogOption_value)
#define LogOption__get(a) ((a).LogOption_value)

void Log(const char* const log, const LogOption logopt)
{
    if (LogOption__equals(logopt, LogOption__info))
    {
        LogInfo(log);
    }
    else if (LogOption__equals(logopt, LogOption__warning))
    {
        LogWarning(log);
    }
    else if (LogOption__equals(logopt, LogOption__error))
    {
        LogError(log);
    }
}
```
#### Example - Typenum with char as byte
##### AlloyC
```c
typenum(char) GraphicMode
{
    performance = 'p',
    quality = 'q'
};
```
##### Transpiled C
```c
struct GraphicMode { char GraphicMode_value; };
typedef struct GraphicMode GraphicMode;
#define GraphicMode__performance ((const GraphicMode){ .GraphicMode_value = 'p' })
#define GraphicMode__quality ((const GraphicMode){ .GraphicMode_value = 'q' })
#define GraphicMode__count 2
#define GraphicMode__equals(a, b) ((a).GraphicMode_value == (b).GraphicMode_value)
#define GraphicMode__get(a) ((a).GraphicMode_value)
```

### Automatic memory management with keyword cleanpop
With the keyword **cleanpop** a degree of RAII and memory management is introduced. It is a keyword applied to variables which will automatically call type__populate(type* t) function when first created and type__cleanup(type* t) at all exit points. It optionally also allows move semantics with the operator **move** which must be supported by type__move(type* from, type* to). This makes memory management much easier and can greatly reduce visual bloat of code, and reduces risk of mistakes for multiple function exit points. It comes with a set of rules enforced by the transpiler:
- The **cleanpop** variables must be mutable and may not be const (so initialization and cleanup works properly).
- They must be used only with their addresses and pointers, making ownership clear everywhere and bad behavior stand out in code (e.g. accessing fields, which is still allowed because it is C and we trust developers (to some degree)).
- They may not be returned from functions. 
- Functions for populating and cleaning up variables of this type must be manually implemented, with the standardized names, for **cleanpop** variables to work.
- The **move** operater is optional for initialization and requires move functions to be manually implemented (further moves are done with the function itself).
- Variables may not be assigned values manually when initialized.  

#### Example - Defining simple string struct used in examples
##### AlloyC
```c
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
```
##### Transpiled C
```c
struct String
{
    char* data;
    unsigned int size;
    unsigned int capacity;
};
typedef struct String String;
void String__populate(String* const str)
{
    ALC__NULL__CHECK(str);
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}
void String__cleanup(String* const str)
{
    ALC__NULL__CHECK(str);
    free(str->data);
    str->data = NULL;
    str->size = 0;
    str->capacity = 0;
}
void String__set(String* const target, const char* const c_string);
```

#### Example - Using cleanpop and showing good const correctness practice with const _view pointers 
##### AlloyC
```c
void foo()
{
    cleanpop mut String str;
    String::set(&str, "Hello there!");
    String* str_view = &str;
    printf("The greeting: %s\n", str_view->data);
}
```
##### Transpiled C
```c
void foo()
{
    String str;
    String__populate(&str);
    String__set(&str, "Hello there!");
    const String* const str_view = &str;
    printf("The greeting: %s\n", str_view->data);
    String__cleanup(&str);
}
```

#### Example - Showcase of more complicated case where automatic cleanup helps
##### AlloyC
```c
void String::add(check mut String* target, check char* addition);
void String::equals(check String* str1, check String* str2);

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
```
##### Transpiled C
```c
void String__add(String* const target, const char* const addition);
int String__equals(const String* const str1, const String* const str2);

void bar()
{
    String greeting_1;
    String__populate(&greeting_1);
    String__set(&greeting_1, "Hello there!");
    const String* const greeting_1_view = &greeting_1;

    String greeting_2;
    String__populate(&greeting_2);
    const String* const greeting_2_view = &greeting_2;

    if (String__equals(greeting_1_view, greeting_2_view))
    {
        String doppelganger;
        String__populate(&doppelganger);
        String__set(&doppelganger, "Wow, they are doppelgangers!");
        printf("%s\n", (&doppelganger)->data);
        String__cleanup(&doppelganger);
        String__cleanup(&greeting_2);
        String__cleanup(&greeting_1);
        return;
    }

    if (greeting_1_view->data == NULL || greeting_2_view->data == NULL)
    {
        printf("Something went wrong");
        String__cleanup(&greeting_2);
        String__cleanup(&greeting_1);
        return;
    }

    printf("Greeting 1: %s\n", greeting_1_view->data);
    printf("Greeting 2: %s\n", greeting_2_view->data);

    String__cleanup(&greeting_2);
    String__cleanup(&greeting_1);
}
```
#### Example - Support more initializers with cleanpop arguments
##### AlloyC
```C
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
```
##### Transpiled C
```C
void String__populate_with_1(String* const str, const char* const c_string)
{
    ALC__NULL__CHECK(str);
    ALC__NULL__CHECK(c_string);
    String__populate(str);
    String__set(str, c_string);
}

void baz()
{
    String str;
    String__populate_with_1(&str, "Initial string!");
    printf("Data: %s\n", (&str)->data);
    String__cleanup(&str);
}

void String__populate_with_2(String* const str, const char c, const int repeat_char_count);
int some_number();

void foofoo()
{
    String str;
    String__populate_with_2(&str, 'A', some_number());
    if ((&str)->size > 5) {
        String__cleanup(&str);
        return;
    }

    // do stuff
    String__cleanup(&str);
}
```
#### Example - Using the move operator
##### AlloyC
```c
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
```
##### Transpiled C
```c
String__move(String* const from, String* const to)
{
    ALC__NULL__CHECK(from);
    ALC__NULL__CHECK(to);
    if (to == from)
    {
        return;
    }

    String__cleanup(to);
    *to = *from;
    from->data = NULL;
    from->size = 0;
    from->capacity = 0;
}

void foobar()
{
    String str_1;
    String__populate_with_1(&str_1, "Start value");
    // do stuff
    String str_2;
    String__populate(&str_2);
    String__move(&str_1, &str_2);
    // do more stuff
    String__cleanup(&str_2);
    String__cleanup(&str_1);
}
```


## TODO
- transform project into a code analysis tool for pure C code
- consider inverting keyword **check** so everything is check by default and make user use keyword **nullable** for pointers that may be null
- consider adding **cleanpop** const variables (could create mutable variable with weird name and a const pointer to it with users original name, or just tell users to create const pointer view variables)
- the transpiler is only used on a file-by-file basis on purpose, the project would need a companion tool or be expanded in order to verify that functions returning **check** pointers have the same signature in declarations and definitions 

## Bugs
As a prototype this mini-project will never be perfect, it is a proof of concept. But less acceptable bugs include
- cannot use * without whitespace after unless dereferencing (* some_ptr not ok) or multiplying (a * b not ok)
- cannot typedef and define struct in same statement
- mut/const management cannot see function arg types and won't adjust const correctness if type does not exist in file in other format (e.g. as a normal variable), also cannot detect typedefs as types to apply mut/const rules to
- fix **typenum** of pointer type not getting different pointers from different source files due to define statements
