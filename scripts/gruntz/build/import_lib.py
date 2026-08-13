#!/usr/bin/env python3
"""import_lib.py - synthesise the import LIBs the toolchain does not ship.

Retail GRUNTZ.EXE load-time-imports 16 DLLs (`gruntz.core.pe.PE.imports`). Fourteen
of them have a real import lib in the toolchain - the Win32 set in `msvc/lib`
(KERNEL32/USER32/GDI32/ADVAPI32/COMCTL32/SHELL32/COMDLG32/WINSPOOL/VERSION/WINMM)
and DirectX 6 in `dx/Lib` (ddraw/dsound/dinput/dplayx). The two that have **no**
lib anywhere are the RAD Game Tools SDKs the devs had and we do not:

  * **mss32.dll**    - Miles Sound System 4.0g  (16 `_AIL_*@n` imports)
  * **smackw32.dll** - Smacker video           (10 `_Smack*@n` imports)

Only their *headers* are vendored (`vendor/miles-6.0c/mss.h`,
`vendor/smacker-3.2f/smack.h`), so every call to them is an unresolved external at
link. This module rebuilds the missing `.lib` from the **retail import table**,
which is ground truth: the names stored there (`_AIL_startup@0`) are exactly what
the original import lib produced, decoration and all.

How (and why not `LIB /DEF:`): LIB.EXE derives an import lib's public symbol from
the .def name by *prefixing an underscore*, so a def naming the true export
`_AIL_startup@0` yields the public symbol `__imp___AIL_startup@0` (one underscore
too many) while a def naming `AIL_startup@0` yields the right symbol but writes the
wrong hint/name string into `.idata$6`. Neither is faithful. Instead we do what the
SDK vendor did: generate a throwaway **stub DLL** whose exports are
`__declspec(dllexport) __stdcall` functions with the matching argument-byte count,
and keep the `/IMPLIB:` that link.exe emits for it. That import lib carries both
`_AIL_startup@0` / `__imp__AIL_startup@0` **and** the `_AIL_startup@0` hint/name
string - byte-identical in shape to retail's own import descriptor.

Hints are reproduced too. A `.idata$6` hint is the export's index in the DLL's
*sorted export-name table*, so the vendor's lib carries the index each name had in
the REAL DLL's full export list (`_AIL_release_sequence_handle@4` = 126 out of
~196 Miles exports), and retail's import table stores those values byte-for-byte -
which makes retail itself the evidence for the vendor DLL's name-table shape. The
stub reproduces it by exporting `__cdecl` FILLER names (export name = the bare
identifier) that sort strictly between the real decorated names, one per unclaimed
index up to the highest retail hint. Retail's hints are strictly ascending in
sorted-name order for both DLLs (asserted), which is exactly what "indices into one
sorted name table" implies, so the interleave always exists. The fillers never
reach the image: nothing references them, so no member of theirs is ever pulled.
`_verify_hints` re-reads the produced lib's `.idata$6` and dies on any mismatch.

The stub DLL itself is discarded; only the `.lib` is a build input. Nothing here
needs the real MSS32/SMACKW32 DLLs (see docs/runtime-dlls.md: those are runtime-only).

  python -m gruntz.build.import_lib            # synthesise every missing lib
  python -m gruntz.build.import_lib --list     # report coverage, build nothing
"""

import argparse
import os
import re
import sys
from pathlib import Path

from gruntz.core.cc_wrap import _run_cl, find_ci, msvc_dir, winepath_w, ensure_wineserver

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
OUT_DIR = REPO / "build" / "lib"

# `_name@n` = __stdcall (n = argument bytes); a bare `name` = __cdecl/data export.
STDCALL = re.compile(r"^_(?P<name>[A-Za-z_][A-Za-z0-9_]*)@(?P<bytes>\d+)$")
PLAIN = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


