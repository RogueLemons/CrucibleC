# EasyC (Prototype)
Ease-of-life features added in a simple superset to C

Includes
- keyword typestruct: automatically creates typedef of struct
- keyword indef: to write define statements inside of functions to keep local values local
- keyword prefix: working similar to C++ namespace to avoid long struct and function names
- keyword mut: working as an inverted const, where everything is const by default unless mut is used

# TODO
- keyword ref: ref type is assigned with type*, dereferencing handled automatically, CHECK__REF(x) automatically called on next line (macro with default behavior, can be overridden)
- keyword cleanpop: variable initialized with cleanpop automatically calls POPULATE__type(t) on next line and before scope exits (and before return statements) calls CLEANUP__type(t), must be defined as functions or macros manually
- keyword typenum: a typesafe enum, struct type { int value; }; under hood with macro definitions for values, if type has option = 5 then type::option is just macro defined type__option (const type)({ .value = 5}), type::count added for bonus help

# Bugs
As a prototype this mini-project will never be perfect, it is a proof of concept. But less acceptable bugs include
- cannot use * without whitespace after unless dereferencing (* some_ptr not ok) or multiplying (a * b not ok)
- cannot typedef and define struct in same statement