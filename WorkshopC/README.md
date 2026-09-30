# WorkshopC
A configurable analyzer that enforces safer C: RAII-style structs, explicit pointer ownership, null checks, and strict style rules.

## Contents
- [How to use](#how-to-use)
  - [Tag headers](#tag-headers)
- [Diagnostic codes](#diagnostic-codes)
- [Config behavior](#config-behavior)
  - [Enum rule](#enum-rule)
  - [Private rule](#private-rule)
  - [Private alternative rule](#private-alternative-rule)
  - [Function pointer rule](#function-pointer-rule)
  - [Typedef struct rule](#typedef-struct-rule)
  - [Assignment rule](#assignment-rule)
  - [Prefix namespace rule](#prefix-namespace-rule)
  - [Null check rule](#null-check-rule)
  - [Argument pointer movement rule](#argument-pointer-movement-rule)
  - [RAII and struct resource management](#raii-and-struct-resource-management)
  - [Interfaces rule](#interfaces-rule)
  - [Restricted malloc rule](#restricted-malloc-rule)
  - [Single return rule](#single-return-rule)
  - [Strict switch rule](#strict-switch-rule)
  - [Global variable rule](#global-variable-rule)
  - [Reference pointer rule](#reference-pointer-rule)
  - [Function discard rule](#function-discard-rule)
  - [Array struct rule](#array-struct-rule)
  - [Span struct rule](#span-struct-rule)
  - [Const field rule](#const-field-rule)
  - [No goto rule](#no-goto-rule)
  - [Disable section](#disable-section)
  - [Adjust code for parser](#adjust-code-for-parser)
- [WorkshopC Build System Documentation](#workshopc-build-system-documentation)
  - [Overview](#overview)
  - [Windows Developer Workflow (Python Script)](#windows-developer-workflow-python-script)
  - [Cross-Platform CMake Workflow (Official Build System)](#cross-platform-cmake-workflow-official-build-system)
  - [Summary](#summary)
- [TODO](#todo)

## How to use
```bash
workshopc [options] <files or folders...>
```

Put a `workshopc.yaml` in the root of your project with `compile_commands_dir` pointing at the folder containing `compile_commands.json` (typically the CMake build folder), and then all that is needed is:

```bash
workshopc src/
```

**Arguments:**
- `<files or folders...>` — The C files to analyze. Folders are searched recursively for `.c` files, skipping `third_party_includes` folders and the compilation database folder. Headers are checked through the files that include them, and a problem in a header included by several files is only reported once.
- `--config <file>` — The config file. Without it, `workshopc.yaml` is searched for in the current folder and then its parents. Other configs are named `<name>.workshopc.yaml` by convention, e.g. `ci.workshopc.yaml`, and given with this option.
- `-p, --build-path <folder>` — The folder containing `compile_commands.json`. Overrides `compile_commands_dir` from the config.
- `--text <file>` — Also write the diagnostics as text to a file, the same lines as printed to the terminal.
- `--json <file>` — Also write the diagnostics as JSON to a file: the warning and error counts and a list of diagnostics, each with its file, line, column, level, code, name and message.
- `--sarif <file>` — Also write the diagnostics as SARIF 2.1.0 to a file, the standard format read by e.g. GitHub code scanning and many IDEs.
- `-q, --quiet` — Print nothing to the terminal: no diagnostics and no warning and error summary. Clang's compile errors and problems that stop the analysis (e.g. a missing config) are still printed, and the exit code is unaffected.
- `--warnings-as-errors` — Report every warning as an error: on the terminal, in the `--text`, `--json` and `--sarif` files, in the counts, and in the exit code. The config still decides which rules run and how serious they normally are, and the flag decides how strict one run is. One config can then serve both a relaxed local run and a strict CI run.
- `--dump-config` — Print the config as it was read, with every setting, and exit without analyzing anything, so no files or folders are needed. The output is a valid config, e.g. `workshopc --dump-config > full.workshopc.yaml` gives a config with every setting written out. It is printed even when the config is invalid, to see how it was read. Empty string settings are left out, since they are empty when not given.
- `-h, --help` — Show the help.

The diagnostics are always printed to the terminal (stderr) unless `--quiet` is given, and `--text`, `--json` and `--sarif` can be combined freely to write any set of files in a single run. Give `-` as the file to write that format to stdout instead (only one format can use stdout). Compile errors from clang are always printed as text and never appear in the files, the exit code tells when they happened.

Before the analysis, the config file and the build folder that are used are printed (unless `--quiet` is given), e.g. `Config: /repo/workshopc.yaml` and `Build folder: /repo/build (compile_commands_dir in the config)`, so a CI log shows which files a run used.

Each diagnostic is printed as `file:line:column: level: message [code]`, see [Diagnostic codes](#diagnostic-codes).

Examples:
```bash
workshopc src/                                  # everything under src/, config found automatically
workshopc src/main.c src/parser.c               # only these files
workshopc --config ci.workshopc.yaml -p out/ src/  # another config and build folder
workshopc -q --sarif results.sarif src/         # only a SARIF file, e.g. for CI
workshopc --text out.txt --json out.json src/   # terminal output plus a text and a JSON file
workshopc -q --json - src/ | jq .errors         # JSON to stdout, for piping
workshopc --warnings-as-errors src/             # fail on any finding, e.g. in CI
workshopc --dump-config                         # the config found, with every setting
```

**Exit codes:**

The exit code reports the result of the analysis. Codes 0-3 form a bitmask (`1` = errors found, `2` = warnings found), so the tool can be used directly in scripts and CI:

| Code | Meaning |
|------|---------|
| `0` | Clean, no warnings or errors |
| `1` | Errors found |
| `2` | Warnings found, no errors |
| `3` | Both errors and warnings found |
| `64` | Bad usage (unknown option, no files given, a file or folder that does not exist, no `.c` files found) |
| `65` | The config file could not be loaded |
| `66` | The compilation database could not be loaded |
| `67` | Clang failed to process the source file (e.g. it does not compile), so the analysis result is not reliable |

Codes of `64` and above always mean the tool itself could not complete the analysis, so they can never be confused with rule results. Note that most build systems treat any non-zero exit code as a failure, so a run with warnings only (`2`) will fail a script using `set -e` unless the caller handles it. With `--warnings-as-errors` there are no warnings, so the code is `0` or `1`.

**Compiler output:**

The tool runs the file through clang, but only WorkshopC's own rules produce warnings. Clang's warnings (unused variables, implicit conversions and so on) are disabled with `-w`, even if the compilation database enables `-Wall` or `-Werror`, since they belong to the project's normal build. Genuine compile errors are still printed in clang's normal format, and the exit code is then `67`. The rules still run on whatever clang could recover from the broken file, but the result should be treated as incomplete until the file compiles.

[Here is a premade config ready for use as is and provide a base to easily edit](./default/default.workshopc.yaml). Copy it to the root of your project as `workshopc.yaml` to have it found automatically.

### Tag headers
Some rules use tags, macros written in the code, e.g. to mark who owns a pointer. Premade headers for them come in two naming styles, with the same file names in both folders, so the style is chosen by the include path alone (e.g. `-I workshopc/tags/lower`):

| Header | [`default/tags/upper`](./default/tags/upper) | [`default/tags/lower`](./default/tags/lower) |
|--------|-------|-------|
| `move_tags.h` ([argument pointer movement](#argument-pointer-movement-rule)) | `MOVED`, `OUTPUT`, `MUTABLE`, `MOVE()`, `OUT()`, `MUT()` | `receives`, `initializes`, `borrows`, `give()`, `overwrite()`, `lend()` |
| `private_tag.h` ([private alternative](#private-alternative-rule)) | `PRIVATE` | `confined` |
| `ref_tag.h` ([reference pointer](#reference-pointer-rule)) | `REF` | `massive` |
| `workshopc_tags.h` | all of the above | all of the above |

The lowercase names describe what each side of a call does with a pointer, so a declaration and its call read like sentences:

```c
void consume(receives item* value);         consume(give(value));         // ownership moves to consume
void create(initializes item** result);      create(overwrite(&value));    // create writes value, the old one is replaced
void edit(borrows item* value);              edit(lend(value));            // edit changes value, the caller keeps it
int read(massive const item* value);         read(&local);                 // value is never null
struct item { confined int secret; };                                      // only item's own functions touch secret
```

A pointer is like a shell around what it points to: a `massive` pointer is never an empty shell, there is always a real object inside it, taken directly from a variable, a field or an array element and never through another pointer. A `confined` field is confined to the functions of its own struct.

A macro replaces its name everywhere after it is defined, also in the headers included after it, so include the tag headers after system and third party headers, and do not use the tag names for anything else. The lowercase names are chosen to be rare, but the uppercase ones are the safest. The headers are only examples, the names can be changed freely, see [naming the tags and operators](#naming-the-tags-and-operators).

## Diagnostic codes
Every diagnostic ends with a code, e.g. `[CCW0101]`, which identifies exactly which check reported it. The JSON and SARIF files carry the code and a readable name as separate fields (`code` and `name` in JSON, `ruleId` in SARIF, whose rule list describes every code).

The codes are `CCWrrcc`: `CCW` for CrucibleC WorkshopC, `rr` the rule (numbered in the order of the default config, with `00` for WorkshopC itself and `14`-`15` reserved for future rules) and `cc` the check within that rule, starting at `01`. The code says nothing about the severity, since every rule's level is set in the config. Codes are never renumbered or reused: a removed check leaves its code unused, and a new check gets the next free number of its rule.

| Code | Name | Reported when |
|------|------|---------------|
| | **00 — [WorkshopC](#disable-section)** | |
| `CCW0001` | `suppression-not-turned-back-on` | 'WorkshopC off' is never turned back on with 'WorkshopC on' in the same file |
| `CCW0002` | `suppression-missing-reason` | 'WorkshopC off' is not followed on the next line by a comment starting with 'Reason: ' |
| `CCW0003` | `suppression-on-without-off` | 'WorkshopC on' without a preceding 'WorkshopC off' in the same file |
| `CCW0004` | `suppression-nested` | 'WorkshopC off' while already turned off, suppressions can not be nested |
| | **01 — [Enum](#enum-rule)** | |
| `CCW0101` | `enum-not-allowed` | Enums are not allowed |
| `CCW0102` | `enum-missing-typedef` | An enum must have a typedef |
| `CCW0103` | `enum-init-not-member` | An enum variable must be initialized with a member of its enum or an explicit cast |
| `CCW0104` | `enum-assignment-not-member` | An enum value must be assigned a member of its enum or an explicit cast |
| `CCW0105` | `enum-arithmetic` | An enum value may not be modified with arithmetic (compound assignment, ++ or --) |
| `CCW0106` | `enum-argument-not-member` | An enum argument must be a member of its enum or an explicit cast |
| | **02 — [Private](#private-rule)** | |
| `CCW0201` | `private-access-outside-function` | A private field is accessed outside of any function |
| `CCW0202` | `private-access` | A private field is accessed from a function that is not a static getter or setter in a .c file |
| | **03 — [Private alternative](#private-alternative-rule)** | |
| `CCW0301` | `private-alternative-access-outside-function` | A private field is accessed outside of any function |
| `CCW0302` | `private-alternative-access` | A private field is accessed from a function that is not named after its struct and does not take 'self' |
| | **04 — [Function pointer](#function-pointer-rule)** | |
| `CCW0401` | `function-pointer-missing-typedef` | A function pointer variable, parameter, struct field or return type is declared without a typedef |
| | **05 — [Typedef struct](#typedef-struct-rule)** | |
| `CCW0501` | `struct-missing-typedef` | A struct must have a typedef |
| | **06 — [Assignment](#assignment-rule)** | |
| `CCW0601` | `variable-uninitialized` | A variable must be initialized at declaration |
| `CCW0602` | `array-uninitialized` | An array must be initialized at declaration |
| `CCW0603` | `pointer-object-zero-initialized` | An object containing pointers may not be initialized with {0} |
| `CCW0604` | `null-pointer-field-initializer` | NULL is used in the initializer of a pointer field |
| `CCW0605` | `null-pointer-array-initializer` | NULL is used in the initializer of an array of pointers |
| `CCW0606` | `pointer-array-partially-initialized` | An array of pointers must explicitly initialize every element |
| `CCW0607` | `mutable-argument-pointer` | The address of an argument is taken as a pointer to non-const |
| `CCW0608` | `pointer-assigned-null` | A pointer is assigned or initialized with NULL |
| `CCW0609` | `pointer-field-assigned-null` | A pointer field is assigned NULL |
| `CCW0610` | `argument-reassigned` | A function argument is reassigned, or changed with ++ or -- |
| `CCW0611` | `by-value-argument-modified` | A field of a by-value argument is modified |
| `CCW0612` | `null-argument` | NULL is passed as an argument |
| | **07 — [Prefix namespace](#prefix-namespace-rule)** | |
| `CCW0701` | `missing-namespace-prefix` | A name does not start with the namespace prefix of its file |
| `CCW0702` | `missing-include-guard` | A header does not have the expected include guard |
| | **08 — [Null check](#null-check-rule)** | |
| `CCW0801` | `dereference-before-null-check` | A pointer parameter is dereferenced before it is checked for null |
| | **09 — [Argument pointer movement](#argument-pointer-movement-rule)** | |
| `CCW0901` | `movement-tag-missing` | A non-const pointer parameter has no movement attribute |
| `CCW0902` | `movement-tag-mismatch` | The movement attribute of a parameter differs between declaration and definition |
| `CCW0903` | `movement-tag-not-on-parameter` | A movement attribute is used on something other than a parameter of a function or function pointer type |
| `CCW0904` | `borrowed-pointer-moved` | A modify or out parameter is moved to another function |
| `CCW0905` | `operator-on-untagged-parameter` | A callsite operator is used for a parameter without a movement attribute |
| `CCW0906` | `operator-disabled` | A callsite operator is used while that kind of callsite operator is disabled |
| `CCW0907` | `operator-missing` | A callsite operator is missing for a parameter with a movement attribute |
| `CCW0908` | `use-after-move` | A pointer is used after it may have been moved |
| `CCW0909` | `function-pointer-movement-tag-mismatch` | A function or function pointer with other movement tags is assigned or passed to a function pointer, or called with it through a conditional |
| | **10 — [Struct resource management: struct definitions](#raii-and-struct-resource-management)** | |
| `CCW1001` | `struct-invalid-constructor` | A struct does not have exactly one pod, raii or free constructor function |
| `CCW1002` | `struct-missing-destroy` | A raii struct is missing its destroy function |
| `CCW1003` | `struct-missing-copy` | A raii struct is missing its copy function |
| `CCW1004` | `struct-missing-move` | A raii struct is missing its move function |
| `CCW1005` | `struct-missing-return` | A raii struct is missing its return function |
| `CCW1006` | `struct-missing-valid` | A raii struct is missing its validation function |
| | **11 — [Struct resource management: initialization and assignment](#raii-and-struct-resource-management)** | |
| `CCW1101` | `pod-init` | A pod struct variable is not initialized from a function return value or another struct variable |
| `CCW1102` | `raii-init` | A raii struct variable is not initialized from a function return value |
| `CCW1103` | `pod-array-init` | A pod struct array does not initialize every element from a function return value or another struct variable |
| `CCW1104` | `raii-array-multidimensional` | A raii struct array outside of a struct has more than one dimension |
| `CCW1105` | `raii-array-init` | A raii struct array does not initialize every element from a function return value |
| `CCW1106` | `raii-array-missing-destroy-array` | A raii struct array is used but the struct has no array destroy function |
| `CCW1107` | `struct-array-not-allowed` | An array of this kind of struct is only allowed inside structs |
| `CCW1108` | `raii-reassigned` | A raii struct is reassigned with another struct value |
| `CCW1109` | `pod-argument` | A pod struct argument is not a function return value or another struct variable |
| `CCW1110` | `raii-argument` | A raii struct argument is not a function return value |
| `CCW1111` | `pod-field-not-pod` | A struct field inside a pod struct is not a pod struct |
| `CCW1112` | `move-not-value-ref` | A raii move function is given something other than the address of a variable |
| | **12 — [Struct resource management: destruction](#raii-and-struct-resource-management)** | |
| `CCW1201` | `raii-not-destroyed` | A raii struct variable or array is not destroyed before scope exit |
| `CCW1202` | `raii-parameter-not-destroyed` | A raii struct parameter is not destroyed before scope exit |
| `CCW1203` | `raii-use-after-destroy` | A raii struct, or a span viewing an array of them, is used after being destroyed |
| `CCW1204` | `destroy-not-value-ref` | A raii destroy function is given something other than the address of a variable |
| `CCW1205` | `destroy-array-first-argument` | The first argument of an array destroy function is not the array itself |
| `CCW1206` | `destroy-array-size` | The second argument of an array destroy function is not the size of the array |
| `CCW1207` | `raii-destroy-not-reverse-order` | A raii struct is destroyed before a later-declared raii struct |
| `CCW1208` | `raii-field-not-destroyed` | A raii field is never destroyed in the destroy function of its struct |
| `CCW1209` | `raii-field-destroy-misplaced` | A raii field is not destroyed directly in the destroy function body or directly inside an if statement there, together with the other raii fields |
| `CCW1210` | `raii-field-multiple-destroy-calls` | A raii field has multiple destroy calls in the destroy function of its struct |
| `CCW1211` | `raii-field-destroys-interrupted` | A return, goto or label between the raii field destroys may leave a struct partly destroyed |
| | **13 — [Struct resource management: return values](#raii-and-struct-resource-management)** | |
| `CCW1301` | `raii-return-function-outside-return` | A raii return function is used outside of a return statement |
| `CCW1302` | `pod-return` | A function returning a pod struct does not return a function return value or another struct variable |
| `CCW1303` | `raii-return` | A function returning a raii struct does not return a function call |
| `CCW1304` | `raii-return-member-access` | A member is accessed directly on a raii struct returned by a function |
| `CCW1305` | `raii-return-discarded` | A raii struct returned by a function is discarded |
| | **14 — [Interfaces: vtables](#interfaces-rule)** | |
| `CCW1401` | `vtable-field-not-function-pointer` | A field of a vtable struct is not a function pointer |
| `CCW1402` | `vtable-function-missing-object-parameter` | A function pointer in a vtable struct does not take a `void*` or `const void*` as its first parameter |
| `CCW1403` | `vtable-not-static-const` | A vtable variable is not declared `static const` |
| `CCW1404` | `vtable-not-initialized` | A vtable variable is not initialized at declaration with braces |
| `CCW1405` | `vtable-missing-function` | A vtable initializer does not give an element for every function pointer of the vtable struct |
| `CCW1406` | `vtable-element-not-function` | A vtable initializer element is not a function, e.g. `NULL` or `0` |
| | **15 — [Interfaces: interfaces](#interfaces-rule)** | |
| `CCW1501` | `interface-field-count` | An interface struct does not have exactly two fields, an object and a vtable |
| `CCW1502` | `interface-object-field` | The first field of an interface struct is not a `void*` named `object`, or a `const void*` for a const interface |
| `CCW1503` | `interface-vtable-field` | The second field of an interface struct is not a pointer to a const vtable struct named `vtable` |
| `CCW1504` | `interface-field-not-private` | A field of an interface struct is not marked private for the private alternative rule |
| | **16 — [Restricted malloc](#restricted-malloc-rule)** | |
| `CCW1601` | `restricted-malloc` | A memory function is used outside of the allowed functions |
| | **17 — [Single return](#single-return-rule)** | |
| `CCW1701` | `multiple-returns` | A function has more than a single return |
| `CCW1702` | `missing-final-return` | A function does not end with a return statement |
| | **18 — [Strict switch](#strict-switch-rule)** | |
| `CCW1801` | `switch-fallthrough` | A switch case does not end with a break or return |
| `CCW1802` | `switch-missing-default` | A switch statement has no default case |
| | **19 — [Global variable](#global-variable-rule)** | |
| `CCW1901` | `global-missing-prefix` | A global variable does not start with the required prefix |
| `CCW1902` | `global-not-capitals` | A global variable is not written in capital letters |
| `CCW1903` | `global-not-static` | A global variable is not static |
| `CCW1904` | `global-not-const` | A global variable is not const |
| `CCW1905` | `global-static-in-header` | A static variable is defined in a header |
| | **20 — [Reference pointer](#reference-pointer-rule)** | |
| `CCW2001` | `reference-invalid-argument` | The argument for a reference parameter is not the address of an object or another reference |
| `CCW2002` | `reference-reassigned` | A reference pointer is reassigned |
| `CCW2003` | `reference-tag-on-non-pointer` | A reference tag is used on a parameter that is not a pointer |
| `CCW2004` | `reference-tag-not-on-parameter` | A reference tag is used on something other than a parameter of a function or function pointer type |
| `CCW2005` | `function-pointer-reference-tag-mismatch` | A function or function pointer with other reference tags is assigned or passed to a function pointer, or called with it through a conditional |
| | **21 — [Function discard](#function-discard-rule)** | |
| `CCW2101` | `function-return-discarded` | A non-void function return value is discarded |
| | **22 — [Array struct](#array-struct-rule)** | |
| `CCW2201` | `array-outside-struct` | An array is declared outside a struct field |
| `CCW2202` | `array-passed-to-non-library-function` | A struct array field is passed to a non-standard-library and non-third-party function |
| `CCW2203` | `array-struct-name-ending` | A struct holding only an array does not end with the array struct suffix and/or the element counts of the array |
| `CCW2204` | `array-struct-name-prefix` | A struct holding only an array of project structs does not start with the name of the element struct |
| | **23 — [Span struct](#span-struct-rule)** | |
| `CCW2301` | `span-invalid-definition` | A span struct does not contain only `data` and `size` fields |
| `CCW2302` | `span-uninitialized` | A span struct is not initialized at declaration |
| `CCW2303` | `span-array-count` | A span array initializer or pod creator does not use the array element count |
| `CCW2304` | `span-array-passed-to-non-library-function` | An array is passed to a function that is not a standard-library, third-party, span, or pod function |
| `CCW2305` | `span-const-mismatch` | A span struct holds a pointer to const data, or a const span struct a pointer to non-const data |
| `CCW2306` | `span-data-passed-to-non-library-function` | The data of a span, or a pointer returned by a function named after it, is passed to a function that is not a standard-library or third-party function |
| `CCW2307` | `span-missing-after-array` | An array outside of a struct is not followed right away by a span variable holding the whole array |
| `CCW2308` | `span-data-outside-wrapper` | The data of a span is passed on outside of a static function with a single statement |
| `CCW2309` | `array-used-after-span` | An array that has a span is used directly instead of through its span |
| | **24 — [Const field](#const-field-rule)** | |
| `CCW2401` | `const-field` | A struct field is const itself, e.g. `const int` or `int* const`, a pointer to const is allowed |
| | **25 — [No goto](#no-goto-rule)** | |
| `CCW2501` | `goto-not-allowed` | A goto statement is used, including a computed goto |

## Config behavior
The config is a yaml file that must have a certain format, as shown in the default (linked above). It first sets a list of third party folders which become unaffected by the parser, and the folder containing `compile_commands.json` (`compile_commands_dir`, relative to the config file), and then provides multiple individual rules can be set to `Off`, `Warning`, or `Error` in their `level` setting. This way the user can selectively enable only the rules that help their project.

```yaml
third_party_includes:
  - external/
  - vendor/

compile_commands_dir: build

rules:
  enum:
    level: Warning
    allow_enum_typedef: true
  null_check:
    level: Error
```

- A rule that is left out of the config is `Off`. An option that is left out gets its default, which is `false` for the on/off options unless its description below says otherwise. Starting from [the default config](./default/default.workshopc.yaml) is the easiest way to see every option.
- A file is third party when its path contains one of the `third_party_includes` entries, e.g. `external/` matches `project/external/json/json.h`. Code in third party files and system headers is never reported, but the project's own code that uses it still is.
- `Warning` and `Error` only differ in how the result is reported (the level in the output and the [exit code](#how-to-use)), the checks are the same.

### Enum rule
An `enum` argument can take any kind of integer which easily creates bugs and mistakes. This rule either forbids enums completely or, with `allow_enum_typedef: true`, allows them under strict rules.

```yaml
enum:
    level: Warning
    allow_enum_typedef: false
```

#### Forbid enums (`allow_enum_typedef: false`, the default)
The rule gets triggered whenever an `enum` is defined.

```c
enum Color { // Triggers parser
    RED,
    GREEN,
    BLUE
};
```

Instead a user can create a set of static or extern struct objects, or declare a struct and then typedef its pointer as the "enum".

```c
// This is one option to mimic enums with type safety
// The functions return pointers to static ColorTag objects
struct ColorTag;
typedef const struct ColorTag* Color;
Color color_red();
Color color_green();
Color color_blue();

// Adding these define statements makes end usage look as expected from enum usage
#define RED color_red()
#define GREEN color_green()
#define BLUE color_blue()
```

#### Allow typedef enums (`allow_enum_typedef: true`)
Enums are allowed, but two requirements apply.

**1. Every enum must have a typedef.** The typedef may come before or after the definition, and an anonymous enum is fine if it is defined together with its typedef.

```c
typedef enum Color { RED, GREEN, BLUE } Color;      // Good
typedef enum { SMALL, MEDIUM, LARGE } Size;         // Good, anonymous with typedef

enum Direction { NORTH, SOUTH };
typedef enum Direction Direction;                   // Good, typedef after the definition

enum Shape { CIRCLE, SQUARE };                      // Bad, no typedef
```

**2. A variable, assignment target or function argument of a typedef enum type may only be given a member of that enum.** Copying a value that already has the enum type (a variable, or the result of a function) is fine, and so is an explicit cast, which is the way to deliberately convert an integer. A conditional expression is fine if both branches are members.

```c
Color a = RED;                  // Good
Color b = a;                    // Good, value that already is a Color
Color c = (Color)1;             // Good, explicit cast
Color d = flag ? RED : BLUE;    // Good

Color e = 0;                    // Bad, plain integer
Color f = i;                    // Bad, integer variable
Color g = SMALL;                // Bad, member of a different enum
Color h = RED | GREEN;          // Bad, arithmetic

a = 2;                          // Bad
a = (Color)i;                   // Good, explicit cast
a++;                            // Bad, arithmetic
a |= GREEN;                     // Bad, arithmetic

void takes_color(Color color);
takes_color(GREEN);             // Good
takes_color(2);                 // Bad
takes_color((Color)2);          // Good, explicit cast
```

Details:
- Initializations, assignments (including through struct fields and pointers) and call arguments are checked. Compound assignments (`+=`, `|=`, ...), `++` and `--` are always reported, since they can produce values outside of the enum.
- A parameter that is a plain `int` accepts enum members without complaint, since it is not an enum type.
- Enums defined in system headers or `third_party_includes` folders are not checked, neither their definitions nor the code using them.
- The compiler can report its own warning when a member of one enum is used as a different enum (`-Wimplicit-enum-enum-cast`), which comes in addition to this rule's warning.

### Private rule
This rule makes a struct field private by its name: every field named `private_field` (e.g. `_private`), in any struct, may only be accessed from a few accessor functions in a source file. Keeping the private data behind a handful of functions means that the rest of the code can not put the struct in an inconsistent state, and that changing the private layout only affects one file.

```yaml
private:
    level: Error
    private_field: _private
    setter_contains: pset
    getter_contains: pget
```

For this config, the following triggers an error.

```c
struct Color
{
  int weight;
  struct {
      int r, g, b;
  } _private;
};
typedef struct Color Color;

void access_color_members(Color* color)
{
  int weight = color->weight; // OK, not private
  int r = color->_private.r;  // Triggers Error
}
```

Instead the private field must be accessed through getter and setter functions. An accessor must meet all three requirements:
- it is `static`,
- it is defined in a `.c` file (a static function in a header is copied into every file that includes it, so it is reported there),
- its name contains `getter_contains` or `setter_contains`, anywhere in the name, e.g. `pget_red` or `color_pset_red`.

```c
// color.c
static int pget_red(const Color* const color)
{
  return color->_private.r;       // OK
}

static void pset_red(Color* const color, int new_value)
{
  color->_private.r = new_value;  // OK
}

int color_red(const Color* const color)
{
  return pget_red(color);         // OK, the public function goes through the accessor
}
```

If the private field is a named struct, two accessors are enough, since they can hand out a pointer to the whole private part.

```c
struct ColorPrivate
{
  int r, g, b;
};
typedef struct ColorPrivate ColorPrivate;

struct Color
{
  int weight;
  ColorPrivate _private;
};
typedef struct Color Color;

static const ColorPrivate* pget(const Color* const color)
{
  return &color->_private;
}

static ColorPrivate* pset(Color* const color)
{
  return &color->_private;
}

void color_set_red(Color* const color, int red)
{
  pset(color)->r = red;           // OK, r is a field of ColorPrivate, which is not private itself
}
```

Details:
- Only the field with exactly the configured name is private. Its own fields are reached through it, so `color->_private.r` counts as an access of `_private`.
- An access outside of any function, e.g. in the initializer of a global variable (`static int* g_red = &g_color._private.r;`), is always reported, even inside `sizeof`.
- Accesses in system headers and `third_party_includes` folders are not checked.
- Use either this rule or the [private alternative rule](#private-alternative-rule), which marks private fields with a tag instead of a name.

### Private alternative rule
This rule is an alternative to the [private rule](#private-rule) that, instead of matching a field by name, matches any field tagged with the privacy tag. A [premade tag header](#tag-headers) can be used as is. The tag is a macro that adds an annotation while WorkshopC parses the code and expands to nothing otherwise, so the macro name can be changed freely.

```yaml
private_alternative:
    level: Error
```

A tagged field may only be accessed from a function whose name starts with the owning struct's own name, i.e. the struct tag (`Color` for `struct Color`), not a typedef alias of it. The function must also reach the field in one of two ways:
- through its first parameter, which must be a pointer to the owning struct named `self` (const or not, typedef or not), e.g. `self->r`,
- or with `.` directly on a variable or parameter whose type is the owning struct itself (not a pointer to it), e.g. `color.r` on a local `Color_t color`. This is what lets the [pod and raii creator functions](#raii-and-struct-resource-management) set up the struct they return.

> *Note: This rule works really well with* [RAII and struct resource management](#raii-and-struct-resource-management)*, where the constructor functions can also access the private fields.*

This example uses the macro `PRIVATE` for the privacy tag.

```c
#include "private_tag.h"

struct Color
{
  int weight;
  PRIVATE int r;
  PRIVATE int g;
  PRIVATE int b;
};
typedef struct Color Color_t;

void access_color_members(Color_t* color)
{
  int weight = color->weight; // OK, not tagged
  int r = color->r;           // Triggers Error
}
```

Instead the user must define accessor functions whose name starts with the struct's own name and whose first parameter is a pointer named `self`.

```c
int Color_get_r(const Color_t* self)
{
  return self->r;             // OK
}

void Color_set_r(Color_t* self, int value)
{
  self->r = value;            // OK
}

Color_t Color_pod(int r, int g, int b)
{
  Color_t color = {0};
  color.r = r;                // OK, a local value of the struct itself in a function named after it
  color.g = g;
  color.b = b;
  return color;
}

int Color_get_other_r(const Color_t* other)
{
  return other->r;            // Triggers Error, the pointer is not named self
}

int get_r(const Color_t* self)
{
  return self->r;             // Triggers Error, the name does not start with Color
}
```

Note that reaching into a *different* struct's tagged field through a field of the current struct still requires that other struct's own accessor, even from inside a function that is otherwise a valid accessor for the current struct.

```c
struct Wrapper
{
  PRIVATE Color_t color;
};
typedef struct Wrapper Wrapper_t;

void Wrapper_bad(Wrapper_t* self)
{
  self->color.weight = 5; // OK: color is Wrapper's own field, and weight is not tagged
  self->color.r = 5;      // Triggers Error: r belongs to Color, not Wrapper
}
```

As with the private rule, an access outside of any function is always reported, and accesses in system headers and `third_party_includes` folders are not checked.

### Function pointer rule
This rule gets triggered when a function pointer is declared without a typedef, whether it is a variable, a parameter, a struct field or the return type of a function. Using a typedef makes function signatures clearer, reduces errors when the signature changes (you only need to update it in one place), and is less error-prone since it's harder to accidentally mistype the function signature.

```yaml
function_pointer:
    level: Warning
```

```c
typedef int (*math_function)(int, int);

// Triggers parser, the parameter is spelled out without a typedef
void bad_perform_math(int (*math_func)(int, int), int a, int b, int* out_res)
{
  *out_res = math_func(a, b);
}

// OK
void perform_math(math_function math_func, int a, int b, int* out_res)
{
  *out_res = math_func(a, b);
}

int add(int a, int b);

void foo(void)
{
  int (*raw)(int, int) = add;           // Triggers parser
  math_function typed = add;            // OK
  int (*table[2])(int, int) = {add, add}; // Triggers parser, an array of function pointers
  math_function typed_table[2] = {add, add}; // OK
}

struct calculator
{
  int (*raw_operation)(int, int);       // Triggers parser
  math_function operation;              // OK
};

int (*get_raw_operation(void))(int, int); // Triggers parser, the return type
math_function get_operation(void);        // OK
```

Details:
- Global, static and local variables, function parameters, struct fields and return types are checked, also when the function pointer is the element of an array or is pointed to by another pointer (`int (**)(int)`).
- Any typedef works, whatever its name, as long as the function pointer itself is written with it. A typedef used only for the return or parameter types of the function (`size_t (*f)(int)`) does not count.
- Declarations in system headers and `third_party_includes` folders are not checked, and a macro that expands to a function pointer declaration is reported with `(macro expansion)` added to the message.

### Typedef struct rule
This rule gets triggered when a struct is defined but has no typedef. Requiring a typedef ensures consistency across the codebase, reduces repetition by eliminating the need to write `struct` every time you use the type, and improves code clarity by enforcing a uniform naming pattern.

```yaml
typedef_struct:
    level: Warning
```

```c
struct Position                 // Triggers parser, no typedef
{
  int x, y, z;
};

typedef struct Size             // OK
{
  int width, height;
} Size;

struct Point                    // OK, the typedef can come later in the file
{
  int x, y;
};
typedef struct Point Point_t;   // Any name works

typedef struct                  // OK, anonymous struct with a typedef
{
  int r, g, b;
} Color;

typedef struct Line             // OK
{
  struct Segment                // Triggers parser, a named nested struct needs its own typedef
  {
    int start, end;
  } segment;
} Line;
```

Details:
- Only struct definitions are checked, a forward declaration (`struct Position;`) alone is not reported.
- The typedef may come before or after the definition, anywhere at file scope in the same translation unit, and may have any name.
- Anonymous structs are never reported, since they can not be named anyway (e.g. an anonymous nested struct or a single `struct { ... } instance;`).
- A struct defined by a macro is checked where the macro is used, even when the macro itself comes from a third party header. Structs defined in system headers and `third_party_includes` folders are not checked.

### Assignment rule
This rule gets triggered whenever a variable of a basic/primitive type (e.g. `int`, `float`, `char`, `_Bool`) or a pointer is declared but not initialized in the same statement, and the same goes for arrays of them. Requiring initialization prevents use-before-initialization bugs and makes developer intent clearer (a variable that's initialized is ready to use). Structs and unions are not required to be initialized by this rule, see [RAII and struct resource management](#raii-and-struct-resource-management) for that.

```yaml
assignment:
    level: Warning
    forbid_null_assign: true
    forbid_null_as_arg: true
    forbid_zero_init_for_objects_with_pointers: true
    forbid_arg_reassign: true
    forbid_mut_arg_pointer: true
```

```c
static int g_count;         // Triggers parser, globals and statics too, even though C sets them to 0
int a;                      // Triggers parser
int b = 5;                  // OK
int* ptr;                   // Triggers parser
int values[4];              // Triggers parser
int values_2[4] = {0};      // OK
Position position;          // OK, structs are not checked by this rule
```

The extra settings are all `false` unless enabled.

#### Null assignment
The first three settings help forbid null for codebases that follow a strict "nothing may be null" rule, to make sure no null dereferences occur. `NULL`, `0` and casts of `0` such as `(void*)0` all count as null.

```c
struct Setting
{
  const char* name;
  int x, y, z;
};
typedef struct Setting Setting;

void example(int* a, int* b)
{
  int* ptr = a;
  ptr = NULL;                                 // Not OK with forbid_null_assign: true
  int* empty = NULL;                          // Not OK with forbid_null_assign: true
  foo(NULL, 3, 7);                            // Not OK with forbid_null_as_arg: true

  Setting setting = {0};                      // Not OK with forbid_zero_init_for_objects_with_pointers: true
  Setting named = { .name = NULL, .x = 1 };   // Not OK with forbid_null_assign: true, NULL in a pointer field
  named.name = 0;                             // Not OK with forbid_null_assign: true

  int* pair[2] = { a, b };                    // OK
  int* partial[3] = { a, b };                 // Not OK with forbid_null_assign: true, the third element is left null
  int* explicit_null[2] = { a, NULL };        // Not OK with forbid_null_assign: true
}
```

- `forbid_null_assign`: a pointer may not be initialized with null, a pointer or a pointer field may not be assigned null, a pointer field in an initializer list may not be given null, and an array of pointers must explicitly initialize every element (at every level of an array of arrays) with a non-null value, since elements left out become null pointers.
- `forbid_null_as_arg`: null may not be passed as an argument to any function.
- `forbid_zero_init_for_objects_with_pointers`: an object that contains a pointer, directly or in a field of a field, may not be initialized with `{0}`, since that silently makes every pointer in it null. Objects without pointers may still use `{0}`.

#### Argument modification
Many programmers are not fond of const correctness when it comes to arguments, but that does not mean it is not important for code clarity. Instead of enforcing const correctness for argument values, rules can be enabled to simply forbid modifying argument values. Note that this does not affect the data a pointer argument points to.

> *Note: For const correctness across all variables, a tool like clang-tidy can be used.*

```c
typedef struct RGB
{
  int r, g, b;
} RGB;

void foo(const char* name, int x, RGB color, RGB* rgb)
{
  name = "New string";  // Not OK with forbid_arg_reassign: true
  x = 5;                // Not OK with forbid_arg_reassign: true
  x += 1;               // Not OK with forbid_arg_reassign: true
  x++;                  // Not OK with forbid_arg_reassign: true
  color.r = 0;          // Not OK with forbid_arg_reassign: true, a field of a struct passed by value
  int* x_ptr = &x;      // Not OK with forbid_mut_arg_pointer: true
  set_int(&x);          // Not OK with forbid_mut_arg_pointer: true (void set_int(int* out))

  const int* x_view = &x; // OK, the argument can not be modified through it
  print_int(&x);          // OK (void print_int(const int* value))
  int y = x;              // OK
  rgb->r = x;             // OK, writing through a pointer argument is fine
}
```

- `forbid_arg_reassign`: an argument may not be assigned a new value (`=`, `+=`, `|=`, ...) or be changed with `++` or `--`, and neither may the fields of a struct passed by value, since the change would only affect the local copy.
- `forbid_mut_arg_pointer`: the address of an argument, or of a field of a by-value argument, may only be taken as a pointer to const, wherever it is taken: in a declaration, an assignment, a function call or a return. A cast decides the type, so `(const int*)&x` is OK while `(int*)&x` is not. Without this, `forbid_arg_reassign` could be worked around by writing through a pointer to the argument.

### Prefix namespace rule
This rule requires the names declared in a header to start with a prefix built from the header's folder path, which mimics namespaces in other languages such as C++. This prevents naming collisions in large projects, makes it obvious which module code belongs to at a glance, and enables simulation of C++ namespace organization in pure C. Optionally the header must also have an include guard built from its path.

```yaml
prefix_namespace:
    level: Warning
    top_dir: src
    work_from_top: true
    stop_at_count: 10
    use_seperator: true
    seperator: __
    apply_to_functions: true
    apply_to_structs: true
    apply_to_typedefs: true
    require_ifndef_for_filepath: true
    case_insensitive: false
```

- `top_dir` (required, e.g. `src`): the folder the namespaces start in. The prefix is built from the folders after the first folder with this name in the header's path, not including it. A header that is not inside such a folder is not checked.
- `stop_at_count` (default `10`) and `work_from_top` (default `false`): at most `stop_at_count` folders are used. With `work_from_top: true` these are the folders closest to `top_dir`, otherwise the folders closest to the file.
- `use_seperator` and `seperator` (note the spelling of the keys): the separator is written between the folder names and after the last one. Without it the folder names are joined directly, e.g. `appchrono`.
- `apply_to_functions`, `apply_to_structs` and `apply_to_typedefs`: which names must have the prefix. Static functions never need it, since they are not visible outside the file.
- `require_ifndef_for_filepath`: the first `#ifndef` in the first 10 lines of the header must be the path after `top_dir`, including the file name, in capital letters and joined with `_`, whatever the separator setting.
- `case_insensitive` (default `false`): a name only needs the same letters as the prefix, in upper or lower case, so for the folders `App/Chrono` both `App__Chrono__start` and `app__chrono__start` are fine. Without it the case must match the folders exactly. The include guard is not affected, it is always in capital letters.

Folder names keep their case in the prefix, and every character that is not a letter or digit becomes `_`, so a folder `My-Lib` becomes `My_Lib` in the prefix (e.g. `My_Lib__open`) and `MY_LIB` in the include guard. Finding `top_dir` in the path does not depend on case. Only headers (`.h`, `.hpp`, `.hh`, `.hxx`) are checked, since their names are the ones other files see, and headers in `third_party_includes` folders are skipped.

Here is what the above settings expect from a header.

```c
// This file is in c/workspace/repos/project/src/app/chrono/timer.h

#ifndef APP_CHRONO_TIMER_H
#define APP_CHRONO_TIMER_H

struct app__chrono__timer;
typedef struct app__chrono__timer app__chrono__timer_t;
typedef int (*app__chrono__callback)(void);

void app__chrono__start_timer_with_callback(app__chrono__timer_t* timer,
                                            app__chrono__callback callback,
                                            int seconds);

void start_timer(app__chrono__timer_t* timer);      // Triggers parser, missing the app__chrono__ prefix

static inline int timer_helper(void) { return 0; }  // OK, static functions are not checked

#endif
```

With `work_from_top: false` and `stop_at_count: 1`, only the folder closest to the file is used, so the prefix for the same header would be `chrono__`.

### Null check rule
Every pointer parameter must be checked for null before it is dereferenced. A function can then never crash on a null argument, and the check documents that null is an input the function handles.

```yaml
null_check:
    level: Error
    allow_direct_ptr_in_if_statement: true
```

```c
int dereference_without_check(int* i_ptr)
{
  return *i_ptr;  // Triggers parser
}

int dereference_safely(int* i_ptr)
{
  if (i_ptr == NULL)
  {
    return -1;
  }
  return *i_ptr;  // OK
}
```

`*i_ptr`, `i_ptr->field` and `i_ptr[index]` all count as dereferences. A dereference is only allowed where the pointer is known to be non-null on every path of the function that reaches it, so the rule follows the control flow of the function rather than just looking for a check somewhere before it. A pointer is known to be non-null:
- In the branch where a condition says so: the `if` branch of `if (i_ptr != NULL)`, the `else` branch of `if (i_ptr == NULL)`, the right side of `i_ptr && *i_ptr` or `!i_ptr || *i_ptr`, the matching branch of `?:`, and a loop body behind such a loop condition.
- After a check whose null branch always leaves: `return`, `break`, `continue`, `goto`, or a call to a function that never returns, such as `abort()` or `exit()`. `assert(i_ptr != NULL)` works the same way, since a failed assert aborts.

These conditions count as null checks:
- A comparison with null, e.g. `i_ptr == NULL`, `NULL != i_ptr`, or a comparison inside a macro such as `IS_NULL(i_ptr)`. It can also be stored in a variable first: `bool valid = i_ptr != NULL; if (valid) ...`.
- With `allow_direct_ptr_in_if_statement: true`, a plain `i_ptr` or `!i_ptr`, e.g. `if (!i_ptr) return;` or `return i_ptr ? *i_ptr : 0;`. When it is `false`, the comparison must be written out.

```c
int sum(int* a, int* b, int flag)
{
  if (a == NULL)
    return 0;

  if (flag)
  {
    if (!b)
      return *a;      // OK, a is not null on every path here
    return *a + *b;   // OK, b is not null in this branch
  }

  return *a + *b;     // Triggers parser for b, it is only checked when flag is set
}

int log_only(int* value)
{
  if (value == NULL)
    log_error("null");  // execution goes on with a null pointer

  return *value;        // Triggers parser, the null branch does not leave
}
```

Where paths come together, e.g. after an `if`/`else`, a loop or a `switch`, or at a label reached by a `goto`, the pointer is only known to be non-null if it is on all of them. So a check in one branch of an `if` does not count after it unless the other branches check too, and a check inside a loop does not count after the loop, since the loop may not run at all. Giving a parameter a new value (`i_ptr = other;`) forgets that it was checked. Passing the pointer on to another function is not a check, and neither is a function's result, e.g. `if (is_valid(i_ptr))`, since the rule can not know what the function checks. Each parameter is checked on its own and reported at most once, at its first unchecked dereference. Reference parameters of the [reference pointer rule](#reference-pointer-rule) can be exempted, since they can never be null.

### Argument pointer movement rule
This rule makes the ownership of pointers visible. Every non-const pointer parameter must be tagged with how the function uses it, and the call sites can be required to mark the argument with a matching "operator", so both the function and the callsite show what happens to the pointer. A [premade tag header](#tag-headers) can be used as is, but all macro names can be changed as well.

There are three tags:
- **move**: the function takes ownership of the pointer, e.g. it stores it or frees it. The caller may not use the pointer again until it has been given a new value.
- **out**: the function writes a result through the pointer, typically a `Data**` it points at newly created data, or a value it fills in.
- **modify**: the function changes the data the pointer points to, but the caller keeps ownership.

A pointer to const needs no tag, since the function can only read through it.

```yaml
argument_pointer_movement:
    level: Warning
    require_operator_for_move_callsite: true
    require_operator_for_out_callsite: true
    require_operator_for_modify_callsite: false
```

Each `require_operator_for_..._callsite` setting decides whether that kind of argument is marked at the callsite. When it is `true`, the argument must be wrapped in the operator; when it is `false`, the operator may not be used at all, so that every callsite in the project looks the same.

For the settings above, and the [lowercase tag header](#tag-headers) with the tags `receives`, `initializes` and `borrows` and the operators `give()`, `overwrite()` and `lend()`, the parser expects code to look like this:

```c
struct Data
{
  int i, j, k;
};
typedef struct Data Data;

int get_data_i(const Data* data);                           // No tag needed, const
void initialize_data(initializes Data** data);
void edit_data(borrows Data* data, int i, int j, int k);
void give_data_to_other_section(receives Data* data);
void no_tag(Data* data);                                    // Triggers parser, non-const pointer without a tag

void example(void)
{
  Data* data = NULL;
  initialize_data(overwrite(&data));                        // OK
  edit_data(data, 3, 5, 7);                                 // OK, the modify operator is disabled
  int i = get_data_i(data);                                 // OK
  give_data_to_other_section(give(data));                   // OK

  Data* other = NULL;
  initialize_data(&other);                                  // Triggers parser, missing overwrite()
  edit_data(lend(other), 1, 2, 3);                          // Triggers parser, the modify operator is disabled
  initialize_data(give(&other));                            // Triggers parser, wrong operator
  get_data_i(give(other));                                  // Triggers parser, the parameter has no tag
}
```

Notice how everything that is const does not need tagging because it almost handles itself, and notice how the need for the `lend` "operator" was disabled in the settings. This is a good middle ground so that the ownership transfer is the most noticeable parts of the code. With these rules in place, the code becomes self-documenting and if a function is ever changed in future in taking a `const`, `borrows`, `initializes`, or `receives` pointer then the parser will trigger and catch that the callsite is unedited and may have unexpected behavior.

A function declared more than once (e.g. in a header and in the source file) must use the same tags every time. Functions declared in system headers and `third_party_includes` folders have no tags, so passing a pointer to them needs no operator, and using one is reported.

Once a pointer has been moved, the called function owns it, so the pointer may not be used again (read, moved again, compared, passed on, its address taken) until it has been reassigned, either with `=` or by declaring it again. Passing its address to an `out` parameter does not count as reassigning it, since the called function is not guaranteed to write a new value, for example when it fails and returns an error code early. Reassign it explicitly first, e.g. `data = NULL;`. The check follows the control flow of the function: a use is reported if the pointer may have been moved on any path that reaches it, so a move inside one branch of an `if` counts after the `if`, unless that branch returns, and a move inside a loop counts in the next iteration. Only plain pointer variables are followed, not struct fields or array elements.

```c
void example(void)
{
  Data* data = NULL;
  initialize_data(overwrite(&data));
  give_data_to_other_section(give(data));
  edit_data(data, 1, 2, 3);                 // Triggers parser, data was moved
  initialize_data(overwrite(&data));        // Triggers parser, out may not write a new value
  data = NULL;                              // OK, data is reassigned
  initialize_data(overwrite(&data));        // OK
  edit_data(data, 1, 2, 3);                 // OK
}
```

The movement tags may only be written on parameters, of function declarations and definitions and of function pointer types, not on variables, struct fields, functions or typedefs.

A parameter tagged `borrows` or `initializes` is only borrowed by the function, so it may not be moved away. The same goes for what an `initializes` parameter points to.

```c
void process(borrows Data* data)
{
  give_data_to_other_section(give(data));   // Triggers parser, the function does not own data
}
```

#### Function pointers
A function pointer type carries the tags on its parameters, just like a function, and a non-const pointer parameter of it needs a tag too. A function, or another function pointer, that is assigned or passed to a function pointer must have the same tags, parameter by parameter. This is checked wherever a function pointer gets its value: initializations (also of arrays and struct fields), compound literals such as `(handlers){ .on_event = function }`, assignments, arguments and return statements. A cast does not hide a mismatch.

```c
typedef void (*consumer_t)(receives Data* data);

void give_data_to_other_section(receives Data* data);
void edit_data(borrows Data* data, int i, int j, int k);
void borrow_data(borrows Data* data);

consumer_t consumer = give_data_to_other_section;  // OK, the same tags
consumer_t wrong = borrow_data;                    // Triggers parser, borrows instead of receives
void register_consumer(consumer_t consumer);
register_consumer(borrow_data);                    // Triggers parser, same
```

Calls through a function pointer then follow the tags of its type, exactly like calls to a function: the callsite operators, the use of a pointer after it was moved, and the borrowed parameters that may not be moved. The function pointer can be a variable, a parameter, a struct field, an array element or a conditional expression. A call through a conditional expression, e.g. `(flag ? first : second)(give(data))`, follows the tags its branches agree on, and is reported when they tag a parameter differently, since the callsite can only follow one set of tags.

```c
void example(consumer_t consumer)
{
  Data* data = NULL;
  initialize_data(overwrite(&data));
  consumer(give(data));                     // OK, the operator of consumer_t's receives parameter
  edit_data(data, 1, 2, 3);                 // Triggers parser, data was moved to consumer
}
```

A function pointer type from a system header or a `third_party_includes` folder has no tags, so any function may be assigned or passed to it.

#### Naming the tags and operators
The tags and operators are just macros, so any names can be used instead of the [premade ones](#tag-headers), in capital letters or not. What WorkshopC recognizes is what they expand to while it parses the code (when `WORKSHOPC_PARSING` is defined):
- A tag must expand to `__attribute__((annotate("workshopc_move")))`, `"workshopc_out"` or `"workshopc_modify"`.
- An operator must call a function named `workshopc_move`, `workshopc_out` or `workshopc_modify` that takes the argument and returns it unchanged.

When `WORKSHOPC_PARSING` is not defined, the tags expand to nothing and the operators to their argument, so the real build is unaffected. For example, a header with the tags `moved`, `output` and `mutable` and the operators `move()`, `out()` and `mut()`:

```c
#ifdef WORKSHOPC_PARSING

static inline void* workshopc_move(void* arg) { return arg; }
static inline void* workshopc_out(void* arg) { return arg; }
static inline void* workshopc_modify(void* arg) { return arg; }

#define move(expr) ((__typeof__(expr))workshopc_move((void*)(expr)))
#define out(expr)  ((__typeof__(expr))workshopc_out((void*)(expr)))
#define mut(expr)  ((__typeof__(expr))workshopc_modify((void*)(expr)))

#define moved   __attribute__((annotate("workshopc_move")))
#define output  __attribute__((annotate("workshopc_out")))
#define mutable __attribute__((annotate("workshopc_modify")))

#else

#define move(expr) (expr)
#define out(expr)  (expr)
#define mut(expr)  (expr)

#define moved
#define output
#define mutable

#endif
```

```c
void give_data_to_other_section(moved Data* data);      give_data_to_other_section(move(data));
void initialize_data(output Data** data);                initialize_data(out(&data));
void edit_data(mutable Data* data, int i, int j, int k); edit_data(mut(data), 1, 2, 3);
```

Names like these read well but are easy to clash with: every variable called `output` breaks once the header is included, and `mutable` is a C++ keyword. Pick names that are rare in your code.

### RAII and struct resource management
This rule introduces struct categorization and centralizes resource management within them to a given set of functions, effectively creating a RAII system. 

The structs are given their category "type" by the creation function they are accompanied with, whose name is the struct name plus a suffix given by the config file. A struct must have exactly one creation function: a pod, a raii or a free one. The creation function and the other lifecycle functions of a struct are exempt from its rules, meaning any need to e.g. initialize a variable from a function does not exist in these functions, since they are where the struct is actually built. The following is true for these settings:

```yaml
struct_resource_management:
    level: Error
    pod_struct_creator_suffix: _pod
    raii_struct_creator_suffix: _make
    raii_struct_destroyer_suffix: _destroy
    raii_may_only_destroy_value_ref: true
    raii_struct_copy_suffix: _copy
    raii_struct_move_suffix: _move
    raii_may_only_move_value_ref: true
    raii_struct_return_suffix: _return
    raii_struct_valid_suffix: _valid
    raii_use_after_destroy: false
    raii_struct_array_destroyer_suffix: _destroy_array
    allow_raii_struct_arrays: true
    raii_destroy_in_reverse_order: true
    raii_standardized_destroy_definitions: true
    free_struct_creator_suffix: _init
```

- `raii_use_after_destroy` (default `true`): when `false`, a raii struct that has been destroyed with its destroy function may not be referenced again, neither passed to a function nor accessed through a field. The same goes for a span or const span initialized from a destroyed [array of raii structs](#arrays-of-raii-structs), e.g. `vectors_span.data[0]` after `int_vector_destroy_array(vectors, 2)`. Only the span declared from the array is followed, not copies of it.
  - `raii_destroy_in_reverse_order` (default `false`): when `true`, raii structs must be destroyed in reverse declaration order. If `a`, `b` and `c` are initialized in that order, they must be destroyed as `c`, `b`, then `a`. When `false`, destruction order is unrestricted as before.
- `allow_raii_struct_arrays` (default `false`): enables arrays of raii structs outside of structs, see [arrays of raii structs](#arrays-of-raii-structs). When `false`, arrays of raii structs are only allowed inside structs.
- `raii_struct_array_destroyer_suffix`: the suffix of the array destroy function, required when `allow_raii_struct_arrays` is `true`.
- `raii_may_only_move_value_ref` (default `false`): when `true`, a raii move function may only be given the address of a variable, e.g. `dynamic_string_move(&name)`, so that only an object owned by the calling scope can be moved from. A pointer (`dynamic_string_move(name_ptr)`), a struct field (`&holder.name`, `&holder->name`) or anything behind a pointer (`&*name_ptr`) is reported. The lifecycle functions of any struct may still work through their `self` pointer, e.g. a struct's own return function moving `self`, or the move function of a struct with a raii field moving `&self->field`.
- `raii_may_only_destroy_value_ref` (default `false`): the same for the destroy function, e.g. `dynamic_string_destroy(&name)`. The destroy function of a struct with raii fields may still destroy them with `&self->field`. Fields of [free structs](#free-struct) are not affected and may be destroyed from anywhere, e.g. `dynamic_string_destroy(&holder.name)` or `dynamic_string_destroy(&holder->name)`, since free structs come with no rules.
- `raii_standardized_destroy_definitions` (default `false`): when `true`, the destroy function of a raii struct must destroy each of its raii fields exactly once. A field is destroyed with `<field struct>_destroy(&self->field)`, or `<field struct>_destroy_array(self->field, count)` for an array field. All the field destroys are statements side by side in one block, either:
  - the body of the destroy function, or
  - the then-block of an `if` statement directly in the body. The condition can be anything.

  Destroys in a nested `if`, an `else` branch, a nested `{ }` block, a loop or a `switch` are reported. So are destroys split between two blocks. A `return` or `goto` before all the destroys, e.g. an early return when `self` is null, skips the struct as a whole and is allowed by this rule, though e.g. the [single return rule](#single-return-rule) may not allow it. A `return` or `goto` between the destroys is reported, and so is a label between them: both would leave the struct partly destroyed. Other statements, e.g. freeing a buffer, may come anywhere. With `raii_destroy_in_reverse_order`, the fields must be destroyed in reverse declaration order. Pod, free and pointer fields are not owned raii fields and are not affected.

  A raii struct is always either fully alive or fully destroyed, never partly. Its make function brings every raii field to life, and its destroy function destroys every raii field together. A condition in the destroy function decides whether the struct as a whole is destroyed, e.g. when it is null or has been moved from, never which of its fields are. When every raii type has a valid empty state, a moved-from struct usually needs no condition at all: its fields are already empty, so destroying them does nothing.

  ```c
  struct owner { first_t first; char* buffer; second_t second; bool moved; };

  void owner_destroy(owner_t* self)
  {
      if (self != NULL && !self->moved)       // OK: any condition
      {
          second_destroy(&self->second);      // OK: reverse declaration order
          memory_free(self->buffer);          // OK: other statements may come in between
          first_destroy(&self->first);        // OK
      }
  }

  void split_destroy(split_t* self)
  {
      second_destroy(&self->second);

      if (self->buffer != NULL)
      {
          first_destroy(&self->first);        // Triggers parser, not in the same block as 'second'
      }
  }

  void early_return_destroy(early_return_t* self)
  {
      if (self == NULL)
          return;                             // OK: skips the struct as a whole

      second_destroy(&self->second);          // OK
      first_destroy(&self->first);            // OK
  }

  void return_between_destroy(return_between_t* self)
  {
      second_destroy(&self->second);

      if (self->moved)
          return;                             // Triggers parser, 'first' would be left alive

      first_destroy(&self->first);
  }
  ```

#### POD structs
Plain Old Data (POD) structs come with only one rule: they must always be initialized correctly. A pod struct requires a create function of signature `struct <structname> <structname>_pod(...)`. They are used to avoid uninitialized variables and make sure they are always initialized correctly. Every value of a pod struct must come from a function return value or another struct variable, whether it initializes a variable, is passed as an argument or is returned, so `{0}` and brace literals are only allowed in the `_pod` function.

```c
typedef struct position
{
  int x, y, z;
} position_t;

static inline position_t position_pod(int x, int y, int z)
{
  return (position_t){x, y, z};   // OK, brace literals are only legal in the _pod function
}

static inline position_t position_default()
{
  return position_pod(0, 0, 0);   // This function is based on the core create function above
}

int position_sum(position_t position);

void foo()
{
  position_t no_init_pos;                         // Triggers parser, not initialized
  position_t zero_pos = {0};                      // Triggers parser, not from a function or variable
  position_t pos = position_default();            // OK
  pos = position_pod(1, 2, 3);                    // OK, pod structs may be reassigned
  position_t pos_2 = pos;                         // OK, copied from another variable
  pos_2.y = 10;                                   // OK
  int sum = position_sum((position_t){1, 2, 3});  // Triggers parser, brace literal as an argument
}
```

A pod struct may only contain other pod structs, not raii or free structs, since a copy of a pod struct is just a copy of its bytes.

Arrays of pod structs are allowed outside of structs when every element, at every level of an array of arrays, is explicitly initialized from a function return value or another struct variable, e.g. `position_t line[2] = { position_pod(0, 0, 0), pos };`, so `{0}`, brace literals and missing elements are not allowed. Arrays of raii structs follow their own rules, see [arrays of raii structs](#arrays-of-raii-structs).

#### RAII struct
Resource Acquisition Is Initialization (raii) structs are more powerful but also come with a lot more rules. They can only be assigned *once* and must be so through function calls (they can however still be edited). Furthermore, the rule ensures that no scope exit occurs without either a destroy function call or a return function call. They also require a set of functions to be declared (definition is optional):

- make function: creates the struct and initializes all members (constructor)
- copy function: safely creates a copy of another struct (copy constructor)
- move function: creates a new struct and transfers ownership of resources to it (move constructor), good practice to leave argument object in a valid but "empty" state
- destroy function: cleans up and frees all resources in the struct (destructor), checked if used before scope exit
- return function: used in return statements to safely move ownership out of function (effectively an easy to optimize combination of copy and destroy), checked if used before scope exit
- valid function: used to verify if the struct is in a valid and usable state (similar to catching an exception from a constructor), where a raii struct object may be valid or invalid but never in an illegal state

The parser helps make sure all functions exist. All functions other than the make function must have its first argument be a pointer to the struct type and name it `self`. Here is a simple example:

```c
struct dynamic_string
{
    char* data;
    size_t size;
    size_t capacity;
};
typedef struct dynamic_string d_str;

// These functions are mandatory
d_str dynamic_string_make(const char* c_str);   // Performs malloc and copies c_string argument
d_str dynamic_string_copy(const d_str* self);   // Performs malloc for new object copy of argument
d_str dynamic_string_move(d_str* self);         // Moves data from argument to new object, and sets argument data to null
void  dynamic_string_destroy(d_str* self);      // Frees data memory, possibly makes object invalid
d_str dynamic_string_return(d_str* self);       // Copies data in struct to new (no cleanup needed in this case)
_Bool dynamic_string_valid(d_str* self);        // Checks internal logic if struct instance is valid (e.g. size > capacity as primitive invalid state)

// These functions are optional and an example
const char* dynamic_string_data(const d_str* self);
void dynamic_string_add(d_str* self, const char* c_str);
void dynamic_string_concat(d_str* self, const d_str* addition);
void dynamic_string_reset(d_str* self);
```

With the example raii struct set up, here is an example of usage.

```c
// Example of setting up dynamic string and moving its ownership to print function
void take_dynamic_string_and_print(d_str string_as_value);
void print_big_greeting()
{
  d_str greeting = dynamic_string_make("Hello, world!");
  if (!dynamic_string_valid(&greeting))
  {
    dynamic_string_destroy(&greeting);
    return;
  }

  dynamic_string_add(&greeting, " Hello, everyone!");
  take_dynamic_string_and_print(dynamic_string_move(&greeting));

  dynamic_string_destroy(&greeting);
}
```

A raii struct owns its resources, so every value of it must have exactly one owner, and the rules follow from that:
- A variable must be initialized from a function return value, e.g. the make, copy or move function, never from another variable (`d_str b = a;` would make two owners of the same memory).
- It may not be reassigned once initialized, not even through a pointer, a field or an array element, since the old value would leak.
- It may only be passed by value as a function return value, e.g. `take(dynamic_string_copy(&s))` or `take(dynamic_string_move(&s))`, so that the called function gets its own value, which it then must destroy.
- Every variable, and every parameter passed by value, must be destroyed or returned before every scope exit (the end of its block, `return`, `break`, `continue` or `goto`). A `goto` exits the blocks that do not contain its label, so the common `goto cleanup;` pattern works as long as the label is in the same block as the variables it destroys.
- A function returning a raii struct must return a function call, and a local variable is returned with the return function.
- A raii struct returned by a function may not be discarded or have a field read directly on the returned value, since it could then never be destroyed.

```c
d_str make_greeting(const char* name)
{
  d_str greeting = dynamic_string_make("Hello, ");
  dynamic_string_add(&greeting, name);
  return dynamic_string_return(&greeting);          // OK, ownership moves to the caller
}

void use_greetings(void)
{
  d_str a = make_greeting("world");                 // OK
  d_str b = a;                                      // Triggers parser, two owners of the same memory
  a = dynamic_string_make("again");                 // Triggers parser, the old value would leak
  make_greeting("world");                           // Triggers parser, the result can never be destroyed
  size_t size = make_greeting("world").size;        // Triggers parser, same
  d_str c = dynamic_string_return(&a);              // Triggers parser, return functions only belong in return statements

  dynamic_string_destroy(&a);
}                                                   // Triggers parser, b and c are never destroyed
```

##### Raii structs with raii fields
A raii struct can hold other raii structs as fields. Since a raii value may not be reassigned, such a field is set in the initializer of the value the make, copy or move function builds, and not assigned afterwards. Only these lifecycle functions may initialize the struct with a brace initializer.

```c
typedef struct message
{
  d_str text;
  int priority;
} message;

message message_make(const char* text, int priority)
{
  message self = { .text = dynamic_string_make(text), .priority = priority };  // OK
  return self;
}

message message_copy(const message* self)
{
  return (message){ .text = dynamic_string_copy(&self->text), .priority = self->priority };  // OK
}

message message_make_later(const char* text)
{
  message self = message_make("", 0);
  self.text = dynamic_string_make(text);            // Triggers parser, the old text would leak
  return message_return(&self);
}
```

If building a field needs steps first, e.g. checking that it is valid, do them with a local variable and move it into the initializer: `message self = { .text = dynamic_string_move(&text), .priority = 1 };`.

This rule works better when combined with the [private members rule](#private-rule) or the [private alternative rule](#private-alternative-rule) since a major point to the raii struct is to make sure the internal state of the struct is always controlled.

##### Arrays of raii structs
With `allow_raii_struct_arrays: true` and `raii_struct_array_destroyer_suffix` set (e.g. `_destroy_array`), one-dimensional arrays of raii structs are allowed outside of structs. Every element must be initialized from a function return value, and the array must be destroyed before every scope exit with the array destroy function, whose first argument must be the array itself and whose second argument must be the size of the array. The function is not required for a raii struct in general, only once an array of it is declared, and it must have the signature `void <struct name><suffix>(<struct name>* self, size_t n)`.

```c
void dynamic_string_destroy_array(d_str* self, size_t n);

void foo()
{
  d_str names[2] = { dynamic_string_make("a"), dynamic_string_make("b") };     // OK

  dynamic_string_destroy_array(names, sizeof(names) / sizeof(names[0]));         // OK, any expression with the right value works, e.g. 2
}
```

An array destroyed element by element, with the wrong size, or through a pointer is reported, as is an array of arrays of raii structs. With the span struct rule's `require_span_immediately_after_array: true`, the array destroy function is still given the array itself, not its span.

#### Free struct
Finally, there is also a free struct supported where no rules apply to how the struct is used. The pod struct and raii struct work on a safety-first rule and the assumption that the compilers can handle copy elision and `static inline` functions effectively. The free struct is instead about complete freedom for the programmer with no restrictions, other than needing a function called `<void or any> <struct name>_init(<struct name>* self, ...);`. This allows users to optimize without restriction when needed. Here is an example:

```c
struct graphics_renderer
{
  // Add fields here
};
typedef struct graphics_renderer graphics_renderer;
typedef const char* graphics_error_msg;
int graphics_renderer_init(graphics_renderer* self, int arg1, float arg2, graphics_error_msg* result_msg);
```

It can be given an enforced initializer function name that makes user act more carefully such as `free_struct_creator_suffix: _init_manual_management_struct` by editing the config.

The init function sets up the free struct itself, so it may assign the struct's own members freely, even when a member is a raii struct. Normally reassigning a raii struct is forbidden since the old value would leak, but in the init function there is no old value yet.

#### Standard and 3rd party structs
Structs from the standard library or 3rd party libraries are unaffected. Therefore, to ensure that these are always properly initialized and their memory and resources are taken care of, they can be wrapped in pod and raii structs. 

### Interfaces rule
This rule standardizes how interfaces and their vtables are written. A vtable is a set of functions, and an interface pairs an object with the vtable of functions that work on it.

```yaml
interfaces:
  level: Error
  vtable_suffix: _vtable
  interface_suffix: _interface
  const_interface_suffix: _const_interface
  interface_must_have_fields_that_are_private_alternative: true
```

#### Vtables

A vtable struct is a struct whose name ends with `vtable_suffix`. It may only hold function pointers, directly or through a typedef. Every function pointer takes a `void*` or `const void*` as its first parameter: the object the function works on.

A vtable variable must be `static const`, and it must be initialized at declaration with braces, with an element for every function pointer. Designated elements and `&function` are fine. Every element must be an actual function, never `NULL` or `0`. A vtable is used through a pointer to it, so a vtable parameter passed by value is reported too.

When [struct resource management](#raii-and-struct-resource-management) is on, a vtable struct without a creator function (no make, pod or init function) is a free struct instead of an invalid one.

```c
struct shape_vtable
{
  double (*area)(const void* self);             // OK
  void (*scale)(void* self, double factor);     // OK
  int count;                                    // Triggers parser, not a function pointer
  void (*reset)(void);                          // Triggers parser, no void* or const void* first
};

static const shape_vtable_t circle_vtable = { circle_area, circle_scale };           // OK
static const shape_vtable_t square_vtable = { .area = square_area, .scale = NULL };  // Triggers parser, NULL is not a function
const shape_vtable_t shared_vtable = { circle_area, circle_scale };                  // Triggers parser, not static
static const shape_vtable_t partial_vtable = { circle_area };                        // Triggers parser, 'scale' is missing
```

#### Interfaces
An interface struct is a struct whose name ends with `interface_suffix`. It holds exactly two fields, in this order:
- a `void*` named `object`: the object the functions work on;
- a pointer to a const vtable struct named `vtable`, e.g. `const shape_vtable_t* vtable`. Any vtable struct will do.

A const interface, whose name ends with `const_interface_suffix`, gives read-only access to its object, so it holds a `const void* object` instead. The const suffix is checked first, so it may end with the interface suffix, as `_const_interface` ends with `_interface`.

With `interface_must_have_fields_that_are_private_alternative: true` and the [private alternative rule](#private-alternative-rule) on, both fields must be marked private. Only the interface's own functions can then reach the object and the vtable.

```c
struct shape_interface
{
  PRIVATE void* object;                   // OK
  PRIVATE const shape_vtable_t* vtable;   // OK
};

struct shape_const_interface
{
  PRIVATE const void* object;             // OK: a const interface holds a const void*
  PRIVATE const shape_vtable_t* vtable;   // OK
};

struct broken_interface
{
  PRIVATE void* self;                     // Triggers parser, must be named object
  PRIVATE shape_vtable_t* vtable;         // Triggers parser, the vtable must be const
  PRIVATE int count;                      // Triggers parser, only two fields are allowed
};
```

### Restricted malloc rule
This rule keeps all dynamic memory handling in one place. The memory functions `malloc`, `calloc`, `realloc`, `free`, `strdup`, `strndup`, `asprintf`, `getline` and `realpath` may only be used inside the functions listed in the config. All of them either allocate or free memory, some of them without it being obvious (`getline` grows the buffer it is given and `realpath` allocates when given `NULL`).

```yaml
restricted_malloc:
    level: Error
    list_of_allowed_malloc_functions:
      - memory_alloc
      - memory_alloc_array
      - memory_free
```

Only the name of the enclosing function is checked, it must match a list entry exactly (case sensitive), and there are no requirements on its return type or arguments. The list can be empty (`list_of_allowed_malloc_functions: []`), in which case the memory functions may not be used anywhere.

```c
void* memory_alloc(size_t size)
{
    return malloc(size);                        // OK, inside an allowed function
}

void memory_free(void* memory)
{
    free(memory);                               // OK
}

void foo(void)
{
    char* name = strdup("name");                // Not OK
    void* (*allocator)(size_t) = malloc;        // Not OK, taking the function as a pointer is a use too
    void* memory = memory_alloc(16);            // OK, calls the wrapper
    memory_free(memory);                        // OK
}
```

A macro that expands to one of the functions counts as using it where the macro is used. Uses inside system headers and `third_party_includes` folders are not checked.

### Single return rule
This rule requires every function to have a single return statement, which must be the last statement of the function. A function with one exit point is easier to follow, and cleanup code placed before the return can never be skipped.

```yaml
single_return:
    level: Warning
    allow_early_return: true
    require_return_for_void: true
```

```c
int sum(const int* values, int count)
{
  int result = 0;

  for (int i = 0; i < count; ++i)
  {
    if (values[i] < 0)
      return -1;      // Triggers parser, return inside a loop

    result += values[i];
  }

  return result;      // OK, the final return
}
```

With `allow_early_return: true`, guard clauses are allowed as well: an `if` without an `else`, placed directly in the function's top-level block, may return from its branch, either as its only statement or as the last statement of its block. They do not have to come before all other code. A macro used as a statement at the top level, such as `RETURN_IF_NULL(ptr);`, counts as an early return too.

```c
int get_value(const int* value)
{
  if (!value)
    return -1;        // OK with allow_early_return: true

  if (*value > 100)
  {
    log_error("too large");
    return -2;        // OK with allow_early_return: true
  }

  return *value;      // OK, the final return
}

int pick(int flag)
{
  if (flag)
    return 1;         // Triggers parser, an if with an else is not a guard clause
  else
    return 2;         // Triggers parser
}                     // Triggers parser, the function does not end with a return
```

With `require_return_for_void: true`, `void` functions must also end with `return;`. When it is `false`, the final return is optional for them, but any other return still has to follow the rules above.

### Strict switch rule
This rule requires every `switch` statement to have a `default` case, and every case (including `default`) to end with a `break` or `return`, so that a missing `break` can never silently fall through into the next case.

```yaml
strict_switch:
    level: Warning
```

```c
switch (value)
{
  case 1:
  case 2:             // OK, stacked labels share one body
    result = 1;
    break;
  case 3:             // Triggers parser, falls through into case 4
    result = 3;
  case 4:
    result += 4;
    break;
}                     // Triggers parser, no default case
```

A case also ends correctly with `continue` or `goto`, with a call to a function that never returns (such as `abort()` or `exit()`), with a block whose last statement ends the case, or with an `if`/`else` where both branches end the case. A nested `switch` as the last statement of a case is reported too, since its `break` only leaves the inner switch.

### Global variable rule
This rule enforces conventions for global (file scope) variables, which makes them easy to spot and limits who can change them. Each option can be enabled separately.

```yaml
global_variable:
    level: Warning
    require_prefix: true
    prefix: g_
    must_be_caps: false
    must_be_static: true
    must_be_const: false
    treat_local_static_as_global: false
    require_local_static_prefix: true
    local_static_prefix: s_
    forbid_static_in_header: true
```

- `require_prefix`: the name must start with `prefix`.
- `must_be_caps`: the name may not contain lowercase letters. When `require_prefix` is also enabled, only the part after the prefix is checked, so `g_MAX_SIZE` is fine.
- `must_be_static`: the variable must be `static`, so that it is only visible inside its own file. An `extern` declaration is therefore reported too.
- `must_be_const`: the variable must be `const`. A pointer must be const on every level, both the pointer itself and what it points to, e.g. `const int* const`. For arrays the elements must be const.
- `forbid_static_in_header`: a `static` global may not be declared in a header, since every file that includes the header would get its own separate copy of it. Together with `must_be_static` this means globals can not be declared in headers at all. When `treat_local_static_as_global` is `true`, this also covers static locals in functions defined in a header (e.g. `static inline` functions), which get a separate copy per file in the same way.
- `treat_local_static_as_global`: static variables inside functions follow the capital letter and const settings too. They keep their value between calls just like globals, but look like ordinary local variables where they are used.
- `require_local_static_prefix` and `local_static_prefix`: static locals must start with `local_static_prefix`, whether or not they are treated as globals, so that e.g. `g_` marks globals and `s_` marks static locals. When `require_local_static_prefix` is `false`, static locals treated as globals follow `require_prefix` and `prefix` like globals instead. Static locals are only left completely unchecked when both `treat_local_static_as_global` and `require_local_static_prefix` are `false`.

Two common setups are internal state and constants:

```c
// require_prefix: true, prefix: g_, must_be_static: true
static int g_counter = 0;               // OK
static int counter = 0;                 // Triggers parser, missing prefix
int g_exported = 0;                     // Triggers parser, not static

// must_be_caps: true, must_be_const: true
const int MAX_SIZE = 10;                // OK
const char* const APP_NAME = "app";     // OK
const char* NAME_POINTER = "name";      // Triggers parser, the pointer itself is not const
int COUNTER = 0;                        // Triggers parser, not const
```

Local variables that are not static are never checked. A variable declared more than once (e.g. `extern` in a header and the definition in the source file) is only checked once, at its definition.

### Reference pointer rule
A reference pointer is a pointer parameter that always points to a real object, so it can never be null. It is marked with a tag, a macro that the parser sees as an annotation. A [premade tag header](#tag-headers) can be used as is, and the macro name can be changed freely.

```yaml
reference_pointer:
    level: Warning
    disable_null_check_rule_for_reference_pointers: true
```

```c
void set_value(REF int* value, int new_value);

void example(REF int* reference, int* pointer)
{
  int local = 1;
  int numbers[3] = {1, 2, 3};

  set_value(&local, 2);         // OK, the address of a variable
  set_value(&numbers[1], 2);    // OK, the address of an array element
  set_value(reference, 2);      // OK, a reference passed on

  set_value(pointer, 2);        // Triggers parser, a normal pointer may be null
  set_value(NULL, 2);           // Triggers parser

  reference = pointer;          // Triggers parser, a reference may not be reassigned
  *reference = 5;               // OK, writing through a reference is fine
}
```

The argument for a reference parameter must be the address of an object: a variable, a field of a variable (`&point.x`), an array element (`&numbers[1]`) or a field reached through another reference (`&reference->field`). An array or a string literal is accepted too, since it decays to a pointer to its first element, and so is another reference parameter passed on. A pointer variable, `NULL`, a pointer returned from a function, or anything reached through a normal pointer is reported. A reference parameter may not be reassigned (`=`, `+=`, `++`, ...), and the tag may only be used on pointers.

A function pointer can be a reference too, so the function can call it without a null check. Its argument must then be a function, or another reference:

```c
typedef int (*math_func_t)(int a, int b);

int apply(REF math_func_t func, int a, int b)
{
  return func(a, b);            // OK, a reference is never null
}

apply(add, 1, 2);               // OK, a function is never null
apply(&add, 1, 2);              // OK
apply(NULL, 1, 2);              // Triggers parser, and so do 0, (void*)0 and (math_func_t)0
apply(maybe_null, 1, 2);        // Triggers parser, a normal function pointer may be null
```

With `disable_null_check_rule_for_reference_pointers: true`, the [null check rule](#null-check-rule) does not require a null check for reference parameters, since they can never be null.

The reference tag may only be written on parameters, of function declarations and definitions and of function pointer types, not on variables, struct fields, functions or typedefs.

A function pointer type follows the same rules as for the [movement tags](#function-pointers): a function assigned or passed to a function pointer must have the reference tag on the same parameters as the function pointer type, and the arguments of a call through a function pointer must be valid for its reference parameters.

```c
typedef void (*reader_t)(REF const Data* data);

void read(REF const Data* data);
void read_nullable(const Data* data);

reader_t reader = read;                     // OK
reader_t nullable = read_nullable;          // Triggers parser, read_nullable does not take a reference
reader(NULL);                               // Triggers parser, a reference can never be null
```

The tag works side by side with the [argument pointer movement rule](#argument-pointer-movement-rule), in any order, e.g. `void scale(MUTABLE REF float* value, float factor);` called as `scale(MUT(&value), 2.0f);`, or `void scale(borrows massive float* value, float factor);` called as `scale(lend(&value), 2.0f);` with the lowercase headers. The reference tag is not a movement tag, so a non-const reference still needs one of the movement tags when that rule is enabled.

### Function discard rule
This rule requires the result of every non-void function call to be used. An explicit cast to `void` documents that discarding the result is intentional and is allowed.

```yaml
function_discard:
  level: Warning
```

```c
int get_value(void);

get_value();             // Triggers parser
(void)get_value();       // OK: intentional discard
int value = get_value(); // OK: result is used
```

### Array struct rule
This safety feature requires arrays to be declared only as fields inside structs. It helps prevent accidentally passing the wrong array size through C's array-to-pointer decay: once an array becomes a pointer, a receiving function cannot know its length unless the caller also supplies it correctly.

```yaml
array_struct:
  level: Warning
  only_allow_array_passing_to_library_functions: true
  enforce_size_suffix_for_array_structs: true
  size_suffix_with_underscore: true
  flexible_size_name: flexible
  enforce_suffix_for_array_structs: true
  array_struct_suffix: _array
  struct_name_as_prefix: false
  use_naming_rules_on_one_array_field_structs_without_forbidding_public_arrays: false
```

With `only_allow_array_passing_to_library_functions: false`, the rule only checks that arrays are struct fields. With it set to `true`, an array field may only be passed directly to a function declared by the standard library or by a configured `third_party_includes` path. Calls to project functions are reported, so project code must expose a safer wrapper or another deliberate interface instead.

```c
struct image
{
  unsigned char pixels[1024];
};
typedef struct image image;

void project_process(unsigned char* pixels, size_t count);

void use_image(image* image)
{
  project_process(image->pixels, 1024); // Triggers parser when the option is true
  memcpy(image->pixels, image->pixels, sizeof(image->pixels)); // OK: standard library
}

int loose_buffer[4]; // Triggers parser: not a struct field
```

Standard-library functions are recognized from system headers. Third-party functions are recognized when their declarations come from a path listed in `third_party_includes`.

#### Naming array structs
The naming options only apply to a struct whose single field is an array. They are all off by default.

- `enforce_suffix_for_array_structs`: the name must end with `array_struct_suffix`.
- `enforce_size_suffix_for_array_structs`: the name must end with the element count of every dimension, e.g. `_5_10_15` for `int values[5][10][15]`. A flexible array, with no count, uses `flexible_size_name` instead, e.g. `_flexible` for `unsigned char bytes[]`. With `size_suffix_with_underscore: false`, the counts get no underscores, e.g. `51015`.
- When both are on, the array struct suffix comes first, e.g. `_array_5`.
- `struct_name_as_prefix`: when the elements are structs of the project, the name must start with the element struct's name, e.g. `point` for `point_t points[7]`. An anonymous element struct is named by its typedef. Elements that are structs from the standard library or a third party, or that are not structs at all, need no prefix.

An anonymous array struct is checked by its typedef name. With every option on:

```c
struct int_array_5 { int values[5]; };                  // OK
struct float_array_5_10 { float values[5][10]; };       // OK
struct bytes_array_flexible { unsigned char bytes[]; }; // OK
struct point_array_7 { point_t points[7]; };            // OK
struct time_array_2 { struct tm times[2]; };            // OK: struct tm is from the standard library
struct polygon { point_t points[4]; size_t count; };    // OK: more than one field

struct int_array_6 { int values[5]; };                  // Triggers parser, must end with '_array_5'
struct int_5_array { int values[5]; };                  // Triggers parser, the suffix comes before the count
struct path_array_7 { point_t points[7]; };             // Triggers parser, must start with 'point'
```

#### Naming rules without forbidding arrays
With `use_naming_rules_on_one_array_field_structs_without_forbidding_public_arrays: true`, arrays may be declared anywhere, e.g. as globals or local variables, and only the naming options above apply. This fits a project that wants consistent names for its array structs without moving every array into a struct. `only_allow_array_passing_to_library_functions` still applies to array fields. At least one of the naming options must be on, or the config is reported as invalid, since the rule would then check nothing.

```c
int loose_buffer[4];                                    // OK with the option
struct int_values { int values[5]; };                   // Triggers parser, must end with '_array_5'
```

### Span struct rule
This rule standardizes span types that pair a pointer with an element count. Any struct whose name ends with `span_struct_suffix` or `const_span_struct_suffix` must contain exactly two fields: a pointer named `data` and a `size_t` named `size`.

```yaml
span_struct:
  level: Warning
  only_allow_array_passing_to_library_functions_and_spans: true
  span_struct_suffix: _span
  const_span_struct_suffix: _const_span
  allow_spans_to_be_given_fewer_elements_than_their_size: false
  require_span_immediately_after_array: false
  only_allow_span_data_passing_in_one_line_static_functions: false
  allow_pod_span_to_be_initialized_manually_if_static: false
```

A const span is a read-only view: it works exactly like a span, but holds a pointer to const data and is named with `const_span_struct_suffix`. A span may change the data it points to, so its pointer may not point to const data, and a const span may not, so its pointer must. A const span may still view data that can change, e.g. a normal array. The const suffix is checked first, so it may end with the span suffix, as `_const_span` ends with `_span`.

```c
struct int_span { int* data; size_t size; };                    // OK
struct int_const_span { const int* data; size_t size; };        // OK
struct const_data_span { const int* data; size_t size; };       // Triggers parser, const data needs a const span
struct mutable_data_const_span { int* data; size_t size; };     // Triggers parser, a const span needs const data

const int constants[2] = { 4, 5 };
int_const_span_t view = { constants, 2 };                       // OK
```

Only the data decides the kind of span: a const pointer to non-const data, `int* const data`, is a normal span.

A span variable must be initialized at declaration. When an array is used in its initializer, the `size` value must be a constant expression equal to the array's element count. When struct resource management is enabled, the same count is required when an array is passed to the span's pod creator: a function named `<span struct name><pod struct creator suffix>` that returns the span, e.g. `int_span_t int_span_pod(int* data, size_t size)`. A function that only has a name like it, e.g. one ending with the pod suffix for a span that does not exist, or one that returns something else, is a normal project function. With `allow_spans_to_be_given_fewer_elements_than_their_size: true`, the count may also be smaller than the array's element count, so a span can cover only the start of an array, e.g. `{ values, 2 }` for a 3 element array. It may never be larger, and it must still be a constant so it can be checked.

A span can also start inside an array, at `&array[index]` or `array + index`. Its count is then checked against the elements left from that index, so `{ &values[3], 7 }` is right for a 10 element array. The index must be a constant within the array, and an index past its end is reported.

```c
int values[10];
int_span_t tail = { &values[3], 7 };           // OK, the 7 elements left from index 3
int_span_t rest = { values + 3, 7 };           // OK, the same
int_span_t wrong = { &values[3], 10 };         // Triggers parser, only 7 elements are left
int_span_t outside = { &values[12], 1 };       // Triggers parser, index 12 is outside of the array
```

When [struct resource management](#raii-and-struct-resource-management) is enabled and a span has a pod creator, the span is a pod struct, and a pod struct may only be initialized from a function or another struct variable. A span with static storage duration can't call its pod creator, though: a global or `static` initializer must be a constant expression. With `allow_pod_span_to_be_initialized_manually_if_static: true`, a span or const span with static storage duration may therefore be initialized with braces. That covers globals, `static` globals and `static` locals. The braces are still checked like any span initializer, e.g. the count of the array. Other spans, other pod structs, and brace-built spans passed as arguments or returned are still reported.

```c
int g_values[4];
static int_span_t s_span = { g_values, 4 };      // OK with the option, static storage duration
static int_span_t s_long = { g_values, 5 };      // Triggers parser, the count is still checked
static pos_t s_origin = { 0, 0 };                // Triggers parser, not a span

void example(void)
{
  static int_span_t local = { g_values, 4 };     // OK with the option, a static local
  int_span_t span = { g_values, 4 };             // Triggers parser, not static, use int_span_pod
}
```

With `require_span_immediately_after_array: true`, every array outside of a struct, local, static or global, must be followed right away by a span or const span variable holding the whole array: for a local array in the next statement, for a global in the next declaration of the file. The span must start at the beginning of the array, `values` or `&values[0]`, and hold its full size, with an initializer or the span's pod creator. After that, the span is the one way to reach the array: the array may not be named again, except in the span's initializer, in code that is never evaluated, `sizeof`, `_Alignof` and `__typeof__`, and as the first argument of the array destroy function of an [array of raii structs](#arrays-of-raii-structs), e.g. `int_vector_destroy_array(vectors, 2)`, when that function has the exact signature `void <struct name><suffix>(<struct name>* self, size_t n)` and the struct is a raii struct, with its raii creator declared before the call, since the struct resource management rule tracks the array itself. Elements are reached through the span, e.g. `values_span.data[1]`, or through the span's functions, and a matrix through its span of rows, e.g. `matrix_span.data[1][2]`. An array without its span only gets the missing span reported, not every use of it.

```c
int values[3] = { 1, 2, 3 };
int_span_t values_span = { values, 3 };          // OK

int a[2], b[3];
int_span_t a_span = { a, 2 }, b_span = { b, 3 }; // OK, one declaration for both

int lonely[3];                                   // Triggers parser, no span follows
int count = 0;

values_span.data[1] = 4;                         // OK, through the span
size_t bytes = sizeof(values);                   // OK, never evaluated
values[1] = 4;                                   // Triggers parser, use values_span instead
int* pointer = values;                           // Triggers parser, same
```

With `only_allow_span_data_passing_in_one_line_static_functions: true`, the data of a span, including a pointer returned by a function named after it, may only be passed on inside a static function whose body is a single statement: one expression or one `return`, and not a comma expression. Every place that hands raw data to an unsafe function, e.g. from the standard library, is then a small wrapper that takes spans and is easy to review on its own, and all other code passes the spans themselves. Combined with `only_allow_array_passing_to_library_functions_and_spans`, a wrapper may still only hand the data to standard-library and third-party functions.

```c
static void int_span_copy(int_span_t destination, int_span_t source)
{
  memcpy(destination.data, source.data, source.size * sizeof(int));   // OK, a one-statement static wrapper
}

void example(int_span_t a, int_span_t b)
{
  int_span_copy(a, b);                                                // OK, spans all the way
  memcpy(a.data, b.data, 3 * sizeof(int));                            // Triggers parser, raw data outside a wrapper
}
```

With `only_allow_array_passing_to_library_functions_and_spans: false`, only the span shape and initialization/count rules apply. With it set to `true`, arrays may only be passed directly to standard-library functions, configured third-party functions, span pod creators, or struct initializers. This prevents array-to-pointer decay from hiding the array length and helps avoid passing an incorrect size to a function. The data of a span or const span is the array it views, so it is restricted too: `span.data`, `span->data` and pointer arithmetic on it such as `span.data + 1` may only be passed to standard-library and third-party functions. A project function takes the span itself instead of its data and size separately, and not even a pod creator may get the data, since the size given with it could not be checked. The same goes for the pointer returned by a function named after the span, e.g. a getter `int* int_span_data(const int_span_t* self)`, when it is passed on directly. A pointer to a single element, e.g. `&span.data[i]`, reading or writing elements, and a pointer to the span itself are not restricted.

```c
void process_ints(int* data, size_t size);
void process_span(int_span_t span);

process_span(span);                                     // OK
memcpy(span.data, other, 3 * sizeof(int));              // OK, standard library
edit_value(&span.data[0]);                              // OK, a single element
process_ints(span.data, span.size);                     // Triggers parser, pass the span instead
process_ints(int_span_data(&span), 3);                  // Triggers parser, the getter returns the span's data
int_span_t rest = int_span_pod(span.data + 1, 2);       // Triggers parser, the size can not be checked
```

Making a smaller span from a span belongs in the span's own functions, where the data can be combined with a checked size, e.g. with an initializer. Together with the [private alternative rule](#private-alternative-rule) and private `data` and `size` fields, only those functions can reach them at all.

```c
struct integer_span
{
  int* data;
  size_t size;
};
typedef struct integer_span integer_span_t;

integer_span_t integer_span_pod(int* data, size_t size)
{
  return (integer_span_t){ data , size };
}

void foo()
{
  int array[110] = { 0 };
  integer_span_t array_span = integer_span_pod(array, 110);
  use_integers(&array_span);
}
```

### Const field rule
This rule forbids struct and union fields that are const themselves. A const field makes the whole struct impossible to assign or to set up in place after its declaration. For example, a make function can't fill in a struct through a pointer. A pointer field may still point to const data, since the field itself can still be assigned.

```yaml
const_field:
  level: Warning
```

```c
struct values
{
  const int* pointer_to_const;      // OK: only the data it points to is const
  const char* name;                 // OK
  int* const* to_const_pointer;     // OK: the field itself is not const

  const int constant;               // Triggers parser
  int* const const_pointer;         // Triggers parser, only the data it points to may be const
  const int* const both;            // Triggers parser
  const int constants[3];           // Triggers parser, an array of const elements
  const_int_t typedef_constant;     // Triggers parser, const through a typedef
};
```

### No goto rule
This rule forbids `goto` statements. That covers `goto label;` and the computed `goto *address;` of GNU C, also when hidden in a macro. Loops, `break`, `continue` and `return` cover the same needs in a structured way. Labels on their own are not reported, only the jumps to them.

```yaml
no_goto:
  level: Error
```

```c
int scale(int value)
{
  if (value < 0)
    goto done;        // Triggers parser

  value = value * 2;

done:                 // OK: a label is not a jump
  return value;
}
```

### Disable section
Rules can be temporarily and locally disabled with a comment saying `// WorkshopC off` and then `// WorkshopC on`.

```c
// WorkshopC off
// Reason: The enum values are dictated by a legacy protocol
enum Color { // Does not trigger enum rule
  RED,
  GREEN,
  BLUE
};
// WorkshopC on
```

A suppression only applies to the file it is written in, so a header that turns WorkshopC off does not affect the files that include it. Every `// WorkshopC off` must be turned back on with `// WorkshopC on` in the same file, in headers and source files alike, since a forgotten `on` would silently disable every rule for the rest of the file. This check is always enabled and reported as an error, together with an `on` without a preceding `off`, and an `off` while already turned off (suppressions can not be nested, the first `on` ends the suppression).

#### Suppression reason rule
Every suppression should say why it exists. When `suppression_reason_rule` is enabled, each `// WorkshopC off` comment must be followed **on the very next line** by a comment that starts with `Reason: ` and gives a non-empty reason. A `/* Reason: ... */` block comment is also accepted.

```yaml
rules:
  suppression_reason_rule:
    level: Error
```

```c
// WorkshopC off
// Reason: The enum values are dictated by a legacy protocol
enum Color { RED, GREEN, BLUE }; // Good

// WorkshopC on
// WorkshopC off
enum Shape { CIRCLE, SQUARE };   // Bad: no reason given, the rule reports the 'WorkshopC off' line
// WorkshopC on
```

Details:
- `Reason: ` is case sensitive, and a blank line between the two comments does not count.
- Only the `WorkshopC off` that opens a suppressed range needs a reason. A repeated `off` inside an already suppressed range is ignored.
- The check covers the analyzed file and every project header it includes, but not system headers or `third_party_includes` folders.
- This rule reports on the `WorkshopC off` line itself, so it can not be silenced by the suppression it is checking.

### Adjust code for parser
The parser runs with `WORKSHOPC_PARSING` defined as a macro. This allows users to create `#ifdef` and `#ifndef` guards to adjust code for parsing and usage. The tag headers use it to turn the tags into annotations only while WorkshopC parses the code:

```c
#ifdef WORKSHOPC_PARSING
#define PRIVATE __attribute__((annotate("workshopc_private_field")))
#else
#define PRIVATE
#endif
```

It can also hide code from the parser that clang can not handle, e.g. an extension of another compiler:

```c
#ifndef WORKSHOPC_PARSING
#pragma some_vendor_specific_pragma
#endif
```

Code hidden this way is not checked at all, so to only silence a rule, prefer [disabling the section](#disable-section) with a reason.

## WorkshopC Build System Documentation

### Overview

This project supports two build workflows:

1. **Windows Python build script** (developer convenience)
   - Fast setup for MSYS2-based LLVM/Clang environments
   - Assumes a known toolchain layout

2. **Cross-platform CMake build** (official build system)
   - Works on Windows, Linux, macOS
   - Requires user-provided LLVM/Clang installation or system packages
   - No hardcoded paths

### Windows Developer Workflow (Python Script)

#### Purpose

The Python script is a convenience wrapper for developers working on Windows using MSYS2 LLVM/Clang.

It is **NOT** a portable build system. It assumes:

- MSYS2 is installed in `C:/msys64`
- LLVM + Clang are installed via MSYS2 UCRT64 packages
- Ninja is available in `PATH`
- CMake is installed and available in `PATH`

#### What it does

After building, you will get the `workshopc` executable in the `release/` folder. See [How to use](#how-to-use) for running it, its output and its exit codes.

#### Notes

- The compilation database of the project being analyzed is typically located in its CMake build directory (e.g., `build/`)
- It's generated automatically by CMake when configured with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
- Output is intended for use in CI pipelines or pre-commit checks, see the [exit codes](#how-to-use)
- If the compilation database cannot be found, the tool will fail with an error message

#### Requirements

##### 1. Install MSYS2

https://www.msys2.org/

##### 2. Install required packages (from the UCRT64 shell)

```bash
pacman -S mingw-w64-ucrt-x86_64-toolchain
pacman -S mingw-w64-ucrt-x86_64-llvm
pacman -S mingw-w64-ucrt-x86_64-clang
pacman -S mingw-w64-ucrt-x86_64-ninja
pacman -S mingw-w64-ucrt-x86_64-cmake
```

##### 3. Install Python on Windows

```bash
python --version
```

#### Running the build

From PowerShell or CMD:

```bash
python windows_rebuild.py
```

The script will:

1. Delete the `build/` directory
2. Configure CMake using Ninja (generates `compile_commands.json` in the build directory)
3. Build the project
4. Copy the executable and configuration files to the `release/` folder

After building, you can run the tool using the compilation database from the build directory.

#### What the Python script assumes

The script hardcodes:

```
LLVM_DIR  = C:/msys64/ucrt64/lib/cmake/llvm
Clang_DIR = C:/msys64/ucrt64/lib/cmake/clang
```

This means:

- It only works with MSYS2 LLVM
- It does **NOT** auto-detect toolchains
- It avoids system CMake guessing

#### Why this script is NOT portable

This is intentional:

- Windows LLVM setups differ (MSYS2, vcpkg, LLVM installer, WSL)
- MSYS2 environment variables can break builds
- Mixing environments causes LLVM/Clang linking issues

This script enforces a single known-good configuration.

### Cross-Platform CMake Workflow (Official Build System)

#### Purpose

The CMake configuration is designed to be:

- Portable
- Environment-agnostic
- Compatible with Linux, macOS, Windows
- Independent of MSYS2 or any specific package manager

#### Requirements

##### 1. CMake >= 3.20

https://cmake.org/download/

##### 2. C++ Compiler

Supported compilers:

- GCC
- Clang
- MSVC

##### 3. LLVM + Clang development packages

Must provide:

- LLVMConfig.cmake
- ClangConfig.cmake

Examples:

###### Linux

```bash
sudo apt install llvm clang libclang-dev
```

###### macOS

```bash
brew install llvm
```

###### Windows

https://llvm.org/

#### Configuring the project

##### Basic configuration

```bash
cmake -S . -B build
```

##### Explicit LLVM paths

```bash
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DClang_DIR=/path/to/clang/lib/cmake/clang
```

#### Building

```bash
cmake --build build
```

This generates:
- The `workshopc` executable
- `compile_commands.json` in the build directory (required by the tool)

#### Running the tool

After building, use the tool with the compilation database from the build directory:

```bash
# Example: analyze a test file
./build/workshopc --config tests/cases/enum/enum.workshopc.yaml -p build-tests/ tests/cases/enum/enum.c
```

The tool requires access to the compilation database to understand compiler flags and include paths. The database must describe the code being analyzed, not the tool itself, so the test files have their own (see below).

Clang's builtin headers (`stddef.h`, `mm_malloc.h`, ...) are looked up next to the executable first. If they are not there, the tool falls back to the resource directory of the clang it was built against (`<LLVM lib dir>/clang/<version>`, recorded at build time and printed as `Clang resource dir` when configuring). Without this, including a header such as `<stdlib.h>` fails with `'mm_malloc.h' file not found` on MSYS2.

#### Running the tests

```bash
python run_tests.py
```

Every test case is a folder in `tests/cases/` holding three files with the folder's name, e.g. `tests/cases/struct_usage/`:
- `struct_usage.c`: the code to analyze, with a `// good` or `// bad: reason` comment on the lines that matter,
- `struct_usage.workshopc.yaml`: the config it is analyzed with,
- `struct_usage.expected.txt`: every diagnostic the tool must report, one per line as `level: message [code]`, in any order.

Headers shared by the tests are in `tests/headers/`, and `tests/external/` stands in for third party code (every test config lists `external/` in `third_party_includes`). The tests include them as `"headers/..."` and `"external/..."`, which works from every test case folder since `tests/` is on the include path.

The test files are a standalone project described by `tests/CMakeLists.txt`. They are never built: `run_tests.py` only **configures** that project into `build-tests/` (with clang and Ninja), which writes a `compile_commands.json` for the test files, and then passes that directory to the tool for every test. New test case folders are picked up automatically, since the project is configured again on every run.

The expected files list every diagnostic with its code. One more test writes the diagnostics of `tests/cases/suppression_balance/suppression_balance.c` to a text, a JSON and a SARIF file in a single `--quiet` run, into `tests/output/` (ignored by git), and checks that nothing was printed and that all three files hold exactly the expected diagnostics. Another runs the same file with `--warnings-as-errors`, and checks that every warning is reported as an error on the terminal and in a JSON file, with no warnings left in the counts and an exit code of `1`.

#### What makes this CMake portable

##### 1. No hardcoded paths

It does **NOT** assume:

- MSYS2
- Windows layout
- Specific install directories

Instead it uses:

```cmake
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
```

##### 2. Uses LLVM’s official CMake targets

This ensures:

- Compatibility across LLVM versions
- Works with system packages
- No manual `.a` or `.lib` linking

##### 3. Cross-platform compile definitions

Safe macros:

- `NOMINMAX`
- `_CRT_SECURE_NO_WARNINGS`
- `_FILE_OFFSET_BITS=64`

LLVM-required macros:

- `__STDC_CONSTANT_MACROS`
- `__STDC_FORMAT_MACROS`
- `__STDC_LIMIT_MACROS`

##### 4. Minimal platform-specific logic

```cmake
if (MINGW)
    target_link_libraries(workshopc PRIVATE ws2_32 version bcrypt)
endif()
```

##### 5. Proper LLVM component linking

Instead of manually listing libraries, we use:

```cmake
llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
)
```

This ensures:

- Correct dependency resolution
- No duplicate symbols
- Works across LLVM builds

### Summary

- Python script = developer shortcut (Windows only)
- CMake = real portable build system
- Users only need a working LLVM + CMake setup
- You do **NOT** need to support all environments in Python
- CMake is the production-grade interface

#### Python script = opinionated convenience tool

- Assumes MSYS2
- Hardcoded paths
- Optimized for your environment only
- Not portable

#### CMake = universal build contract

- Must work anywhere LLVM is installed correctly
- No assumptions about environment
- Minimal platform-specific logic
- Official build system for the project

## TODO

For Beta V1 it shall
- Verify build for Linux
- Reorganize README and documentation
- Use githubs release system to make linux and windows releases
- Add config presets (e.g. embedded, safety, exisiting_project, new_project, opinionated, strict)
- Override third party includes with arg?

For Beta V1.1 it shall
- Add LSP support
- Add a python script for installing dependencies, that shall work on windows/linux/iOS
