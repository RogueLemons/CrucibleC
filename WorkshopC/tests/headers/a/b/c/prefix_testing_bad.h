#ifndef PREFIX_TESTING_H
#define PREFIX_TESTING_H

struct bad_name
{
    int x, y, z;
};

typedef struct bad_name bad_typedef;

int bad_function_name(int i, int j, char c);

static void* bad_header_static_function_name(const char* str);

#endif // PREFIX_TESTING_H