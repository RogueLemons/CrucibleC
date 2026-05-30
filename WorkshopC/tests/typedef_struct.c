#include <stdint.h>
#include "headers/missing_struct_typedef.h"
#include "external/missing_struct_typedef.h"

/*
========================================================
GOOD CASES
These should NOT trigger.
========================================================
*/

/* Typical C typedef style */
typedef struct GoodStructA {
    int value;
} GoodStructA;

/* Different typedef name is allowed */
typedef struct GoodStructB {
    int value;
} MyStructAlias;

/* Anonymous struct with typedef */
typedef struct {
    int value;
} AnonymousGood;

/* Forward declaration + typedef */
struct GoodStructC;

typedef struct GoodStructC GoodStructC;

struct GoodStructC {
    int value;
};

/* Multiple typedefs are allowed */
typedef struct GoodStructD {
    int value;
} GoodStructD;

typedef GoodStructD GoodStructD_Alias;

#define DECLARE_GOOD_STRUCT(name) \
    typedef struct name {         \
        int value;                \
    } name

/* Should NOT trigger */
DECLARE_GOOD_STRUCT(MacroGood);

/* Typedef comes after declaration */
struct EdgeStructA {
    int value;
};

typedef struct EdgeStructA EdgeStructA;

/* Separate declaration + later typedef */
struct EdgeStructB;

typedef struct EdgeStructB EdgeStructB;

struct EdgeStructB {
    int value;
};

typedef struct GoodStructWithAnonNested
{
    int x;
    struct 
    {
        int i, j, k;
    } anon_nested;
} GoodStructWithAnonNested;

/* Anonymous struct without typedef */
struct {
    int value;
} ok_global_instance;


/*
========================================================
BAD CASES
These SHOULD trigger.
========================================================
*/

/* No typedef */
struct BadStructA {
    int value;
};

/* Forward declaration only, no typedef */
struct BadStructB;

/* Definition without typedef */
struct BadStructC {
    float value;
};

/* Nested struct without typedef */
struct BadStructD {
    struct NestedBad {
        int x;
    } nested;
};

#define DECLARE_BAD_STRUCT(name) \
    struct name {                \
        int value;               \
    }


/* Should trigger unless macro is defined
in a third-party header */
DECLARE_BAD_STRUCT(MacroBad);

DECLARE_BAD_STRUCT_FROM_HEADER(BadStructFromHeaderDefine);

DECLARE_BAD_STRUCT_FROM_EXTERNAL_HEADER(BadStructFromExternalHeaderDefine);