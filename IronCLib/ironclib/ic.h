#ifndef IRON_C_FULL_H
#define IRON_C_FULL_H

// No internal ic dependencies
#include "ic_inline.h"
#include "ic_static_assert.h"
#include "ic_bounded_loop.h"
// Internally dependent on the above (no circular dependencies)
#include "ic_memory.h"
#include "ic_num_cast.h"
#include "ic_typenum.h"
#include "ic_result.h"
#include "ic_opaque_storage.h"
#include "ic_concurrency.h"
#include "ic_concurrency_signal.h"
#include "ic_co_job.h"

#endif // IRON_C_FULL_H
