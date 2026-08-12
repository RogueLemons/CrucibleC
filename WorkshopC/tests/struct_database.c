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

struct PodStruct2
{
    int x;
    int y;
};
typedef struct PodStruct2 PodStruct2_t;
PodStruct2_t PodStruct2_pod(int x, int y);

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
RaiiStruct RaiiStruct_return(RaiiStruct* self);
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

// Bad code below

struct InvalidStruct
{
    int x;
    int y;
};
typedef struct InvalidStruct InvalidStruct;
InvalidStruct InvalidStruct_make();
InvalidStruct InvalidStruct_pod();

struct BadFreeStruct
{
    int x;
    int y;
};
typedef struct BadFreeStruct BadFreeStruct;
int BadFreeStruct_init(BadFreeStruct* selfie);

struct BadPodStruct
{
    int x;
    int y;
};
typedef struct BadPodStruct BadPodStruct_t;
int BadPodStruct_pod(BadPodStruct_t* self, int x, int y);

struct BadRaiiStruct
{
    int x;
    int y;
};
typedef struct BadRaiiStruct BadRaiiStruct;
BadRaiiStruct BadRaiiStruct_make();
BadRaiiStruct BadRaiiStruct_copy(const BadRaiiStruct* bad_self);
BadRaiiStruct BadRaiiStruct_move(BadRaiiStruct* bad_self);
void BadRaiiStruct_destroy(BadRaiiStruct* bad_self);
BadRaiiStruct BadRaiiStruct_return(BadRaiiStruct* bad_self);
_Bool BadRaiiStruct_valid(const BadRaiiStruct* bad_self);

struct BadRaiiStruct2
{
    int x;
    int y;
};
typedef struct BadRaiiStruct2 BadRaiiStruct2_t;
BadRaiiStruct2_t BadRaiiStruct2_make();
BadRaiiStruct2_t BadRaiiStruct2_copy(BadRaiiStruct2_t* self);
BadRaiiStruct2_t BadRaiiStruct2_move(const BadRaiiStruct2_t* self);
int BadRaiiStruct2_destroy(BadRaiiStruct2_t* self);
float BadRaiiStruct2_valid(const BadRaiiStruct2_t* self);