#include <stddef.h>

#include "headers/managed_structs.h"

// -------------------------------------------------------------
// With raii_standardized_destroy_definitions, the destroy function of
// a raii struct destroys each of its raii fields exactly once, all
// together in one block: the body, or directly inside an if statement
// in the body, with any condition. With raii_destroy_in_reverse_order
// they come in reverse declaration order.
// -------------------------------------------------------------

void int_vector_destroy_array(int_vector_t *self, size_t n);
void log_text(const char *text);
void memory_free(void *pointer);

// Every owner struct holds the raii fields "first" and "second"

// -------------------------------------------------------------
// Good
// -------------------------------------------------------------

struct in_body
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct in_body in_body_t;
in_body_t in_body_make(void);
in_body_t in_body_copy(const in_body_t *const self);
in_body_t in_body_move(in_body_t *const self);
void in_body_destroy(in_body_t *const self);
in_body_t in_body_return(in_body_t *self);
_Bool in_body_valid(const in_body_t *const self);

void in_body_destroy(in_body_t *const self)
{
    int_vector_destroy(&self->second); // good
    memory_free(self->buffer); // good: other statements may come in between
    int_vector_destroy(&self->first); // good
}

struct in_if
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct in_if in_if_t;
in_if_t in_if_make(void);
in_if_t in_if_copy(const in_if_t *const self);
in_if_t in_if_move(in_if_t *const self);
void in_if_destroy(in_if_t *const self);
in_if_t in_if_return(in_if_t *self);
_Bool in_if_valid(const in_if_t *const self);

void in_if_destroy(in_if_t *const self)
{
    if (self != NULL && !self->moved && in_if_valid(self))
    {
        int_vector_destroy(&self->second); // good: directly inside an if, with any condition
        int_vector_destroy(&self->first); // good
    }
}

struct in_if_with_else
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct in_if_with_else in_if_with_else_t;
in_if_with_else_t in_if_with_else_make(void);
in_if_with_else_t in_if_with_else_copy(const in_if_with_else_t *const self);
in_if_with_else_t in_if_with_else_move(in_if_with_else_t *const self);
void in_if_with_else_destroy(in_if_with_else_t *const self);
in_if_with_else_t in_if_with_else_return(in_if_with_else_t *self);
_Bool in_if_with_else_valid(const in_if_with_else_t *const self);

void in_if_with_else_destroy(in_if_with_else_t *const self)
{
    if (!self->moved)
    {
        int_vector_destroy(&self->second); // good
        int_vector_destroy(&self->first); // good
    }
    else
    {
        log_text("already moved"); // good: an else branch may do anything but destroy fields
    }
}

struct with_logic
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct with_logic with_logic_t;
with_logic_t with_logic_make(void);
with_logic_t with_logic_copy(const with_logic_t *const self);
with_logic_t with_logic_move(with_logic_t *const self);
void with_logic_destroy(with_logic_t *const self);
with_logic_t with_logic_return(with_logic_t *self);
_Bool with_logic_valid(const with_logic_t *const self);

void with_logic_destroy(with_logic_t *const self)
{
    int flags = self->moved ? 0 : 3;

    if (flags & 1)
        memory_free(self->buffer);

    if (flags & 2)
    {
        int_vector_destroy(&self->second); // good: complex logic around the destroys is fine
        int_vector_destroy(&self->first); // good
    }
}

struct single_statement_if
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct single_statement_if single_statement_if_t;
single_statement_if_t single_statement_if_make(void);
single_statement_if_t single_statement_if_copy(const single_statement_if_t *const self);
single_statement_if_t single_statement_if_move(single_statement_if_t *const self);
void single_statement_if_destroy(single_statement_if_t *const self);
single_statement_if_t single_statement_if_return(single_statement_if_t *self);
_Bool single_statement_if_valid(const single_statement_if_t *const self);

void single_statement_if_destroy(single_statement_if_t *const self)
{
    int_vector_destroy(&self->second); // good
    int_vector_destroy(&self->first); // good

    if (self->buffer != NULL)
        memory_free(self->buffer); // good: not a raii field
}

