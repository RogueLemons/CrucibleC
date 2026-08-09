
struct pos {
    int x;
    int y;
};
typedef struct pos pos_t;
pos_t pos_pod(int x, int y);

struct int_vector {
    int* data;
    int size;
    int capacity;
};
typedef struct int_vector int_vector_t;
int_vector_t int_vector_make(int capacity);
int_vector_t int_vector_copy(const int_vector_t* const self);
int_vector_t int_vector_move(int_vector_t* const self);
void int_vector_destroy(int_vector_t* const self);
_Bool int_vector_valid(const int_vector_t* const self);

struct free_struct {
    int x;
    int y;
};
typedef struct free_struct free_struct_t;
void free_struct_init(free_struct_t* const self, int x, int y);