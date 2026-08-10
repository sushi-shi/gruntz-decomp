#!/usr/bin/env python3
"""link.py - link the recompiled base objs into a candidate GRUNTZ.EXE (+ .map).

This is graph phase 2 (see docs/build-system.md). It runs the genuine VC5
`link.exe` (version 5.10.7303 - the linker that built retail GRUNTZ.EXE) under
wine over our base `.obj`s. The `.map` gives every function's link-assigned RVA
and its source object, which is what lets us reverse-engineer the retail build
order (intra-TU order = source-definition order; cross-TU order = object link
order). See docs/link-order-investigation.md.

**There is no `/FORCE`, and it must never come back** (2026-08-03). It was the
scaffolding for the partial-reconstruction era; the tree now links with ZERO
unresolved externals and ZERO duplicate symbols, so a real link is available -
and a real link is an ORACLE. `/FORCE` would silently re-swallow exactly the
defects this phase exists to catch: an unresolved extern (a fabricated name, a
body homed nowhere) and an LNK2005/LNK4006 duplicate (an identity defect - a
symbol the CRT/MFC owns that we also defined, or one global defined in two TUs).
Every one of those the link found was a real bug. Dropping it also un-blocks
`/INCREMENTAL`, which ANY `/FORCE` makes link ignore (LNK4075) - and retail IS
an incremental link (the E9 thunk band at the top of .text, the padded IAT, the
separate .idata), so incremental is now reachable for the layout campaign.
A link failure here is a FINDING: read the LNK codes, fix the source.

What it does:
  1. ensure_msdis() - make MSDIS100.DLL resolvable so link.exe even loads under
     wine (real DLL if the toolchain has one, else a generated stub; link output
     is identical either way - see msdis_stub.py).
  2. assemble the obj list (a dir of <unit>.obj, explicit --obj, or an --order
     file giving the exact link order to test), winepath-translate every path,
     and write a `@response` file (VC5 link has a short argv limit under wine).
  3. resolve the LIBRARY set (see below) and append it to the response file.
  4. run `wine link.exe @rsp`; success signal is "the .EXE exists" (wine spews
     unrelated noise and can return odd exit codes, exactly like cc_wrap.py).

Libraries. The objs cl.exe emits already carry the retail lib set in their
`.drectve` directives - `/MT` writes `-defaultlib:LIBCMT` + `-defaultlib:OLDNAMES`,
and MFC's headers add `nafxcw kernel32 user32 gdi32 comdlg32 winspool advapi32
shell32 comctl32` - so we simply do NOT pass `/NODEFAULTLIB` and let them fire,
which is exactly what the devs' link did. Three groups declare themselves nowhere
and must be named explicitly, all corroborated by retail's own import table
(`gruntz.core.pe.PE.imports`):

  * `version.lib` + `winmm.lib` - imported by the game, requested by no header;
  * DirectX 6 (`ddraw dsound dinput dplayx` + the static `dxguid` GUIDs) - the DX
    SDK ships no `#pragma comment(lib)`;
  * `mss32` + `smackw32` - the RAD SDKs we do not have, so their import libs are
    SYNTHESISED from retail's import table by gruntz.build.import_lib.

`LINK_LIBS` then names the whole set (Win32 included, redundantly) in **retail's
import-descriptor order**, because that order is what link.exe reproduces - see the
comment there. The candidate's import table comes out with the same 16 DLLs and the
same imported-name set per DLL as retail; the order WITHIN each DLL is still ~random
against retail, since that follows the object link order, not any flag.

The historical `/NODEFAULTLIB` objects-only probe is GONE with `/FORCE`: it was
viable only while unresolved externals were tolerated (it leaves every CRT/MFC
symbol unresolved by construction), and the real link's map supersedes it.

Other defaults are tuned for layout study, not a shippable binary:
  /SUBSYSTEM:WINDOWS /BASE:0x400000 /INCREMENTAL:YES /MAP
  /OPT:NOREF /OPT:NOICF   (keep EVERY function so the map is complete)

Run inside `nix develop`.
"""

import re
import argparse
import collections
import os
import shutil
import signal
import subprocess
import struct
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)
sys.path.insert(0, str(SCRIPT_DIR))
from msdis_stub import ensure_msdis  # noqa: E402

