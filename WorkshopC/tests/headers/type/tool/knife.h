#ifndef TYPE_TOOL_KNIFE_H
#define TYPE_TOOL_KNIFE_H

// This will trigger error for lacking prefix, typedef, and create function
struct knife
{
    int field;
};

// This follows rules of the config
struct type__tool__knife
{
    const float sharpness;
    const float weight;
    const float length;
    const char* const manufacturer;
    const char* const owner;
};
typedef struct type__tool__knife type__tool__knife;

static inline type__tool__knife type__tool__knife__pod( float sharpness,
                                                        float weight,
                                                        float length,
                                                        const char* manufacturer,
                                                        const char* owner)
{
    return (type__tool__knife){ sharpness, weight, length, manufacturer, owner };
}

#endif // TYPE_TOOL_KNIFE_H