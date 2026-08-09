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

void initialization_testing()
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

    int_vector_destroy(&vec_1);                                      // good
    int_vector_destroy(&vec_2);                                      // good
    int_vector_destroy(&vec_3);                                      // good
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

void take_raii_struct_by_value(int_vector_t raii_value_arg)
{
    int_vector_foo(&raii_value_arg);
    int_vector_destroy(&raii_value_arg);
}

void give_raii_struct_by_value()
{
    int_vector_t vec = int_vector_make(10);              // good
    take_raii_struct_by_value(vec);                      // bad
    
    int_vector_t* vec_ptr = &vec;                        // good               
    take_raii_struct_by_value(*vec_ptr);                 // bad

    struct int_vector_array vec_array = int_vector_array_make(); // good
    take_raii_struct_by_value(vec_array.vecs[0]);         // bad

    int int_arr[5] = {1, 2, 3, 4, 5};                    // good
    take_raii_struct_by_value((int_vector_t){int_arr, 5, 5}); // bad

    take_raii_struct_by_value(int_vector_make(10));      // good
    take_raii_struct_by_value(int_vector_copy(&vec));    // good
    take_raii_struct_by_value(int_vector_move(&vec));    // good

    int_vector_array_destroy(&vec_array);                // good
    int_vector_destroy(&vec);                            // good
}

int take_pod_by_value(pos_t pod_value_arg)
{
    return pod_value_arg.x + pod_value_arg.y;
}

int give_pod_struct_by_value()
{
    pos_t pos = pos_pod(1, 2);                          // good
    int res1 = take_pod_by_value(pos);                  // good
    pos_t* pos_ptr = &pos;                              // good
    int res2 = take_pod_by_value(*pos_ptr);             // good
    struct pos_array pos_arr = pos_array_pod();         // good
    int res3 = take_pod_by_value(pos_arr.positions[1]); // good

    int res4 = take_pod_by_value((pos_t){3,4});         // bad

    return res1 + res2 + res3 + res4;
}

pos_t bad_pod_return()
{
    return (pos_t){5, 6}; // bad
}

pos_t good_pod_return()
{
    return pos_pod(7, 8); // good
}

pos_t good_pod_return_2()
{
    pos_t pos = pos_pod(9, 10);
    return pos;
}

pos_t good_pod_return_3()
{
    pos_t pos = pos_pod(11, 12);
    pos_t* pos_ptr = &pos;
    return *pos_ptr;
}

int_vector_t good_raii_return()
{ 
    return int_vector_make(10);
}

int_vector_t good_raii_return_2(int_vector_t* vec_ptr)
{
    return int_vector_copy(vec_ptr);
}

int_vector_t bad_raii_return()
{
    int int_arr[3] = { 1, 2, 3};
    return (int_vector_t){int_arr, 3, 3};
}

int_vector_t bad_raii_return_2()
{
    int_vector_t vec = int_vector_make(10);
    int_vector_destroy(&vec);
    return vec;
}

int_vector_t bad_raii_return_3()
{
    int_vector_t vec = int_vector_make(10);
    int_vector_t* vec_ptr = &vec;
    int_vector_destroy(&vec);
    return *vec_ptr;
}

void detect_no_cleanup()
{
    int_vector_t no_cleanup_var = int_vector_make(10);
    int_vector_foo(&no_cleanup_var);
    // Cleanup is missing here, should be:
    // int_vector_destroy(&no_cleanup_var);
}

void detect_no_cleanup_of_arg(int_vector_t no_cleanup_arg)
{
    int_vector_foo(&no_cleanup_arg);
    // Cleanup is missing here, should be:
    // int_vector_destroy(&no_cleanup_arg);
}

int detect_many_missing_cleanups(int_vector_t arg1, int_vector_t arg2)
{
    int_vector_t local1 = int_vector_make(10);
    int_vector_t local2 = int_vector_copy(&arg1);
    int_vector_t local3 = int_vector_move(&arg2);

    // Cleanup is missing for arg1, arg2, local1, local2, local3
    return 0;
}

void complex_missing_cleanup(int i, int_vector_t arg)
{
    int_vector_t local1 = int_vector_make(10);
    int_vector_t local2 = int_vector_copy(&arg);

    if (i > 0) {
        int_vector_t local3 = int_vector_move(&local1);
        int_vector_t local4 = int_vector_copy(&local3);

        // Cleanup is missing for local1, local2, local3
        int_vector_destroy(&arg);
        int_vector_destroy(&local4);
        return;
    }

    // Cleanup missing for local1
    int_vector_destroy(&local2);
    int_vector_destroy(&arg);
}

void complex_missing_cleanup_2(int i)
{
    int_vector_t local_vec_1 = int_vector_make(10);

    for (int j = 0; j < i; ++j) {
        int_vector_t local_vec_2 = int_vector_copy(&local_vec_1);

        if (j % 2 == 0) {
            int_vector_t local_vec_3 = int_vector_move(&local_vec_2);
            // Cleanup missing for local_vec_3
            int_vector_destroy(&local_vec_1);
            int_vector_destroy(&local_vec_2);
            continue;
        }

        if (i > 10)
        {
            int_vector_destroy(&local_vec_2);
            break;
        }
        // Cleanup missing for local_vec_2
    }
    int_vector_destroy(&local_vec_1);
}