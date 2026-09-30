// Parsed: a .c file in a subfolder

void nested(void)
{
    goto nested_file; // bad
nested_file:
    return;
}
