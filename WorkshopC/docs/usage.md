[← Previous: A tour of WorkshopC](tour.md)

# Running WorkshopC

This page explains how to run WorkshopC on a project, what its options do, and how to read its output, output files and exit codes.

## Contents

- [Before you start](#before-you-start)
- [Running an analysis](#running-an-analysis)
- [Which files are analyzed](#which-files-are-analyzed)
- [Options](#options)
  - [Config and input options](#config-and-input-options)
  - [Output options](#output-options)
- [Reading the output](#reading-the-output)
- [Output files](#output-files)
- [Exit codes](#exit-codes)
- [Compiler output](#compiler-output)
- [Running in CI](#running-in-ci)
- [Examples](#examples)

## Before you start

WorkshopC needs four things:

1. **The `workshopc` executable.** See [Building and testing](building.md) to build it from source.
2. **A compilation database** (`compile_commands.json`) for your project. WorkshopC analyzes every file with the same compiler flags and include paths as your build, and finds them in this file.
   - CMake writes it to the build folder when configured with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`.
   - Meson writes it to the build folder automatically.
   - For Make and other build systems, a tool such as [Bear](https://github.com/rizsotto/Bear) can record it, e.g. `bear -- make`.
3. **A config**: a `workshopc.yaml` file, or one of the [built-in presets](configuration.md#built-in-presets).
4. **The tag headers**, if your code uses tags, on the include path of your build. See [Tag headers](tag-headers.md).

## Running an analysis

```bash
workshopc [options] <files or folders...>
```

The quickest setup is to copy [the default config](../default/configs/default.workshopc.yaml) to the root of your project as `workshopc.yaml` and point its `compile_commands_dir` at the folder that holds `compile_commands.json` (typically the CMake build folder). WorkshopC finds the config by itself, so all that is needed is:

```bash
workshopc src/
```

Without a config file, a built-in preset works just as well, together with `-p` for the build folder:

```bash
workshopc --config default -p build/ src/
```

The presets set `compile_commands_dir: build`, which for a preset is relative to the current folder. When the build folder is called `build` and WorkshopC runs from the project root, `-p` can be left out.

Before the analysis, WorkshopC prints the config and the build folder it uses (unless `--quiet` is given), so that a CI log shows which files a run used:

```text
Config: /repo/workshopc.yaml
Build folder: /repo/build (compile_commands_dir in the config)
```

## Which files are analyzed

- A file given on the command line is analyzed.
- A folder is searched recursively for `.c` files. Folders that match `third_party_includes` and the folder of the compilation database are skipped.
- Headers are checked through the `.c` files that include them. A problem in a header that is included by several files is only reported once.
- Code in system headers and in `third_party_includes` folders is never reported, see [third-party code](configuration.md#third-party-code).

## Options

### Config and input options

| Option | Description |
|--------|-------------|
| `--config <file\|preset>` | The config file, or the name of a built-in preset: `adopt`, `adopt-more`, `adopt-even-more`, `default`, `opinionated`, `strict`, `embedded` or `nevernull`. Without it, `workshopc.yaml` is searched for in the current folder and then its parents. Other configs are named `<name>.workshopc.yaml` by convention, e.g. `ci.workshopc.yaml`. |
| `-p`, `--build-path <folder>` | The folder that holds `compile_commands.json`. Overrides `compile_commands_dir` from the config. |
| `--third-party-include <folder>` | Add a folder to `third_party_includes`. Repeat the option to add several folders. It adds to the list of the config or preset. |
| `--no-config-third-party-includes` | Drop the `third_party_includes` of the config or preset, so that only the folders given with `--third-party-include` are third party. Together, the two options replace the list without editing the config, e.g. when a default folder holds the project's own code. |
| `--prefix-top-dir <folder>` | Override `prefix_namespace.top_dir` for this run. |
| `--dump-config` | Print the effective config, including the command-line overrides above, with every setting, and exit without analyzing anything. No files or folders are needed. See [inspecting the effective config](configuration.md#inspecting-the-effective-config). |

### Output options

| Option | Description |
|--------|-------------|
| `--text <file>` | Also write the diagnostics as text to a file: the same lines as printed to the terminal. |
| `--json <file>` | Also write the diagnostics as JSON to a file. |
| `--sarif <file>` | Also write the diagnostics as SARIF 2.1.0 to a file, the standard format read by e.g. GitHub code scanning and many IDEs. |
| `-q`, `--quiet` | Print nothing to the terminal: no diagnostics and no summary. Clang's compile errors and problems that stop the analysis (e.g. a missing config) are still printed, and the exit code is unaffected. |
| `--warnings-as-errors` | Report every warning as an error: on the terminal, in the output files, in the counts and in the exit code. The config still decides which rules run and how serious they normally are, while the option decides how strict one run is. One config can then serve both a relaxed local run and a strict CI run. |
| `-h`, `--help` | Show the help. |

The output files are described in [Output files](#output-files).

## Reading the output

Each diagnostic is printed as one line: the file, line and column, a tab, the level (`warning` or `error`), the message, and the [diagnostic code](diagnostic-codes.md) in brackets.

```text
src/deliver.c:16:19:	error: pointer 'hello' may have been moved to 'inbox_push' and can not be used until it is reassigned [CCW0908]
```

The code identifies exactly which check reported the problem, and every code is listed with a link to its rule in [Diagnostic codes](diagnostic-codes.md). After the analysis, a summary gives the number of warnings and errors:

```text
Warnings: 0
Errors: 1
```

The diagnostics and the summary are printed to stderr, so that stdout is free for an output file written to `-`.

## Output files

`--text`, `--json` and `--sarif` can be combined freely to write any set of files in a single run. Give `-` as the file to write that format to stdout instead. Only one format can use stdout, and with `--text -` the diagnostics go to stdout instead of the terminal's stderr.

**Text** files hold the same lines as the terminal.

**JSON** files hold the warning and error counts and a list of diagnostics, each with its file, line, column, level, code, name and message:

```json
{
  "warnings": 0,
  "errors": 1,
  "diagnostics": [
    {"file": "src/deliver.c", "line": 16, "column": 19, "level": "error", "code": "CCW0908", "name": "use-after-move", "message": "pointer 'hello' may have been moved to 'inbox_push' and can not be used until it is reassigned"}
  ]
}
```

**SARIF** files follow [SARIF 2.1.0](https://docs.oasis-open.org/sarif/sarif/v2.1.0/sarif-v2.1.0.html). The tool's rule list describes every diagnostic code with its name and a short description, and each result carries its code as `ruleId`, its level, its message and its location. GitHub code scanning, and editors with a SARIF viewer, can show the results next to the code.

Compile errors from clang are always printed as text and never appear in the files. The [exit code](#exit-codes) tells when they happened.

## Exit codes

The exit code reports the result of the analysis. Codes 0 to 3 form a bitmask, `1` for errors and `2` for warnings, so the tool can be used directly in scripts and CI:

| Code | Meaning |
|------|---------|
| `0` | Clean: no warnings or errors |
| `1` | Errors found |
| `2` | Warnings found, no errors |
| `3` | Both errors and warnings found |
| `64` | Bad usage: an unknown option, no files given, a file or folder that does not exist, or no `.c` files found |
| `65` | The config could not be loaded, or is [invalid](configuration.md#config-validation) |
| `66` | The compilation database could not be found or loaded |
| `67` | Clang failed to process a source file (e.g. it does not compile), so the analysis result is not reliable |

Codes of `64` and above always mean that the tool itself could not complete the analysis, so they can never be confused with rule results.

> [!NOTE]
> Most build systems treat any non-zero exit code as a failure, so a run with warnings only (`2`) fails a script that uses `set -e` unless the caller handles it. With `--warnings-as-errors` there are no warnings, so the code is `0` or `1`.

## Compiler output

WorkshopC runs every file through clang, but only WorkshopC's own rules produce warnings. Clang's warnings (unused variables, implicit conversions and so on) are turned off with `-w`, even if the compilation database enables `-Wall` or `-Werror`, since they belong to the project's normal build.

Genuine compile errors are still printed in clang's normal format, and the exit code is then `67`. The rules still run on whatever clang could recover from the broken file, but the result should be treated as incomplete until the file compiles.

## Running in CI

A typical CI step builds the compilation database and runs WorkshopC on the project's sources:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
workshopc --warnings-as-errors --sarif workshopc.sarif src/
```

`--warnings-as-errors` fails the step on any finding, while the config can keep warnings for local runs. The SARIF file can be uploaded to GitHub code scanning, or kept as a build artifact. The same command also works as a pre-commit check.

To fail only on errors and on tool failures, but not on warnings, check the bits of the exit code:

```bash
status=0
workshopc src/ || status=$?

# Fail on errors (bit 1) and on tool failures (64 and above), not on warnings alone
if [ "$status" -ge 64 ] || [ $((status & 1)) -ne 0 ]; then
  exit 1
fi
```

When an existing project adopts WorkshopC step by step, CI can enforce the preset the project has already reached while developers work toward the next one. See [the adoption path](configuration.md#the-adoption-path).

## Examples

```bash
workshopc src/                                                  # everything under src/, config found automatically
workshopc src/main.c src/parser.c                               # only these files
workshopc --config ci.workshopc.yaml -p out/ src/               # another config and build folder
workshopc --config default -p build/ src/                       # a built-in preset
workshopc --config default --third-party-include vendor/ --third-party-include CMSIS/ src/  # add to the preset's third-party folders
workshopc --config default --no-config-third-party-includes --third-party-include sdk/ src/ # replace the preset's third-party folders
workshopc --config embedded --prefix-top-dir firmware src/      # adjust the preset's namespace root
workshopc -q --sarif results.sarif src/                         # only a SARIF file, e.g. for CI
workshopc --text out.txt --json out.json src/                   # terminal output plus a text and a JSON file
workshopc -q --json - src/ | jq .errors                         # JSON to stdout, for piping
workshopc --warnings-as-errors src/                             # fail on any finding, e.g. in CI
workshopc --dump-config                                         # the config found, with every setting
```

---

[Next: Configuration →](configuration.md) · [Back to README](../README.md)
