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

# The test whose diagnostics are written to every output format at once
OUTPUT_FILES_TEST = CASES / "suppression_balance" / "suppression_balance.c"

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

        success = (not missing and not unexpected)

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

            if result.returncode != 0:
                print(f"\nProcess returned non-zero exit code: {result.returncode}")

            failed += 1

    special_tests = [run_output_files_test, run_warnings_as_errors_test, run_folder_input_test]

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