#ifndef TYPE_TOOL_BOWL_H
#define TYPE_TOOL_BOWL_H

struct type__tool__bowl
{
    float radius;
    float height;
    float weight;
    const char* const manufacturer;
    const char* const owner;
};
typedef struct type__tool__bowl type__tool__bowl;

type__tool__bowl type__tool__bowl__pod( float radius, 
                                        float height, 
                                        float weight, 
                                        const char* manufacturer, 
                                        const char* owner)
{
    return (type__tool__bowl){ radius, height, weight, manufacturer, owner};
}

#endif // TYPE_TOOL_BOWL_H