# The explicit library line. Order is load-bearing and this exact sequence
# REPRODUCES RETAIL'S IMPORT-DESCRIPTOR ORDER (`PE.imports` on GRUNTZ.EXE) - all 16
# DLLs, 0/120 inversions. Two rules produce it:
#
#   1. link.exe emits a DLL's `__IMPORT_DESCRIPTOR_*` when a library search first
#      satisfies an undefined symbol, so LIB ORDER = DESCRIPTOR ORDER.
#   2. ...but only for symbols that are already undefined when that lib is searched.
#      Most of the Win32 surface (all of comctl32/winspool/comdlg32/shell32) is
#      referenced by NOTHING in our objs - it arrives via MFC. So **nafxcw/libcmt
#      must be searched FIRST**; otherwise those four resolve only on a later pass
#      and their descriptors sink to the end (17/120 inversions). Naming them here
#      rather than leaving them to the objs' `-defaultlib:` directives is what fixes
#      it - and it says the retail link line searched MFC/CRT before Win32 too.
#
# `dxguid` is static GUID data, contributes no descriptor, so it rides at the end.
LINK_LIBS = ["nafxcw.lib", "libcmt.lib",
             "kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib", "comctl32.lib",
             "mss32.lib", "winmm.lib", "dplayx.lib", "smackw32.lib", "version.lib",
             "winspool.lib", "comdlg32.lib", "shell32.lib",
             "dinput.lib", "dsound.lib", "ddraw.lib", "dxguid.lib"]
# The retail entry point, once LIBCMT + NAFXCW are actually on the link line
# (LIBCMT defines _WinMainCRTStartup, NAFXCW defines the _WinMain@16 it calls).
ENTRY_LINKED = "WinMainCRTStartup"


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


def ensure_import_libs() -> list:
    """The synthesised import libs (mss32/smackw32) - built on demand, cached."""
    if str(REPO / "scripts") not in sys.path:
        sys.path.insert(0, str(REPO / "scripts"))
    from gruntz.build.import_lib import ensure_all
    return ensure_all()


def _has_rsrc(exe: Path) -> bool:
    """True when the PE carries a .rsrc section.

    Read straight out of the section table - no external tool, so the check costs
    nothing and cannot be skipped because a helper is missing.
    """
    try:
        data = exe.read_bytes()
        pe = struct.unpack_from("<I", data, 0x3c)[0]
        n_sections = struct.unpack_from("<H", data, pe + 6)[0]
        opt_size = struct.unpack_from("<H", data, pe + 20)[0]
        first = pe + 24 + opt_size
        return any(data[first + i * 40:first + i * 40 + 8].rstrip(b"\0") == b".rsrc"
                   for i in range(n_sections))
    except Exception:
        return False


def unresolved(output: str) -> set:
    """The DECORATED unresolved-external names in a link log.

    LNK2001 prints a C symbol bare (`_malloc`) but a C++ one as its demangled
    prose FOLLOWED by the real name in parentheses:
        unresolved external symbol "public: void __thiscall X::f(void)" (?f@X@@QAEXXZ)
    A `(\\S+)` grab therefore collapses EVERY C++ blocker into the handful of
    distinct first words of that prose (`"public:`, `"class`, ...), which silently
    hid the entire C++ backlog from the punch list. Take the trailing parenthesised
    name when there is one."""
    out = set()
    for ln in output.splitlines():
        m = re.search(r"unresolved external symbol (.*)$", ln)
        if not m:
            continue
        rest = m.group(1).strip()
        paren = re.search(r"\(([^()]+)\)\s*$", rest)
        out.add(paren.group(1) if paren else rest.split()[0])
    return out


def classify(sym: str) -> str:
    """Which kind of link blocker `sym` is - the three buckets that need three
    different fixes: a missing import lib, a missing C body, a missing C++ body."""
    if sym.startswith("__imp_"):
        return "import (no import lib on the line)"
    if sym.startswith("?"):
        return "C++ (undefined method/variable - reconstruction backlog)"
    return "C (undefined free function/variable)"


