"""Compile/run only a console application shutdown component test under Wine. Never launch the game."""
import os
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parent
os.environ["GRUNTZ_DIR"] = str(ROOT)
os.environ["WINEPREFIX"] = str(ROOT / "build/wineprefix")
sys.path.insert(0, str(ROOT / "scripts"))
from gruntzbuild.tool import cl, link
from gruntzbuild.tool.wine import init_prefix, verify_prefix, winepath, run, shutdown_wineserver
SOURCES = ["src/Runtime/FrameTiming.cpp", "src/Runtime/FrameScheduler.cpp", "src/Runtime/ShutdownRequest.cpp", "src/Wap32/GameApp.cpp", "src/Wap32/GameWnd.cpp", "tests/shutdown_app.cpp"]
def main():
    output = ROOT / "build/shutdown-tests/windows"
    output.mkdir(parents=True, exist_ok=True)
    try:
        init_prefix(); verify_prefix()
        objects = []
        for source in SOURCES:
            obj = output / (Path(source).stem + ".obj")
            cl.compile(ROOT / source, obj, ["/nologo", "/c", "/O2", "/MT", "/GX"])
            objects.append(obj)
        binary = output / "shutdown-app-tests.exe"
        link.link(["/NOLOGO", "/SUBSYSTEM:CONSOLE", "/OUT:" + winepath(binary),
            *(winepath(obj) for obj in objects), "nafxcw.lib", "libcmt.lib", "kernel32.lib",
            "winmm.lib", "user32.lib", "gdi32.lib", "advapi32.lib", "shell32.lib", "comdlg32.lib", "winspool.lib"],
            cwd=output, expect=[binary])
        text, code = run(["wine", str(binary)], timeout=60)
        print(text, end="")
        if code or "Production application shutdown callback" not in text:
            raise SystemExit(code or 1)
    finally:
        shutdown_wineserver()
if __name__ == "__main__":
    main()
