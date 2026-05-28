#ifndef TESTS_HEADERS_A_B_C_PREFIX_TESTING_GOOD_H
#define TESTS_HEADERS_A_B_C_PREFIX_TESTING_GOOD_H

struct tests__headers__a__b__c__good_name
{
    int x, y, z;
};

typedef struct tests__headers__a__b__c__good_name tests__headers__a__b__c__good_typedef;

int tests__headers__a__b__c__good_function_name(int i, int j, char c);

static void* tests__headers__a__b__c__good_header_static_function_name(const char* str);

#endif // TESTS_HEADERS_A_B_C_PREFIX_TESTING_GOOD_H