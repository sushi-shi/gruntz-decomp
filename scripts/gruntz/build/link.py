#!/usr/bin/env python3
"""link.py - link the recompiled base objs into a candidate GRUNTZ.EXE (+ .map).

This is graph phase 2 (see docs/build-system.md). It runs the genuine VC5
`link.exe` (version 5.10.7303 - the linker that built retail GRUNTZ.EXE) under
wine over our base `.obj`s. The reconstruction is only partial (most of the game
is not decompiled yet), so the EXE is NOT runnable; we link it anyway, with
`/FORCE`, to study the LAYOUT the linker produces: the `.map` gives every
function's link-assigned RVA and its source object, which is what lets us
reverse-engineer the retail build order (intra-TU order = source-definition
order; cross-TU order = object link order). See docs/link-order-investigation.md.

What it does:
  1. ensure_msdis() - make MSDIS100.DLL resolvable so link.exe even loads under
     wine (real DLL if the toolchain has one, else a generated stub; link output
     is identical either way - see msdis_stub.py).
  2. assemble the obj list (a dir of <unit>.obj, explicit --obj, or an --order
     file giving the exact link order to test), winepath-translate every path,
     and write a `@response` file (VC5 link has a short argv limit under wine).
  3. run `wine link.exe @rsp`; success signal is "the .EXE exists" (wine spews
     unrelated noise and can return odd exit codes, exactly like cc_wrap.py).

Defaults are tuned for layout study, not a shippable binary:
  /FORCE /NODEFAULTLIB /SUBSYSTEM:WINDOWS /BASE:0x400000 /INCREMENTAL:NO /MAP
  /OPT:NOREF /OPT:NOICF   (keep EVERY function so the map is complete)

Run inside `nix develop`.
"""

import re
import argparse
import os
import shutil
import signal
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)
sys.path.insert(0, str(SCRIPT_DIR))
from msdis_stub import ensure_msdis  # noqa: E402


