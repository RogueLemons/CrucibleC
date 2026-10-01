#ifndef TESTS_HEADERS_REFERENCE_DECLARATION_TAGS_H
#define TESTS_HEADERS_REFERENCE_DECLARATION_TAGS_H

#include "ref_tag.h"

void header_definition_mismatch(REF int* value);
void header_redeclaration_mismatch(REF int* value);
void matching_multiple_references(
    REF int* first,
    REF int* second,
    int* third);
void mismatched_multiple_references(
    REF int* first,
    REF int* second,
    int* third);

#endif