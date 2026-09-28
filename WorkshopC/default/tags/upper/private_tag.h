#ifndef CRUCIBLEC_WORKSHOPC_PRIVATE_TAG_H
#define CRUCIBLEC_WORKSHOPC_PRIVATE_TAG_H

#ifdef WORKSHOPC_PARSING

#define PRIVATE __attribute__((annotate("workshopc_private_field")))

#else

#define PRIVATE

#endif

#endif // CRUCIBLEC_WORKSHOPC_PRIVATE_TAG_H