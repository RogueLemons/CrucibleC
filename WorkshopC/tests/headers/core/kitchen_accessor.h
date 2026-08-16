#ifndef CORE_KITCHEN_ACCESSOR_H
#define CORE_KITCHEN_ACCESSOR_H

#include "../../external/move_tags.h"
#include "../type/tool/bowl.h"
#include "../type/tool/knife.h"
#include "../type/food/fruit.h"

typedef struct core__kitchen_accessor
{
    // Fields for kitchen, mutex, lists, hardware
} core__kitchen_accessor;

int core__kitchen_accessor__init_manually(mutable core__kitchen_accessor* self);
void core__kitchen_accessor__cleanup(mutable core__kitchen_accessor* self);

// requesters and returners
type__tool__knife* core__kitchen_accessor__request_knife(mutable core__kitchen_accessor* self, int timeout_ms);
void core__kitchen_accessor__return_knife(mutable core__kitchen_accessor* self, moved type__tool__knife* knife);
type__tool__bowl* core__kitchen_accessor__request_bowl(mutable core__kitchen_accessor* self, int timeout_ms);
void core__kitchen_accessor__return_bowl(mutable core__kitchen_accessor* self, moved type__tool__bowl* bowl);
type__food__fruit* core__kitchen_accessor__take_fruit_if_remaining(mutable core__kitchen_accessor* self, type__food__fruit_option fruit_option);

#endif // CORE_KITCHEN_ACCESSOR_H