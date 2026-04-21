#ifndef IC_GLUE_MACRO_H
#define IC_GLUE_MACRO_H

#define IC_INNER_GLUE_IMPL(a, b) a##b
#define IC_GLUE(a, b) IC_INNER_GLUE_IMPL(a, b)
#define IC_GLUE3(a, b, c) IC_GLUE(IC_GLUE(a, b), c)
#define IC_GLUE4(a, b, c, d) IC_GLUE(IC_GLUE3(a, b, c), d)
#define IC_GLUE5(a, b, c, d, e) IC_GLUE(IC_GLUE4(a, b, c, d), e)
#define IC_GLUE6(a, b, c, d, e, f) IC_GLUE(IC_GLUE5(a, b, c, d, e), f)
#define IC_GLUE7(a, b, c, d, e, f, g) IC_GLUE(IC_GLUE6(a, b, c, d, e, f), g)
#define IC_GLUE8(a, b, c, d, e, f, g, h) IC_GLUE(IC_GLUE7(a, b, c, d, e, f, g), h)
#define IC_GLUE9(a, b, c, d, e, f, g, h, i) IC_GLUE(IC_GLUE8(a, b, c, d, e, f, g, h), i)


#endif // IC_GLUE_MACRO_H