struct goto_into_cleanup
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct goto_into_cleanup goto_into_cleanup_t;
goto_into_cleanup_t goto_into_cleanup_make(void);
goto_into_cleanup_t goto_into_cleanup_copy(const goto_into_cleanup_t *const self);
goto_into_cleanup_t goto_into_cleanup_move(goto_into_cleanup_t *const self);
void goto_into_cleanup_destroy(goto_into_cleanup_t *const self);
goto_into_cleanup_t goto_into_cleanup_return(goto_into_cleanup_t *self);
_Bool goto_into_cleanup_valid(const goto_into_cleanup_t *const self);

void goto_into_cleanup_destroy(goto_into_cleanup_t *const self)
{
    if (self->moved)
        goto cleanup; // good: jumps to the destroys, not past them

    log_text("destroying");

cleanup:
    int_vector_destroy(&self->second); // good
    int_vector_destroy(&self->first); // good
}

struct with_array
{
    int_vector_t items[3];
    int_vector_t last;
};
typedef struct with_array with_array_t;
with_array_t with_array_make(void);
with_array_t with_array_copy(const with_array_t *const self);
with_array_t with_array_move(with_array_t *const self);
void with_array_destroy(with_array_t *const self);
with_array_t with_array_return(with_array_t *self);
_Bool with_array_valid(const with_array_t *const self);

void with_array_destroy(with_array_t *const self)
{
    int_vector_destroy(&self->last); // good
    int_vector_destroy_array(self->items, 3); // good: an array field with the array destroy function
}

struct without_raii_fields
{
    pos_t position;
    free_struct_t free;
    int_vector_t *borrowed;
};
typedef struct without_raii_fields without_raii_fields_t;
without_raii_fields_t without_raii_fields_make(void);
without_raii_fields_t without_raii_fields_copy(const without_raii_fields_t *const self);
without_raii_fields_t without_raii_fields_move(without_raii_fields_t *const self);
void without_raii_fields_destroy(without_raii_fields_t *const self);
without_raii_fields_t without_raii_fields_return(without_raii_fields_t *self);
_Bool without_raii_fields_valid(const without_raii_fields_t *const self);

void without_raii_fields_destroy(without_raii_fields_t *const self) // good: pod, free and pointer fields are not owned raii fields
{
    (void)self;
}

// -------------------------------------------------------------
// Bad
// -------------------------------------------------------------

struct nested_if
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct nested_if nested_if_t;
nested_if_t nested_if_make(void);
nested_if_t nested_if_copy(const nested_if_t *const self);
nested_if_t nested_if_move(nested_if_t *const self);
void nested_if_destroy(nested_if_t *const self);
nested_if_t nested_if_return(nested_if_t *self);
_Bool nested_if_valid(const nested_if_t *const self);

void nested_if_destroy(nested_if_t *const self)
{
    if (self != NULL)
    {
        if (!self->moved)
        {
            int_vector_destroy(&self->second); // bad: inside a nested if
            int_vector_destroy(&self->first); // bad: inside a nested if
        }
    }
}

struct in_else
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct in_else in_else_t;
in_else_t in_else_make(void);
in_else_t in_else_copy(const in_else_t *const self);
in_else_t in_else_move(in_else_t *const self);
void in_else_destroy(in_else_t *const self);
in_else_t in_else_return(in_else_t *self);
_Bool in_else_valid(const in_else_t *const self);

void in_else_destroy(in_else_t *const self)
{
    if (self->moved)
    {
        log_text("already moved");
    }
    else
    {
        int_vector_destroy(&self->second); // bad: inside an else branch
        int_vector_destroy(&self->first); // bad: inside an else branch
    }
}

struct split
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct split split_t;
split_t split_make(void);
split_t split_copy(const split_t *const self);
split_t split_move(split_t *const self);
void split_destroy(split_t *const self);
split_t split_return(split_t *self);
_Bool split_valid(const split_t *const self);

