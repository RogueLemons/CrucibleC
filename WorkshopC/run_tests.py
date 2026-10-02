import sys
import json
import re
from pathlib import Path
import subprocess
from collections import Counter

ROOT = Path(__file__).resolve().parent

RELEASE = ROOT / "release"
TESTS = ROOT / "tests"
CASES = TESTS / "cases"

# Compilation database for the test files, generated from
# tests/CMakeLists.txt. The test files are never built.
TESTS_COMPDB = ROOT / "build-tests"

# Where the output files test writes its text, JSON and SARIF files
TESTS_OUTPUT = TESTS / "output"

# The folder given whole to the parser, instead of a single file
FOLDER_INPUT = TESTS / "folder_input"

# Configs that break the checks of the config validation, each with the
# problems it must be reported for in <name>.expected.txt
BAD_CONFIG = TESTS / "bad_config"

# The test whose diagnostics are written to every output format at once
OUTPUT_FILES_TEST = CASES / "suppression_balance" / "suppression_balance.c"
DEFAULT_CONFIG_TESTS = TESTS / "default_configs"

CODE_PATTERN = re.compile(r" \[(CCW\d{4})\]$")


def generate_tests_compdb():
    """
    Configure (but never build) the tests project so that CMake writes
    compile_commands.json for the test files.
    """
    result = subprocess.run(
        [
            "cmake",
            "-S", str(TESTS),
            "-B", str(TESTS_COMPDB),
            "-G", "Ninja",
            "-DCMAKE_C_COMPILER=clang",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        ],
        text=True,
        capture_output=True
    )

    if result.returncode != 0 or not (TESTS_COMPDB / "compile_commands.json").exists():
        print("Failed to generate compile_commands.json for the tests")
        print(result.stdout)
        print(result.stderr)
        sys.exit(1)


def find_tests():
    """
    Every test case is a folder tests/cases/<name>/ holding <name>.c,
    <name>.workshopc.yaml and <name>.expected.txt.
    """
    return sorted(
        folder / f"{folder.name}.c"
        for folder in CASES.iterdir()
        if (folder / f"{folder.name}.c").is_file()
    )


def get_exe():
    if sys.platform == "win32":
        return RELEASE / "workshopc.exe"
    else:
        return RELEASE / "workshopc"


def load_expected(test_file):
    expected_file = test_file.with_suffix(".expected.txt")

    if not expected_file.exists():
        print(f"Missing expected file: {expected_file}")
        return None

    lines = [
        line.strip()
        for line in expected_file.read_text().splitlines()
        if line.strip()
    ]

    if not lines:
        print(f"Empty expected file: {expected_file}")
        return None

    return lines


def load_config(test_file):
    config_file = test_file.with_suffix(".workshopc.yaml")

    if not config_file.exists():
        print(f"Missing config file: {config_file}")
        return None

    return config_file


def normalize(line: str):
    """
    Extract diagnostic messages only.
    """

    line = line.strip()

    warning_index = line.find("warning:")
    if warning_index != -1:
        return "warning: " + line[warning_index + len("warning:"):].strip()

    error_index = line.find("error:")
    if error_index != -1:
        return "error: " + line[error_index + len("error:"):].strip()

    return None


def collect_messages(output: str):
    messages = []

    for line in output.splitlines():
        n = normalize(line)
        if n is not None:
            messages.append(n)

    return messages


def expected_exit_code(expected_counter):
    """
    The exit code a run with these diagnostics must end with: 1 for errors
    plus 2 for warnings, or 67 when the compiler itself reports an error,
    which is the only kind of message without a CCW code. Anything else,
    e.g. a crash, is a failure.
    """
    if any(not CODE_PATTERN.search(msg) for msg in expected_counter):
        return 67

    errors = any(msg.startswith("error:") for msg in expected_counter)
    warnings = any(msg.startswith("warning:") for msg in expected_counter)

    return (1 if errors else 0) | (2 if warnings else 0)


def check(condition, problem, problems):
    if not condition:
        problems.append(problem)


