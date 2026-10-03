"""Build and run the portable text/sequence tests; does not build or launch the game."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent

def main():
    build = ROOT / "build/stdlib-tests"
    build.mkdir(parents=True, exist_ok=True)
    executable = build / "stdlib-tests"
    subprocess.run([
        os.environ.get("CXX", "clang++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-DGRUNTZ_PORTABLE_TEST", "-D_GLIBCXX_ASSERTIONS", "-I" + str(ROOT / "include"),
        str(ROOT / "src/Utils/Text.cpp"), str(ROOT / "tests/stdlib.cpp"),
        "-o", str(executable)
    ], check=True)
    subprocess.run([str(executable)], check=True)
    print("Portable stdlib tests passed (ASan/UBSan).")

if __name__ == "__main__":
    main()
