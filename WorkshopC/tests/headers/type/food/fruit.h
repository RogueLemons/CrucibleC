#ifndef TYPE_FOOD_FRUIT_H
#define TYPE_FOOD_FRUIT_H

// This will trigger error because config is set to disallow enums
enum error_fruit_option
{
    APPLE,
    BANANA,
    ORANGE
};

// This option provides type safety instead
struct type__food__fruit_option_tag;
typedef const struct type__food__fruit_option_tag* type__food__fruit_option;
type__food__fruit_option type__food__fruit__apple();
#define TYPE_FOOD_FRUIT_APPLE type__food__fruit__apple()
type__food__fruit_option type__food__fruit__banana();
#define TYPE_FOOD_FRUIT_BANANA type__food__fruit__banana()
type__food__fruit_option type__food__fruit__orange();
#define TYPE_FOOD_FRUIT_ORANGE type__food__fruit__orange()

struct type__food__fruit
{
    float weight;
    type__food__fruit_option option;
};
typedef struct type__food__fruit type__food__fruit;

static inline type__food__fruit type__food__fruit__pod(float weight, type__food__fruit_option option)
{
    return (type__food__fruit){weight, option};
}

#endif // TYPE_FOOD_FRUIT_H