def run_output_files_test(exe):
    """
    Writes the diagnostics of one test as text, JSON and SARIF in a single
    quiet run, and checks every file against the test's expected file.
    """
    print("\n==============================")
    print("Running: output files (--quiet --text --json --sarif)")
    print("==============================")

    TESTS_OUTPUT.mkdir(exist_ok=True)

    text_file = TESTS_OUTPUT / "results.txt"
    json_file = TESTS_OUTPUT / "results.json"
    sarif_file = TESTS_OUTPUT / "results.sarif"

    for file in (text_file, json_file, sarif_file):
        file.unlink(missing_ok=True)

    result = subprocess.run(
        [
            str(exe),
            "--quiet",
            "--config", str(OUTPUT_FILES_TEST.with_suffix(".workshopc.yaml")),
            "-p", str(TESTS_COMPDB),
            "--text", str(text_file),
            "--json", str(json_file),
            "--sarif", str(sarif_file),
            str(OUTPUT_FILES_TEST)
        ],
        text=True,
        capture_output=True
    )

    problems = []

    expected = Counter(
        normalize(x)
        for x in load_expected(OUTPUT_FILES_TEST)
        if normalize(x) is not None
    )

    expected_warnings = sum(n for msg, n in expected.items() if msg.startswith("warning:"))
    expected_errors = sum(n for msg, n in expected.items() if msg.startswith("error:"))

    check(not (result.stdout + result.stderr).strip(),
          "--quiet printed output:\n" + result.stdout + result.stderr, problems)

    expected_exit = (1 if expected_errors else 0) | (2 if expected_warnings else 0)
    check(result.returncode == expected_exit,
          f"exit code {result.returncode}, expected {expected_exit}", problems)

    for file in (text_file, json_file, sarif_file):
        check(file.exists(), f"{file.name} was not written", problems)

    if problems:
        return report_output_problems(problems)

    # Text: the same lines as the terminal
    text = Counter(collect_messages(text_file.read_text()))
    check(text == expected, "results.txt does not match the expected diagnostics", problems)

    # JSON: code and name as their own fields, message without the code
    data = json.loads(json_file.read_text())
    check(data["warnings"] == expected_warnings, "wrong warning count in results.json", problems)
    check(data["errors"] == expected_errors, "wrong error count in results.json", problems)

    from_json = Counter(
        f"{d['level']}: {d['message']} [{d['code']}]"
        for d in data["diagnostics"]
    )
    check(from_json == expected, "results.json does not match the expected diagnostics", problems)
    check(all(d["name"] for d in data["diagnostics"]), "a diagnostic in results.json has no name", problems)

    # SARIF: ruleId on every result, described by the tool's rule list
    sarif = json.loads(sarif_file.read_text())
    check(sarif["version"] == "2.1.0", "results.sarif is not SARIF 2.1.0", problems)

    run = sarif["runs"][0]
    rules = run["tool"]["driver"]["rules"]

    from_sarif = Counter(
        f"{r['level']}: {r['message']['text']} [{r['ruleId']}]"
        for r in run["results"]
    )
    check(from_sarif == expected, "results.sarif does not match the expected diagnostics", problems)

    for r in run["results"]:
        check(rules[r["ruleIndex"]]["id"] == r["ruleId"],
              f"ruleIndex of {r['ruleId']} points at the wrong rule", problems)

    ids = [rule["id"] for rule in rules]
    check(len(ids) == len(set(ids)), "results.sarif lists a code twice", problems)
    check(all(CODE_PATTERN.match(" [" + i + "]") for i in ids),
          "results.sarif lists a code that is not CCWrrcc", problems)

    if problems:
        return report_output_problems(problems)

    print("--PASSED--")
    return True


def run_warnings_as_errors_test(exe):
    """
    Runs a test with both warnings and errors with --warnings-as-errors,
    and checks that every warning is reported as an error: on the terminal,
    in the JSON file, in the counts and in the exit code.
    """
    print("\n==============================")
    print("Running: --warnings-as-errors")
    print("==============================")

    TESTS_OUTPUT.mkdir(exist_ok=True)

    json_file = TESTS_OUTPUT / "warnings_as_errors.json"
    json_file.unlink(missing_ok=True)

    result = subprocess.run(
        [
            str(exe),
            "--warnings-as-errors",
            "--config", str(OUTPUT_FILES_TEST.with_suffix(".workshopc.yaml")),
            "-p", str(TESTS_COMPDB),
            "--json", str(json_file),
            str(OUTPUT_FILES_TEST)
        ],
        text=True,
        capture_output=True
    )

    problems = []

    # The expected diagnostics, with every warning turned into an error
    original = [
        normalize(x)
        for x in load_expected(OUTPUT_FILES_TEST)
        if normalize(x) is not None
    ]

    check(any(msg.startswith("warning:") for msg in original),
          "the test has no warnings to turn into errors", problems)

    expected = Counter(
        "error:" + msg[len("warning:"):] if msg.startswith("warning:") else msg
        for msg in original
    )

    terminal = Counter(collect_messages(result.stdout + "\n" + result.stderr))
    check(terminal == expected,
          "the terminal output does not report every warning as an error", problems)

    # Errors only, no warnings
    check(result.returncode == 1,
          f"exit code {result.returncode}, expected 1 (errors only)", problems)

    check(json_file.exists(), f"{json_file.name} was not written", problems)

    if problems:
        return report_output_problems(problems)

    data = json.loads(json_file.read_text())
    check(data["warnings"] == 0, "results.json still counts warnings", problems)
    check(data["errors"] == len(original), "wrong error count in the JSON file", problems)
    check(all(d["level"] == "error" for d in data["diagnostics"]),
          "a diagnostic in the JSON file is not an error", problems)

    if problems:
        return report_output_problems(problems)

    print("--PASSED--")
    return True


