"""Build/run portable frame timing on Linux and wasm32; never launch the game."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent
SOURCES = ["src/Runtime/FrameTiming.cpp", "src/Runtime/FrameScheduler.cpp", "src/Runtime/ShutdownRequest.cpp", "tests/timing.cpp"]
FLAGS = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
         "-DGRUNTZ_PORTABLE_TEST", "-I" + str(ROOT / "include")]


def executable(variable, default):
    name = os.environ.get(variable, default)
    found = shutil.which(name)
    if not found:
        raise SystemExit(f"Missing {name}; use nix develop .#portable")
    return found


def native():
    output = ROOT / "build/timing-tests/native"
    output.mkdir(parents=True, exist_ok=True)
    program = output / "timing-tests"
    subprocess.run([executable("CXX", "clang++"), *FLAGS,
                    "-fsanitize=address,undefined", "-fno-sanitize-recover=undefined",
                    "-fno-omit-frame-pointer",
                    *(str(ROOT / source) for source in SOURCES), "-o", str(program)], check=True)
    subprocess.run([str(program)], check=True)
    print("Native Linux timing tests passed (ASan/UBSan).", flush=True)


def wasm():
    output = ROOT / "build/timing-tests/wasm"
    output.mkdir(parents=True, exist_ok=True)
    program = output / "timing-tests.js"
    environment = os.environ.copy()
    environment.setdefault("EM_CACHE", str(ROOT / "build/emscripten-cache"))
    subprocess.run([executable("EMXX", "em++"), *FLAGS,
                    *(str(ROOT / source) for source in SOURCES),
                    "-sENVIRONMENT=node", "-sEXIT_RUNTIME=1", "-sASSERTIONS=2",
                    "-o", str(program)], env=environment, check=True)
    subprocess.run([executable("NODE", "node"), str(program)], check=True)
    print("wasm32 timing tests passed (Node).", flush=True)


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
