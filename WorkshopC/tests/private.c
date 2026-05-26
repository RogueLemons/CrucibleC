#include "headers/private_access.h"

static RGB pget_rgb(const Color* c)
{
    RGB rgb = {0};
    rgb.r = c->_private.r;
    rgb.g = c->_private.g;
    rgb.b = c->_private.b;
    return rgb;
}

static void pset_rgb(Color* c, int r, int g, int b)
{
    c->_private.r = r;
    c->_private.g = g;
    c->_private.b = b;
}

RGB get_rgb(const Color* c)
{
    return pget_rgb(c);
}

void set_color(Color* c, int r, int g, int b)
{
    pset_rgb(c, r, g, b);
}

void bad_set(Color* c, int r, int g, int b)
{
    c->_private.r = r;
    // c->_private.g = g;
    // c->_private.b = b;
}