#!/usr/bin/env python3
"""cc_wrap.py - the `wine cl` compiler wrapper that ninja's `cl` rule invokes.

ninja drives the base/recompile side natively on Linux; the actual compiler is
MSVC 5.0's CL.EXE run under Wine. This wrapper does the Windows-path translation
(winepath -w) for the source + output object, runs

    wine cl.exe <flags> /Fo<obj.w> <src.w>

and exits non-zero (so ninja sees the failure) if cl did not produce the object.
That last check matters because Wine spews unrelated driver/EGL noise and can
return a non-cl exit code; the real success signal is "the .obj exists".

Toolchain + prefix come from the environment that `nix develop .#build` exports
(MSVC_DIR, WINEPREFIX). Run scripts/setup-toolchain.py once to initialise the
prefix (PATH/INCLUDE/LIB in the Wine registry) before the first build.

A persistent wineserver is kept alive (`wineserver -p`) so each TU's `wine cl`
reuses one server instead of paying the cold-start cost per object - this is
what makes `ninja -j` parallelism actually fast.

Usage (normally only emitted into build.ninja by configure.py):
    cc_wrap.py --out <obj> --src <src> -- <cl flags...>
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
GRUNTZ_DIR = SCRIPT_DIR.parent


def die(msg: str) -> None:
    print(f"[cc_wrap] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def find_ci(d: Path, name: str):
    """Case-insensitive file lookup (the toolchain mixes CL.EXE / cl.exe case)."""
    if not d.is_dir():
        return None
    for p in d.iterdir():
        if p.name.lower() == name.lower():
            return p
    return None


def msvc_dir() -> Path:
    """MSVC root: $MSVC_DIR (build dev shell) or /tmp/gtc/msvc fallback."""
    return Path(os.environ["MSVC_DIR"]) if os.environ.get("MSVC_DIR") \
        else Path("/tmp/gtc/msvc")


def winepath_w(p: Path) -> str:
    return subprocess.check_output(["winepath", "-w", str(p)], text=True).strip()


def ensure_wineserver() -> None:
    """Keep a persistent wineserver alive so each `wine cl` reuses it.

    `wineserver -p` makes the server persist after the last client exits, which
    avoids a cold start per TU under `ninja -j`. Harmless if one is already up.
    """
    ws = shutil.which("wineserver")
    if ws is None:
        return
    subprocess.run([ws, "-p"], check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main() -> None:
    ap = argparse.ArgumentParser(description="wine cl wrapper for ninja.")
    ap.add_argument("--out", required=True, help="output .obj path (Linux).")
    ap.add_argument("--src", required=True, help="source path (Linux).")
    ap.add_argument("flags", nargs=argparse.REMAINDER,
                    help="cl flags after `--` (e.g. /nologo /c /O2 /MT).")
    args = ap.parse_args()

    flags = args.flags
    if flags and flags[0] == "--":
        flags = flags[1:]

    msvc = msvc_dir()
    cl = find_ci(msvc / "bin", "cl.exe")
    if not cl:
        die(f"CL.EXE not found under {msvc}/bin - run inside `nix develop .#build` "
            "(or build .#gruntz-toolchain).")
    if shutil.which("wine") is None:
        die("wine not found - run inside `nix develop .#build`.")

    src = Path(args.src).resolve()
    out = Path(args.out).resolve()
    if not src.exists():
        die(f"source missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists():
        out.unlink()

    ensure_wineserver()

    src_w = winepath_w(src)
    out_w = winepath_w(out)
    cmd = ["wine", str(cl), *flags, f"/Fo{out_w}", src_w]

    r = subprocess.run(cmd, cwd=str(out.parent), text=True, capture_output=True)

    # Wine prints unrelated noise; the real success signal is the produced .obj.
    if not out.exists():
        sys.stderr.write(f"[cc_wrap] FAILED to compile {src.name} -> {out}\n")
        tail = "\n".join((r.stdout + r.stderr).strip().splitlines()[-12:])
        sys.stderr.write(tail + "\n")
        sys.exit(r.returncode or 1)

    sys.exit(0)


if __name__ == "__main__":
    main()
