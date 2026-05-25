import os
import shutil
import subprocess
import sys
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent
BUILD_DIR = BASE_DIR / "build"

EXE_NAME = "workshopc.exe"


def run(cmd, env=None):
    print("\n>>>", " ".join(map(str, cmd)))
    subprocess.check_call(cmd, env=env)


def clean():
    print("Cleaning build directory...")
    shutil.rmtree(BUILD_DIR, ignore_errors=True)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)


def configure():
    print("Configuring...")

    env = os.environ.copy()
    env.pop("MSYSTEM", None)
    env.pop("MSYS2_PATH_TYPE", None)

    run([
        "cmake",
        "-S", str(BASE_DIR),
        "-B", str(BUILD_DIR),
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DLLVM_DIR=C:/msys64/ucrt64/lib/cmake/llvm",
        "-DClang_DIR=C:/msys64/ucrt64/lib/cmake/clang",
    ], env=env)


def build():
    print("Building...")

    run([
        "cmake",
        "--build", str(BUILD_DIR),
        "--clean-first",
        "-j"
    ])


def main():
    clean()
    configure()
    build()

    print("\nBuild complete")
    print("Release folder updated automatically via CMake")


if __name__ == "__main__":
    main()