"""Run portable PCX/PID decoder tests; read optional local archives without launching the game."""
import argparse
import os
from pathlib import Path
import subprocess
from importlib.machinery import SourceFileLoader
io = SourceFileLoader("io_checks", str(Path(__file__).with_name("check-io.py"))).load_module()
ROOT = Path(__file__).resolve().parent
SOURCES = ["src/Io/File.cpp", "src/Io/Bytes.cpp", "src/Rez/ArchiveData.cpp", "src/Image/RasterData.cpp", "tests/raster.cpp"]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", choices=["native", "wasm", "all"], default="native")
    parser.add_argument("archives", nargs="*", type=Path)
    args = parser.parse_args()
    sources = [str(ROOT / path) for path in SOURCES]
    output = ROOT / "build/raster-tests"
    output.mkdir(parents=True, exist_ok=True)
    if args.target in ("native", "all"):
        binary = output / "native"
        subprocess.run([io.executable("CXX", "clang++"), *io.FLAGS,
            "-fsanitize=address,undefined", "-fno-sanitize-recover=undefined", "-fno-omit-frame-pointer",
            *sources, "-o", str(binary)], check=True)
        subprocess.run([str(binary), *(str(path.resolve()) for path in args.archives)], check=True)
    if args.target in ("wasm", "all"):
        binary = output / "wasm.js"
        environment = os.environ.copy()
        environment.setdefault("EM_CACHE", str(ROOT / "build/emscripten-cache"))
        subprocess.run([io.executable("EMXX", "em++"), *io.FLAGS, *sources,
            "-sENVIRONMENT=node", "-sEXIT_RUNTIME=1", "-sASSERTIONS=2",
            "-sALLOW_MEMORY_GROWTH=1", "-o", str(binary)], env=environment, check=True)
        subprocess.run([io.executable("NODE", "node"), str(binary)], check=True)

if __name__ == "__main__":
    main()