void split_destroy(split_t *const self)
{
    int_vector_destroy(&self->second); // good: the first destroy decides the block

    if (self->buffer != NULL)
    {
        int_vector_destroy(&self->first); // bad: not in the same block as 'second'
    }
}

struct both_branches
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct both_branches both_branches_t;
both_branches_t both_branches_make(void);
both_branches_t both_branches_copy(const both_branches_t *const self);
both_branches_t both_branches_move(both_branches_t *const self);
void both_branches_destroy(both_branches_t *const self);
both_branches_t both_branches_return(both_branches_t *self);
_Bool both_branches_valid(const both_branches_t *const self);

void both_branches_destroy(both_branches_t *const self)
{
    if (self->moved)
    {
        int_vector_destroy(&self->second); // good
        int_vector_destroy(&self->first); // good
    }
    else
    {
        memory_free(self->buffer);
        int_vector_destroy(&self->second); // bad: 'second' has two destroy calls, move the destroys after the if
        int_vector_destroy(&self->first); // bad: 'first' has two destroy calls
    }
}

struct wrong_order
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct wrong_order wrong_order_t;
wrong_order_t wrong_order_make(void);
wrong_order_t wrong_order_copy(const wrong_order_t *const self);
wrong_order_t wrong_order_move(wrong_order_t *const self);
void wrong_order_destroy(wrong_order_t *const self);
wrong_order_t wrong_order_return(wrong_order_t *self);
_Bool wrong_order_valid(const wrong_order_t *const self);

void wrong_order_destroy(wrong_order_t *const self)
{
    int_vector_destroy(&self->first); // bad: 'second' was declared later, so it must be destroyed first
    int_vector_destroy(&self->second); // good
    int_vector_destroy(&self->second); // bad: a second destroy call for 'second'
}

struct missing
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct missing missing_t;
missing_t missing_make(void);
missing_t missing_copy(const missing_t *const self);
missing_t missing_move(missing_t *const self);
void missing_destroy(missing_t *const self);
missing_t missing_return(missing_t *self);
_Bool missing_valid(const missing_t *const self);

void missing_destroy(missing_t *const self) // bad: 'first' is never destroyed
{
    int_vector_destroy(&self->second); // good
    memory_free(self->buffer);
}

struct in_loop
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct in_loop in_loop_t;
in_loop_t in_loop_make(void);
in_loop_t in_loop_copy(const in_loop_t *const self);
in_loop_t in_loop_move(in_loop_t *const self);
void in_loop_destroy(in_loop_t *const self);
in_loop_t in_loop_return(in_loop_t *self);
_Bool in_loop_valid(const in_loop_t *const self);

void in_loop_destroy(in_loop_t *const self)
{
    for (int i = 0; i < 1; ++i)
        int_vector_destroy(&self->second); // bad: inside a loop

    switch (self->moved)
    {
        case 0:
            int_vector_destroy(&self->first); // bad: inside a switch
            break;
        default:
            break;
    }
}

struct in_block
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct in_block in_block_t;
in_block_t in_block_make(void);
in_block_t in_block_copy(const in_block_t *const self);
in_block_t in_block_move(in_block_t *const self);
void in_block_destroy(in_block_t *const self);
in_block_t in_block_return(in_block_t *self);
_Bool in_block_valid(const in_block_t *const self);

void in_block_destroy(in_block_t *const self)
{
    {
        int_vector_destroy(&self->second); // bad: inside a nested block
        int_vector_destroy(&self->first); // bad: inside a nested block
    }
}

struct early_return
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct early_return early_return_t;
early_return_t early_return_make(void);
early_return_t early_return_copy(const early_return_t *const self);
early_return_t early_return_move(early_return_t *const self);
void early_return_destroy(early_return_t *const self);
early_return_t early_return_return(early_return_t *self);
_Bool early_return_valid(const early_return_t *const self);

