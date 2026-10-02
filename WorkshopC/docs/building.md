[← Previous: Diagnostic codes](diagnostic-codes.md)

# Building and testing

This page explains how to build WorkshopC from source, how to run its test suite, and what to keep in sync when changing a check.

## Contents

- [Build workflows](#build-workflows)
- [Requirements](#requirements)
- [Building with CMake](#building-with-cmake)
  - [Configuring](#configuring)
  - [Building](#building)
  - [What the build produces](#what-the-build-produces)
- [Windows quick build](#windows-quick-build)
- [How the presets are built in](#how-the-presets-are-built-in)
- [Clang's builtin headers](#clangs-builtin-headers)
- [Running the tests](#running-the-tests)
  - [Test cases](#test-cases)
  - [Other tests](#other-tests)
  - [Adding a test case](#adding-a-test-case)
- [Changing a check](#changing-a-check)
- [Design notes](#design-notes)

## Build workflows

WorkshopC supports two ways to build:

| | CMake | Windows quick build |
|-|-------|---------------------|
| **Purpose** | The official, portable build system | A convenience script for developers on Windows |
| **Platforms** | Windows, Linux and macOS | Windows with MSYS2 (UCRT64) |
| **LLVM and Clang** | Found by CMake, or given with `LLVM_DIR` and `Clang_DIR` | Fixed MSYS2 paths |
| **Command** | `cmake -S . -B build` and `cmake --build build` | `python scripts/windows_rebuild.py` |

All commands on this page are run from the `WorkshopC` folder.

## Requirements

- **CMake 3.20** or newer: <https://cmake.org/download/>
- **A C++20 compiler**: GCC, Clang or MSVC.
- **Python 3**, which the build uses to embed the [built-in presets](#how-the-presets-are-built-in).
- **LLVM and Clang development packages** that provide `LLVMConfig.cmake` and `ClangConfig.cmake`:

  | Platform | Example |
  |----------|---------|
  | Linux (Debian, Ubuntu) | `sudo apt install llvm llvm-dev clang libclang-dev` |
  | macOS (Homebrew) | `brew install llvm` |
  | Windows | The MSYS2 packages in [Windows quick build](#windows-quick-build) |

  Package names vary between distributions. Any LLVM installation works as long as it includes the development libraries and their CMake files.

The tests additionally need **Ninja** and **clang** on `PATH`, see [Running the tests](#running-the-tests).

> [!NOTE]
> The Linux build has not been verified yet, see the [roadmap](roadmap.md).

## Building with CMake

### Configuring

```bash
cmake -S . -B build
```

When CMake can not find LLVM and Clang by itself, give their CMake folders explicitly:

```bash
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DClang_DIR=/path/to/llvm/lib/cmake/clang
```

Homebrew's LLVM, for example, is not on the default search path: use `$(brew --prefix llvm)/lib/cmake/llvm` and `$(brew --prefix llvm)/lib/cmake/clang`.

### Building

```bash
cmake --build build
```

### What the build produces

- The `workshopc` executable (`workshopc.exe` on Windows) in the build folder.
- A `release/` folder, refreshed on every build, with everything needed to use WorkshopC:
  - the executable,
  - `configs/`: the [preset files](configuration.md#built-in-presets),
  - `tags/`: the [tag headers](tag-headers.md),
  - `LICENSE` and `THIRD-PARTY-NOTICES.txt`.
- A `compile_commands.json` in the build folder for WorkshopC's own C++ sources, which is useful for editors. WorkshopC itself needs the compilation database of the code it *analyzes*, see [Running WorkshopC](usage.md#before-you-start).

For example, to analyze one of the test files once the test project has been configured (see [Running the tests](#running-the-tests)):

```bash
./release/workshopc --config tests/cases/enum/enum.workshopc.yaml -p build-tests/ tests/cases/enum/enum.c
```

## Windows quick build

`scripts/windows_rebuild.py` is a convenience wrapper for developers working on Windows with LLVM and Clang from MSYS2. It is **not** a portable build system. It assumes that:

- MSYS2 is installed in `C:/msys64`,
- LLVM and Clang are installed with the MSYS2 UCRT64 packages,
- CMake and Ninja are available on `PATH`.

**Setup:**

1. Install MSYS2: <https://www.msys2.org/>
2. Install the required packages from the UCRT64 shell:

   ```bash
   pacman -S mingw-w64-ucrt-x86_64-toolchain
   pacman -S mingw-w64-ucrt-x86_64-llvm
   pacman -S mingw-w64-ucrt-x86_64-clang
   pacman -S mingw-w64-ucrt-x86_64-ninja
   pacman -S mingw-w64-ucrt-x86_64-cmake
   ```

3. Install Python for Windows, and check it with `python --version`.

**Building**, from PowerShell or CMD:

```bash
python scripts/windows_rebuild.py              # clean build
python scripts/windows_rebuild.py --noclean    # keep the build folder and only rebuild what changed
```

The script:
1. deletes the `build/` folder (unless `--noclean` is given),
2. configures CMake with Ninja, in Release mode, with LLVM and Clang from `C:/msys64/ucrt64/lib/cmake/llvm` and `C:/msys64/ucrt64/lib/cmake/clang`,
3. builds the project, which also refreshes the `release/` folder.

The paths are hardcoded on purpose, see [design notes](#design-notes).

> [!TIP]
> Git Bash puts Git's own MinGW folder before `C:\msys64\ucrt64\bin` on `PATH`, so `workshopc.exe` can load an older `libstdc++` and fail to start (exit code `0xC0000139`). Run it from PowerShell, or put the MSYS2 folder first, e.g. `PATH="/c/msys64/ucrt64/bin:$PATH" python scripts/run_tests.py`.

## How the presets are built in

The [built-in presets](configuration.md#built-in-presets) are compiled into the executable, so they work without any files next to it. At build time, `scripts/config_to_string.py` converts every `default/configs/*.workshopc.yaml` file into a C++ header in `generated/` (ignored by git), and the build reruns the conversion whenever a preset file changes. After editing a preset, rebuild to update the executable.

Adding a new preset takes a new file in `default/configs`, and its name registered in `src/main.cpp`: the include of its generated header, the name lookup in `embeddedPresetConfig`, and the list of presets in the help text. A file in `tests/default_configs` then tests its behavior.

## Clang's builtin headers

Clang's builtin headers (`stddef.h`, `mm_malloc.h`, ...) are looked up next to the executable first. If they are not there, WorkshopC falls back to the resource folder of the clang it was built against, `<LLVM lib dir>/clang/<version>`, which is recorded at build time and printed as `Clang resource dir` when configuring. Without this, including a header such as `<stdlib.h>` fails with `'mm_malloc.h' file not found` on MSYS2.

## Running the tests

```bash
python scripts/run_tests.py
```

The tests use the executable in `release/`, so build first. They need CMake, Ninja and clang on `PATH`.

The test files form a standalone project, described by `tests/CMakeLists.txt`. It is never built: `scripts/run_tests.py` only **configures** it into `build-tests/` with clang and Ninja, which writes a `compile_commands.json` for the test files, and then passes that folder to WorkshopC for every test. The project is configured again on every run, so new test cases are picked up automatically.

### Test cases

Every test case is a folder in `tests/cases/` holding three files named after the folder, e.g. `tests/cases/struct_usage/`:

| File | Contents |
|------|----------|
| `struct_usage.c` | The code to analyze, with a `// good` or `// bad: reason` comment on the lines that matter |
| `struct_usage.workshopc.yaml` | The config it is analyzed with |
| `struct_usage.expected.txt` | Every diagnostic WorkshopC must report, one per line as `level: message [code]`, in any order |

A test passes when WorkshopC reports exactly the expected diagnostics, no more and no fewer, and exits with the matching code.

Headers shared by the tests are in `tests/headers/`, and `tests/external/` stands in for third-party code: every test config lists `external/` in `third_party_includes`. The tests include them as `"headers/..."` and `"external/..."`, which works from every test case folder, since `tests/` is on the include path.

### Other tests

Besides the test cases, the runner checks the tool as a whole:

| Test | What it checks |
|------|----------------|
| Output files | The diagnostics of `tests/cases/suppression_balance/suppression_balance.c` are written to a text, a JSON and a SARIF file in a single `--quiet` run, into `tests/output/` (ignored by git). Nothing may be printed, and all three files must hold exactly the expected diagnostics. |
| Warnings as errors | The same file with `--warnings-as-errors`: every warning must be reported as an error on the terminal and in a JSON file, with no warnings left in the counts and an exit code of `1`. |
| Folder input | `tests/folder_input/` is given as a folder: every `.c` file in it must be analyzed, also in subfolders, but no header and nothing in a third-party folder. |
| Bad configs | Every config in `tests/bad_config/` must be reported as invalid, with exactly the problems listed in its `.expected.txt` file, and the run must stop before any analysis. |
| Dump config | The output of `--dump-config` must read back as the same config and analyze exactly like the original, and a normal run must name the config and build folder it uses. |
| Config overrides | `--third-party-include`, `--no-config-third-party-includes` and `--prefix-top-dir` must show up in `--dump-config`, both for a config file and for a built-in preset. |
| Built-in presets | Every file in `tests/default_configs/` is analyzed with the preset of the same name and compared with its expected file. |

### Adding a test case

1. Create a folder `tests/cases/<name>/` with `<name>.c`, `<name>.workshopc.yaml` and `<name>.expected.txt`.
2. Mark the lines that matter in the `.c` file with `// good` or `// bad: reason`.
3. List every diagnostic the file must produce in the expected file. Running WorkshopC on the file shows the diagnostics it currently reports:

   ```bash
   ./release/workshopc --config tests/cases/<name>/<name>.workshopc.yaml -p build-tests/ tests/cases/<name>/<name>.c
   ```

4. Run the tests. The new folder is picked up automatically.

## Changing a check

When adding or changing a check, keep these in sync:

- **Diagnostic codes.** A new check gets the next free code of its rule in `src/diagnostic_codes.hpp`. Codes are never renumbered or reused. Add the code to [Diagnostic codes](diagnostic-codes.md).
- **Documentation.** Describe the behavior, with examples, in [Rules](rules.md).
- **Tests.** Add or extend a test case with the expected diagnostics.
- **Options.** A new option needs its field in `src/config.hpp`, reading, validation and `--dump-config` output in `src/config_parser.cpp`, a value in every preset in `default/configs`, and a row in the rule's options table in [Rules](rules.md).
- **Sources.** A new source file is added to `SOURCES` in `CMakeLists.txt`.

## Design notes

The CMake build is the build contract: it must work anywhere LLVM is installed correctly, and makes no assumptions about the environment.

- **No hardcoded paths.** It does not assume MSYS2, a Windows layout or specific install folders, and finds LLVM and Clang with `find_package(LLVM REQUIRED CONFIG)` and `find_package(Clang REQUIRED CONFIG)`.
- **LLVM's official CMake targets.** The LLVM libraries are resolved with `llvm_map_components_to_libnames` (`Core`, `Support`, `IRReader`) instead of being listed by hand, which gives correct dependency resolution, no duplicate symbols and no manual `.a` or `.lib` linking, across LLVM versions and system packages.
- **Cross-platform compile definitions.** `NOMINMAX`, `_CRT_SECURE_NO_WARNINGS` and `_FILE_OFFSET_BITS=64` are safe everywhere, and `__STDC_CONSTANT_MACROS`, `__STDC_FORMAT_MACROS` and `__STDC_LIMIT_MACROS` are required by LLVM.
- **Minimal platform-specific logic.** The only special case is MinGW, which links `ws2_32`, `version` and `bcrypt`.

The Windows quick build is the opposite, and deliberately so. Windows LLVM setups differ (MSYS2, vcpkg, the LLVM installer, WSL), MSYS2 environment variables can break builds, and mixing environments causes LLVM and Clang linking problems. The script enforces one known-good configuration instead: it uses fixed MSYS2 paths, does not try to detect toolchains, and removes the MSYS2 environment variables before configuring. Supporting every environment is the job of the CMake build, not of the script.

---

[Next: Roadmap →](roadmap.md) · [Back to README](../README.md)
