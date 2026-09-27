#ifndef TESTS_HEADERS_MIXED_CASE_PREFIX_TESTING_CASE_H
#define TESTS_HEADERS_MIXED_CASE_PREFIX_TESTING_CASE_H

// The prefix keeps the case of the folder names

struct tests__headers__Mixed__Case__good_name // good
{
    int x, y, z;
};

typedef struct tests__headers__Mixed__Case__good_name tests__headers__Mixed__Case__good_typedef; // good

int tests__headers__Mixed__Case__good_function_name(int i); // good

int tests__headers__mixed__case__lowercase_function_name(int i); // bad unless case_insensitive: the folders are Mixed and Case

int tests__headers__other__function_name(int i); // bad: different letters, whatever the case

#endif // TESTS_HEADERS_MIXED_CASE_PREFIX_TESTING_CASE_H
