// -------------------------------------------------------------
// Arrays are allowed outside of structs, while a struct that holds
// only an array is still named after what it holds
// -------------------------------------------------------------

struct point
{
    int x;
    int y;
};
typedef struct point point_t;

int global_values[4] = { 1, 2, 3, 4 }; // good: arrays may be declared outside of structs
static char global_name[16] = { 0 }; // good

void local_arrays(void)
{
    int values[3] = { 1, 2, 3 }; // good
    static float samples[8] = { 0 }; // good
    point_t points[2] = { { 0, 0 }, { 1, 1 } }; // good
    int matrix[2][2] = { { 1, 2 }, { 3, 4 } }; // good

    (void)values; (void)samples; (void)points; (void)matrix;
}

// -------------------------------------------------------------
// The naming rules still apply
// -------------------------------------------------------------

struct int_array_5 // good
{
    int values[5];
};

struct point_array_2 // good
{
    point_t points[2];
};

struct int_values // bad: must end with '_array_5'
{
    int values[5];
};

struct path_array_3 // bad: must start with 'point'
{
    point_t points[3];
};

struct int_array_2 // bad: must end with '_array_2_4'
{
    int values[2][4];
};
