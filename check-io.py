"""Compile/run portable I/O tests; never launch the game."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent

def main():
    output = ROOT / "build/io-tests"
    output.mkdir(parents=True, exist_ok=True)
    exe = output / "io-tests"
    subprocess.run([os.environ.get("CXX", "clang++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-g", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-DGRUNTZ_PORTABLE_TEST", "-I" + str(ROOT / "include"),
        str(ROOT / "src/Io/File.cpp"), str(ROOT / "src/Io/Settings.cpp"), str(ROOT / "src/Io/StreamArchive.cpp"), str(ROOT / "src/Io/Bytes.cpp"), str(ROOT / "src/Font/FontData.cpp"), str(ROOT / "tests/io.cpp"), "-o", str(exe)], check=True)
    with tempfile.TemporaryDirectory(prefix="gruntz-io-") as directory:
        subprocess.run([str(exe), str(Path(directory) / "roundtrip.bin")], check=True)
    print("Portable I/O tests passed (ASan/UBSan).")

if __name__ == "__main__":
    main()
