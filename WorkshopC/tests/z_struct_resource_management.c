#include "external/some_struct.h"

// WorkshopC off
struct IgnoredPodStruct
{
    int x;
    int y;
};
typedef struct IgnoredPodStruct IgnoredPodStruct;

static inline IgnoredPodStruct IgnoredPodStruct_pod(int x, int y)
{
    IgnoredPodStruct self = {0};
    self.x = x;
    self.y = y;
    return self;
}
// WorkshopC on

struct PodStruct
{
    int x;
    int y;
};
typedef struct PodStruct PodStruct;


static inline PodStruct PodStruct_pod(int x, int y)
{
    PodStruct self = {0};
    self.x = x;
    self.y = y;
    return self;
}

struct RaiiStruct
{
    int x;
    int y;
};
typedef struct RaiiStruct RaiiStruct;

static inline RaiiStruct RaiiStruct_make()
{
    RaiiStruct self = {0};
    return self;
}

RaiiStruct RaiiStruct_copy(const RaiiStruct* self);
RaiiStruct RaiiStruct_move(RaiiStruct* self);
void RaiiStruct_destroy(RaiiStruct* self);
_Bool RaiiStruct_valid(const RaiiStruct* self);

struct FreeStruct
{
    int x;
    int y;
};
typedef struct FreeStruct FreeStruct;

static inline int FreeStruct_init(FreeStruct* self, int x, int y)
{
    if (!self)
        return -1;

    self->x = x;
    self->y = y;
    return 0;
}

struct InvalidStruct
{
    int x;
    int y;
};
typedef struct InvalidStruct InvalidStruct;

InvalidStruct InvalidStruct_make();
InvalidStruct InvalidStruct_pod();