def run_folder_input_test(exe):
    """
    Gives the parser a folder instead of a file, and checks that it parses
    every .c file in it, also in subfolders, but no header and nothing in
    a third-party folder.
    """
    print("\n==============================")
    print("Running: folder input")
    print("==============================")

    result = subprocess.run(
        [
            str(exe),
            "--config", str(FOLDER_INPUT / "folder_input.workshopc.yaml"),
            "-p", str(TESTS_COMPDB),
            str(FOLDER_INPUT)
        ],
        text=True,
        capture_output=True
    )

    combined_output = result.stdout + "\n" + result.stderr

    if combined_output.strip():
        print(combined_output)

    problems = []

    expected = Counter(
        normalize(x)
        for x in (FOLDER_INPUT / "folder_input.expected.txt").read_text().splitlines()
        if normalize(x) is not None
    )

    actual = Counter(collect_messages(combined_output))
    check(actual == expected,
          "the diagnostics do not match folder_input.expected.txt", problems)

    # Each diagnostic comes from the file it belongs to
    for name in ("top_level.c", "nested.c"):
        check(name in combined_output, f"{name} was not parsed", problems)

    for name in ("not_a_source.h", "third_party.c"):
        check(name not in combined_output, f"{name} was parsed", problems)

    check(result.returncode == 1,
          f"exit code {result.returncode}, expected 1 (errors only)", problems)

    if problems:
        return report_output_problems(problems)

    print("--PASSED--")
    return True


def run_bad_config_test(exe):
    """
    Gives the parser each config in tests/bad_config/, which break the
    checks of the config validation, and checks that it reports exactly
    the problems in the config's expected file, one per line under
    'Invalid config', and stops before any analysis.
    """
    print("\n==============================")
    print("Running: bad config")
    print("==============================")

    problems = []
    configs = sorted(BAD_CONFIG.glob("*.workshopc.yaml"))

    check(configs, "no configs found in tests/bad_config", problems)

    for config in configs:
        name = config.name[:-len(".workshopc.yaml")]

        result = subprocess.run(
            [
                str(exe),
                "--config", str(config),
                "-p", str(TESTS_COMPDB),
                str(OUTPUT_FILES_TEST)
            ],
            text=True,
            capture_output=True
        )

        combined_output = result.stdout + "\n" + result.stderr

        if combined_output.strip():
            print(combined_output)

        expected = Counter(
            line.strip()
            for line in (BAD_CONFIG / f"{name}.expected.txt").read_text().splitlines()
            if line.strip()
        )

        # The problems are listed indented under the 'Invalid config' line
        lines = result.stderr.splitlines()
        header = next((i for i, line in enumerate(lines) if line.startswith("Invalid config:")), None)

        check(header is not None, f"{name}: no 'Invalid config' line was printed", problems)

        if header is not None:
            actual = Counter(
                line.strip()
                for line in lines[header + 1:]
                if line.startswith("  ")
            )

            for msg in expected - actual:
                problems.append(f"{name}: missing: {msg}")

            for msg in actual - expected:
                problems.append(f"{name}: unexpected: {msg}")

        check(not collect_messages(combined_output),
              f"{name}: diagnostics were reported, the analysis should not have run", problems)

        check(result.returncode == 65,
              f"{name}: exit code {result.returncode}, expected 65 (config failed)", problems)

    if problems:
        return report_output_problems(problems)

    print("--PASSED--")
    return True


