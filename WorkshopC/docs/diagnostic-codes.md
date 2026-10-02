[← Previous: Preset rationale](preset-rationale.md)

# Diagnostic codes

Every diagnostic that WorkshopC reports ends with a code, e.g. `[CCW0801]`, which identifies exactly which check reported it. This page explains how the codes are built and lists every code with the rule it belongs to.

## Contents

- [Code format](#code-format)
- [Where codes appear](#where-codes-appear)
- [All codes](#all-codes)

## Code format

Codes have the form `CCWrrcc`:

| Part | Meaning |
|------|---------|
| `CCW` | CrucibleC WorkshopC |
| `rr` | The rule, numbered in the order of the default config. `00` is WorkshopC itself, e.g. its suppression checks. |
| `cc` | The check within the rule, starting at `01`. |

A code says nothing about severity, since the level of every rule is set in the config. The same code can be a warning in one project and an error in another.

Codes are stable. They are never renumbered or reused: a removed check leaves its code unused, and a new check gets the next free number of its rule. Scripts, CI filters and documentation can rely on them.

## Where codes appear

- **Terminal and text files:** at the end of each diagnostic line, in brackets. See [reading the output](usage.md#reading-the-output).
- **JSON files:** as `code`, together with a readable `name`, e.g. `use-after-move`.
- **SARIF files:** as the `ruleId` of each result. The run's rule list describes every code with its name and a short description.

Suppressions work on sections of code, not on codes: `// WorkshopC off` turns off every rule for the lines it covers. See [Suppressing rules](rules.md#suppressing-rules).

## All codes

The group rows link to the rule that reports the codes below them. `CCW0001`, `CCW0003` and `CCW0004` are always checked and reported as errors. Every other code belongs to a rule that is configured in the config.

| Code | Name | Reported when |
|------|------|---------------|
| | **00 — [WorkshopC suppressions](rules.md#suppressing-rules)** | |
| `CCW0001` | `suppression-not-turned-back-on` | 'WorkshopC off' is never turned back on with 'WorkshopC on' in the same file |
| `CCW0002` | `suppression-missing-reason` | 'WorkshopC off' is not followed on the next line by a comment starting with 'Reason: ' |
| `CCW0003` | `suppression-on-without-off` | 'WorkshopC on' without a preceding 'WorkshopC off' in the same file |
| `CCW0004` | `suppression-nested` | 'WorkshopC off' while already turned off, suppressions can not be nested |
| | **01 — [Enum](rules.md#enum-rule)** | |
| `CCW0101` | `enum-not-allowed` | Enums are not allowed |
| `CCW0102` | `enum-missing-typedef` | An enum must have a typedef |
| `CCW0103` | `enum-init-not-member` | An enum variable must be initialized with a member of its enum or an explicit cast |
| `CCW0104` | `enum-assignment-not-member` | An enum value must be assigned a member of its enum or an explicit cast |
| `CCW0105` | `enum-arithmetic` | An enum value may not be modified with arithmetic (compound assignment, ++ or --) |
| `CCW0106` | `enum-argument-not-member` | An enum argument must be a member of its enum or an explicit cast |
| | **02 — [Private](rules.md#private-rule)** | |
| `CCW0201` | `private-access-outside-function` | A private field is accessed outside of any function |
| `CCW0202` | `private-access` | A private field is accessed from a function that is not a static getter or setter in a .c file |
| | **03 — [Private alternative](rules.md#private-alternative-rule)** | |
| `CCW0301` | `private-alternative-access-outside-function` | A private field is accessed outside of any function |
| `CCW0302` | `private-alternative-access` | A private field is accessed from a function that is not named after its struct and does not take 'self' |
| | **04 — [Function pointer](rules.md#function-pointer-rule)** | |
| `CCW0401` | `function-pointer-missing-typedef` | A function pointer variable, parameter, struct field or return type is declared without a typedef |
| | **05 — [Typedef struct](rules.md#typedef-struct-rule)** | |
| `CCW0501` | `struct-missing-typedef` | A struct must have a typedef |
| | **06 — [Assignment](rules.md#assignment-rule)** | |
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
| | **07 — [Prefix namespace](rules.md#prefix-namespace-rule)** | |
| `CCW0701` | `missing-namespace-prefix` | A name does not start with the namespace prefix of its file |
| `CCW0702` | `missing-include-guard` | A header does not have the expected include guard |
| | **08 — [Null check](rules.md#null-check-rule)** | |
| `CCW0801` | `dereference-before-null-check` | A pointer parameter is dereferenced before it is checked for null |
| | **09 — [Argument pointer movement](rules.md#argument-pointer-movement-rule)** | |
| `CCW0901` | `movement-tag-missing` | A non-const pointer parameter has no movement attribute |
| `CCW0902` | `movement-tag-mismatch` | The movement attribute of a parameter differs between declaration and definition |
| `CCW0903` | `movement-tag-not-on-parameter` | A movement attribute is used on something other than a parameter of a function or function pointer type |
| `CCW0904` | `borrowed-pointer-moved` | A modify or out parameter is moved to another function |
| `CCW0905` | `operator-on-untagged-parameter` | A callsite operator is used for a parameter without a movement attribute |
| `CCW0906` | `operator-disabled` | A callsite operator is used while that kind of callsite operator is disabled |
| `CCW0907` | `operator-missing` | A callsite operator is missing for a parameter with a movement attribute |
| `CCW0908` | `use-after-move` | A pointer is used after it may have been moved |
| `CCW0909` | `function-pointer-movement-tag-mismatch` | A function or function pointer with other movement tags is assigned or passed to a function pointer, or called with it through a conditional |
| | **10 — [Struct resource management: struct definitions](rules.md#struct-resource-management-rule-raii)** | |
| `CCW1001` | `struct-invalid-constructor` | A struct does not have exactly one pod, raii or free constructor function |
| `CCW1002` | `struct-missing-destroy` | A raii struct is missing its destroy function |
| `CCW1003` | `struct-missing-copy` | A raii struct is missing its copy function |
| `CCW1004` | `struct-missing-move` | A raii struct is missing its move function |
| `CCW1005` | `struct-missing-return` | A raii struct is missing its return function |
| `CCW1006` | `struct-missing-valid` | A raii struct is missing its validation function |
| | **11 — [Struct resource management: initialization and assignment](rules.md#struct-resource-management-rule-raii)** | |
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
| | **12 — [Struct resource management: destruction](rules.md#struct-resource-management-rule-raii)** | |
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
| | **13 — [Struct resource management: return values](rules.md#struct-resource-management-rule-raii)** | |
| `CCW1301` | `raii-return-function-outside-return` | A raii return function is used outside of a return statement |
| `CCW1302` | `pod-return` | A function returning a pod struct does not return a function return value or another struct variable |
| `CCW1303` | `raii-return` | A function returning a raii struct does not return a function call |
| `CCW1304` | `raii-return-member-access` | A member is accessed directly on a raii struct returned by a function |
| `CCW1305` | `raii-return-discarded` | A raii struct returned by a function is discarded |
| | **14 — [Interfaces: vtables](rules.md#vtables)** | |
| `CCW1401` | `vtable-field-not-function-pointer` | A field of a vtable struct is not a function pointer |
| `CCW1402` | `vtable-function-missing-object-parameter` | A function pointer in a vtable struct does not take a `void*` or `const void*` as its first parameter |
| `CCW1403` | `vtable-not-static-const` | A vtable variable is not declared `static const` |
| `CCW1404` | `vtable-not-initialized` | A vtable variable is not initialized at declaration with braces |
| `CCW1405` | `vtable-missing-function` | A vtable initializer does not give an element for every function pointer of the vtable struct |
| `CCW1406` | `vtable-element-not-function` | A vtable initializer element is not a function, e.g. `NULL` or `0` |
| | **15 — [Interfaces: interfaces](rules.md#interfaces)** | |
| `CCW1501` | `interface-field-count` | An interface struct does not have exactly two fields, an object and a vtable |
| `CCW1502` | `interface-object-field` | The first field of an interface struct is not a `void*` named `object`, or a `const void*` for a const interface |
| `CCW1503` | `interface-vtable-field` | The second field of an interface struct is not a pointer to a const vtable struct named `vtable` |
| `CCW1504` | `interface-field-not-private` | A field of an interface struct is not marked private for the private alternative rule |
| | **16 — [Restricted malloc](rules.md#restricted-malloc-rule)** | |
| `CCW1601` | `restricted-malloc` | A memory function is used outside of the allowed functions |
| | **17 — [Single return](rules.md#single-return-rule)** | |
| `CCW1701` | `multiple-returns` | A function has more than a single return |
| `CCW1702` | `missing-final-return` | A function does not end with a return statement |
| | **18 — [Strict switch](rules.md#strict-switch-rule)** | |
| `CCW1801` | `switch-fallthrough` | A switch case does not end with a break or return |
| `CCW1802` | `switch-missing-default` | A switch statement has no default case |
| | **19 — [Global variable](rules.md#global-variable-rule)** | |
| `CCW1901` | `global-missing-prefix` | A global variable does not start with the required prefix |
| `CCW1902` | `global-not-capitals` | A global variable is not written in capital letters |
| `CCW1903` | `global-not-static` | A global variable is not static |
| `CCW1904` | `global-not-const` | A global variable is not const all the way through |
| `CCW1905` | `global-static-in-header` | A static variable is defined in a header |
| | **20 — [Reference pointer](rules.md#reference-pointer-rule)** | |
| `CCW2001` | `reference-invalid-argument` | The argument for a reference parameter is not the address of an object or another reference |
| `CCW2002` | `reference-reassigned` | A reference pointer is reassigned |
| `CCW2003` | `reference-tag-on-non-pointer` | A reference tag is used on a parameter that is not a pointer |
| `CCW2004` | `reference-tag-not-on-parameter` | A reference tag is used on something other than a parameter of a function or function pointer type |
| `CCW2005` | `function-pointer-reference-tag-mismatch` | A function or function pointer with other reference tags is assigned or passed to a function pointer, or called with it through a conditional |
| `CCW2006` | `reference-declaration-tag-mismatch` | A reference tag differs between declarations of the same function |
| | **21 — [Function discard](rules.md#function-discard-rule)** | |
| `CCW2101` | `function-return-discarded` | A non-void function return value is discarded |
| | **22 — [Array struct](rules.md#array-struct-rule)** | |
| `CCW2201` | `array-outside-struct` | An array is declared outside a struct field |
| `CCW2202` | `array-passed-to-non-library-function` | A struct array field is passed to a non-standard-library and non-third-party function |
| `CCW2203` | `array-struct-name-ending` | A struct holding only an array does not end with the array struct suffix and/or the element counts of the array |
| `CCW2204` | `array-struct-name-prefix` | A struct holding only an array of project structs does not start with the name of the element struct |
| | **23 — [Span struct](rules.md#span-struct-rule)** | |
| `CCW2301` | `span-invalid-definition` | A span struct does not contain only `data` and `size` fields |
| `CCW2302` | `span-uninitialized` | A span struct is not initialized at declaration |
| `CCW2303` | `span-array-count` | A span array initializer or pod creator does not use the array element count |
| `CCW2304` | `span-array-passed-to-non-library-function` | An array is passed to a function that is not a standard-library, third-party, span, or pod function |
| `CCW2305` | `span-const-mismatch` | A span struct holds a pointer to const data, or a const span struct a pointer to non-const data |
| `CCW2306` | `span-data-passed-to-non-library-function` | The data of a span, or a pointer returned by a function named after it, is passed to a function that is not a standard-library or third-party function |
| `CCW2307` | `span-missing-after-array` | An array outside of a struct is not followed right away by a span variable holding the whole array |
| `CCW2308` | `span-data-outside-wrapper` | The data of a span is passed on outside of a static function with a single statement |
| `CCW2309` | `array-used-after-span` | An array that has a span is used directly instead of through its span |
| | **24 — [Const field](rules.md#const-field-rule)** | |
| `CCW2401` | `const-field` | A struct field is const itself, e.g. `const int` or `int* const`, a pointer to const is allowed |
| | **25 — [No goto](rules.md#no-goto-rule)** | |
| `CCW2501` | `goto-not-allowed` | A goto statement is used, including a computed goto |

---

[Next: Building and testing →](building.md) · [Back to README](../README.md)
