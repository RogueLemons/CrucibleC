#include <stddef.h>

#include "headers/move_tags.h"
#include "headers/ref_tag.h"
#include "external/untagged_callback.h"

struct data
{
    int value;
};
typedef struct data data;

// -------------------------------------------------------------
// The tags of a function pointer are written on the parameters
// of its type
// -------------------------------------------------------------

typedef void (*consumer_t)(move data* item); // good
typedef void (*borrower_t)(mod data* item); // good: the same signature as consumer_t, other tags
typedef void (*editor_t)(mod data* item, int value); // good
typedef void (*reader_t)(REF const data* item); // good
typedef void (*untagged_t)(data* item); // bad: a non-const pointer parameter needs a movement tag

void create(out data** item);
void consume(move data* item);
void borrow(mod data* item);
void edit(mod data* item, int value);
void read(REF const data* item);
void read_nullable(const data* item);

// -------------------------------------------------------------
// A function assigned or passed to a function pointer must have
// the same tags, parameter by parameter
// -------------------------------------------------------------

consumer_t global_consumer = consume; // good: the same tags
consumer_t global_wrong_consumer = borrow; // bad: modify instead of move

void initialization(void)
{
    consumer_t consumer = consume; // good: the same tags
    consumer_t through_address = &consume; // good
    consumer_t wrong = borrow; // bad: modify instead of move
    consumer_t cast = (consumer_t)borrow; // bad: a cast does not hide the mismatch
    editor_t editor = edit; // good
    reader_t reader = read; // good: both are references
    reader_t nullable = read_nullable; // bad: not a reference, but reader_t has one
    consumer_t table[2] = { consume, borrow }; // bad: the second element is modify instead of move
    consumer_t copy = consumer; // good: the same function pointer type
    borrower_t borrower = consumer; // bad: a function pointer with other tags
    void (*raw)(move data* item) = consume; // good: tags on a function pointer written without a typedef
    void (*raw_wrong)(move data* item) = borrow; // bad: modify instead of move
}

void assignment(int flag)
{
    consumer_t consumer = consume; // good
    consumer = consume; // good
    consumer = borrow; // bad: modify instead of move
    consumer = flag ? consume : borrow; // bad: the second branch is modify instead of move
}

struct handlers
{
    consumer_t on_consume;
    reader_t on_read;
};
typedef struct handlers handlers;

void struct_fields(void)
{
    handlers good = { consume, read }; // good
    handlers named = { .on_consume = consume, .on_read = read_nullable }; // bad: on_read needs a reference
    good.on_consume = borrow; // bad: modify instead of move
}

void register_consumer(consumer_t consumer);

void arguments(void)
{
    register_consumer(consume); // good
    register_consumer(borrow); // bad: modify instead of move
}

consumer_t get_consumer(void)
{
    return consume; // good
}

consumer_t get_wrong_consumer(void)
{
    return borrow; // bad: modify instead of move
}

void on_third_party_item(move struct third_party_item* item);

void third_party(void)
{
    third_party_register(on_third_party_item); // good: a type from a third party header has no tags to match
}

// -------------------------------------------------------------
// Calls through a function pointer follow the tags of its type
// -------------------------------------------------------------

void calls(consumer_t consumer, editor_t editor, reader_t reader)
{
    data local = { 0 };
    data* item = NULL;
    create(out_cast(&item));

    reader(&local); // good: the address of an object for a reference
    reader(item); // bad: a normal pointer may be null
    editor(mod_cast(item), 1); // good: the operator of a modify parameter of editor_t
    editor(item, 2); // bad: missing mod_cast
    editor(move_cast(item), 3); // bad: the wrong operator for a modify parameter
    consumer(move_cast(item)); // good: ownership moves through the function pointer
    editor(mod_cast(item), 4); // bad: item was moved to consumer
}

void calls_through_fields(const handlers* callbacks)
{
    data* item = NULL;
    create(out_cast(&item));

    if (callbacks)
        callbacks->on_consume(item); // bad: missing move_cast for on_consume
}

void pass_on_borrowed(mod data* item, consumer_t consumer)
{
    consumer(move_cast(item)); // bad: a modify parameter is only borrowed, it can not be moved
}

// -------------------------------------------------------------
// Compound literals are initializers too
// -------------------------------------------------------------

void register_handlers(const handlers* callbacks);

void compound_literals(void)
{
    register_handlers(&(handlers){ consume, read }); // good
    register_handlers(&(handlers){ .on_consume = borrow, .on_read = read }); // bad: modify instead of move
    handlers assigned = { consume, read }; // good
    assigned = (handlers){ consume, read_nullable }; // bad: on_read needs a reference
    consumer_t braced = (consumer_t){ borrow }; // bad: modify instead of move
}

// -------------------------------------------------------------
// A call through a conditional expression follows the tags its
// branches agree on
// -------------------------------------------------------------

void conditional_calls(int flag, consumer_t first, consumer_t second, borrower_t borrower)
{
    data* item = NULL;
    create(out_cast(&item));
    (flag ? first : second)(move_cast(item)); // good: both branches tag it move

    data* other = NULL;
    create(out_cast(&other));
    (flag ? first : second)(other); // bad: missing move_cast, both branches tag it move

    data* third = NULL;
    create(out_cast(&third));
    (flag ? first : borrower)(move_cast(third)); // bad: the branches tag the parameter differently
}
