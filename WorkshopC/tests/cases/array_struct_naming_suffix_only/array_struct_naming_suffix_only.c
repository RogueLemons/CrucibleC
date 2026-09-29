// Only the array struct suffix ends the name, with the element struct name first

struct point
{
    int x;
    int y;
};
typedef struct point point_t;

struct int_array // good
{
    int values[5];
};

struct int_matrix_array // good: no counts needed for a matrix either
{
    int values[2][3];
};

struct point_array // good
{
    point_t points[7];
};

struct int_array_5 // bad: the count may not come after the suffix
{
    int values[5];
};

struct int_values // bad: missing the suffix
{
    int values[5];
};

struct path_array // good: does not need to start with 'point'
{
    point_t points[7];
};
