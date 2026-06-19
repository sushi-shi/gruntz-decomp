#!/usr/bin/env python3
"""labels.py - derive the rva -> mangled-name, unit label map from src/.

Replaces the hand-maintained config/symbol_names.csv. The only manual input is
the address annotation on each matched function/global; everything else is
derived. See docs/source-consolidation-investigation.md and docs/build-system.md.

ANNOTATIONS (src/rva.h macros, compiled out under MSVC):

    RVA(0x13dc70, 0x1d)        // a matched FUNCTION: rva + (optional) byte size
    SYMBOL(?Mangled@@...)      // explicit mangled-name override for a function
    DATA(0x253c70)             // the DATA symbol a matched `extern` global uses

How the map is built per TU:

  1. FUNCTIONS / SYMBOL come from LLVM IR. `clang ... -S -emit-llvm` emits
     `@llvm.global.annotations`, which pairs the MANGLED SYMBOL of each annotated
     function DIRECTLY with its annotation string (e.g. "rva:0x13dc70 size:0x1d").
     There is NO positional "nearest definition below the comment" join, so an
     inline header definition can no longer steal a nearby address (the old, and
     fragile, `// @address:` comment path - now gone).

  2. DATA comes from the clang AST. An `extern` declaration is not a definition,
     so clang DROPS its annotation from IR (measured: `used` does not rescue it,
     even when the global is referenced - see src/rva.h). So DATA addresses are
     scanned from the `DATA(...)` macro text and bound to the AST VarDecl
     mangledName below them. A file-scope/extern variable carries a linkage
     mangledName (`?g_foo@@3...`, or `_g_foo` for extern "C"); locals do not, so
     they are excluded naturally. This is the same non-fragile variable join used
     before, keyed off the macro instead of a `// @data:` comment.

  3. AUTHORITY CHECK: a function mangled name is kept iff it is a code symbol in
     the base <unit>.obj (llvm-nm). A DATA name is kept iff it is any symbol in
     the obj (a matched global is only *referenced* there, so it appears as an
     undefined `U` external). clang's MS mangling reproduced the real VC5 symbols
     in the spike, but is not contractually VC5 - the nm membership check is what
     makes it safe. An explicit SYMBOL override is trusted as-is.

  4. unit comes from config/units.toml via the source path.

A compiler-generated thunk with no source body (a `??_G` scalar-deleting dtor)
cannot hang an RVA() attribute, so it is pinned with a self-contained comment
`// @rva-symbol: <mangled> <rva> [<size>]` (read in every TU - the name is given
verbatim, so no join and no IR).

VENDORED PATH: vendored C TUs (vendor/zlib-1.0.4/*.c) keep their source PRISTINE -
no labels in the source at all. They are mostly `static`/`local` K&R functions
that clang DROPS from IR when unused, so neither attributes nor a source join can
carry their labels. Instead their rva->symbol map lives in config/zlib_labels.csv
(a static table - the retail binary never changes - generated once) and is emitted
directly, authority-checked against the base obj: no source parsing, no positional
join. A TU is routed to the config path iff it carries NO src/rva.h macro.

Output: build/gen/symbol_names.csv  (rva,name,unit,size,kind) - for synth_pdb.
`kind` is func or data; `size` is the RVA size (hex) or empty.

Without --obj the authority check is skipped (candidate names emitted with a
WARN) - useful for inspecting clang's mangling before base objs exist.
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)
INC = str(REPO / "include")   # repo-local headers (mirror src/) live here; on the clang -I path

TARGET = "i686-pc-windows-msvc"
MSC_COMPAT = "1100"
MS_FLAGS = [f"--target={TARGET}", f"-fms-compatibility-version={MSC_COMPAT}",
            "-fms-extensions"]

# DATA(0x...) macro invocation - scanned from source text (IR drops extern
# annotations). The address is bound to the AST VarDecl below it.
DATA_MACRO_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
# `// @rva-symbol: <mangled> <rva> [<size>]` - a self-contained function label for
# a compiler-generated thunk that has NO source definition to hang an RVA()
# attribute on (a `??_G` scalar-deleting destructor). The mangled name is given
# verbatim, so there is no join and no IR - it is read in every TU.
RVA_SYMBOL_RE = re.compile(
    r"@rva-symbol:\s*(\S+)\s+(0x[0-9a-fA-F]+)(?:\s+(0x[0-9a-fA-F]+|\d+))?")
# Annotation strings carried in @llvm.global.annotations (emitted by src/rva.h).
ANN_RVA_RE = re.compile(r"^rva:(0x[0-9a-fA-F]+)(?:\s+size:(0x[0-9a-fA-F]+|\d+))?$")
ANN_SYM_RE = re.compile(r"^symbol:(\S+)$")
# Macro annotation markers (presence of any -> a migrated TU; functions/SYMBOL
# come from IR for these). A TU with none falls back to the legacy comment path.
MACRO_RE = re.compile(r"\b(?:RVA|DATA|SYMBOL)\s*\(")

# Static rva->symbol table for vendored C TUs whose source carries no labels.
LABEL_CONFIG = REPO / "config/zlib_labels.csv"


def log(msg):
    print(f"[labels] {msg}", file=sys.stderr)


def line_index(text):
    """offsets -> 1-based line. Returns a function offset->line."""
    import bisect
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    return lambda off: bisect.bisect_right(starts, off)


# --- LLVM IR annotation parsing --------------------------------------------
# @llvm.global.annotations = appending global [...] [
#   { ptr, ptr, ptr, i32, ptr } { ptr @"?Mangled@@...", ptr @.str, ... }, ... ]
# @.str = private unnamed_addr constant [N x i8] c"rva:0x13dc70 size:0x1d\00", ...
_STR_DEF_RE = re.compile(
    r'^(@[\w.$"]+)\s*=.*?\bc"((?:[^"\\]|\\.)*)"', re.M)
_ANN_TUPLE_RE = re.compile(
    r'\{\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,')


def _unescape_ir_cstr(s):
    """Decode an LLVM `c"..."` string body (\\NN hex escapes, trailing \\00)."""
    out = bytearray()
    i = 0
    while i < len(s):
        if s[i] == "\\" and len(s) - i >= 3 and \
                all(c in "0123456789abcdefABCDEF" for c in s[i + 1:i + 3]):
            out.append(int(s[i + 1:i + 3], 16))
            i += 3
        else:
            out.append(ord(s[i]))
            i += 1
    if out and out[-1] == 0:
        out.pop()
    return out.decode("utf-8", "replace")


def _ir_symbol_name(ref):
    """`@"?Foo@@..."` / `@_foo` -> the bare symbol string.

    LLVM prefixes a symbol whose final name is given verbatim (clang's `asm`
    label, e.g. an `extern "C" __stdcall` function `_Foo@12`) with a `\\01` escape
    so the backend skips mangling. The retail/base-obj symbol has no such prefix,
    so strip it for the authority check.
    """
    ref = ref[1:]                       # drop leading '@'
    if ref.startswith('"') and ref.endswith('"'):
        ref = ref[1:-1]
    if ref.startswith("\\01"):
        ref = ref[3:]
    return ref


def parse_ir_annotations(ir):
    """Parse @llvm.global.annotations -> [(mangled_symbol, annotation_string)].

    The annotations global is one flat array of {ptr fn, ptr str, ptr file, i32
    line, ptr null} tuples; we map each tuple's first ptr (the annotated symbol)
    to the second ptr's `@.str` constant body. clang lays the string constants
    out elsewhere in the module, so resolve them by name.
    """
    strings = {}
    for m in _STR_DEF_RE.finditer(ir):
        strings[m.group(1)] = _unescape_ir_cstr(m.group(2))

    # Isolate the annotations array initialiser (one logical line in -emit-llvm
    # output) so unrelated `{ ptr, ptr } { ... }` tuples elsewhere are not read.
    out = []
    for line in ir.splitlines():
        if "@llvm.global.annotations" not in line:
            continue
        for sym_ref, str_ref in _ANN_TUPLE_RE.findall(line):
            s = strings.get(str_ref)
            if s is not None:
                out.append((_ir_symbol_name(sym_ref), s))
    return out


def emit_ir(clang, tu, flags, cl_flags=None):
    """Compile a TU to textual LLVM IR.

    `-emit-llvm` stops at a fatal error (unlike `-ast-dump`, which recovers), so
    the system headers MUST resolve - the IR compile uses the clangd compdb's
    per-TU clang-cl flags (`/imsvc` lowercase-mirror includes + defines) when
    available, falling back to the bare MS_FLAGS otherwise.

    In clang-cl driver mode `-S`/`-o -` are rejected, and `-Xclang -emit-llvm`
    writes textual IR only to a real `-o` file - so write to a temp file and read
    it back. The plain driver streams to stdout.
    """
    if cl_flags is not None:
        with tempfile.NamedTemporaryFile(suffix=".ll", delete=False) as tf:
            ll = tf.name
        try:
            cmd = [clang, "--driver-mode=cl", "/c", *cl_flags, f"/I{INC}",
                   "-Xclang", "-emit-llvm", "-o", ll, tu]
            res = subprocess.run(cmd, capture_output=True, text=True)
            ir = Path(ll).read_text() if os.path.getsize(ll) else ""
        finally:
            try:
                os.unlink(ll)
            except OSError:
                pass
        if not ir:
            log(f"ERROR {tu}: clang -emit-llvm produced no IR\n{res.stderr[:400]}")
            return None
        return ir
    cmd = [clang, *MS_FLAGS, *flags, f"-I{INC}", "-S", "-emit-llvm", "-o", "-", tu]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if not res.stdout:
        log(f"ERROR {tu}: clang -emit-llvm produced no IR\n{res.stderr[:400]}")
        return None
    return res.stdout


def func_labels_from_ir(ir):
    """[(rva, mangled_or_None, size_or_None)] keyed by the symbol the annotation
    is attached to. Returns rows as (rva, candidate_name, size, override).

    `rva:`/`symbol:` annotations attached to the SAME symbol are merged: the
    symbol the IR pairs the annotation with IS the function's mangled name, so a
    standalone SYMBOL override is rarely needed, but is honoured when present.
    """
    rows = {}            # mangled symbol -> {"rva":..,"size":..,"override":..}
    for sym, ann in parse_ir_annotations(ir):
        ma = ANN_RVA_RE.match(ann)
        if ma:
            size = None
            if ma.group(2):
                s = ma.group(2)
                size = int(s, 16) if s.lower().startswith("0x") else int(s)
            rows.setdefault(sym, {})["rva"] = int(ma.group(1), 16)
            rows[sym].setdefault("size", None)
            if size is not None:
                rows[sym]["size"] = size
            continue
        ms = ANN_SYM_RE.match(ann)
        if ms:
            rows.setdefault(sym, {})["override"] = ms.group(1)
    out = []
    for sym, d in rows.items():
        if "rva" not in d:
            continue                     # SYMBOL with no RVA -> nothing to label
        out.append((d["rva"], sym, d.get("size"), d.get("override")))
    return out


# --- DATA via AST (extern annotations are dropped from IR) ------------------
def collect_vars(ast, main_file):
    """[(mangledName, offset)] for global VARIABLE declarations in main_file.

    A file-scope / `extern` variable carries a linkage mangledName
    (`?g_foo@@3...`, `_g_foo` for `extern "C"`); function locals do not, so they
    are excluded naturally. Main-file-only (like collect_defs): a header-declared
    global's offset is into the header, so without this guard it could be misread
    against the .cpp line index and a `DATA()` could bind to the wrong VarDecl.
    """
    main_real = os.path.realpath(main_file)
    out = []
    state = {"in_main": True}

    def update_file(node):
        for loc in (node.get("loc"), (node.get("range") or {}).get("begin")):
            if isinstance(loc, dict) and loc.get("file") is not None:
                state["in_main"] = os.path.realpath(loc["file"]) == main_real

    def visit(node):
        if isinstance(node, dict):
            update_file(node)
            if (state["in_main"] and node.get("kind") == "VarDecl"
                    and "mangledName" in node and not node.get("isImplicit")):
                loc = node.get("loc") or {}
                off = loc.get("offset")
                if off is not None:
                    out.append((node["mangledName"], off))
            for c in node.get("inner", []) or []:
                visit(c)
    visit(ast)
    return out


def clang_ast(clang, tu, flags, cl_flags=None):
    if cl_flags is not None:
        cmd = [clang, "--driver-mode=cl", *cl_flags, f"/I{INC}", tu, "-fsyntax-only",
               "-Xclang", "-ast-dump=json"]
    else:
        cmd = [clang, *MS_FLAGS, *flags, f"-I{INC}", tu, "-fsyntax-only",
               "-Xclang", "-ast-dump=json"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    try:
        return json.loads(res.stdout)
    except json.JSONDecodeError:
        log(f"ERROR {tu}: clang produced no JSON AST\n{res.stderr[:400]}")
        return None


def load_compdb(path):
    """compile_commands.json -> {realpath(source): [clang-cl flags]}.

    The clangd compdb carries the per-TU MS flags + `/imsvc` lowercase-mirror
    include dirs that let clang's header lookup succeed on case-sensitive Linux.
    We drop the driver (clang-cl), the `/c`, and the source file - emit_ir/clang_ast
    re-add their own driver mode, action, and the TU.
    """
    try:
        db = json.loads(Path(path).read_text())
    except (OSError, json.JSONDecodeError):
        return {}
    out = {}
    for e in db:
        args = e.get("arguments") or []
        if not args:
            continue
        src = e.get("file")
        flags = [a for a in args[1:] if a != "/c" and a != src]
        if src:
            out[os.path.realpath(src)] = flags
    return out


def data_labels(text, ast, main_file):
    """[(rva, mangledName)] for each DATA(0x..) macro bound to the AST VarDecl
    just below it (by line). The variable pool and macro sites never cross with
    functions, so a non-matched global cannot steal a function's address.
    """
    off2line = line_index(text)
    var_defs = sorted((off2line(off), mn) for (mn, off) in collect_vars(ast, main_file))
    out = []
    line_no = 1
    for m in re.finditer(r"[^\n]*\n", text):
        seg = m.group(0)
        dm = DATA_MACRO_RE.search(seg)
        if dm:
            rva = int(dm.group(1), 16)
            cand = next((mn for (dl, mn) in var_defs if dl >= line_no), None)
            out.append((rva, cand))
        line_no += 1
    return out


# --- static config path (vendored C TUs with pristine source; see docstring) --
def load_label_config(path):
    """unit -> [(rva, name, size, kind)] from the static rva->symbol table
    (config/zlib_labels.csv): the labels for vendored C TUs whose source carries
    no annotations. Generated once (the retail binary never changes)."""
    import csv
    cfg = {}
    if not Path(path).exists():
        return cfg
    with open(path) as f:
        for r in csv.reader(f):
            if not r or r[0].strip() in ("", "rva") or r[0].lstrip().startswith("#"):
                continue
            try:
                rva = int(r[0], 16)
            except ValueError:
                continue
            unit = r[2] if len(r) > 2 else ""
            size = int(r[3], 16) if len(r) > 3 and r[3] else None
            kind = r[4] if len(r) > 4 and r[4] else "func"
            cfg.setdefault(unit, []).append((rva, r[1], size, kind))
    return cfg


def config_tu(unit, entries, obj_syms, all_syms, rows, misses, addr_sites):
    """Emit symbol_names rows for a vendored C TU straight from the static config
    table, authority-checked against the base obj. No source parse, no join."""
    for rva, name, size, kind in entries:
        if kind != "data":
            addr_sites.setdefault(rva, []).append((unit, name))
        pool = all_syms if kind == "data" else obj_syms
        if obj_syms is None or name in pool:
            rows.append((rva, name, unit, size, kind))
        else:
            misses.append((rva, name, unit, "config candidate not in base obj"))


def nm_symbols(obj, nm="llvm-nm"):
    """Defined CODE symbols (functions) in the obj.

    Restricted to text/code symbol types (T/t/W/w) so data artifacts - notably
    clang's `@4HA` static-init guards that demangle to the same `~Class` identity
    as the real destructor - don't pollute the function-symbol set.
    """
    res = subprocess.run([nm, "--defined-only", obj],
                         capture_output=True, text=True)
    syms = set()
    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2 and len(parts[-2]) == 1 and parts[-2] in "TtWw":
            syms.add(parts[-1])
    return syms


def nm_all_symbols(obj, nm="llvm-nm"):
    """Every symbol name in the obj, defined or undefined.

    A matched global is only *referenced* in the base obj (declared `extern`), so
    its mangled name appears as an undefined (`U`) external - the data authority
    check accepts those alongside defined data symbols.
    """
    res = subprocess.run([nm, obj], capture_output=True, text=True)
    syms = set()
    for line in res.stdout.splitlines():
        parts = line.split()
        if parts:
            syms.add(parts[-1])
    return syms


# A destructor-variant operator code: ??_D (vbase), ??_E (vector deleting),
# ??_G (scalar deleting), ??1 (the real one). clang's AST/IR reports ??_D for an
# out-of-line dtor definition, so the candidate misses the obj.
_DTOR_CAND = re.compile(r"^\?\?(?:_[DEG]|1)([\w@?$]+?@@)")


def plain_dtor_symbol(candidate, obj_syms):
    """clang's dtor candidate (e.g. `??_DCFileIO@@QAEXXZ`) misses the real `??1`.

    Resolve it by matching the CANONICAL plain-destructor mangling
    `??1<class>@@<quals>@XZ` directly in the obj's code symbols. Returns the
    unique symbol or None.
    """
    m = _DTOR_CAND.match(candidate)
    if not m:
        return None
    classpart = m.group(1)                      # e.g. "CFileIO@@"
    pat = re.compile(r"^\?\?1" + re.escape(classpart) + r"[A-Z]+@XZ$")
    hits = [s for s in obj_syms if pat.match(s)]
    return hits[0] if len(hits) == 1 else None


def units_from_toml(path):
    """source-path (repo-relative) -> unit stem."""
    import tomllib
    with open(path, "rb") as f:
        data = tomllib.load(f)
    return {u["source"]: u["unit"] for u in data.get("unit", [])}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--clang", default=os.environ.get("GRUNTZ_CLANG") or "clang")
    ap.add_argument("--nm", default="llvm-nm")
    ap.add_argument("--tu", action="append", default=[], required=True,
                    help="source TU(s) to read annotations from.")
    ap.add_argument("--flag", action="append", default=[])
    ap.add_argument("--obj", action="append", default=[],
                    help="base <unit>.obj for the authority check (same order/count "
                         "as --tu, or omit to skip).")
    ap.add_argument("--unit", action="append", default=[],
                    help="unit stem per --tu (else derived from units.toml).")
    ap.add_argument("--units-toml", default=str(REPO / "config/units.toml"))
    ap.add_argument("--compdb", default=str(REPO / "build/clangd/compile_commands.json"),
                    help="clangd compile_commands.json for per-TU MS/include flags "
                         "(the IR emit needs system headers to resolve).")
    ap.add_argument("--out", default=str(REPO / "build/gen/symbol_names.csv"))
    args = ap.parse_args()

    unit_map = {}
    if not args.unit and Path(args.units_toml).exists():
        unit_map = units_from_toml(args.units_toml)

    compdb = load_compdb(args.compdb) if args.compdb else {}
    label_config = load_label_config(LABEL_CONFIG)

    rows = []          # (rva, name, unit, size, kind)
    misses = []        # (rva, candidate, unit, reason)
    addr_sites = {}    # rva -> [(tu, "fn")] for every function rva, to catch dups
    for i, tu in enumerate(args.tu):
        text = Path(tu).read_text()
        if args.unit:
            unit = args.unit[i]
        else:
            rel = str(Path(tu))
            unit = unit_map.get(rel) or unit_map.get("./" + rel) or Path(tu).stem

        have_obj = i < len(args.obj)
        obj_syms = nm_symbols(args.obj[i], args.nm) if have_obj else None
        all_syms = nm_all_symbols(args.obj[i], args.nm) if have_obj else None
        cl_flags = compdb.get(os.path.realpath(tu))

        # A TU with no src/rva.h macro is a vendored C TU with pristine source;
        # its rva->symbol map comes from config/zlib_labels.csv (static, emitted
        # directly - no parse, no join). zlib's static/K&R functions drop from IR
        # when unused, so labels can't live in the source; src/ uses the macros.
        if not MACRO_RE.search(text):
            config_tu(unit, label_config.get(unit, []), obj_syms, all_syms,
                      rows, misses, addr_sites)
            continue

        # --- functions / SYMBOL via LLVM IR (mangled symbol paired directly) ---
        ir = emit_ir(args.clang, tu, args.flag, cl_flags)
        if ir is None:
            continue
        for rva, ir_sym, size, override in func_labels_from_ir(ir):
            addr_sites.setdefault(rva, []).append((tu, ir_sym))
            # The IR pairs the annotation with the function's own mangled symbol;
            # an explicit SYMBOL override wins over it when present.
            name = override or ir_sym
            if obj_syms is None:                  # no authority check (inspection)
                rows.append((rva, name, unit, size, "func"))
                continue
            if name in obj_syms:                  # candidate confirmed in base obj
                rows.append((rva, name, unit, size, "func"))
                continue
            if override:                          # trust explicit SYMBOL
                rows.append((rva, name, unit, size, "func"))
                continue
            # clang's mangledName misses the destructor (it emits the `??_D vbase
            # dtor` variant, not the real `??1`). Resolve the canonical
            # `??1<class>@@...@XZ` directly from the obj's code symbols.
            resolved = plain_dtor_symbol(name, obj_syms)
            if resolved:
                rows.append((rva, resolved, unit, size, "func"))
            else:
                misses.append((rva, name, unit, "candidate not in base obj"))

        # --- standalone `// @rva-symbol:` thunks (no source body for an RVA()) ---
        for m in RVA_SYMBOL_RE.finditer(text):
            sym, rva_s, size_s = m.group(1), m.group(2), m.group(3)
            rva = int(rva_s, 16)
            size = (int(size_s, 16) if size_s and size_s.lower().startswith("0x")
                    else int(size_s) if size_s else None)
            addr_sites.setdefault(rva, []).append((tu, sym))
            if obj_syms is None or sym in obj_syms:
                rows.append((rva, sym, unit, size, "func"))
            else:
                misses.append((rva, sym, unit, "@rva-symbol not in base obj"))

        # --- DATA via AST (IR drops the extern's annotation) ---
        if DATA_MACRO_RE.search(text):
            ast = clang_ast(args.clang, tu, args.flag, cl_flags)
            if ast is not None:
                for rva, cand in data_labels(text, ast, tu):
                    if cand is None:
                        misses.append((rva, None, unit, "no VarDecl below DATA()"))
                        continue
                    if obj_syms is None or cand in all_syms:
                        rows.append((rva, cand, unit, None, "data"))
                    else:
                        misses.append((rva, cand, unit,
                                       "data candidate not in base obj"))

    # A function address identifies exactly one function: the same rva labeled
    # twice (within or across units) is always a mistake - fail loudly rather than
    # silently let one row win.
    dup_addrs = {rva: sites for rva, sites in addr_sites.items() if len(sites) > 1}
    if dup_addrs:
        for rva, sites in sorted(dup_addrs.items()):
            where = ", ".join(f"{tu} ({sym})" for tu, sym in sites)
            log(f"ERROR duplicate RVA 0x{rva:06x}: {where}")
        log(f"{len(dup_addrs)} duplicate RVA label(s); refusing to write {args.out}")
        return 1

    rows.sort()
    # DATA dedup: the same `extern` declared in N TUs emits N rows for one rva.
    # The function dup-guard (addr_sites) deliberately doesn't track data, and
    # synth_pdb keys by rva (last wins), so collapse to one data row per rva
    # (keeping the last, matching synth_pdb). If two declarations give one rva
    # DIFFERENT mangled names, they disagree on the global's type - surface it.
    last_data, data_names, out_rows = {}, {}, []
    for row in rows:
        if row[4] == "data":
            data_names.setdefault(row[0], set()).add(row[1])
            last_data[row[0]] = row     # rows are sorted -> keeps the last per rva
        else:
            out_rows.append(row)
    for rva, names in sorted(data_names.items()):
        if len(names) > 1:
            log(f"WARN data 0x{rva:06x}: conflicting names {sorted(names)} - kept the last")
    out_rows.extend(last_data.values())
    out_rows.sort()
    rows = out_rows
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as f:
        # `size` is the RVA byte extent (hex), the authoritative boundary the
        # synth-PDB uses for matched functions Ghidra's auto-analysis never
        # recovered as objects (empty when a row carries no size).
        # `kind` is func (a matched function) or data (the DATA symbol a matched
        # global is referenced through); synth_pdb routes data rows to the
        # data-symbol table instead of synthesising a function record.
        f.write("rva,name,unit,size,kind\n")
        for rva, name, unit, size, kind in rows:
            size_s = f"0x{size:x}" if size else ""
            f.write(f"0x{rva:06x},{name},{unit},{size_s},{kind}\n")
    log(f"wrote {len(rows)} label(s) -> {out}")
    for rva, cand, unit, why in misses:
        log(f"  MISS 0x{rva:x} [{unit}] {why}" + (f" (cand {cand})" if cand else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
