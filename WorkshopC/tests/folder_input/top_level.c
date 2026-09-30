// Parsed: a .c file directly in the folder

void top_level(void)
{
    goto top_level_file; // bad
top_level_file:
    return;
}