def run_dump_config_test(exe):
    """
    Dumps the default config with --dump-config, and checks that the dump
    reads back as the same config and gives the same diagnostics as the
    original. Also checks that a normal run names the config and build
    folder it uses.
    """
    print("\n==============================")
    print("Running: --dump-config")
    print("==============================")

    TESTS_OUTPUT.mkdir(exist_ok=True)

    original = ROOT / "default" / "configs" / "default.workshopc.yaml"
    dumped = TESTS_OUTPUT / "dumped.workshopc.yaml"
    dumped.unlink(missing_ok=True)

    problems = []

    first = subprocess.run(
        [str(exe), "--config", str(original), "--dump-config"],
        text=True,
        capture_output=True
    )

    check(first.returncode == 0,
          f"--dump-config exit code {first.returncode}, expected 0", problems)
    check("rules:" in first.stdout, "--dump-config printed no rules", problems)

    if problems:
        return report_output_problems(problems)

    dumped.write_text(first.stdout)

    second = subprocess.run(
        [str(exe), "--config", str(dumped), "--dump-config"],
        text=True,
        capture_output=True
    )

    check(second.stdout == first.stdout,
          "the dumped config does not read back as the same config", problems)

    # The dump analyzes like the original
    def analyze(config):
        return subprocess.run(
            [
                str(exe),
                "--config", str(config),
                "-p", str(TESTS_COMPDB),
                str(OUTPUT_FILES_TEST)
            ],
            text=True,
            capture_output=True
        )

    from_original = analyze(original)
    from_dump = analyze(dumped)

    check(Counter(collect_messages(from_dump.stderr)) ==
          Counter(collect_messages(from_original.stderr)),
          "the dumped config gives other diagnostics than the original", problems)
    check(from_dump.returncode == from_original.returncode,
          f"exit code {from_dump.returncode} with the dump, "
          f"{from_original.returncode} with the original", problems)

    # A normal run names the files it uses
    check(f"Config: {original.resolve().as_posix()}" in from_original.stderr.replace("\\", "/"),
          "the config file used was not printed", problems)
    check(f"Build folder: {TESTS_COMPDB.resolve().as_posix()}" in from_original.stderr.replace("\\", "/"),
          "the build folder used was not printed", problems)

    if problems:
        return report_output_problems(problems)

    print("--PASSED--")
    return True


def run_config_override_test(exe):
    """Checks config overrides and their --dump-config output."""
    print("\n==============================")
    print("Running: config overrides")
    print("==============================")

    problems = []
    config_file = ROOT / "default" / "configs" / "default.workshopc.yaml"
    config_sources = [
        ("config file", str(config_file)),
        ("built-in preset", "default"),
    ]
    expected_lines = {
        "  - external/",
        "  - cli_vendor_one/",
        "  - cli_vendor_two/",
        "    top_dir: cli_source_root",
    }

    for source_kind, config_source in config_sources:
        result = subprocess.run(
            [
                str(exe),
                "--config", config_source,
                "--third-party-include", "cli_vendor_one/",
                "--third-party-include=cli_vendor_two/",
                "--prefix-top-dir", "cli_source_root",
                "--dump-config",
            ],
            text=True,
            capture_output=True
        )

        check(result.returncode == 0,
              f"{source_kind}: --dump-config exit code {result.returncode}, expected 0",
              problems)

        dumped_lines = set(result.stdout.splitlines())
        for expected in expected_lines:
            check(expected in dumped_lines,
                  f"{source_kind}: --dump-config is missing override {expected!r}",
                  problems)

        check(not result.stderr.strip(),
              f"{source_kind}: unexpected stderr:\n{result.stderr}",
              problems)

        # Dropping the config's list keeps only the command-line folders
        replaced = subprocess.run(
            [
                str(exe),
                "--config", config_source,
                "--no-config-third-party-includes",
                "--third-party-include", "cli_vendor_one/",
                "--dump-config",
            ],
            text=True,
            capture_output=True
        )

        check(replaced.returncode == 0,
              f"{source_kind}: --no-config-third-party-includes exit code "
              f"{replaced.returncode}, expected 0",
              problems)

        replaced_lines = set(replaced.stdout.splitlines())
        check("  - cli_vendor_one/" in replaced_lines,
              f"{source_kind}: --no-config-third-party-includes dropped the "
              "--third-party-include folder",
              problems)
        check("  - external/" not in replaced_lines,
              f"{source_kind}: --no-config-third-party-includes kept the "
              "config's folders",
              problems)

        # Without folders of its own, the list is empty
        emptied = subprocess.run(
            [
                str(exe),
                "--config", config_source,
                "--no-config-third-party-includes",
                "--dump-config",
            ],
            text=True,
            capture_output=True
        )

        check("third_party_includes: []" in emptied.stdout.splitlines(),
              f"{source_kind}: --no-config-third-party-includes alone did not "
              "empty the list",
              problems)

    if problems:
        return report_output_problems(problems)

    print("--PASSED--")
    return True