# The engine projects (`C:\Proj\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr}` + the shared Bute/
# Rez/Image/Wwd/Utils/Crypto code and vendored zlib) shipped as static LIBRARIES, not as
# objects on the link line. Retail proves it: MSVC's incremental linker emits an `E9`
# thunk for every cross-object call between OBJECTS but never for a `.lib` member, and
# retail's thunk targets stop dead at 0x11c860 - below that line 2 cross-unit direct
# calls, above it 4664. Sorting our units by that boundary lands 227 of 237 src/Gruntz
# units below it and the engine modules above: a project boundary, exactly the leaked
# layout. Archiving those modules takes our thunk count 4559 -> 2976 vs retail's 2695.
# There were at least TWO archives, not one: inside the library region the vendored
# zlib units form a PERFECTLY CONTIGUOUS run (0x185320..0x18b440, 12 units, no engine
# unit interleaved) after the engine block (0x134cb0..0x1852e0, 73 units) - the
# signature of a second library searched after the first. So we emit both.
ENGINE_MODULES = {"DDrawMgr", "DinMgr2", "Dsndmgr", "Image", "Bute", "Rez",
                  "Wwd", "Utils", "Crypto", "Font"}
ZLIB_UNITS = {"uncompr", "wapuncompress", "inflate", "deflate", "infblock", "adler32",
              "zutil", "trees", "inftrees", "infcodes", "infutil", "inffast"}
# A third block sits AFTER the CRT/MFC code (0x193080..0x1936e0, past a ~31 KB gap).
# Being pulled only once the CRT had been searched is what a LATER archive on the link
# line looks like, so it is emitted as its own library rather than folded into engine.lib.
TAIL_UNITS = {"bitarraystream", "butetree", "projactcache", "bitarray"}


def engine_units() -> set:
    """Unit names whose source lives in an engine module (or vendor/)."""
    import tomllib
    manifest = REPO / "config" / "units.toml"
    if not manifest.exists():
        return set()
    with manifest.open("rb") as fh:
        units = tomllib.load(fh).get("unit", [])
    out = set()
    for u in units:
        src = u.get("source", "")
        parts = Path(src).parts
        mod = parts[1] if src.startswith("src/") and len(parts) > 1 else "vendor"
        if mod in ENGINE_MODULES or not src.startswith("src/"):
            out.add(u["unit"])
    return out


def archive_engine(objs: list, out_dir: Path) -> tuple:
    """Split `objs` into (link-line objs, engine.lib) using the real VC5 LIB.EXE."""
    eng_names = engine_units() - ZLIB_UNITS - TAIL_UNITS
    eng = [o for o in objs if o.stem in eng_names]
    zl = [o for o in objs if o.stem in ZLIB_UNITS]
    tail = [o for o in objs if o.stem in TAIL_UNITS]
    claimed = eng_names | ZLIB_UNITS | TAIL_UNITS
    rest = [o for o in objs if o.stem not in claimed]
    if not (eng or zl or tail):
        return objs, []
    msvc = Path(os.environ.get("MSVC_DIR", "/tmp/gtc/msvc"))
    libexe = find_ci(msvc / "bin", "lib.exe")
    if not libexe:
        die("LIB.EXE not found - cannot archive the engine modules.")
    out_dir.mkdir(parents=True, exist_ok=True)
    made = []
    for name, members in (("engine.lib", eng), ("zlib.lib", zl), ("utils.lib", tail)):
        if not members:
            continue
        lib = out_dir / name
        lib.unlink(missing_ok=True)
        rsp = out_dir / (name.replace(".lib", ".rsp"))
        rsp.write_text(f"/OUT:{winepath_w(lib)}\n"
                       + "\n".join(f'"{winepath_w(o)}"' for o in members) + "\n")
        run_wine(["wine", str(libexe), "/NOLOGO", f"@{winepath_w(rsp)}"], out_dir, lib)
        if not lib.exists():
            die(f"LIB.EXE failed to build {name}.")
        print(f"[link] archived {len(members)} obj(s) -> {name} ({lib.stat().st_size:,} B)")
        made.append(lib)
    print(f"[link] {len(rest)} obj(s) stay on the link line")
    return rest, made


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
    # Only objs the MANIFEST still owns. Deleting a [[unit]] does not delete its
    # stale build/objdiff/base/<unit>.obj, and a bare glob then links the orphan -
    # which shows up as a phantom LNK2005 against the TU that legitimately owns
    # the symbol now. Since there is no /FORCE any more, that FAILS the link and
    # looks like a real identity defect. Filter instead of trusting the directory.
    import tomllib
    manifest = REPO / "config" / "units.toml"
    objs, orphans = [], []
    if manifest.exists():
        with manifest.open("rb") as fh:
            owned = {u["unit"] for u in tomllib.load(fh).get("unit", []) if "unit" in u}
        for p in sorted(objs_dir.glob("*.obj")):
            (objs if p.stem in owned else orphans).append(p)
        if orphans:
            print(f"[link] skipping {len(orphans)} orphaned obj(s) with no [[unit]]: "
                  + ", ".join(p.name for p in orphans[:6])
                  + (" ..." if len(orphans) > 6 else ""))
        return objs
    return sorted(objs_dir.glob("*.obj"))