void early_return_destroy(early_return_t *const self)
{
    if (self == NULL)
        return; // good: an early return skips the struct as a whole

    int_vector_destroy(&self->second); // good
    int_vector_destroy(&self->first); // good
}

struct goto_past
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct goto_past goto_past_t;
goto_past_t goto_past_make(void);
goto_past_t goto_past_copy(const goto_past_t *const self);
goto_past_t goto_past_move(goto_past_t *const self);
void goto_past_destroy(goto_past_t *const self);
goto_past_t goto_past_return(goto_past_t *self);
_Bool goto_past_valid(const goto_past_t *const self);

void goto_past_destroy(goto_past_t *const self)
{
    if (self->moved)
        goto done; // good: jumps past all the destroys, the struct as a whole

    int_vector_destroy(&self->second); // good
    int_vector_destroy(&self->first); // good

done:
    log_text("done");
}

struct return_in_if
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct return_in_if return_in_if_t;
return_in_if_t return_in_if_make(void);
return_in_if_t return_in_if_copy(const return_in_if_t *const self);
return_in_if_t return_in_if_move(return_in_if_t *const self);
void return_in_if_destroy(return_in_if_t *const self);
return_in_if_t return_in_if_return(return_in_if_t *self);
_Bool return_in_if_valid(const return_in_if_t *const self);

void return_in_if_destroy(return_in_if_t *const self)
{
    if (self != NULL)
    {
        if (self->moved)
            return; // good: an early return inside the if block too

        int_vector_destroy(&self->second); // good
        int_vector_destroy(&self->first); // good
    }
}

struct return_between
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct return_between return_between_t;
return_between_t return_between_make(void);
return_between_t return_between_copy(const return_between_t *const self);
return_between_t return_between_move(return_between_t *const self);
void return_between_destroy(return_between_t *const self);
return_between_t return_between_return(return_between_t *self);
_Bool return_between_valid(const return_between_t *const self);

void return_between_destroy(return_between_t *const self)
{
    int_vector_destroy(&self->second); // good

    if (self->moved)
        return; // bad: 'first' would be left alive

    int_vector_destroy(&self->first); // good
}

struct goto_between
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct goto_between goto_between_t;
goto_between_t goto_between_make(void);
goto_between_t goto_between_copy(const goto_between_t *const self);
goto_between_t goto_between_move(goto_between_t *const self);
void goto_between_destroy(goto_between_t *const self);
goto_between_t goto_between_return(goto_between_t *self);
_Bool goto_between_valid(const goto_between_t *const self);

void goto_between_destroy(goto_between_t *const self)
{
    int_vector_destroy(&self->second); // good

    if (self->moved)
        goto done; // bad: jumps past the destroy of 'first'

    int_vector_destroy(&self->first); // good

done:
    log_text("done");
}

struct label_between
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct label_between label_between_t;
label_between_t label_between_make(void);
label_between_t label_between_copy(const label_between_t *const self);
label_between_t label_between_move(label_between_t *const self);
void label_between_destroy(label_between_t *const self);
label_between_t label_between_return(label_between_t *self);
_Bool label_between_valid(const label_between_t *const self);

void label_between_destroy(label_between_t *const self)
{
    if (self->moved)
        goto half;

    int_vector_destroy(&self->second); // good

half: // bad: a goto here skips the destroy of 'second'
    int_vector_destroy(&self->first); // good
}

struct not_self
{
    int_vector_t first;
    char *buffer;
    int_vector_t second;
    _Bool moved;
};
typedef struct not_self not_self_t;
not_self_t not_self_make(void);
not_self_t not_self_copy(const not_self_t *const self);
not_self_t not_self_move(not_self_t *const self);
void not_self_destroy(not_self_t *const self);
not_self_t not_self_return(not_self_t *self);
_Bool not_self_valid(const not_self_t *const self);

in_body_t *g_other;

void not_self_destroy(not_self_t *const self) // bad: 'second' is never destroyed on self
{
    int_vector_destroy(&g_other->second); // does not count: not on self
    int_vector_destroy(&self->first); // good
}
