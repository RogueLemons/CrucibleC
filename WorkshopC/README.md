# WorkshopC (in progress)
An upcoming project for parsing C code and generating tips for generating safer code or adhere to a certain style.

## TODO
A parser must be implemented to transfer goals of EasyCTranspiler into a warning/suggestion system for pure C code.

It shall
- Verify all structs are initialized with either a _populate or _init function
- Verify if _init is used all exit paths must include _cleanup
- Verify variables are not called with _populate or _init multiple times in same scope
- Optional rules for enforcing Copied<Struct> and Moved<Struct>, or Owned<Struct>
- **Make pod structs require a make_ function and class structs require an _init function (optionally _or_die)**
- Add rules for vtables and interfaces
- **Add note to use clang tidy for const correctness**
- Improve assignment rules
- **Make release folder visible in git at the end**
- Build for Linux
- Add ability to take folder of source code instead of single file
- **Enter V1**

# WorkshopC Build System Documentation

## Overview

This project supports two build workflows:

1. **Windows Python build script** (developer convenience)
   - Fast setup for MSYS2-based LLVM/Clang environments
   - Assumes a known toolchain layout

2. **Cross-platform CMake build** (official build system)
   - Works on Windows, Linux, macOS
   - Requires user-provided LLVM/Clang installation or system packages
   - No hardcoded paths

---

# Windows Developer Workflow (Python Script)

## Purpose

The Python script is a convenience wrapper for developers working on Windows using MSYS2 LLVM/Clang.

It is **NOT** a portable build system. It assumes:

- MSYS2 is installed in `C:/msys64`
- LLVM + Clang are installed via MSYS2 UCRT64 packages
- Ninja is available in `PATH`
- CMake is installed and available in `PATH`

## How to Use

After building, you will get the `workshopc` executable in the `release/` folder.

### Command Line Arguments

The tool requires three arguments:

```bash
workshopc <config.yaml> <source-file.c> <compdb-dir>
```

**Arguments:**
- `<config.yaml>` — Path to the YAML rule configuration file
- `<source-file.c>` — Path to the C source file to analyze
- `<compdb-dir>` — Directory containing `compile_commands.json` (typically the CMake build directory)

### Example

```bash
workshopc workshopc.config.yaml tests/enum.c build/
```

### What it does

- Reads your YAML rule configuration from the config file
- Loads the compilation database to understand compiler flags and settings
- Parses the provided C source file
- Analyzes code using Clang AST
- Prints warnings and errors to the console
- Returns:
  - `0` if only warnings or no issues
  - `1` if errors were found

### Notes

- The compilation database is typically located in your CMake build directory (e.g., `build/`)
- It's generated automatically by CMake when configured with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`
- Warnings do not fail the tool by default
- Errors are considered hard failures
- Output is intended for use in CI pipelines or pre-commit checks
- If the compilation database cannot be found, the tool will fail with an error message

## Requirements

### 1. Install MSYS2

https://www.msys2.org/

### 2. Install required packages (from the UCRT64 shell)

```bash
pacman -S mingw-w64-ucrt-x86_64-toolchain
pacman -S mingw-w64-ucrt-x86_64-llvm
pacman -S mingw-w64-ucrt-x86_64-clang
pacman -S mingw-w64-ucrt-x86_64-ninja
pacman -S mingw-w64-ucrt-x86_64-cmake
```

### 3. Install Python on Windows

```bash
python --version
```

## Running the build

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

## What the Python script assumes

The script hardcodes:

```
LLVM_DIR  = C:/msys64/ucrt64/lib/cmake/llvm
Clang_DIR = C:/msys64/ucrt64/lib/cmake/clang
```

This means:

- It only works with MSYS2 LLVM
- It does **NOT** auto-detect toolchains
- It avoids system CMake guessing

## Why this script is NOT portable

This is intentional:

- Windows LLVM setups differ (MSYS2, vcpkg, LLVM installer, WSL)
- MSYS2 environment variables can break builds
- Mixing environments causes LLVM/Clang linking issues

This script enforces a single known-good configuration.

---

# Cross-Platform CMake Workflow (Official Build System)

## Purpose

The CMake configuration is designed to be:

- Portable
- Environment-agnostic
- Compatible with Linux, macOS, Windows
- Independent of MSYS2 or any specific package manager

## Requirements

### 1. CMake >= 3.20

https://cmake.org/download/

### 2. C++ Compiler

Supported compilers:

- GCC
- Clang
- MSVC

### 3. LLVM + Clang development packages

Must provide:

- LLVMConfig.cmake
- ClangConfig.cmake

Examples:

#### Linux

```bash
sudo apt install llvm clang libclang-dev
```

#### macOS

```bash
brew install llvm
```

#### Windows

https://llvm.org/

## Configuring the project

### Basic configuration

```bash
cmake -S . -B build
```

### Explicit LLVM paths

```bash
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DClang_DIR=/path/to/clang/lib/cmake/clang
```

## Building

```bash
cmake --build build
```

This generates:
- The `workshopc` executable
- `compile_commands.json` in the build directory (required by the tool)

## Running the tool

After building, use the tool with the compilation database from the build directory:

```bash
# Example: analyze a test file
./build/workshopc workshopc.config.yaml tests/enum.c build/
```

The tool requires access to the compilation database to understand compiler flags and include paths.

## What makes this CMake portable

### 1. No hardcoded paths

It does **NOT** assume:

- MSYS2
- Windows layout
- Specific install directories

Instead it uses:

```cmake
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
```

### 2. Uses LLVM’s official CMake targets

This ensures:

- Compatibility across LLVM versions
- Works with system packages
- No manual `.a` or `.lib` linking

### 3. Cross-platform compile definitions

Safe macros:

- `NOMINMAX`
- `_CRT_SECURE_NO_WARNINGS`
- `_FILE_OFFSET_BITS=64`

LLVM-required macros:

- `__STDC_CONSTANT_MACROS`
- `__STDC_FORMAT_MACROS`
- `__STDC_LIMIT_MACROS`

### 4. Minimal platform-specific logic

```cmake
if (MINGW)
    target_link_libraries(workshopc PRIVATE ws2_32 version bcrypt)
endif()
```

### 5. Proper LLVM component linking

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

---

# Design Philosophy

## Python script = opinionated convenience tool

- Assumes MSYS2
- Hardcoded paths
- Optimized for your environment only
- Not portable

## CMake = universal build contract

- Must work anywhere LLVM is installed correctly
- No assumptions about environment
- Minimal platform-specific logic
- Official build system for the project

---

# Summary

- Python script = developer shortcut (Windows only)
- CMake = real portable build system
- Users only need a working LLVM + CMake setup
- You do **NOT** need to support all environments in Python
- CMake is the production-grade interface