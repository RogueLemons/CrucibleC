# Introductory example with uppercase tags

```c
#include <stddef.h>
#include "workshopc_tags.h"

typedef struct inbox inbox;
typedef struct message message;

void message_create(OUTPUT message** result);             // writes a new message to *result
void inbox_push(MUTABLE inbox* box, MOVED message* item); // takes ownership of item
void message_print(const message* item);                  // only reads item

void deliver(MUTABLE inbox* box)
{
    message* hello = NULL;
    message_create(OUT(&hello));    // the call site shows that hello is written
    inbox_push(box, MOVE(hello));   // ...and that ownership moves to the inbox
    message_print(hello);           // Reported: hello was given away
}
```

[Back to README](../../README.md)