#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "headers/has_enum.h"
#include "external/has_enum.h"

// bad
enum Color {
    RED,
    GREEN,
    BLUE
};

DEFINE_SOME_UNDETECTED_ENUM_VALUE // good

// bad
DEFINE_SOME_DETECTED_ENUM_VALUE

int main() {
    enum Color c = RED;
    printf("Color: %d\n", c);
    return 0;
}
