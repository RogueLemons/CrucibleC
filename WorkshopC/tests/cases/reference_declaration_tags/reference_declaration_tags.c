#include "headers/reference_declaration_tags.h"

void header_definition_mismatch(int* value)
{
}

void header_redeclaration_mismatch(int* value);
void header_redeclaration_mismatch(REF int* value)
{
}

void source_definition_mismatch(REF int* value);
void source_definition_mismatch(int* value)
{
}

void matching_multiple_references(
    REF int* first,
    REF int* second,
    int* third)
{
}

void mismatched_multiple_references(
    REF int* first,
    int* second,
    REF int* third)
{
}