#ifndef CRUCIBLEC_WORKSHOPC_REFERENCE_TAG_LOWER_H
#define CRUCIBLEC_WORKSHOPC_REFERENCE_TAG_LOWER_H

// A massive pointer is never an empty shell: there is always a real object
// inside it, taken directly from a variable, field or array element, and
// never through another pointer, so it can never be null.
//
//   int read(massive const item* value);      read(&local);
//
// A macro replaces its name everywhere after this header, so include it
// after system and third party headers.

#ifdef WORKSHOPC_PARSING

#define massive __attribute__((annotate("workshopc_reference_pointer")))

#else

#define massive

#endif

#endif // CRUCIBLEC_WORKSHOPC_REFERENCE_TAG_LOWER_H
