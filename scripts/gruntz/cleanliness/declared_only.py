#!/usr/bin/env python3
"""gruntz.cleanliness.declared_only - declarations without definitions (the alias ratchet).

A declaration that no definition ever satisfies is how an alias hack hides: a
.cpp-local prototype (`i32 __stdcall Check4_2ce8(i32);`) or a phantom extern
makes a call/reference compile, reloc-masking hides the target from the diff,
and the fabricated NAME survives - undefined in every base obj, and (because
retail linked, so every real name eventually pairs) absent from the delinked
target namespace too. view_debt only catches this for methods on pure-phantom
CLASSES; this gate covers the rest: free functions, methods/dtors on REAL
classes, and data externs.

A symbol is DECLARED-ONLY debt iff:
  * some base obj references it (undefined `U`) and NO base obj defines it, and
  * it does not exist in the delinked TARGET objs' namespace either (defined or
    referenced) - retail has no such name, so no future reconstruction can ever
    pair it: it must be renamed to the real symbol or its owner defined, and
  * it is not a static-library external (MFC / CRT / iostream / Afx / global
    operators), which the linker resolves from libs, never from our TUs.

The set is RATCHETED against config/declared-only-baseline.tsv: any symbol not
in the baseline is FATAL (a new alias); fixing rows shrinks the file via
--update. Drive to 0.

    python -m gruntz.cleanliness.declared_only            # report vs baseline
    python -m gruntz.cleanliness.declared_only --list     # print the full set
    python -m gruntz.cleanliness.declared_only --fatal    # gate: fail on NEW symbols
    python -m gruntz.cleanliness.declared_only --update   # rewrite the baseline
"""
import glob
import re
import subprocess
import sys
from pathlib import Path

from gruntz.cleanliness.view_debt import LIBRARY_CLASSES, _current_objs

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
TARGET = REPO / "build/delink/named"
BASELINE = REPO / "config/declared-only-baseline.tsv"

# Static-lib classes beyond view_debt's MFC set: the iostream family + MFC odds the
# corpus references but never defines (resolved from LIBCMT / MFC42 at link).
LIBRARY_EXTRA = {
    "ios", "istream", "ostream", "iostream", "ifstream", "ofstream", "fstream",
    "filebuf", "streambuf", "strstreambuf", "istrstream", "ostrstream", "strstream",
    "stdiobuf", "Iostream_init", "exception", "type_info", "bad_cast", "bad_typeid",
    "CMemFile", "CStdioFile", "CPaintDC", "CDumpContext", "CArchiveException",
    "CGdiObject", "CSize", "AFX_MODULE_STATE",
    # MFC collection classes the corpus references but never defines: their bodies
    # live in the statically-linked MFC band (??1CStringArray -> 0x1bef92) and the
    # linker resolves them from the .LIB - library externs, not fabricated aliases.
    "CStringArray", "CByteArray", "CDWordArray", "CWordArray", "CUIntArray",
}


def _sym_class(sym):
    """Owning class of a mangled symbol, or None for global fns/operators/data.
    Handles both spellings: ?Method@Class@@... and ??1Class@@ (operator members)."""
    m = re.match(r"\?\?(?:_?[0-9A-Z])([A-Za-z_]\w*)?@@", sym)
    if m:
        return m.group(1)                          # ??1CFoo@@ -> CFoo; ??2@ -> None
    m = re.match(r"\?[^@]+((?:@[A-Za-z_]\w*)+)@@", sym)
    if m:
        return m.group(1).split("@")[1]            # innermost (owning) scope
    return None


def _is_library(sym):
    if not sym.startswith("?"):
        return True                                # C symbol / __imp_ - the link audit's turf
    c = _sym_class(sym)
    if c is not None:
        return c in LIBRARY_CLASSES or c in LIBRARY_EXTRA or c.startswith("std")
    if sym.startswith("??"):
        return True                                # class-less global operator (??2/??3/??H/??_L..)
    return bool(re.match(r"\?_?Afx", sym))         # Afx* free functions (MFC)


def _nm(objs):
    defined, undef = set(), set()
    for o in objs:
        out = subprocess.run(["llvm-nm", str(o)], capture_output=True, text=True).stdout
        for ln in out.splitlines():
            p = ln.split()
            if len(p) == 2 and p[0] == "U":
                undef.add(p[1])
            elif len(p) == 3 and p[1] != "U":
                defined.add(p[2])
    return defined, undef


def declared_only():
    """The current declared-only debt set (game symbols only)."""
    bdef, bund = _nm(_current_objs())
    tdef, tund = _nm(sorted(glob.glob(str(TARGET / "*.obj"))))
    never = bund - bdef
    alias = never - tdef - tund
    # A symbol the toolchain .LIB tables can supply (CRT/MFC/Win32/DirectX statics)
    # resolves at link no matter what - a library extern, never a fabricated alias.
    # This is the LINKER'S answer (?SetRange@CSliderCtrl was baselined as debt for
    # weeks because the name-pattern filter had no CSliderCtrl row).
    try:
        from gruntz.audit.link_defects import lib_symbols
        libs = lib_symbols()
    except Exception:
        libs = set()
    return {s for s in alias if not _is_library(s) and s not in libs}


def _read_baseline():
    if not BASELINE.is_file():
        return set()
    return {ln.split("\t")[0].strip() for ln in BASELINE.read_text().splitlines()
            if ln.strip() and not ln.startswith("#")}


def _write_baseline(syms):
    rows = "\n".join(sorted(syms))
    BASELINE.write_text(
        "# declared-only debt (gruntz.cleanliness.declared_only): symbols some base obj\n"
        "# references that NO base obj defines and retail's namespace never names -\n"
        "# fabricated aliases / phantom externs. RATCHET: new rows are FATAL; fix rows\n"
        "# (rename to the real symbol / define the owner), then --update. Drive to 0.\n"
        + rows + ("\n" if rows else ""))


def main():
    args = sys.argv[1:]
    if not TARGET.is_dir() or not any(TARGET.glob("*.obj")):
        print("declared-only: no target objs (run `gruntz build` first)")
        return 0
    cur = declared_only()
    base = _read_baseline()
    new = cur - base
    fixed = base - cur
    nf = sum(1 for s in cur if not re.search(r"@@\d", s))
    nd = len(cur) - nf
    print(f"declared-only: {len(cur)} symbol(s) defined nowhere & unknown to retail "
          f"({nf} func, {nd} data; baseline {len(base)}, fixed {len(fixed)}, NEW {len(new)})")
    if "--update" in args:
        _write_baseline(cur)
        print(f"declared-only: baseline rewritten ({len(cur)} row(s))")
        return 0
    if "--list" in args:
        for s in sorted(cur):
            print(("  NEW " if s in new else "      ") + s)
    elif new:
        for s in sorted(new):
            print("  NEW " + s)
    if fixed and not new:
        print(f"declared-only: {len(fixed)} baselined row(s) now resolved - run "
              f"`python -m gruntz.cleanliness.declared_only --update` to ratchet down")
    if "--fatal" in args and new:
        print("declared-only: FAIL (--fatal) - new declared-only alias(es); rename the decl "
              "to the real symbol (or define the owner) - never re-baseline up", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
