// Not parsed: a third-party folder from the config

void third_party(void)
{
    goto third_party_file; // never reported
third_party_file:
    return;
}
