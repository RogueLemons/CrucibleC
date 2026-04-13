# EasyC Lib
Write safe C code more easily. 

Contains a header library and a code parser (TODO) to provide warnings.

## Header Library
A plug-and-play header library that must only be added to your include folder in your build system. 

### ec.h
### ec_static_assert.h
### ec_inline.h
### ec_typenum.h
### ec_opaque_storage.h
### ec_result.h

# TODO

## Lib
- Document the headers (what and why) and give examples on how to use

## Parser
Parser must be implemented to transfer goals of EasyCTranspiler into a warning/suggestion system for pure C code.

It shall
- Tokenize code properly and use no naive regex tricks.
- Verify const correctness of variables (possibly ignoring variables used in functions as first iteration).
- Make all mutable pointer function arguments start their names with out_ for maximum clarity (or move_ to show a transfer of ownership).
- Warn against using enum.
- Look at file path and if file path is included in names (e.g. function or struct) then suggest splitting by __ (optionally user defined) to mimic namespaces and improve readability, also warn if name contains e.g. 7 underscores.
- Verify all structs immediately contain a typedef statement on next line.
- Verify that #define statements appear right above the functions they are used in if only used once in file (if in header can be used 0 times).
- Forbid null pointers and uninitialized pointers, optionally enforce all pointers to be null-checked
- Verify all structs are initialized with either a _populate or _init function
- Verify if _init is used all exit paths must include _cleanup
- Verify variables are not called with _populate or _init multiple times in same scope
- Be able to turn off warnings in-file by writing "// EasyC off" and turn back on with "// EasyC on"
- Look for EasyCSettings.csv file and use its settings (default if not exist)
- Number of warnings be return value of script/app main function.