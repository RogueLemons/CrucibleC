#ifndef APP_FRUITBOWL_H
#define APP_FRUITBOWL_H

#include "../../external/move_tags.h"
#include "../type/food/fruit.h"
#include "../type/tool/bowl.h"
#include "../type/tool/knife.h"
#include "../core/kitchen_accessor.h"

typedef struct app__fruitbowl_private
{
    type__food__fruit** fruits;
    int count;
    int capacity;
    _Bool mixed;
    type__tool__bowl* bowl;
    core__kitchen_accessor* kitchen;
    _Bool valid;
} app__fruitbowl_private;
app__fruitbowl_private app__fruitbowl_private__pod(); // No impl needed, used to declare struct type

typedef struct app__fruitbowl
{
    app__fruitbowl_private _private;
} app__fruitbowl;

app__fruitbowl app__fruitbowl__make(moved type__tool__bowl* bowl, mutable core__kitchen_accessor* kitchen);
app__fruitbowl app__fruitbowl__copy(const app__fruitbowl* self);
app__fruitbowl app__fruitbowl__move(mutable app__fruitbowl* self);
void app__fruitbowl__destroy(mutable app__fruitbowl* self);
app__fruitbowl app__fruitbowl__return(mutable app__fruitbowl* self);
_Bool app__fruitbowl__valid(const app__fruitbowl* self);

void app__fruitbowl__add_fruit(mutable app__fruitbowl* self, moved type__food__fruit* fruit);
void app__fruitbowl__chop_and_mix(mutable app__fruitbowl* self, const type__tool__knife* knife);
void app__fruitbowl__eat_all(mutable app__fruitbowl* self);

#endif // APP_FRUITBOWL_H