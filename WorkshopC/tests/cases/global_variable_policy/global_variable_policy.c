int g_count = 0;
int count = 0;

const int G_LIMIT = 1;
const int G_limit = 2;
const int LIMIT = 3;
const int G_ARRAY[1] = {0};
const int* const G_POINTER = 0;
const int* G_MUTABLE_POINTER = 0;
int G_MUTABLE_ARRAY[1] = {0};

int use_static_locals(void)
{
    static int s_calls = 0;
    static int calls = 0;
    static const int c_LIMIT = 1;
    static const int c_limit = 2;
    static const int LIMIT = 3;

    return g_count + count + G_LIMIT + G_limit + LIMIT + G_ARRAY[0] +
        s_calls + calls + c_LIMIT + c_limit + LIMIT +
        (G_POINTER != 0) + (G_MUTABLE_POINTER != 0) + G_MUTABLE_ARRAY[0];
}
