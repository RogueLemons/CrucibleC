import sys
from pathlib import Path
import subprocess

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


def normalize(line: str) -> str:
    """
    Converts full compiler diagnostics into comparable message-only form.
    Keeps prefix (error:/warning:) but removes file/line/column.
    """
    line = line.strip()

    if "error:" in line:
        return "error: " + line.split("error:", 1)[1].strip()

    if "warning:" in line:
        return "warning: " + line.split("warning:", 1)[1].strip()

    return line


def collect_messages(output: str):
    """
    Extract normalized diagnostic messages.
    """
    messages = []

    for line in output.splitlines():
        line = line.strip()
        if not line:
            continue

        messages.append(normalize(line))

    return messages


def main():
    exe = get_exe()
    config = RELEASE / "workshopc_config.yaml"

    if not exe.exists():
        print("Missing release executable")
        sys.exit(1)

    if not config.exists():
        print("Missing config in release/")
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

        if expected_lines is None:
            print("--FAILED--")
            failed += 1
            continue

        result = subprocess.run(
            [
                str(exe),
                str(config),
                str(test),
                str(BUILD)
            ],
            text=True,
            capture_output=True
        )

        combined_output = result.stdout + "\n" + result.stderr

        if combined_output.strip():
            print(combined_output)

        # normalize actual output
        actual_messages = collect_messages(combined_output)
        actual_set = set(actual_messages)

        missing = []

        # expected is also normalized before comparison
        for expected in expected_lines:
            expected_norm = normalize(expected)

            if expected_norm not in actual_set:
                missing.append(expected)

        if result.returncode == 0 and not missing:
            print("--PASSED--")
            passed += 1
        else:
            print("--FAILED--")

            if missing:
                print("\nMissing expected diagnostics:")

                for m in missing:
                    print("  ", m)

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