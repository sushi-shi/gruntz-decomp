#!/usr/bin/env python3
"""
setup-toolchain.py - set up the Wine environment so `wine cl.exe` builds against
our MSVC 5.0 SP3 toolchain (the byte-match toolchain for the Gruntz decompilation).

Sets up the Wine prefix for our (older, simpler) toolchain:
  * MSVC *5.0* (VS97 + SP3). VC++ 5.0 BUNDLES the Win32 API
    headers/libs in VC/INCLUDE + VC/LIB (merged into msvc/include + msvc/lib by
    the toolchain build), so there is NO separate Windows SDK to wire. Only the
    DirectX 6 SDK (dx/) is layered on top.
  * No vcproj2ninja / .sln / target-side generation - we have no project tree
    yet; the base/recompile side starts as hand-compiled `.cpp -> .obj` fed to
    objdiff. (When a real source tree exists, re-add a graph step here.)

Run inside `nix develop .#build`, which exports:
  MSVC_DIR   - toolchain msvc/ dir  (bin/CL.EXE, include/, lib/)
  DXSDK_DIR  - toolchain dx/  dir   (Include/, Lib/)
  WINEPREFIX - Wine prefix to set up
(optional: NINJA_DIR, GRUNTZ_DIR)

Usage:
  python3 scripts/setup-toolchain.py           # init prefix + set PATH/INCLUDE/LIB
  python3 scripts/setup-toolchain.py --smoke    # + compile a hello-world to verify cl runs
  python3 scripts/setup-toolchain.py --force     # re-init the prefix and reconfigure

Note: a plain `cl /c` (no /Zi) needs no PDB writer; if you later compile with /Zi
and hit "C1902 / mspdb50.dll not found", add MSPDB50.DLL (VS97 Disc 3
SHAREDIDE/BIN) next to cl.exe and use wine-staging (see docs/toolchain-vc50-sp3.md).
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

# Native Windows runs cl.exe directly: no wine prefix, no wine registry. The
# header/lib search dirs are ordinary INCLUDE/LIB env vars (exported by
# build/win-toolchain/activate.{ps1,bat}); see docs/windows-setup.md.
IS_WINDOWS = os.name == "nt"


def log(m): print("[setup] " + m, flush=True)


def die(m):
    log("ERROR: " + m)
    sys.exit(1)


def require_env(name):
    v = os.environ.get(name)
    if not v:
        die(f"{name} not set - run this inside `nix develop .#build`")
    return v


def find_ci(d, name):
    """Case-insensitive file lookup (the toolchain mixes CL.EXE / link.exe case)."""
    d = Path(d)
    if not d.is_dir():
        return None
    for p in d.iterdir():
        if p.name.lower() == name.lower():
            return p
    return None


def winepath_w(p):
    return subprocess.check_output(["winepath", "-w", str(p)], text=True).strip()


def wine_reg(*args, capture=False):
    kw = {"check": False, "text": True}
    if capture:
        kw["stdout"] = subprocess.PIPE
        kw["stderr"] = subprocess.PIPE
    else:
        kw["stdout"] = subprocess.DEVNULL
        kw["stderr"] = subprocess.DEVNULL
    return subprocess.run(["wine", "reg", *args], **kw)


def init_prefix(prefix, force=False):
    if not force and (prefix / "drive_c").is_dir():
        log("Wine prefix already initialised.")
        return
    prefix.mkdir(parents=True, exist_ok=True)
    log(f"Initialising Wine prefix at {prefix} ...")
    subprocess.run(["wineboot", "--init"], check=True)
    subprocess.run(["wineserver", "--wait"], check=False)


def configure_registry(msvc, dx):
    """Set PATH/INCLUDE/LIB in the Wine registry so `wine cl` finds tools/headers/libs.
    INCLUDE/LIB list the VC5 dirs first (CRT + Win32 + MFC, all bundled), then DX6."""
    vc_bin = winepath_w(msvc / "bin")
    include = ";".join([winepath_w(msvc / "include"), winepath_w(dx / "Include")])
    lib = ";".join([winepath_w(msvc / "lib"), winepath_w(dx / "Lib")])

    reg = (r"HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control"
           r"\Session Manager\Environment")

    cur = wine_reg("query", reg, "/v", "PATH", capture=True)
    cur_path = ""
    for line in cur.stdout.splitlines():
        if "REG_" in line:
            cur_path = line.split()[-1]
            break

    if not cur_path:
        new_path = f"{vc_bin};%SystemRoot%\\system32;%SystemRoot%"
    elif vc_bin not in cur_path:
        new_path = f"{vc_bin};{cur_path}"
    else:
        new_path = None

    if new_path is not None:
        wine_reg("add", reg, "/v", "PATH", "/t", "REG_EXPAND_SZ", "/d", new_path, "/f")
    wine_reg("add", reg, "/v", "INCLUDE", "/t", "REG_SZ", "/d", include, "/f")
    wine_reg("add", reg, "/v", "LIB", "/t", "REG_SZ", "/d", lib, "/f")

    log("Wine environment configured:")
    log(f"  PATH   += {vc_bin}")
    log(f"  INCLUDE = {include}")
    log(f"  LIB     = {lib}")


def smoke_test(msvc, gruntz_dir):
    cl = find_ci(msvc / "bin", "cl.exe")
    if not cl:
        die(f"CL.EXE not found under {msvc}/bin")
    work = gruntz_dir / "build" / "smoke"
    work.mkdir(parents=True, exist_ok=True)
    src = work / "hello.cpp"
    src.write_text('#include <stdio.h>\nint main(void){ printf("hello from VC5\\n"); return 0; }\n')
    obj = work / "hello.obj"
    if obj.exists():
        obj.unlink()
    log(f"Smoke test: wine cl /c {src.name}")
    r = subprocess.run(
        ["wine", str(cl), "/nologo", "/c", winepath_w(src)],
        cwd=str(work), text=True, capture_output=True,
    )
    if r.stdout.strip():
        print(r.stdout)
    if r.stderr.strip():
        print(r.stderr, file=sys.stderr)
    if obj.exists():
        log(f"OK - produced {obj} ({obj.stat().st_size} bytes) - the toolchain compiles.")
    else:
        die("smoke compile produced no .obj - check the cl output above "
            "(missing INCLUDE? missing DLL next to cl.exe? need wine-staging?).")


def smoke_test_native(msvc, gruntz_dir):
    """Native (no-wine) variant of smoke_test - just `cl /c hello.cpp`."""
    cl = find_ci(msvc / "bin", "cl.exe")
    work = gruntz_dir / "build" / "smoke"
    work.mkdir(parents=True, exist_ok=True)
    src = work / "hello.cpp"
    src.write_text('#include <stdio.h>\nint main(void){ printf("hello from VC5\\n"); return 0; }\n')
    obj = work / "hello.obj"
    if obj.exists():
        obj.unlink()
    log(f"Smoke test: cl /c {src.name}")
    r = subprocess.run([str(cl), "/nologo", "/c", str(src)],
                       cwd=str(work), text=True, capture_output=True)
    if r.stdout.strip():
        print(r.stdout)
    if r.stderr.strip():
        print(r.stderr, file=sys.stderr)
    if obj.exists():
        log(f"OK - produced {obj} ({obj.stat().st_size} bytes) - the toolchain compiles.")
    else:
        die("smoke compile produced no .obj - check INCLUDE/LIB and the cl output above.")


def windows_setup(msvc, dx, gruntz_dir, smoke=False):
    """Native MSVC 5.0: nothing to register, no prefix. Ensure INCLUDE/LIB exist
    (the activate scripts set them; default them here so a standalone run + smoke
    still works)."""
    os.environ.setdefault(
        "INCLUDE", os.pathsep.join([str(msvc / "include"), str(dx / "Include")]))
    os.environ.setdefault(
        "LIB", os.pathsep.join([str(msvc / "lib"), str(dx / "Lib")]))
    log("native Windows: no wine prefix needed; INCLUDE/LIB come from the env.")
    if smoke:
        smoke_test_native(msvc, gruntz_dir)
    log("done.")


def main():
    ap = argparse.ArgumentParser(description="Set up the MSVC-5.0 build environment "
                                             "(Wine on Linux; native on Windows).")
    ap.add_argument("--force", action="store_true", help="re-init the Wine prefix (Linux)")
    ap.add_argument("--smoke", action="store_true", help="compile a hello-world to verify cl runs")
    args = ap.parse_args()

    msvc = Path(require_env("MSVC_DIR"))
    dx = Path(require_env("DXSDK_DIR"))
    gruntz_dir = Path(os.environ.get("GRUNTZ_DIR", os.getcwd()))

    if not find_ci(msvc / "bin", "cl.exe"):
        die(f"CL.EXE not under {msvc}/bin - is the toolchain provisioned?")

    if IS_WINDOWS:
        windows_setup(msvc, dx, gruntz_dir, smoke=args.smoke)
        return

    prefix = Path(require_env("WINEPREFIX"))
    os.environ["WINEPREFIX"] = str(prefix)
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    os.environ.setdefault("WINEDLLOVERRIDES", "mscoree,mshtml=")

    init_prefix(prefix, force=args.force)
    configure_registry(msvc, dx)
    if args.smoke:
        smoke_test(msvc, gruntz_dir)
    log("done.")


if __name__ == "__main__":
    main()
