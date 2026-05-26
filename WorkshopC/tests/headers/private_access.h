#ifndef TESTS_HEADERS_PRIVATE_ACCESS_H
#define TESTS_HEADERS_PRIVATE_ACCESS_H

struct Color
{
    int weight;
    struct {
        int r, g, b;
    } _private;
};
typedef struct Color Color;

struct RGB
{
    int r, g, b;
};
typedef struct RGB RGB;

RGB get_rgb(const Color* c);
void set_color(Color* c, int r, int g, int b);

static RGB pget_color_in_header(const Color* c)
{
    RGB rgb = {0};
    rgb.r = c->_private.r;
    // rgb.g = c->_private.g;
    // rgb.b = c->_private.b;
    return rgb;
}

#endif // TESTS_HEADERS_PRIVATE_ACCESS_H