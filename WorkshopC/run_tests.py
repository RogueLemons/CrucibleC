import sys
from pathlib import Path
import subprocess
from collections import Counter

ROOT = Path(__file__).resolve().parent

RELEASE = ROOT / "release"
BUILD = ROOT / "build"
TESTS = ROOT / "tests"


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
    config_file = test_file.with_suffix(".config.yaml")

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


def main():
    exe = get_exe()

    if not exe.exists():
        print("Missing release executable")
        sys.exit(1)

    test_files = list(TESTS.rglob("*.c"))

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
                str(config_file),
                str(test),
                str(BUILD)
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

    print("\n===================")
    print("Test Summary")
    print("===================")
    print(f"Total : {len(test_files)}")
    print(f"Passed: {passed}")
    print(f"Failed: {failed}")

    if failed > 0:
        sys.exit(1)


if __name__ == "__main__":
    main()