#include <stddef.h>
#include <stdlib.h>

void* probe_embedded_heap(size_t size)
{
    return malloc(size);
}