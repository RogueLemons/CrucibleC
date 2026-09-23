#ifndef TESTS_HEADERS_PRIVATE_TAG_H
#define TESTS_HEADERS_PRIVATE_TAG_H

#ifdef WORKSHOPC_PARSING

#define PRIVATE __attribute__((annotate("workshopc_private_field")))

#else

#define PRIVATE

#endif

#endif // TESTS_HEADERS_PRIVATE_TAG_H