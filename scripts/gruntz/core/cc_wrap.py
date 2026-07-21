#!/usr/bin/env python3
"""cc_wrap.py - the `wine cl` compiler wrapper that ninja's `cl` rule invokes.

ninja drives the base/recompile side natively on Linux; the actual compiler is
MSVC 5.0's CL.EXE run under Wine. This wrapper does the Windows-path translation
(winepath -w) for the source + output object, runs

    wine cl.exe <flags> /Fo<obj.w> <src.w>

and exits non-zero (so ninja sees the failure) if cl did not produce the object.
That last check matters because Wine spews unrelated driver/EGL noise and can
return a non-cl exit code; the real success signal is "the .obj exists".

Toolchain + prefix come from the environment that `nix develop` exports
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
import signal
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
GRUNTZ_DIR = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)


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
    # stderr=DEVNULL is load-bearing: cc_wrap's own stderr IS ninja's capture pipe,
    # and winepath can be the call that boots the persistent (-p) Wine session. If
    # it inherits our stderr, the daemonised session holds ninja's pipe write-end
    # open forever -> ninja never sees EOF and the whole build hangs at zero CPU.
    return subprocess.check_output(["winepath", "-w", str(p)],
                                   text=True, stderr=subprocess.DEVNULL).strip()


def ensure_wineserver() -> None:
    """Keep a persistent wineserver alive so each `wine cl` reuses it.

    `wineserver -p` makes the server persist after the last client exits, which
    avoids a cold start per TU under `ninja -j`. Harmless if one is already up.
    """
    ws = shutil.which("wineserver")
    if ws is None:
        return
    subprocess.run([ws, "-p"], check=False, stdin=subprocess.DEVNULL,
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
        die(f"CL.EXE not found under {msvc}/bin - run inside `nix develop` "
            "(or build .#gruntz-toolchain).")
    if shutil.which("wine") is None:
        die("wine not found - run inside `nix develop`.")

    src = Path(args.src).resolve()
    out = Path(args.out).resolve()
    if not src.exists():
        die(f"source missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists():
        out.unlink()

    # Quieten Wine's fixme spam unless the caller wants it (smaller logs, less
    # overhead under `ninja -j`); an explicit WINEDEBUG still wins.
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-kerberos")
    ensure_wineserver()

    src_w = winepath_w(src)
    out_w = winepath_w(out)
    # Repo-local headers live under include/ (mirrors src/); put it on cl's search
    # path so `#include <Module/Foo.h>` resolves (winepath so cl sees it). Vendored
    # third-party SDK headers live under vendor/<sdk>/ (e.g. vendor/miles-6.0c/mss.h,
    # vendor/smacker-3.2f/smack.h) so `#include <Mss32.h>` resolves like the original
    # toolchain's SDK dirs; the DirectX 6 SDK headers sit one level deeper under
    # vendor/directx6/Include (the version the retail game was built against, pinned
    # ahead of the toolchain's own dx/Include).
    repo = next((p for p in src.parents if (p / "flake.nix").exists()), None)
    inc_dirs = []
    if repo:
        if (repo / "include").is_dir():
            inc_dirs.append(repo / "include")
        if (repo / "vendor").is_dir():
            inc_dirs += sorted(d for d in (repo / "vendor").iterdir() if d.is_dir())
        dx = repo / "vendor" / "directx6" / "Include"
        if dx.is_dir():
            inc_dirs.append(dx)
    inc_flags = [f"/I{winepath_w(d)}" for d in inc_dirs]
    cmd = ["wine", str(cl), *inc_flags, *flags, f"/Fo{out_w}", src_w]

    output, rc = _run_cl(cmd, out)

    # Wine prints unrelated noise; the real success signal is the produced .obj.
    if not out.exists():
        sys.stderr.write(f"[cc_wrap] FAILED to compile {src.name} -> {out}\n")
        tail = "\n".join(output.strip().splitlines()[-12:])
        sys.stderr.write(tail + "\n")
        sys.exit(rc or 1)

    sys.exit(0)


def _run_cl(cmd: list, out: Path) -> tuple[str, int]:
    """Run `wine cl` and return (combined output, returncode), hang-proof.

    Wine intermittently leaves a cl.exe grandchild (mspdbsrv/conhost/...) in a
    finished-but-never-reaped state that keeps the inherited stdio open. With a
    capture PIPE that wedges the parent forever waiting for EOF even though the
    .obj is already written - the build "hangs" at zero CPU for many minutes. So
    send wine's output to a temp FILE (no pipe to block on), run wine in its own
    process group, and bound the wait with a timeout: on a stall, SIGKILL our wine
    launcher group and trust the produced .obj as the real success signal.
    """
    timeout = float(os.environ.get("GRUNTZ_CL_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        proc = subprocess.Popen(cmd, cwd=str(out.parent), stdin=subprocess.DEVNULL,
                                stdout=logf, stderr=subprocess.STDOUT,
                                start_new_session=True)
        try:
            proc.wait(timeout=timeout)
            rc = proc.returncode
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
            proc.wait()
            rc = 0 if out.exists() else 1
        logf.seek(0)
        return logf.read().decode("utf-8", "replace"), rc


if __name__ == "__main__":
    main()
