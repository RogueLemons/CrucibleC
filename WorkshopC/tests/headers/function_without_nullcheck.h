#ifndef TESTS_HEADERS_FUNCTION_WITHOUT_NULLCHECK_H
#define TESTS_HEADERS_FUNCTION_WITHOUT_NULLCHECK_H

static int function_without_nullcheck_2(int* i_ptr_inside_header)
{
    return *i_ptr_inside_header;
}

#endif // TESTS_HEADERS_FUNCTION_WITHOUT_NULLCHECK_H