def main() -> None:
    ap = argparse.ArgumentParser(description="VC5 link.exe wrapper (phase 2).")
    ap.add_argument("--out", default="build/exe/GRUNTZ.candidate.EXE")
    ap.add_argument("--map", dest="mapfile", default=None,
                    help="map path (default: <out> with .map suffix).")
    ap.add_argument("--objs-dir", default="build/objdiff/base")
    ap.add_argument("--obj", action="append", help="explicit obj (repeatable).")
    ap.add_argument("--order", help="file listing obj stems/paths in link order.")
    ap.add_argument("--res", help="optional .RES for runnable candidate resources; "
                                  "the matching build does not use it.")
    ap.add_argument("--lib", action="append", default=[],
                    help="extra import/static lib to pass to link (repeatable).")
    ap.add_argument("--engine-lib", action="store_true",
                    help="archive the engine modules into engine.lib and link THAT, as "
                         "retail did (they were separate .LIB projects). Incremental "
                         "thunks 4559 -> 2976 vs retail's 2695, because a .lib member "
                         "never gets one.")
    ap.add_argument("--no-incremental", action="store_true",
                    help="/INCREMENTAL:NO - the flat layout. Default is YES because "
                         "retail is an incremental link (thunk band, IAT slack, separate "
                         ".idata); use this only to isolate that variable.")
    ap.add_argument("--base", default="0x400000", help="image base (/BASE).")
    ap.add_argument("--entry", default=None,
                    help=f"forced /ENTRY symbol (default: {ENTRY_LINKED}).")
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
    engine_libs = []
    if args.engine_lib:
        objs, engine_libs = archive_engine(objs, Path(args.out).resolve().parent)
    if not objs:
        die("no objects to link.")

    # MSDIS100.DLL must resolve or link.exe won't even load under wine.
    ensure_msdis(prefix, msvc, verbose=True)
    os.environ.setdefault("WINEDEBUG", "fixme-all,err-all")
    ensure_wineserver()

    entry = args.entry or ENTRY_LINKED
    # Retail IS an incremental link - three independent witnesses: the E9 thunk band at
    # the top of .text (retail has 2704 in the first 0x8000 of slots; /INCREMENTAL:NO
    # gives 14), the zero-filled IAT slack after every DLL terminator, and the separate
    # writable .idata section. Measured to cost nothing: per-object fragmentation is
    # IDENTICAL either way (median 1, mean 1.07, 94.7% contiguous, zero objects worse)
    # and gruntz.audit.link_order still reads the map. Only reachable once the tree
    # linked /FORCE-free, since ANY /FORCE makes link ignore it (LNK4075).
    incremental = "/INCREMENTAL:NO" if args.no_incremental else "/INCREMENTAL:YES"
    rsp_lines = [
        f"/OUT:{winepath_w(out)}",
        f"/MAP:{winepath_w(mapf)}",
        "/NOLOGO", "/SUBSYSTEM:WINDOWS",
        f"/BASE:{args.base}", incremental, f"/ENTRY:{entry}",
        # Retail HAS a .reloc (it is why the EXE is delinkable at all), so it was
        # linked /FIXED:NO. Purely additive: .text/.rdata/.data come out byte-identical.
        "/FIXED:NO",
    ]
    if args.keep_all:
        rsp_lines += ["/OPT:NOREF", "/OPT:NOICF"]
    extra = args.flags[1:] if args.flags and args.flags[0] == "--" else args.flags
    rsp_lines += list(extra)
    libs = list(args.lib)
    libs += [str(x) for x in engine_libs]
    # Substitute the synthesised RAD libs IN PLACE so LINK_LIBS' order survives.
    synth = {p.name.lower(): str(p) for p in ensure_import_libs()}
    libs += [synth.get(n, n) for n in LINK_LIBS]
    rsp_lines += [winepath_w(lib) if os.path.exists(lib) else lib for lib in libs]
    rsp_lines += [f'"{winepath_w(o)}"' for o in objs]
    if args.res:
        rsp_lines.append(f'"{winepath_w(os.path.abspath(args.res))}"')

    rsp = out.parent / (out.stem + ".objs.rsp")
    rsp.write_text("\n".join(rsp_lines) + "\n")

    output, rc = run_wine(["wine", str(link), f"@{winepath_w(rsp)}"], out.parent, out)
    logf = out.parent / (out.stem + ".link.log")
    logf.write_text(output)

    # /INCREMENTAL:YES patches the PREVIOUS image instead of rebuilding it, and an
    # incremental pass does NOT re-bind the .res - so a relink over a valid .EXE+.ilk
    # silently emits a binary with NO .rsrc. That is not cosmetic: every MFC dialog
    # (settings, multiplayer battle select, save-game) is created from a DIALOG
    # resource, so a resource-less build has no working dialogs at all, and it looked
    # exactly like a reconstruction bug for a whole session. Incremental stays ON
    # (retail IS an incremental link - the E9 thunk band, the padded IAT - and the
    # layout campaign needs it), so instead: verify, then self-heal by dropping the
    # .ilk and doing the one full link that binds the resources.
    if args.res and out.exists() and not _has_rsrc(out):
        print("[link] .rsrc MISSING after an incremental link - dropping .ilk and "
              "relinking in full (see the /INCREMENTAL note in link.py)")
        for stale in (out, out.with_suffix(".ilk")):
            try:
                stale.unlink()
            except FileNotFoundError:
                pass
        output, rc = run_wine(["wine", str(link), f"@{winepath_w(rsp)}"], out.parent, out)
        logf.write_text(output)
        if out.exists() and not _has_rsrc(out):
            sys.stderr.write(f"[link] FATAL: {out} still has no .rsrc after a full "
                             f"link, though --res {args.res} was supplied\n")
            sys.exit(1)

    if not out.exists():
        sys.stderr.write(f"[link] FAILED to produce {out} (log: {logf})\n")
        sys.stderr.write("\n".join(output.strip().splitlines()[-20:]) + "\n")
        sys.exit(rc or 1)

    # No /FORCE: an unresolved extern or a duplicate symbol FAILS the link above,
    # so reaching here means both are zero. They are still reported (and asserted)
    # because a silent regression to non-zero would mean the link stopped being an
    # oracle - see the /FORCE note in the module docstring.
    warns = sum(1 for ln in output.splitlines() if "LNK4006" in ln)
    unres_syms = sorted(unresolved(output))
    # save the unresolved-externals punch-list (the drive-to-linkable worklist).
    unf = out.parent / (out.stem + ".unresolved.txt")
    unf.write_text("\n".join(unres_syms) + "\n")
    print(f"[link] {len(objs)} objs + {len(libs)} explicit lib(s) -> {out} "
          f"({out.stat().st_size} B) + {mapf.name}")
    print(f"[link] {len(unres_syms)} unresolved externals -> {unf.name}, "
          f"{warns} dup-symbol warnings  (no /FORCE - a real link)")
    if unres_syms or warns:
        die(f"link is no longer clean: {len(unres_syms)} unresolved, {warns} dup(s). "
            "Fix the source - never re-add /FORCE (see the module docstring).")
    for bucket, n in sorted(collections.Counter(
            classify(s) for s in unres_syms).items(), key=lambda kv: -kv[1]):
        print(f"[link]   {n:5d}  {bucket}")


if __name__ == "__main__":
    main()
