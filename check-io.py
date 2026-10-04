"""Build/run the same portable I/O suite on Linux and wasm32; never launch the game."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent
SOURCES = ["src/Io/File.cpp", "src/Io/FileTransaction.cpp", "src/Io/SavePaths.cpp", "src/Io/Bytes.cpp", "src/Io/StreamArchive.cpp",
           "src/Io/Settings.cpp", "src/Font/FontData.cpp", "tests/io.cpp", "tests/save_transactions.cpp"]
FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
         "-DGRUNTZ_PORTABLE_TEST", "-I" + str(ROOT / "include")]


def executable(variable, default):
    name = os.environ.get(variable, default)
    found = shutil.which(name)
    if not found:
        raise SystemExit(f"Missing {name}; use nix develop .#portable")
    return found


def native():
    output = ROOT / "build/io-tests/native"
    output.mkdir(parents=True, exist_ok=True)
    program = output / "io-tests"
    subprocess.run([executable("CXX", "clang++"), *FLAGS,
                    "-fsanitize=address,undefined", "-fno-sanitize-recover=undefined",
                    "-fno-omit-frame-pointer",
                    *(str(ROOT / source) for source in SOURCES), "-o", str(program)], check=True)
    with tempfile.TemporaryDirectory(prefix="gruntz-io-") as directory:
        subprocess.run([str(program), str(Path(directory) / "roundtrip.bin")], check=True)
    print("Native Linux I/O tests passed (ASan/UBSan).", flush=True)


def wasm():
    output = ROOT / "build/io-tests/wasm"
    output.mkdir(parents=True, exist_ok=True)
    program = output / "io-tests.js"
    environment = os.environ.copy()
    environment.setdefault("EM_CACHE", str(ROOT / "build/emscripten-cache"))
    subprocess.run([executable("EMXX", "em++"), *FLAGS,
                    *(str(ROOT / source) for source in SOURCES),
                    "-sENVIRONMENT=node", "-sEXIT_RUNTIME=1", "-sASSERTIONS=2",
                    "-sALLOW_MEMORY_GROWTH=1", "-o", str(program)],
                   env=environment, check=True)
    # This path belongs to Emscripten MEMFS, not the host filesystem.
    subprocess.run([executable("NODE", "node"), str(program), "/roundtrip.bin"], check=True)
    print("wasm32 I/O tests passed (Node + Emscripten MEMFS).", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", choices=["native", "wasm", "all"], default="native")
    args = parser.parse_args()
    if args.target in ("native", "all"):
        native()
    if args.target in ("wasm", "all"):
        wasm()


if __name__ == "__main__":
    main()