def run_default_configs_test(exe):
    """Checks representative behavior for every built-in config preset."""
    print("\n==============================")
    print("Running: built-in config presets")
    print("==============================")

    problems = []
    cases = sorted(DEFAULT_CONFIG_TESTS.glob("*.c"))
    check(cases, "no C files found in tests/default_configs", problems)

    for source in cases:
        preset = source.stem
        expected_file = source.with_suffix(".expected.txt")
        check(expected_file.is_file(), f"{preset}: missing expected file", problems)

        if not expected_file.is_file():
            continue

        result = subprocess.run(
            [
                str(exe),
                "--config", preset,
                "-p", str(TESTS_COMPDB),
                str(source),
            ],
            text=True,
            capture_output=True
        )

        combined_output = result.stdout + "\n" + result.stderr

        if combined_output.strip():
            print(combined_output)

        expected = Counter(
            normalize(line)
            for line in expected_file.read_text().splitlines()
            if normalize(line) is not None
        )
        actual = Counter(collect_messages(combined_output))

        if expected != actual:
            for message, count in (expected - actual).items():
                problems.append(
                    f"{preset}: missing expected diagnostic: {message}"
                    + (f" (missing {count})" if count > 1 else "")
                )
            for message, count in (actual - expected).items():
                problems.append(
                    f"{preset}: unexpected diagnostic: {message}"
                    + (f" (extra {count})" if count > 1 else "")
                )

        expected_exit = expected_exit_code(expected)
        check(result.returncode == expected_exit,
              f"{preset}: exit code {result.returncode}, expected {expected_exit}",
              problems)

    if problems:
        return report_output_problems(problems)

    print("--PASSED--")
    return True


def report_output_problems(problems):
    print("--FAILED--")

    for problem in problems:
        print(f"  {problem}")

    return False


def main():
    exe = get_exe()

    if not exe.exists():
        print("Missing release executable")
        sys.exit(1)

    generate_tests_compdb()

    test_files = find_tests()

    if not test_files:
        print("No test files found")
        sys.exit(1)

    passed = 0
    failed = 0

    for test in test_files:
        print("\n==============================")
        print(f"Running: {test}")
        print("==============================")

        expected_lines = load_expected(test)
        config_file = load_config(test)

        if expected_lines is None or config_file is None:
            print("--FAILED--")
            failed += 1
            continue

        result = subprocess.run(
            [
                str(exe),
                "--config", str(config_file),
                "-p", str(TESTS_COMPDB),
                str(test)
            ],
            text=True,
            capture_output=True
        )

        combined_output = result.stdout + "\n" + result.stderr

        if combined_output.strip():
            print(combined_output)

        actual_messages = collect_messages(combined_output)

        expected_counter = Counter(
            normalize(x)
            for x in expected_lines
            if normalize(x) is not None
        )

        actual_counter = Counter(actual_messages)

        missing = []
        unexpected = []

        for msg, expected_count in expected_counter.items():
            actual_count = actual_counter.get(msg, 0)
            if actual_count < expected_count:
                missing.append((msg, expected_count - actual_count))

        for msg, actual_count in actual_counter.items():
            expected_count = expected_counter.get(msg, 0)
            if actual_count > expected_count:
                unexpected.append((msg, actual_count - expected_count))

        expected_exit = expected_exit_code(expected_counter)

        success = (
            not missing and
            not unexpected and
            result.returncode == expected_exit
        )

        if success:
            print("--PASSED--")
            passed += 1
        else:
            print("--FAILED--")

            if missing:
                print("\nMissing expected diagnostics:")
                for msg, count in missing:
                    print(f"  {msg}" + (f" (missing {count})" if count > 1 else ""))

            if unexpected:
                print("\nUnexpected diagnostics:")
                for msg, count in unexpected:
                    print(f"  {msg}" + (f" (extra {count})" if count > 1 else ""))

            if result.returncode != expected_exit:
                print(f"\nExit code {result.returncode}, expected {expected_exit}")

            failed += 1

    special_tests = [
        run_output_files_test,
        run_warnings_as_errors_test,
        run_folder_input_test,
        run_bad_config_test,
        run_dump_config_test,
        run_config_override_test,
        run_default_configs_test,
    ]

    for special_test in special_tests:
        if special_test(exe):
            passed += 1
        else:
            failed += 1

    print("\n===================")
    print("Test Summary")
    print("===================")
    print(f"Total : {len(test_files) + len(special_tests)}")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")

    if failed > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()