def die(msg: str) -> None:
    print(f"[link] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def find_ci(d: Path, name: str):
    if not d.is_dir():
        return None
    for p in d.iterdir():
        if p.name.lower() == name.lower():
            return p
    return None


def winepath_w(p) -> str:
    return subprocess.check_output(["winepath", "-w", str(Path(p).resolve())],
                                   text=True, stderr=subprocess.DEVNULL).strip()


def ensure_wineserver() -> None:
    ws = shutil.which("wineserver")
    if ws:
        subprocess.run([ws, "-p"], check=False, stdin=subprocess.DEVNULL,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def run_wine(cmd, cwd, produced: Path):
    """Run a wine command hang-proof; return (output, rc). Mirrors cc_wrap._run_cl:
    wine can leave a finished-but-unreaped grandchild holding stdio open, so log to
    a temp FILE (no pipe to block on), own process group, bounded wait."""
    timeout = float(os.environ.get("GRUNTZ_LINK_TIMEOUT", "300"))
    with tempfile.TemporaryFile() as logf:
        proc = subprocess.Popen(cmd, cwd=str(cwd), stdin=subprocess.DEVNULL,
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
            rc = 0 if produced.exists() else 1
        logf.seek(0)
        return logf.read().decode("utf-8", "replace"), rc


def collect_objs(args) -> list:
    """Resolve the obj list + their link ORDER. Priority:
       --order FILE  (one obj stem or path per line; blank/`#` ignored) - the
                     order is significant, this is how we test a hypothesised
                     retail link order;
       --obj ...     explicit paths, in the given order;
       --objs-dir    every *.obj in the dir, sorted by name (stable default).
    """
    objs_dir = Path(args.objs_dir)
    if args.order:
        objs = []
        for ln in Path(args.order).read_text().splitlines():
            s = ln.strip()
            if not s or s.startswith("#"):
                continue
            p = Path(s)
            if not p.suffix:
                p = objs_dir / f"{s}.obj"
            if not p.exists():
                die(f"order entry not found: {s} ({p})")
            objs.append(p)
        return objs
    if args.obj:
        return [Path(o) for o in args.obj]
    if not objs_dir.is_dir():
        die(f"--objs-dir not found: {objs_dir}")
    return sorted(objs_dir.glob("*.obj"))


def main() -> None:
    ap = argparse.ArgumentParser(description="VC5 link.exe wrapper (phase 2).")
    ap.add_argument("--out", default="build/exe/GRUNTZ.candidate.EXE")
    ap.add_argument("--map", dest="mapfile", default=None,
                    help="map path (default: <out> with .map suffix).")
    ap.add_argument("--objs-dir", default="build/objdiff/base")
    ap.add_argument("--obj", action="append", help="explicit obj (repeatable).")
    ap.add_argument("--order", help="file listing obj stems/paths in link order.")
    ap.add_argument("--lib", action="append", default=[],
                    help="extra import/static lib to pass to link (repeatable).")
    ap.add_argument("--base", default="0x400000", help="image base (/BASE).")
    ap.add_argument("--entry", default="_x", help="forced /ENTRY symbol.")
    ap.add_argument("--keep-all", dest="keep_all", action="store_true", default=True,
                    help="/OPT:NOREF /OPT:NOICF - keep every COMDAT (default).")
    ap.add_argument("--opt-ref", dest="keep_all", action="store_false",
                    help="let the linker strip/fold unreferenced COMDATs (/OPT:REF).")
    ap.add_argument("flags", nargs=argparse.REMAINDER,
                    help="extra link flags after `--`.")
    args = ap.parse_args()

    if shutil.which("wine") is None:
        die("wine not found - run inside `nix develop`.")
    msvc = Path(os.environ.get("MSVC_DIR", "/tmp/gtc/msvc"))
    link = find_ci(msvc / "bin", "link.exe")
    if not link:
        die(f"link.exe not found under {msvc}/bin - run inside `nix develop`.")
    prefix = os.environ.get("WINEPREFIX")
    if not prefix:
        die("WINEPREFIX not set - run inside `nix develop`.")

    out = Path(args.out).resolve()
    mapf = Path(args.mapfile).resolve() if args.mapfile else out.with_suffix(".map")
    out.parent.mkdir(parents=True, exist_ok=True)
    for f in (out, mapf):
        if f.exists():
            f.unlink()

    objs = collect_objs(args)
    if not objs:
        die("no objects to link.")

    # MSDIS100.DLL must resolve or link.exe won't even load under wine.
    ensure_msdis(prefix, msvc, verbose=True)
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-all")
    ensure_wineserver()

    rsp_lines = [
        f"/OUT:{winepath_w(out)}",
        f"/MAP:{winepath_w(mapf)}",
        "/NOLOGO", "/FORCE", "/NODEFAULTLIB", "/SUBSYSTEM:WINDOWS",
        f"/BASE:{args.base}", "/INCREMENTAL:NO", f"/ENTRY:{args.entry}",
    ]
    if args.keep_all:
        rsp_lines += ["/OPT:NOREF", "/OPT:NOICF"]
    extra = args.flags[1:] if args.flags and args.flags[0] == "--" else args.flags
    rsp_lines += list(extra)
    rsp_lines += [winepath_w(lib) if os.path.exists(lib) else lib for lib in args.lib]
    rsp_lines += [f'"{winepath_w(o)}"' for o in objs]

    rsp = out.parent / (out.stem + ".objs.rsp")
    rsp.write_text("\n".join(rsp_lines) + "\n")

    output, rc = run_wine(["wine", str(link), f"@{winepath_w(rsp)}"], out.parent, out)

    if not out.exists():
        sys.stderr.write(f"[link] FAILED to produce {out}\n")
        sys.stderr.write("\n".join(output.strip().splitlines()[-20:]) + "\n")
        sys.exit(rc or 1)

    # /FORCE means unresolved externals are EXPECTED (partial reconstruction);
    # surface the counts but treat the produced EXE as success.
    warns = sum(1 for ln in output.splitlines() if "LNK4006" in ln)
    unres_syms = sorted({m.group(1) for ln in output.splitlines()
                         if (m := re.search(r'unresolved external symbol (\S+)', ln))})
    # save the unresolved-externals punch-list (the drive-to-linkable worklist).
    unf = out.parent / (out.stem + ".unresolved.txt")
    unf.write_text("\n".join(unres_syms) + "\n")
    print(f"[link] {len(objs)} objs -> {out} ({out.stat().st_size} B) + {mapf.name}")
    print(f"[link] {len(unres_syms)} unresolved externals -> {unf.name}, "
          f"{warns} dup-symbol warnings (expected: partial reconstruction, /FORCE)")


if __name__ == "__main__":
    main()
