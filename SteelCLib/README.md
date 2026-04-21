# SteelCLib
An upcoming project and expansion of IronCLib that offers more flexibility at a complexity cost.

## TODO
- Add IC_TYPENUM_FULL_HEADER and IC_TYPENUM_FULL_SOURCE macros that allow users to static const and static inline in their header or function defintions and extern varibles
- Consider adding a macro tag for IC_TYPENUM that converts everything to a simple typedef of the inner type
- Add optional system to opaque storage that can be turned on and off with a macro tag, that includes IC_OPAQUE_LOAD and IC_OPAQUE_STORE that handles aliasing safety through hard-copying internal bytes, but will without the tag just to fast pointer casting
- Add debug mode that uses runtime assert that can be turned off with macro tag (e.g. for accessing Result types)
- Consider removing result accessors (e.g. IC_RESULT_VALUE) and replace with functions for const safety (maybe overkill? Could include asserts, probably do this for SteelCLib instead)
- Expand typenum for SteelCLib to take a manually assigned comparitor for compatability with all inner types
- For SteelCLib add macro tag that performs static assert on size of all result types for users to guarantee size of return values?
- For SteelCLib add VoidResult macros
- For SteelCLib, only care about C99 and forward, and add bounded for loop
- Add memory alloc and span helpers? Add easy and safe zero init?
- Make typenum generated functions use pointers (only if starting to allow non-integer internal types, maybe for SteelC)?
- Add tests that can be verified on multiple compilers
- Rename project to IronC (because it is rigid and not using it can cause code to break) with SteelC as name of expanded version (more flexible), and then call the parser WorkshopC because it helps create strong-like-metal C 