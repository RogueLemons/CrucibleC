#include "headers/enum_typedef.h"
#include "external/enum_typedef.h"

typedef enum Color // good
{
    RED,
    GREEN,
    BLUE
} Color;

typedef enum // good: anonymous enum with a typedef
{
    SMALL,
    MEDIUM,
    LARGE
} Size;

enum Direction // good: the typedef comes after the definition
{
    NORTH,
    SOUTH
};
typedef enum Direction Direction;

enum Shape // bad: no typedef
{
    CIRCLE,
    SQUARE
};

enum // bad: anonymous enum without a typedef
{
    ANONYMOUS_A,
    ANONYMOUS_B
};

#define DEFINE_ENUM_WITHOUT_TYPEDEF enum MacroBad { MACRO_BAD_A, MACRO_BAD_B };
DEFINE_ENUM_WITHOUT_TYPEDEF // bad: no typedef

struct Holder
{
    Color color;
    int number;
};

Color get_color(void);
void takes_color(Color color);
void takes_size(Size size);
void takes_int(int value);
void takes_unnamed(Color);

Color global_good = GREEN; // good
Color global_bad = 2; // bad

HeaderGoodEnum header_good = HEADER_GOOD_B; // good
HeaderGoodEnum header_bad = 5; // bad
ExternalEnum external_not_checked = 5; // good: third party enums are not checked

void initialization_test(int i)
{
    Color a = RED; // good
    Color b = a; // good: value that already is a Color
    Color c = get_color(); // good
    Color d = (Color)1; // good: explicit cast
    Color e = i > 0 ? RED : BLUE; // good: both branches are members
    Direction f = SOUTH; // good
    int g = RED; // good: plain int is not a typedef enum

    Color h = 0; // bad
    Color j = i; // bad
    Color k = SMALL; // bad: member of a different enum
    Color l = RED | GREEN; // bad: arithmetic
    Color m = i > 0 ? RED : 2; // bad: one branch is not a member
    Size n = RED; // bad: member of a different enum
}

void assignment_test(int i, Color* color_ptr)
{
    Color a = RED; // good
    Color b = BLUE; // good
    struct Holder holder = { RED, 0 }; // good

    a = GREEN; // good
    a = b; // good
    a = (Color)i; // good: explicit cast
    a = get_color(); // good
    holder.color = BLUE; // good
    holder.number = 5; // good: plain int
    *color_ptr = GREEN; // good

    a = 1; // bad
    a = i; // bad
    a = SMALL; // bad: member of a different enum
    holder.color = 2; // bad
    *color_ptr = 2; // bad
    a += 1; // bad: arithmetic
    a |= GREEN; // bad: arithmetic
    a++; // bad: arithmetic
    --a; // bad: arithmetic
}

void argument_test(int i)
{
    Color a = RED; // good

    takes_color(RED); // good
    takes_color(a); // good
    takes_color(get_color()); // good
    takes_color((Color)2); // good: explicit cast
    takes_size(MEDIUM); // good
    takes_int(RED); // good: parameter is a plain int
    takes_int(i); // good: parameter is a plain int
    takes_unnamed(BLUE); // good

    takes_color(2); // bad
    takes_color(i); // bad
    takes_color(SMALL); // bad: member of a different enum
    takes_size(RED); // bad: member of a different enum
    takes_unnamed(3); // bad
}

void suppression_test(void)
{
    // WorkshopC off
    Color suppressed = 7; // good: suppressed, no diagnostic expected
    // WorkshopC on
}
