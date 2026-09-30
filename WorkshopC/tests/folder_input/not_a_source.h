// Not parsed: only .c files are sources, and nothing includes this header

static void not_a_source(void)
{
    goto header_file; // never reported
header_file:
    return;
}
