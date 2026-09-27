#ifndef CRUCIBLEC_WORKSHOPC_REFERENCE_TAG_H
#define CRUCIBLEC_WORKSHOPC_REFERENCE_TAG_H

#ifdef WORKSHOPC_PARSING

#define REF __attribute__((annotate("workshopc_reference_pointer")))

#else

#define REF

#endif

#endif // CRUCIBLEC_WORKSHOPC_REFERENCE_TAG_H