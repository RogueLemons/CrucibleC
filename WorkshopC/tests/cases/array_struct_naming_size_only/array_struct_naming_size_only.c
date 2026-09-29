// Only the counts end the name, with the element struct name first

struct point
{
    int x;
    int y;
};
typedef struct point point_t;

struct int_5 // good
{
    int values[5];
};

struct int_2_3 // good: a count per dimension
{
    int values[2][3];
};

struct bytes_flexible // good: a flexible array
{
    unsigned char bytes[];
};

struct point_7 // good
{
    point_t points[7];
};

struct int_array_5 // good: any name may come before the count
{
    int values[5];
};

struct int_array // bad: missing the count
{
    int values[5];
};

struct int_4 // bad: the count is 5, not 4
{
    int values[5];
};

struct path_7 // bad: does not start with 'point'
{
    point_t points[7];
};