def die(msg):
    print(f"[import-lib] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def lib_dirs():
    """Where the toolchain's real import libs live: DX6 first, then VC5 (the same
    precedence gruntz.init.toolchain writes into the Wine LIB)."""
    tc = os.environ.get("GRUNTZ_TOOLCHAIN")
    dirs = [Path(tc) / "dx" / "Lib"] if tc else []
    return [d for d in dirs + [msvc_dir() / "lib"] if d.is_dir()]


def toolchain_lib(stem):
    """The toolchain's `<stem>.lib`, case-insensitively, or None."""
    for d in lib_dirs():
        hit = find_ci(d, f"{stem}.lib")
        if hit:
            return hit
    return None


def survey():
    """[(dll, names, existing_lib_or_None)] over retail's whole import table."""
    from gruntz.core.pe import PE
    rows = []
    for dll, names in PE().imports.items():
        rows.append((dll, names, toolchain_lib(Path(dll).stem)))
    return rows


def retail_hints(dll):
    """{decorated_name: hint} for one DLL, straight from retail's import table.

    The hint is the 2-byte prefix of the `.idata$6` hint/name blob - the index
    the name had in the vendor DLL's sorted export-name table, recorded by the
    vendor's import lib and copied into the image by the linker."""
    import struct
    from gruntz.core.pe import PE
    pe = PE()
    d = pe.data
    rva = struct.unpack_from("<I", d, pe._opt + 96 + 1 * 8)[0]
    o = pe.off(rva)
    while True:
        olt, _ts, _fc, nm, fta = struct.unpack_from("<IIIII", d, o)
        if not (olt or nm or fta):
            break
        if pe.cstr(nm) == dll:
            out = {}
            t = pe.off(olt or fta)
            while True:
                v = struct.unpack_from("<I", d, t)[0]
                if v == 0:
                    break
                if not (v & 0x80000000):
                    hn = v & 0x7FFFFFFF
                    out[pe.cstr(hn + 2)] = struct.unpack_from("<H", d, pe.off(hn))[0]
                t += 4
            return out
        o += 20
    return {}


# Filler export names: valid C identifiers (they are compiled as `__cdecl`
# functions, and a cdecl dllexport's export-table string is the identifier as
# written), generated to sort strictly between two decorated real names.
_IDENT = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"


def _gap_base(a, b):
    """An identifier string `base` with a < base + <digits> < b bytewise.

    `a` may be None (any base < b works). Walk the common prefix; at the first
    divergence try a character strictly between; when the two are adjacent,
    extend past `a`'s next character instead. The prefix of a decorated import
    name up to any divergence below '@' is identifier-clean (asserted)."""
    ident = sorted(_IDENT)
    if a is None:
        for j in range(len(b)):
            lo = [c for c in ident if c < b[j]]
            if lo:
                base = b[:j] + lo[-1]
                if all(c in _IDENT for c in base):
                    return base
        raise SystemExit(f"[import-lib] no filler name sorts below {b!r}")
    i = 0
    while i < len(a) and i < len(b) and a[i] == b[i]:
        i += 1
    assert i < len(b), f"{a!r} !< {b!r}"
    mid = [c for c in ident if (i >= len(a) or c > a[i]) and c < b[i]]
    if mid:
        base = a[:i] + mid[0]
    else:
        # adjacent characters: step inside `a` and clear its tail instead
        nxt = [c for c in ident if c > (a[i + 1] if i + 1 < len(a) else "")]
        assert nxt, f"cannot split the gap {a!r} .. {b!r}"
        base = a[: i + 1] + nxt[0]
    assert all(c in _IDENT for c in base), (a, b, base)
    assert (a is None or a < base) and base < b, (a, b, base)
    return base


def _export_table(names, hints):
    """[(export_name, is_filler)] in sorted order, fillers padding every index
    below the retail hint of each real name so the stub DLL's sorted name table
    puts each real export at exactly its retail index."""
    real = sorted(names)
    hs = [hints[n] for n in real]
    if hs != sorted(hs) or len(set(hs)) != len(hs):
        raise SystemExit("[import-lib] retail hints are not ascending in "
                         "sorted-name order - not one sorted name table?")
    table = []
    prev = None
    pos = 0
    for n, h in zip(real, hs):
        k = h - pos
        if k:
            base = _gap_base(prev, n)
            width = len(str(k - 1))
            fillers = [f"{base}{i:0{width}d}" for i in range(k)]
            assert all(prev is None or prev < f for f in fillers)
            assert fillers == sorted(fillers) and fillers[-1] < n, (prev, n, k)
            table += [(f, True) for f in fillers]
            pos += k
        table.append((n, False))
        prev = n
        pos += 1
    flat = [x for x, _f in table]
    assert all(x < y for x, y in zip(flat, flat[1:])), "table not strictly sorted"
    return table


def stub_source(dll, names, hints=None):
    """C for a stub DLL whose exports decorate to exactly `names` - padded with
    filler exports so each name's hint (sorted-name-table index) matches retail."""
    lines = [f"/* GENERATED by gruntz.build.import_lib - stub exports for {dll}.",
             "   Bodies are irrelevant: only the DECORATED export names and their",
             "   sorted-name-table INDICES (the hints) matter, and both come from",
             "   retail GRUNTZ.EXE's own import table. */"]
    table = (_export_table(names, hints) if hints and all(n in hints for n in names)
             else [(n, False) for n in sorted(names)])
    for n, filler in table:
        if filler:
            lines.append(f"__declspec(dllexport) void {n}(void) {{}}")
            continue
        m = STDCALL.match(n)
        if m:
            nargs, rem = divmod(int(m.group("bytes")), 4)
            if rem:
                die(f"{dll}: {n} has a non-dword argument size - cannot express "
                    "as a __stdcall prototype")
            # C definitions need NAMED formals (C2055) even though nothing uses them.
            args = ", ".join(f"int a{i}" for i in range(nargs)) or "void"
            lines.append(f"__declspec(dllexport) void __stdcall "
                         f"{m.group('name')}({args}) {{}}")
        elif PLAIN.match(n):
            # __cdecl exports keep the undecorated name in the export table.
            lines.append(f"__declspec(dllexport) void {n}(void) {{}}")
        else:
            die(f"{dll}: cannot synthesise an export for {n!r} (ordinal-only or "
                "fastcall imports need a hand-written .def)")
    return "\n".join(lines) + "\n"


def _verify_hints(lib, want):
    """Die unless every hint/name blob in `lib`'s .idata$6 matches `want`.

    The hint a member carries is what the linker copies into the image, so this
    re-reads the produced archive rather than trusting the export-table math."""
    import struct
    data = lib.read_bytes()
    if data[:8] != b"!<arch>\n":
        die(f"{lib}: not an archive")
    got = {}
    off = 8
    while off + 60 <= len(data):
        size = int(data[off + 48:off + 58].decode().strip() or "0")
        body = off + 60
        m = data[body:body + size]
        if len(m) > 20 and m[:4] != b"\xff\xff\0\0":     # skip linker members
            try:
                nsec = struct.unpack_from("<H", m, 2)[0]
                for i in range(nsec):
                    raw = m[20 + 40 * i: 20 + 40 * (i + 1)]
                    if raw[:8].rstrip(b"\0") == b".idata$6":
                        rsz, rp = struct.unpack_from("<II", raw, 16, )[0], \
                            struct.unpack_from("<I", raw, 20)[0]
                        blob = m[rp:rp + rsz]
                        if len(blob) > 3:
                            hint = struct.unpack_from("<H", blob, 0)[0]
                            name = blob[2:blob.find(b"\0", 2)].decode("latin1")
                            if name in want:
                                got[name] = hint
            except (struct.error, ValueError):
                pass
        off = body + size + (size & 1)
    bad = {n: (want[n], got.get(n)) for n in want if got.get(n) != want[n]}
    if bad:
        die(f"{lib.name}: hint mismatch after synthesis: {bad}")


def synthesize(dll, names, out_dir=OUT_DIR, verbose=True):
    """Build `<out_dir>/<stem>.lib` for `dll`; returns the lib path."""
    msvc = msvc_dir()
    cl = find_ci(msvc / "bin", "cl.exe")
    link = find_ci(msvc / "bin", "link.exe")
    if not (cl and link):
        die(f"CL.EXE/link.exe not found under {msvc}/bin - run inside `nix develop`.")
    out_dir.mkdir(parents=True, exist_ok=True)
    stem = Path(dll).stem
    src, obj = out_dir / f"{stem}_stub.c", out_dir / f"{stem}_stub.obj"
    lib, stub_dll = out_dir / f"{stem}.lib", out_dir / dll   # /OUT name == the
    hints = retail_hints(dll)                                # recorded DLL name
    src.write_text(stub_source(dll, names, hints))
    for f in (obj, lib, stub_dll):
        f.unlink(missing_ok=True)

    os.environ.setdefault("WINEDEBUG", "fixme-all,err-all")
    ensure_wineserver()
    out, rc = _run_cl(["wine", str(cl), "/nologo", "/c",
                       f"/Fo{winepath_w(obj)}", winepath_w(src)], obj)
    if not obj.exists():
        die(f"{dll}: stub compile failed\n" + "\n".join(out.splitlines()[-10:]))
    out, rc = _run_cl(["wine", str(link), "/NOLOGO", "/DLL", "/NOENTRY",
                       f"/OUT:{winepath_w(stub_dll)}", f"/IMPLIB:{winepath_w(lib)}",
                       winepath_w(obj)], lib)
    if not lib.exists():
        die(f"{dll}: stub link failed\n" + "\n".join(out.splitlines()[-10:]))
    # The stub DLL and its .exp are scaffolding; only the .lib is a build input.
    for f in (stub_dll, out_dir / f"{stem}.exp", obj):
        f.unlink(missing_ok=True)
    named = {n: hints[n] for n in names if n in hints}
    if named:
        _verify_hints(lib, named)
    if verbose:
        print(f"[import-lib] {dll}: {len(names)} import(s) -> {lib}"
              + (f" (hints verified against retail, {len(named)} name(s))"
                 if named else ""))
    return lib


def ensure_all(out_dir=OUT_DIR, verbose=True):
    """Synthesise every import lib the toolchain lacks; returns their paths.

    Cached: a lib newer than both the retail EXE and this module is reused."""
    from gruntz.core.pe import EXE
    stamp = max(p.stat().st_mtime for p in (Path(__file__), EXE) if p.exists())
    libs = []
    for dll, names, existing in survey():
        if existing:
            continue
        lib = out_dir / f"{Path(dll).stem}.lib"
        if lib.exists() and lib.stat().st_mtime >= stamp:
            libs.append(lib)
            continue
        libs.append(synthesize(dll, names, out_dir, verbose))
    return libs


def main(argv=None):
    ap = argparse.ArgumentParser(description="synthesise missing import libs.")
    ap.add_argument("--list", action="store_true",
                    help="report which imported DLLs have a lib; build nothing.")
    ap.add_argument("--out-dir", default=str(OUT_DIR))
    args = ap.parse_args(argv)

    if args.list:
        for dll, names, existing in survey():
            where = existing if existing else "** no lib - synthesised **"
            print(f"{dll:16s} {len(names):4d} import(s)  {where}")
        return 0
    libs = ensure_all(Path(args.out_dir))
    print(f"[import-lib] {len(libs)} synthesised lib(s) in {args.out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
