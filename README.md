# EasyC (Prototype)
Ease-of-life features added in a simple proof-of-concept superset-transpiler to C. It includes
- keyword **typestruct**: automatically creates typedef of struct
- keyword **indef**: to write define statements inside of functions to keep local values local
- keyword **prefix**: working similar to C++ namespace to avoid long struct and function names
- keyword **mut**: working as an inverted const, where everything is const by default unless mut is used
- keyword **safe**: if type is safe then EC__CHECK__NULL(x) automatically called on next line (macro with default behavior, can be overridden)
- keyword **typenum**: a typesafe enum with a struct under hood with macro definitions for values, if type has option = 5 then access with type::option, type::count and type::get added for bonus help, uses type::equals instead of ==
- keyword **cleanpop**: variable initialized with cleanpop automatically calls type::populate(t) on next line, and before scope exits (and before return statements) calls type::cleanup(t), must be defined as functions or macros manually

EasyC sources files end with ".ec" and EasyC header files end with ".eh".

# Proof of concept
As a prototype this mini-project will never be perfect. It is a proof of concept meant to show what C could look like and what can be done with a transpiler that only has to work on a file-by-file basis. A real implementation would require much more rigorous C code parsing and proper pretty-print. 

It started with the idea "What if C variables were const by default?". Since the project is just a proof of concept it serves more as a way to talk about what code safety is and means in C, and what practices can be applied to write safe C code in a standardize style for the whole group. 

"Why use this instead of C++?" I hear you ask. C++ already exists. Nim already exists. C3 already exists. There are better tools and solutions than EasyC out there already. However, C programmers are often really happy about C specifically so converting them to a new language, even if it was deemed better (by whatever metric), is going to be difficult. But letting C coders continue to write C code but with just a few added keywords is an easier sell. It is also not just a people question; it is about compatability. Not all processors come with compilers for C++ or whatever language you might prefer, and gcc might have amazing added features to the language which might not be supported by other compilers a group moves to, so it becomes a matter of portability. Furthermore, with this transpiler the goal becomes to help write safe and readable code, both in the EasyC files and their transpiled C files, so it becomes a way to standardize how the code should look like and avoid easy-to-make mistakes. 

# TODO
- invert keyword **safe** so everything is safe by default and make user use keyword **nullable** for pointers that may be null
- keyword **typenum**: add ability to assign internal type e.g. typenum(char) Status { OK = 0, Error = 1 };
- keyword **cleanpop**: give it ability to take arguments

# Bugs
As a prototype this mini-project will never be perfect, it is a proof of concept. But less acceptable bugs include
- cannot use * without whitespace after unless dereferencing (* some_ptr not ok) or multiplying (a * b not ok)
- cannot typedef and define struct in same statement
- mut/const management cannot see function arg types and won't adjust const correctness if type does not exist in file in other format (e.g. as a normal variable)
