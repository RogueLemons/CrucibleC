static int callback_source(int value)
{
    return value;
}

void probe_adopt_more(void)
{
    int (*callback)(int) = callback_source;
    (void)callback(1);
    return;
}