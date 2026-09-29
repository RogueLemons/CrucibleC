#include <stddef.h>
#include <time.h>

#include "external/some_struct.h"

// -------------------------------------------------------------
// A struct whose single field is an array is named after what it
// holds: the element struct first (unless it comes from the standard
// library or a third party), then '_array', then the count of each
// dimension, e.g. 'point_array_7'. A flexible array is counted as
// 'flexible'.
// -------------------------------------------------------------

struct point
{
    int x;
    int y;
};
typedef struct point point_t;

typedef struct
{
    float x;
    float y;
} vector_t;

// Plain elements, no prefix required

struct int_array_5 // good
{
    int values[5];
};

struct float_array_5_10_15 // good: a matrix has a count per dimension
{
    float values[5][10][15];
};

struct bytes_array_flexible // good: a flexible array
{
    unsigned char bytes[];
};

struct char_array_flexible_4 // good: a flexible array of arrays
{
    char names[][4];
};

typedef int triple_t[3];

struct triple_array_2_3 // good: the dimensions of an array typedef count too
{
    triple_t values[2];
};

typedef struct // good: an anonymous struct is named by its typedef
{
    int values[8];
} anonymous_array_8;

struct int_array_6 // bad: the count is 5, not 6
{
    int values[5];
};

struct int_5 // bad: missing the array suffix
{
    int values[5];
};

struct int_array // bad: missing the count
{
    int values[5];
};

struct int_5_array // bad: the suffix comes before the count
{
    int values[5];
};

struct int_array_510 // bad: every count gets an underscore
{
    int values[5][10];
};

struct int_array_5_10 // bad: the count of the last dimension is missing
{
    int values[5][10][15];
};

struct bytes_array_0 // bad: a flexible array is named 'flexible'
{
    unsigned char bytes[];
};

typedef struct // bad: the typedef name misses the suffix and count
{
    int values[8];
} anonymous_values_t;

// Elements that are project structs start with their name

struct point_array_7 // good
{
    point_t points[7];
};

struct point_array_2_3 // good: a matrix of points
{
    struct point grid[2][3];
};

struct vector_t_array_4 // good: an anonymous element struct is named by its typedef
{
    vector_t vectors[4];
};

struct points_array_7 // good: the name only has to start with the element struct name
{
    point_t points[7];
};

struct path_array_7 // bad: does not start with 'point'
{
    point_t points[7];
};

struct vector_array_4 // bad: does not start with 'vector_t'
{
    vector_t vectors[4];
};

struct pointers_array_3 // bad: 'point' comes first but the count is 2, not 3
{
    point_t points[2];
};

// Elements that are library structs need no prefix

struct time_array_2 // good: struct tm is from the standard library
{
    struct tm times[2];
};

struct some_array_3 // good: struct SomeStruct is from a third party
{
    struct SomeStruct items[3];
};

// Only structs holding a single field which is an array

struct polygon // good: more than one field
{
    point_t points[4];
    size_t count;
};

struct counter // good: the single field is not an array
{
    int count;
};

struct point_pointers // good: pointers are not arrays
{
    point_t *points;
};

union int_or_bytes // good: a union is not a struct
{
    int values[1];
};
