"""Compile/run only a console REZ component test under Wine. Never launch the game."""
import os
from pathlib import Path
import sys
import tempfile
ROOT = Path(__file__).resolve().parent
os.environ["GRUNTZ_DIR"] = str(ROOT)
os.environ["WINEPREFIX"] = str(ROOT / "build/wineprefix")
sys.path.insert(0, str(ROOT / "scripts"))
from gruntzbuild.tool import cl, link
from gruntzbuild.tool.wine import init_prefix, verify_prefix, winepath, run, shutdown_wineserver
SOURCES = ["src/Io/File.cpp", "src/Io/Bytes.cpp", "src/Rez/ArchiveData.cpp",
    "src/Rez/RezArchive.cpp", "src/Rez/RezFile.cpp", "src/Rez/RezHash.cpp",
    "src/Lith/BaseList.cpp", "src/Lith/VirtList.cpp", "src/Lith/BaseHash.cpp",
    "tests/rez_runtime.cpp"]
def main():
    output = ROOT / "build/rez-tests/windows"
    output.mkdir(parents=True, exist_ok=True)
    try:
        init_prefix(); verify_prefix()
        objects = []
        for source in SOURCES:
            obj = output / (Path(source).stem + ".obj")
            cl.compile(ROOT / source, obj, ["/nologo", "/c", "/O2", "/MT", "/GX"])
            objects.append(obj)
        binary = output / "rez-tests.exe"
        link.link(["/NOLOGO", "/SUBSYSTEM:CONSOLE", "/OUT:" + winepath(binary),
            *(winepath(obj) for obj in objects), "nafxcw.lib", "libcmt.lib", "kernel32.lib",
            "user32.lib", "gdi32.lib", "advapi32.lib", "shell32.lib", "comdlg32.lib", "winspool.lib"],
            cwd=output, expect=[binary])
        with tempfile.TemporaryDirectory(prefix="fixtures-", dir=output) as directory:
            text, code = run(["wine", str(binary), winepath(Path(directory) / "fixture.rez")], timeout=60)
            print(text, end="")
            if code or "REZ production import" not in text:
                raise SystemExit(code or 1)
    finally:
        shutdown_wineserver()
if __name__ == "__main__":
    main()
