# WorkshopC
A project for parsing C code and generating tips for writing safer code or adhering to a certain style.

## Contents
- [How to use](#how-to-use)
- [Config behavior](#config-behavior)
  - [Enum rule](#enum-rule)
  - [Private rule](#private-rule)
  - [Function pointer rule](#function-pointer-rule)
  - [Typedef struct rule](#typedef-struct-rule)
  - [Assignment rule](#assignment-rule)
  - [Prefix namespace rule](#prefix-namespace-rule)
  - [Null check rule](#null-check-rule)
  - [Argument pointer movement rule](#argument-pointer-movement-rule)
  - [Disable section](#disable-section)
  - [Adjust code for parser](#adjust-code-for-parser)
- [WorkshopC Build System Documentation](#workshopc-build-system-documentation)
  - [Overview](#overview)
  - [Windows Developer Workflow (Python Script)](#windows-developer-workflow-python-script)
  - [Cross-Platform CMake Workflow (Official Build System)](#cross-platform-cmake-workflow-official-build-system)
  - [Summary](#summary)
- [TODO](#todo)

## How to use
The tool requires three arguments:

```bash
workshopc <config.yaml> <source-file.c> <compdb-dir>
```

**Arguments:**
- `<config.yaml>` — Path to the YAML rule configuration file
- `<source-file.c>` — Path to the C source file to analyze
- `<compdb-dir>` — Directory containing `compile_commands.json` (typically the CMake build directory)

Example:
```bash
workshopc workshopc.config.yaml tests/enum.c build/
```

[Here is a premade config.yaml file ready for use as is and provide a base to easily edit](./default/workshopc.config.yaml).

## Config behavior
The config is a yaml file that must have a certain format, as shown in the default (linked above). It first sets a list of third party folders which become unaffected by the parser, and then provides multiple individual rules can be set to `Off`, `Warning`, or `Error` in their `level` setting. This way the user can selectively enable only the rules that help their project.

### Enum rule
This rule gets triggered when an `enum` is defined. An `enum` argument can take any kind of integer which easily creates bugs and mistakes. 

```c
enum Color { // Triggers parser
    RED,
    GREEN,
    BLUE
};
```

Instead a user can create a set of static or extern struct objects, or declare a struct and then typedef its pointer as the "enum".

```c
struct Color {
  int value;
};
typedef struct Color Color;
extern Color Color_Red;       // Could also be static
extern Color Color_Green;     // Could also be function returning pointer to static
extern Color Color_Blue;      // Could also be #define COLOR_BLUE ((const Color){ .value = 3 })

// Alternatively

struct ColorInternal;
typedef struct ColorInternal* Color;

extern Color Color_Red;
extern Color Color_Green;
extern Color Color_Blue;
```

### Private rule
This rule enforces privacy by forbidding access to a member of a given name (e.g. `_private`) unless it is accessed by certain static functions. 

For this config
```yaml
private:
    level: Error
    private_field: _private
    setter_contains: pset
    getter_contains: pget
```

this will trigger an error.

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
  int weight = color->weight; // OK
  int r = color->_private.r;  // Triggers Error
}
```

Instead the user must defined static getter and setter functions, ideally in the source file. The functions can be named anything as long as they include the config defined part.

```c
static int pget_red(const Color* const color)
{
  return color->_private.r;
}

static void pset_red(Color* const color, int new_value)
{
  color->_private.r = new_value;
}
```

If the private field is a defined (instead of anonymous as in above example) struct then only two accessors are needed.

```c
static const ColorPrivate* pget(const Color* const color)
{
  return color->_private;
}

static ColorPrivate* pset(Color* const color)
{
  return color->_private;
}
```

### Function pointer rule
This rule gets triggered when variables or arguments for function pointers are created without using a typedef. Using a typedef makes function signatures clearer, reduces errors when the signature changes (you only need to update it in one place), and is less error-prone since it's harder to accidentally mistype the function signature.

```c
// This will trigger the parser
void bad_perform_math(int (*math_func)(int, int), int a, int b, int* out_res)
{
  *out_res = math_func(a, b);
}

typedef int (*math_function)(int, int);
// OK
void perform_math(math_function math_func, int a, int b, int* out_res)
{
  *out_res = math_func(a, b);
}
```

### Typedef struct rule
This rule gets triggered when a struct is declared but has no typedef. Requiring a typedef ensures consistency across the codebase, reduces repetition by eliminating the need to write `struct` every time you use the type, and improves code clarity by enforcing a uniform naming pattern.

```c
// This triggers parser if no typedef is found
struct Position
{
  int x, y, z;
};
```

### Assignment rule
This rule gets triggered whenever a primitive type, e.g. a pointer or integer, is declared but not initialized in the same statement. It does however allow unintitialized structs. Requiring initialization prevents use-before-initialization bugs and makes developer intent clearer (a variable that's initialized is ready to use).

```c
int a;      // Triggers parser
int b = 5;  // OK
```

It also comes with extra settings. The first three help forbid null assignment for codebases that follow a strict "nothing may be null" rule to make sure no null dereferences occur. 

```c
int* i_ptr = NULL;  // Not OK with forbid_null_assign: true
foo(NULL, 3, 7);    // Not OK with forbid_null_as_arg: true

struct Setting
{
  const char* name;
  int x, y, z;
}
typedef struct Setting Setting;

Setting setting = {0}; // Not OK with forbid_zero_init_for_objects_with_pointers: true
```

Many programmers are not fond of const correctness when it comes to arguments, but that does not mean it is not important for code clarity. Instead of enforcing const correctness for argument values, rules can be enabled to simply forbid modifying argument values. Note that this does not affect reassignment of data a pointer points to.

```c
typedef struct RGB
{
  int r, g, b;
} RGB;

void foo(const char* name, int x, RGB* rgb)
{
  name = "New string";  // Not OK with forbid_arg_reassign: true
  x = 5;                // Not OK with forbid_arg_reassign: true
  int* x_ptr = &x;      // Not OK with forbid_mut_arg_pointer: true

  int y = x;            // OK
  rgb->r = x;           // OK
}
```

### Prefix namespace rule
This rule enforces various components in a codebase to to require the folder path as a prefix in its naming, which mimics namespaces in other languages such as C++. This prevents naming collisions in large projects, makes it obvious which module code belongs to at a glance, and enables simulation of C++ namespace organization in pure C.

```yaml
# Example
prefix_namespace:
    level: Warning
    stop_at_dir: src
    stop_at_count: 10
    use_seperator: true
    seperator: __
    apply_to_functions: true
    apply_to_structs: true
    apply_to_typedefs: true
    require_ifndef_for_filepath: true
```

For the above example, the folder prefix naming will not require `src` to be in the name. It will also not require more than 10 folders for the prefix naming. If `use_seperator` is `true` then all folder names must be divided be the `seperator`. The `apply_to_` settings set the naming need for their targets, e.g. functions might require namespace prefixing but not structs. Finally, a setting for making sure the file starts with an include guard that is the filepath can be used. Here is an example of what the above settings expect from the code.

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

#endif
```

### Null check rule
This rule gets triggered whenever a pointer argument in a function is used without first checking if it is null. 

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
  return *i_ptr;
}
```

Note that many more options are valid for checking null, including `if (i_ptr) { ... }` if `allow_direct_ptr_in_if_statement` is true in config.  

### Argument pointer movement rule
This rule enforces user defined macro tags and "operators" for handling ownership of pointers, and making sure both the callsite and function match their "operator" and tag. [Here is a premade document for tags](./default/move_tags.h) that can be used as is, but all macro definitions can be changed as well.  

For these settings

```yaml
argument_pointer_movement:
    level: Warning
    require_operator_for_move_callsite: true
    require_operator_for_out_callsite: true
    require_operator_for_modify_callsite: false
```

and the following tags `moved`, `move`, `output`, `out`, `mutable`, `mut`, the parser expects code to look like this:

```c
struct Data
{
  int i, j, k;
};
typedef struct Data Data;

int get_data_i(const Data* data);

void initialize_data(output Data** data);
void edit_data(mutable Data* data, int i, int j, int k);
void give_data_to_other_section(moved Data* data);

void example(void)
{
  Data* data = NULL;
  initialize_data(out(&data));
  edit_data(data, 3, 5, 7);
  int i = get_data_i(data);
  give_data_to_other_section(move(data));
}
```

Notice how everything that is const does not need tagging because it almost handles itself, and notice how the need for the `mut` "operator" was disabled in the settings. This is a good middle ground so that the ownership transfer is the most noticeable parts of the code. With these rules in place, the code becomes self-documenting and if a function is ever changed in future in taking a `const`, `mutable`, `output`, or `moved` pointer then the parser will trigger and catch that the callsite is unedited and may have unexpected behavior. 

As mentioned, the tags are just macro definitions (even if they must follow some simple rules shown in linked document above). This means that the user can provide any names the user wants. For example, `move` and `move_cast()` with `out` and `out_cast`; or `moved` with `move`, `outed` with `out`, and `modded` with `mod`. They can of course all also be caps. 

### Disable section
Rules can be temporarily and locally disabled with a comment saying `// WorkshopC off` and then `// WorkshopC on`.

```c
// WorkshopC off
enum Color { // Does not trigger enum rule
  RED,
  GREEN,
  BLUE
};
// WorkshopC on
```

### Adjust code for parser
The parser runs with `WORKSHOPC_PARSING` defined as a macro. This allows users to create `#ifndef` guards to adjust code for parsing and usage. 

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

After building, you will get the `workshopc` executable in the `release/` folder.


- Reads your YAML rule configuration from the config file
- Loads the compilation database to understand compiler flags and settings
- Parses the provided C source file
- Analyzes code using Clang AST
- Prints warnings and errors to the console
- Returns:
  - `0` if only warnings or no issues
  - `1` if errors were found

#### Notes

- The compilation database is typically located in your CMake build directory (e.g., `build/`)
- It's generated automatically by CMake when configured with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
- Warnings do not fail the tool by default
- Errors are considered hard failures
- Output is intended for use in CI pipelines or pre-commit checks
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
### Example: analyze a test file
./build/workshopc workshopc.config.yaml tests/enum.c build/
```

The tool requires access to the compilation database to understand compiler flags and include paths.

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
A parser must be implemented to transfer goals of AlloyCTranspiler into a warning/suggestion system for pure C code.

For V0.9 it shall
- **Make pod structs require a _pod function and class structs require a _make function,  and "free" structs an _init function**
- verify pod and class structs are initialized with an assignment at declaration (class struct must use function)
- verify class structs are never assigned anything again
- verify class structs call _destroy before all scope exits (unless static)
- verify class structs have _move, _copy, _destroy, and _valid functions (they might be invalid but never illegal)
- **Add note to use clang-tidy for const correctness**
- verify pod structs can only contain pod structs

For V1 it shall
- **Make release folder visible in git at the end**
- Build for Linux
- Add ability to take folder of source code instead of single file

For V1.1 it shall
- Add rules for vtables and interfaces
- Enforce no use after move for pointer tags, and no move of mut or out variable