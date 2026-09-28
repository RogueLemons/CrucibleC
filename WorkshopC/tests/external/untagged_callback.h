#ifndef TESTS_EXTERNAL_UNTAGGED_CALLBACK_H
#define TESTS_EXTERNAL_UNTAGGED_CALLBACK_H

// A third party library knows nothing about the tags

struct third_party_item;

typedef void (*third_party_callback)(struct third_party_item* item);

void third_party_register(third_party_callback callback);

#endif // TESTS_EXTERNAL_UNTAGGED_CALLBACK_H
