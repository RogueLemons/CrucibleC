static int callback_source(int value)
{
    return value;
}

void probe_adopt(void)
{
    int (*callback)(int) = callback_source;
    (void)callback(1);
    return;
}