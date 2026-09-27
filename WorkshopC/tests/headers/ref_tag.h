#ifndef TESTS_HEADERS_REF_TAG_H
#define TESTS_HEADERS_REF_TAG_H

#ifdef WORKSHOPC_PARSING

#define REF __attribute__((annotate("workshopc_reference_pointer")))

#else

#define REF

#endif

#endif // TESTS_HEADERS_REF_TAG_H