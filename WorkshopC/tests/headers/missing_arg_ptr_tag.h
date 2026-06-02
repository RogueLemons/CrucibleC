#ifndef TESTS_HEADERS_MISSING_ARG_PTR_TAG_H
#define TESTS_HEADERS_MISSING_ARG_PTR_TAG_H

#include "move_tags.h"

// Bad
int declared_in_header(int* bad_arg);

// Bad in source
void header_and_source_tag_mismatch(out float* f_ptr);

#endif // TESTS_HEADERS_MISSING_ARG_PTR_TAG_H