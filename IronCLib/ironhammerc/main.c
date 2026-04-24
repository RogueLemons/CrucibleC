#include <stdio.h>

#include <stdint.h>
#include <stddef.h>
#include <float.h>
#include <limits.h>

#include "ironclib/ic_static_assert.h"

IC_STATIC_ASSERT(sizeof(float) == 4, "Expected float to be 4 bytes");

int main(void) 
{
    printf("The value of PI is: %f\n", 3.14f);
    return 0;
}