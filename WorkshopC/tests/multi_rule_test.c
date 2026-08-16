#include "headers/type/food/fruit.h"
#include "headers/type/tool/bowl.h"
#include "headers/type/tool/knife.h"
#include "headers/core/kitchen_accessor.h"
#include "headers/app/runner.h"
#include "headers/app/fruitbowl.h"

// ------------------- app/runner.c -------------------

#define TIMEOUT_MS 5000

static int make_fruit_sallad(mutable core__kitchen_accessor* kitchen, mutable app__fruitbowl* fruitbowl)
{
    if (!kitchen || !fruitbowl || !app__fruitbowl__valid(fruitbowl))
    {
        return 666;
    }

    type__tool__knife* knife = core__kitchen_accessor__request_knife(kitchen, TIMEOUT_MS);
    if (!knife)
    {
        return 4;
    }

    type__food__fruit* apple  = core__kitchen_accessor__take_fruit_if_remaining(kitchen, TYPE_FOOD_FRUIT_APPLE);
    type__food__fruit* banana = core__kitchen_accessor__take_fruit_if_remaining(kitchen, TYPE_FOOD_FRUIT_BANANA);
    type__food__fruit* orange = core__kitchen_accessor__take_fruit_if_remaining(kitchen, TYPE_FOOD_FRUIT_ORANGE);
    if (!apple || !banana || !orange)
    {
        core__kitchen_accessor__return_knife(kitchen, move(knife));
        return 5;
    }

    app__fruitbowl__add_fruit(fruitbowl, move(apple));
    app__fruitbowl__add_fruit(fruitbowl, move(banana));
    app__fruitbowl__add_fruit(fruitbowl, move(orange));

    app__fruitbowl__chop_and_mix(fruitbowl, knife);
    core__kitchen_accessor__return_knife(kitchen, move(knife));

    if (!app__fruitbowl__valid(fruitbowl))
    {
        // Causes error, cannot access e.g. for print
        int number_of_fruits = fruitbowl->_private.count;
        return 6;
    }

    return 0;
}

static int use_tools_and_food(mutable core__kitchen_accessor* kitchen)
{
    if (!kitchen)
    {
        return 666;
    }

    type__tool__bowl* bowl = core__kitchen_accessor__request_bowl(kitchen, TIMEOUT_MS);
    if (!bowl)
    {
        return 2;
    }

    app__fruitbowl fruitbowl = app__fruitbowl__make(move(bowl), kitchen);
    if (!app__fruitbowl__valid(&fruitbowl))
    {
        app__fruitbowl__destroy(&fruitbowl);
        return 3;
    }

    int res = make_fruit_sallad(kitchen, &fruitbowl);
    if (res == 0)
    {
        app__fruitbowl__eat_all(&fruitbowl);
    }
    
    app__fruitbowl__destroy(&fruitbowl);
    return res;
}

int app__run()
{
    int res = 1;

    core__kitchen_accessor kitchen;
    if (core__kitchen_accessor__init_manually(&kitchen) == 0)
    {
        res = use_tools_and_food(&kitchen);

        // Parser will not help to remember cleanup since manual memory management
        core__kitchen_accessor__cleanup(&kitchen);
    }

    return res;
}


// ------------------- app/fruitbowl.c -------------------

// WorkshopC off
// When WorkshopC is on all pod structs must be initialized by its rules.
// app__fruitbowl_private can instead be free struct or a free struct wrapper
// of app__fruitbowl_private could be made for the global static variable.
static app__fruitbowl_private g_null_deref_return = {0};
// WorkshopC on

static const app__fruitbowl_private* pget(const app__fruitbowl* fruitbowl)
{
    if (!fruitbowl)
    {
        g_null_deref_return.valid = 0;
        return &g_null_deref_return;
    }

    return &(fruitbowl->_private);
}
static app__fruitbowl_private* pset(mutable app__fruitbowl* fruitbowl)
{
    if (!fruitbowl)
    {
        g_null_deref_return.valid = 0;
        return &g_null_deref_return;
    }

    return &(fruitbowl->_private);
}

app__fruitbowl app__fruitbowl__make(moved type__tool__bowl* bowl, mutable core__kitchen_accessor* kitchen)
{
    if (!bowl)
    {
        return (app__fruitbowl){ 0, 0, 0, 0, 0, 0, 0 };
    }

    app__fruitbowl self = {0};
    // pset(&self)->fruits malloc for e.g. 10 fruits
    // verify pget(&self)->fruits is not null
    pset(&self)->count = 0;
    pset(&self)->capacity = 10;
    pset(&self)->mixed = 0;
    pset(&self)->bowl = bowl;
    pset(&self)->kitchen = kitchen;
    pset(&self)->valid = 1;

    return self;
}

app__fruitbowl app__fruitbowl__copy(const app__fruitbowl* self); // No support/impl due to using kitchen resources

app__fruitbowl app__fruitbowl__move(mutable app__fruitbowl* self)
{
    if (!self)
    {
        return (app__fruitbowl){ { 0, 0, 0, 0, 0, 0, 0 } };
    }

    app__fruitbowl moved_to = (*self);
    (*self) = (app__fruitbowl){ { 0, 0, 0, 0, 0, 0, 0 } };
    return moved_to;
}

void app__fruitbowl__destroy(mutable app__fruitbowl* self)
{
    if (!self || !pget(self)->valid)
    {
        return;
    }

    // Free memory for self->fruits
    core__kitchen_accessor__return_bowl(pset(self)->kitchen, move(pset(self)->bowl));
    // memset self to zero, maybe even set valid to 0/false manually
}

app__fruitbowl app__fruitbowl__return(mutable app__fruitbowl* self)
{
    if (!self)
    {
        return (app__fruitbowl){ { 0, 0, 0, 0, 0, 0, 0 } };
    }

    return (*self);
}

_Bool app__fruitbowl__valid(const app__fruitbowl* self)
{
    if (!self)
    {
        return 0;
    }
    return pget(self)->valid;
}

void app__fruitbowl__add_fruit(mutable app__fruitbowl* self, moved type__food__fruit* fruit)
{
    if (!self || !fruit || !pget(self)->valid || pget(self)->mixed)
    {
        return;
    }

    // Implement logic for assigning next element in fruits
    // Increase capacity if needed with new malloc and copy of old data
}

void app__fruitbowl__chop_and_mix(mutable app__fruitbowl* self, const type__tool__knife* knife)
{
    if (!self || !knife || !pget(self)->valid)
    {
        return;
    }

    // Perform hardware actions (aka chop fruits in self with knife)
    pset(self)->mixed = 1;
}

void app__fruitbowl__eat_all(mutable app__fruitbowl* self)
{
    if (!self || !pget(self)->valid || !pget(self)->mixed)
    {
        return;
    }

    // Perform hardware actions (aka consume resources)
    // Free memory for fruits or set fruit pointers in 
    // fruit to null (consumed)
}

// ------------------- core/kitchen_accessor.c -------------------
// No impl in this test file, exists to showcase the test