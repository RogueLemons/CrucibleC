#include "headers/managed_structs.h"
#include "external/some_struct.h"
// #include <stdlib.h>
// #include <string.h>

struct ForwardDeclaredStructShallNotTriggerError;

struct free_struct_with_raii
{
    int_vector_t vec;
};
typedef struct free_struct_with_raii free_struct_with_raii_t;
void free_struct_with_raii_init(free_struct_with_raii_t* const self, int capacity);

pos_t pos_pod(int x, int y)
{
    pos_t self = {0};
    self.x = x;
    self.y = y;
    return self;
}

struct good_pod
{
    int a;
    int b;
    pos_t pos;
};
struct good_pod good_pod_pod();

struct bad_pod_with_raii_struct
{
    int a;
    int b;
    int_vector_t vec;
};
struct bad_pod_with_raii_struct bad_pod_with_raii_struct_pod();

struct pod_with_external_struct
{
    int a;
    int b;
    struct SomeStruct some_field;
};
struct pod_with_external_struct pod_with_external_struct_pod();

struct bad_pod_with_free_struct
{
    int a;
    int b;
    free_struct_t free_field;
};
struct bad_pod_with_free_struct bad_pod_with_free_struct_pod();

int_vector_t int_vector_make(int capacity)
{
    int_vector_t self = {0};
    self.size = 0;
    self.capacity = capacity;
    // self.data = (int*)malloc(capacity * sizeof(int));
    // if (self.data == NULL) {
    //     self.capacity = -1;
    //     return self;
    // }
    return self;
}
int_vector_t int_vector_copy(const int_vector_t* const self)
{
    int_vector_t copy = {0};

    if (!self)
    {
        copy.capacity = -1;
        return copy;
    }

    copy.size = self->size;
    copy.capacity = self->capacity;
    if (self->capacity > 0) {
        // copy.data = (int*)malloc(self->capacity * sizeof(int));
        // if (copy.data == NULL) {
        //     copy.capacity = -1;
        //     return copy;
        // }
        // memcpy(copy.data, self->data, self->size * sizeof(int));
    }
    return copy;
}
int_vector_t int_vector_move(int_vector_t* const self)
{
    int_vector_t moved = {0};

    if (!self)
    {
        moved.capacity = -1;
        return moved;
    }

    moved.size = self->size;
    moved.capacity = self->capacity;
    moved.data = self->data;

    self->size = 0;
    self->capacity = 0;
    // self->data = NULL;

    return moved;
}
void int_vector_destroy(int_vector_t* const self)
{
    if (!self)
        return;

    if (self->data) {
        // free(self->data);
        // self->data = NULL;
    }
    self->size = 0;
    self->capacity = 0;
}
_Bool int_vector_valid(const int_vector_t* const self)
{
    if (!self)
        return 0;

    if (self->capacity < 0)
        return 0;

    return 1;
}

void int_vector_foo(int_vector_t* const self)
{
    if (!self || !int_vector_valid(self))
        return;

    int_vector_t vec = int_vector_copy(self); // good
    vec = int_vector_make(15);                // bad
    self->size = 0;                           // good
    int_vector_t* vec_ptr = &vec;             // bad
    *vec_ptr = int_vector_make(5);            // bad
    *self = vec;                              // bad

    int_vector_destroy(&vec);                 // good
}

void int_vector_foo_with_supression(int_vector_t* const self)
{
    if (!self || !int_vector_valid(self))
        return;

    int_vector_t vec = int_vector_copy(self); // good
    // WorkshopC off
    vec = int_vector_make(15);                // bad
    self->size = 0;                           // good
    int_vector_t* vec_ptr = &vec;             // bad
    *vec_ptr = int_vector_make(5);            // bad
    *self = vec;                              // bad
    // WorkshopC on
    int_vector_destroy(&vec);                 // good
}

void foo()
{
    int_vector_t vec = int_vector_make(10);     // good
    int_vector_t vec2;                          // bad
    int_vector_t vec3 = vec;                    // bad

    pos_t pos = pos_pod(1, 2);                  // good
    pos_t pos2;                                 // bad
    pos = pos_pod(3, 4);                        // good
    pos_t pos3 = pos;                           // good
    pos_t pos4 = {0};                           // bad
    pos_t pos5 = {1, 2};                        // bad
    pos_t* pos_ptr = &pos;                      // good
    pos_t pos6 = *pos_ptr;                      // good

    int_vector_destroy(&vec);                   // good
    int_vector_destroy(&vec2);                  // good
    int_vector_destroy(&vec3);                  // good
}

struct int_vector_array
{
    int_vector_t vecs[3];
};
struct int_vector_array int_vector_array_make();
struct int_vector_array int_vector_array_copy(const struct int_vector_array* const self);
struct int_vector_array int_vector_array_move(struct int_vector_array* const self);
void int_vector_array_destroy(struct int_vector_array* const self);
_Bool int_vector_array_valid(const struct int_vector_array* const self);

void raii_array_testing()
{
    int_vector_t matrix[3];                                          // bad
    int_vector_t matrix2[2] = {int_vector_make(10), int_vector_make(10)};      // bad

    struct int_vector_array vec_array = int_vector_array_make();     // good
    int_vector_t vec_1 = int_vector_make(10);                        // good
    vec_array.vecs[0] = vec_1;                                       // bad
    int_vector_t vec_2 = int_vector_copy(&vec_array.vecs[1]);        // good
    int_vector_t vec_3 = vec_array.vecs[2];                          // bad

    int_vector_array_destroy(&vec_array);                            // good
}

struct pos_array
{
    pos_t positions[3];
};
struct pos_array pos_array_pod();

void pod_array_testing()
{
    pos_t positions[3];                                        // bad
    pos_t positions2[2] = {pos_pod(1, 2), pos_pod(3, 4)};      // bad

    struct pos_array pos_array = pos_array_pod();              // good
    pos_t pos_1 = pos_pod(5, 6);                               // good
    pos_array.positions[0] = pos_1;                            // good
    pos_t pos_2 = pos_array.positions[1];                      // good
}

// TODO:
// 1. done! Fix error message to include the variable name (inside foo)
// 2. Make pod_array_testing function
// 3. Add errors for bad pod and raii struct arrays