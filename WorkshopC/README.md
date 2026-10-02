# WorkshopC

**Ownership and lifetimes for C, without leaving C.**

```c
#include <stddef.h>
#include "workshopc_tags.h"

typedef struct inbox inbox;
typedef struct message message;

void message_create(initializes message** result);           // writes a new message to *result
void inbox_push(borrows inbox* box, receives message* item);  // takes ownership of item
void message_print(const message* item);                      // only reads item

void deliver(borrows inbox* box)
{
    message* hello = NULL;
    message_create(overwrite(&hello));    // the call site shows that hello is written
    inbox_push(box, give(hello));         // ...and that ownership moves to the inbox
    message_print(hello);                 // Reported: hello was given away
}
```

```text
deliver.c:16:19:	error: pointer 'hello' may have been moved to 'inbox_push' and can not be used until it is reassigned [CCW0908]
```

## Contents

- [Start here](#start-here)
- [What is WorkshopC?](#what-is-workshopc)
- [Highlights](#highlights)
- [Quick start](#quick-start)
- [Documentation](#documentation)
- [Project status](#project-status)
- [License](#license)

## Start here

- **Starting a new project?** Use the `default` preset: every rule is on, and the rules that protect correctness are errors. → [Built-in presets](docs/configuration.md#built-in-presets)
- **Bringing in existing code?** Follow the adoption path, which raises the bar one step at a time: `adopt` → `adopt-more` → `adopt-even-more`. → [The adoption path](docs/configuration.md#the-adoption-path)
- **Want to see the style first?** → [Take the tour](docs/tour.md)

## What is WorkshopC?

WorkshopC is a configurable Clang-based analyzer that enforces a safer style of C: explicit ownership, RAII-style lifetimes, non-null references and encapsulation, all in plain C that still compiles with any compiler.

Most C linters look for bugs in whatever style the code is already written in. WorkshopC instead enforces a style in which whole classes of bugs are hard to write in the first place. Ownership is part of every function signature. Every resource has exactly one owner and a checked lifetime. Pointers that may be null are checked, and pointers that can never be null say so. Buffers never travel without their size. The tags that make this possible are ordinary macros that disappear in a normal build, so the code stays standard C.

WorkshopC is part of [the Crucible C Project](../README.md), a C ecosystem for clean, predictable and safer code.

## Highlights

- **Ownership on both sides of a call.** `void consume(receives item* value)` is called as `consume(give(value))`. Declarations and call sites read like sentences, and a pointer can not be used after it was given away.
- **RAII in plain C.** A struct's creator function decides its category. Raii structs get checked lifetimes: one owner per value, no leaks on early returns, no use after destroy, destruction in reverse order, and destroy functions that clean up every field.
- **Null safety from both directions.** Pointer parameters are null-checked along every path through a function, unless they are tagged as references, which can never be null and move the responsibility to the call site.
- **A graduated adoption path.** Three presets bring an existing codebase to the full standard in steps, instead of burying it in findings on day one.
- **Buffers that know their size.** Spans pair pointers with their sizes, and arrays reach the rest of the code only through them.
- **Encapsulation.** Private fields that only their struct's own functions can use, and one standard layout for interfaces and vtables.
- **Accountable exceptions.** Every suppression needs a written reason, and must be closed again in the same file.
- **Your vocabulary.** Tag names, function suffixes and naming conventions are all configurable.
- **Ready for CI.** Text, JSON and SARIF output, bitmask exit codes, `--warnings-as-errors` and `--dump-config`.

## Quick start

1. **Build WorkshopC** from source, see [Building and testing](docs/building.md). The `release/` folder then holds the executable, the preset configs and the tag headers.
2. **Create a compilation database** for your project, e.g. with CMake:

   ```bash
   cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   ```

3. **Run WorkshopC** from the project root with a built-in preset:

   ```bash
   workshopc --config default src/     # a new project
   workshopc --config adopt src/       # existing code
   ```

   The presets look for `compile_commands.json` in `build/`. For another build folder, add `-p <folder>`.
4. **Make the config your own.** Copy a preset to the project root as `workshopc.yaml`, e.g. `release/configs/default.workshopc.yaml`, and adjust it. WorkshopC then finds it by itself, and `workshopc src/` is all that is needed.
5. **Add the tag headers** from `release/tags/lower` or `release/tags/upper` to your include path, to mark ownership, private fields and references. See [Tag headers](docs/tag-headers.md).

## Documentation

The documentation is written to be read in order, but every page also stands on its own:

1. [A tour of WorkshopC](docs/tour.md): what code written for WorkshopC looks like, in ten minutes
2. [Running WorkshopC](docs/usage.md): the command line, the output, output files and exit codes
3. [Configuration](docs/configuration.md): the config file, the built-in presets and the adoption path
4. [Rules](docs/rules.md): every rule and option, with examples
5. [Tag headers](docs/tag-headers.md): the macros behind ownership, private and reference tags
6. [Preset rationale](docs/preset-rationale.md): why each preset is set up the way it is
7. [Diagnostic codes](docs/diagnostic-codes.md): every code WorkshopC can report
8. [Building and testing](docs/building.md): building from source and running the tests
9. [Roadmap](docs/roadmap.md): what is planned

## Project status

WorkshopC is in **alpha**: rules, options and presets may still change between versions. See the [roadmap](docs/roadmap.md) for what is planned.

## License

WorkshopC is part of CrucibleC and is released under the [MIT License](../LICENSE). It uses the LLVM and Clang libraries, see the [third-party notices](../THIRD-PARTY-NOTICES.txt).

---

[Start reading: A tour of WorkshopC →](docs/tour.md)
