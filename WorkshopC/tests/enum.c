#include <stdio.h>

#include "headers/has_enum.h"
#include "external/has_enum.h"

// This should trigger a warning
enum Color {
    RED,
    GREEN,
    BLUE
};

int main() {
    enum Color c = RED;
    printf("Color: %d\n", c);
    return 0;
}
