#!/usr/bin/env python3
"""labels.py - derive the rva -> mangled-name, unit label map from src/.

Replaces the hand-maintained config/symbol_names.csv. The only manual input is
the address annotation on each matched function/global; everything else is
derived. See docs/build-system.md.

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
# Vendored third-party SDK headers live one dir deep under vendor/<sdk>/ (e.g.
# vendor/miles/Mss32.h, vendor/smacker/smack.h) so `#include <Mss32.h>` resolves
# like the original toolchain's SDK include dirs.
VENDOR_INCS = sorted(str(d) for d in (REPO / "vendor").iterdir() if d.is_dir()) \
    if (REPO / "vendor").is_dir() else []
# DirectX 6 headers sit one level deeper (vendor/directx6/Include), like cc_wrap.py.
if (REPO / "vendor" / "directx6" / "Include").is_dir():
    VENDOR_INCS.append(str(REPO / "vendor" / "directx6" / "Include"))
INC_CL = [f"/I{p}" for p in (INC, *VENDOR_INCS)]   # clang-cl driver (/I)
INC_GCC = [f"-I{p}" for p in (INC, *VENDOR_INCS)]  # plain clang driver (-I)

# The single consolidated-globals unit (src/Globals.cpp). Its DATA() rows are
# TRUSTED: the base obj is all unused externs (no symbols), so the authority
# check cannot confirm them - but each name was authority-checked in the matched
# TU it came from before `gruntz.analysis.consolidate_globals` moved it here.
GLOBALS_UNIT = "globals"

TARGET = "i686-pc-windows-msvc"
MSC_COMPAT = "1100"
MS_FLAGS = [f"--target={TARGET}", f"-fms-compatibility-version={MSC_COMPAT}",
            "-fms-extensions"]

# DATA(0x...) macro invocation - scanned from source text (IR drops extern
# annotations). The address is bound to the AST VarDecl below it.
DATA_MACRO_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
# VTBL(Class, 0x...) macro (src/rva.h) - the single-source-of-truth vtable-catalog
# annotation placed atop a class. It expands (under clang) to a `gruntz_clsmeta_*`
# annotate carrier that DOES reach the IR, but it is read from source text
# TREE-WIDE (src/ + include/) in the merge step rather than per-TU IR: the scan is
# include-independent, so a VTBL in a header not pulled into any built TU is still
# catalogued (and its invocation syntax is placement-independent, so atop-the-class
# and legacy .cpp-EOF sites parse identically). It is lowered to a `??_7<Class>@@6B@`
# DATA row. The name is TARGET-side (the EXE has no debug symbols) and reloc-masked,
# so it is matching-neutral tracking, not a match lever (hence NO authority check -
# it names the datum in the catalog regardless of whether any base obj references
# it yet). Only simple global-namespace class names lower cleanly here;
# templated/namespaced vtables stay in vtable_names.csv.
VTBL_MACRO_RE = re.compile(r"\bVTBL\s*\(\s*([A-Za-z_]\w*)\s*,\s*(0x[0-9a-fA-F]+)\s*\)")
# `// @rva-symbol: <mangled> <rva> [<size>]` - a self-contained function label for
# a compiler-generated thunk that has NO source definition to hang an RVA()
# attribute on (a `??_G` scalar-deleting destructor). The mangled name is given
# verbatim, so there is no join and no IR - it is read in every TU.
RVA_SYMBOL_RE = re.compile(
    r"@rva-symbol:\s*(\S+)\s+(0x[0-9a-fA-F]+)(?:\s+(0x[0-9a-fA-F]+|\d+))?")
# `// @data-symbol: <mangled> <rva> [<size>]` - the DATA analog of @rva-symbol: a
# compiler-emitted DATUM with no source VarDecl to hang DATA() on (a `??_7`
# vftable or `??_R*` RTTI record). The EXE carries no debug symbols, so the
# delinked datum's name is ours to assign - this names it to match what cl emits
# for the real polymorphic class, so the ctor's `mov [this],offset ??_7...` reloc
# pairs and can flip to byte-exact. Emitted with kind=data (checked vs all_syms).
DATA_SYMBOL_RE = re.compile(
    r"@data-symbol:\s*(\S+)\s+(0x[0-9a-fA-F]+)(?:\s+(0x[0-9a-fA-F]+|\d+))?")


def resolve_pool_id(sym, all_syms):
    """`_s_fmtNotFound$S*` -> the one base-obj symbol with that prefix.

    cl decorates every file-scope static / function-local static with `$S<n>`, where <n>
    is a per-OBJECT CodeView symbol counter: it walks every symbol the TU emits, so the
    ids SHIFT whenever the TU gains or loses ANY symbol. Pinning a literal id therefore
    rots on the next edit to that .cpp, silently unbinding every reloc that pointed at it
    (this bit the ButeMgr string pool twice). A `$S*` pin is matched by prefix instead, so
    it survives the churn. Ambiguous or unmatched -> return as-is and let the caller's
    authority check report the miss."""
    if "$S*" not in sym or not all_syms:
        return sym
    pref = sym.replace("$S*", "$S")
    cands = [s for s in all_syms if s.startswith(pref) and s[len(pref):].isdigit()]
    return cands[0] if len(cands) == 1 else sym
# Annotation strings carried in @llvm.global.annotations (emitted by src/rva.h).
ANN_RVA_RE = re.compile(r"^rva:(0x[0-9a-fA-F]+)(?:\s+size:(0x[0-9a-fA-F]+|\d+))?$")
ANN_SYM_RE = re.compile(r"^symbol:(\S+)$")
# Macro annotation markers (presence of any -> a migrated TU; functions/SYMBOL
# come from IR for these). A TU with none falls back to the legacy comment path.
# A TU "carries labels" if it invokes an rva.h macro OR carries standalone
# `// @rva-symbol:` / `// @data-symbol:` pins - a pin-only TU (e.g. an explicit
# template-instantiation host whose every function is a compiler-emitted COMDAT,
# like ArraySerialize.cpp's CArray<PLAYLISTINFOSTRUCT*>) has no macro to invoke,
# and without this alternation it silently fell through to the vendored-C path
# and contributed ZERO rows.
MACRO_RE = re.compile(r"\b(?:RVA|DATA|SYMBOL)\s*\(|@(?:rva|data)-symbol:")

# Static rva->symbol table for vendored C TUs whose source carries no labels.
LABEL_CONFIG = REPO / "config/zlib_labels.csv"
# Deterministic ??_7<Class>@@6B@ -> (rva, size) map for the RTTI vtables,
# generated from the retail EXE's RTTI by `gruntz.core.vtable_scan
# --emit-names`. The EXE has no debug symbols, so these target-side names are
# ours and fully derivable from the class name + the vtable RVA. A row is applied
# ONLY when a TU's base obj actually emits that ??_7 (the class was made
# real-polymorphic in src) -> inert (no target symbol) until a class is converted.
VTABLE_NAMES = REPO / "config/vtable_names.csv"


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
        # Retry once: under heavy parallel label-regen the temp .ll can go missing
        # (clang left nothing / a tmp race), which is transient - a re-run succeeds.
        res = None
        for _attempt in range(2):
            with tempfile.NamedTemporaryFile(suffix=".ll", delete=False) as tf:
                ll = tf.name
            try:
                cmd = [clang, "--driver-mode=cl", "/c", "/DGRUNTZ_EMIT_META",
                       *cl_flags, *INC_CL, "-Xclang", "-emit-llvm", "-o", ll, tu]
                res = subprocess.run(cmd, capture_output=True, text=True)
                # exists-guard: getsize() FileNotFounds on a vanished temp; treat as no-IR.
                ir = Path(ll).read_text() if (os.path.exists(ll) and os.path.getsize(ll)) else ""
            finally:
                try:
                    os.unlink(ll)
                except OSError:
                    pass
            if ir:
                return ir
        log(f"ERROR {tu}: clang -emit-llvm produced no IR\n"
            f"{(res.stderr[:400] if res else '')}")
        return None
    cmd = [clang, "-DGRUNTZ_EMIT_META", *MS_FLAGS, *flags, *INC_GCC,
           "-S", "-emit-llvm", "-o", "-", tu]
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
    """[(mangledName, offset, qualType)] for global VARIABLE decls in main_file.

    A file-scope / `extern` variable carries a linkage mangledName
    (`?g_foo@@3...`, `_g_foo` for `extern "C"`); function locals do not, so they
    are excluded naturally. The qualType is the declared C/C++ type (e.g. "int",
    "CGameReg *"), carried into globals.json so apply.py can type the global.
    Main-file-only (like collect_defs): a header-declared global's offset is into
    the header, so without this guard it could be misread against the .cpp line
    index and a `DATA()` could bind to the wrong VarDecl.
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
            # `gruntz_clsmeta_*` are the SIZE/SIZE_UNKNOWN/VTBL class-metadata
            # carriers (src/rva.h): file-scope `used` statics that DO carry a
            # mangledName. Skip them so a carrier written between a DATA(...) and
            # its extern can never steal the DATA binding (data_labels picks the
            # first VarDecl below the macro).
            if (state["in_main"] and node.get("kind") == "VarDecl"
                    and "mangledName" in node and not node.get("isImplicit")
                    and not (node.get("name") or "").startswith("gruntz_clsmeta_")):
                loc = node.get("loc") or {}
                off = loc.get("offset")
                if off is not None:
                    ty = node.get("type") or {}
                    qt = ty.get("qualType") or ""
                    # clang's own desugaring of a typedef (HWND -> HWND__ *), used
                    # only to SIZE the global; the sugar type stays the reported one.
                    out.append((node["mangledName"], off, qt,
                                ty.get("desugaredQualType") or ""))
            for c in node.get("inner", []) or []:
                visit(c)
    visit(ast)
    return out


# --- DATA() sizeof: the EXACT extent of a matched global from its declared type
# A CodeView data record carries no length and the retail EXE has no symbol sizes,
# so the DECLARED C++ TYPE is the only authority for a global's byte extent (the
# same role homm2's libclang VarDecl inventory plays; see docs/data-attribution.md).
# Without it every extent degrades to the next-symbol GAP, which is only an upper
# bound (it swallows padding and any unlabelled neighbour) and is useless as a
# delinker data-manifest extent.
#
# SAFETY: a WRONG size would make the delinker materialize the wrong bytes, so this
# resolver is deliberately conservative - it returns a size ONLY for shapes it can
# prove (i386/MSVC5 scalars, any pointer/reference, a sized array of a resolvable
# element, or a record whose clang-computed layout is in structs.json) and None for
# everything else. None simply leaves the column empty = today's behaviour.
_SCALAR_SIZES = {
    # Rust-style aliases (include/Ints.h) - Gruntz's own, unambiguous.
    "i8": 1, "u8": 1, "i16": 2, "u16": 2, "i32": 4, "u32": 4,
    "i64": 8, "u64": 8, "f32": 4, "f64": 8,
    # C/C++ scalars, i386 / MSVC 5.0 ABI.
    "char": 1, "signed char": 1, "unsigned char": 1, "bool": 1,
    "short": 2, "short int": 2, "unsigned short": 2, "unsigned short int": 2,
    "wchar_t": 2,
    "int": 4, "signed int": 4, "unsigned int": 4, "unsigned": 4,
    "long": 4, "long int": 4, "unsigned long": 4, "unsigned long int": 4,
    "float": 4,
    "double": 8, "long double": 8, "__int64": 8, "unsigned __int64": 8,
}
_ARRAY_RE = re.compile(r"^(.*?)\s*\[(\d+)\]$")
_RECORD_SIZES_CACHE = {}


def record_sizes(path=None):
    """{record name: byte size} from the generated clang record layouts.

    build/gen/structs.json is produced by ghidra_metadata_generate with the same
    i386/MSVC target as the build, so its sizes ARE the MSVC layout. Absent file =>
    no record sizes (scalars/pointers still resolve); never fatal."""
    path = str(path or (REPO / "build/gen/structs.json"))
    if path not in _RECORD_SIZES_CACHE:
        out = {}
        try:
            for rec in json.loads(Path(path).read_text()):
                name, size = rec.get("name"), rec.get("size")
                if name and isinstance(size, int) and size > 0:
                    # Same record from several TUs must agree; a disagreement means
                    # the layout is TU-dependent -> refuse to size it.
                    if out.get(name, size) != size:
                        out[name] = None
                    else:
                        out.setdefault(name, size)
        except (OSError, json.JSONDecodeError, TypeError):
            pass
        _RECORD_SIZES_CACHE[path] = {k: v for k, v in out.items() if v}
    return _RECORD_SIZES_CACHE[path]


# Trailing/leading cv-qualifier, with or without a separating space ("void *const").
_CV_TRAIL_RE = re.compile(r"\s*\b(?:const|volatile)\s*$")
_CV_LEAD_RE = re.compile(r"^\s*\b(?:const|volatile)\b\s*")


def _strip_cv(t):
    t = t.strip()
    for _ in range(4):
        before = t
        t = _CV_LEAD_RE.sub("", t)
        t = _CV_TRAIL_RE.sub("", t)
        t = t.strip()
        if t == before:
            break
    return t


def sizeof_qualtype(qt, records=None, desugared=None):
    """Exact sizeof for a clang qualType, or None when not provable.

    `desugared` is clang's own desugaredQualType for the same declaration. Win32/MFC
    typedefs (HWND, WORD, HINSTANCE, ...) are resolved through it rather than a
    hand-rolled typedef table, so the REAL headers stay the authority (see CLAUDE.md).
    """
    for cand in (qt, desugared):
        size = _sizeof_one(cand, records)
        if size:
            return size
    return None


def _sizeof_one(qt, records=None):
    if not qt:
        return None
    t = _strip_cv(qt)
    if not t or "(" in t:          # function / function-pointer types: not sized here
        return None
    if t.endswith(("*", "&")):     # any pointer/reference is 4 on i386
        return 4
    m = _ARRAY_RE.match(t)
    if m:
        elem = _sizeof_one(m.group(1), records)
        n = int(m.group(2))
        return elem * n if elem else None
    if t.endswith("[]"):           # incomplete array: extent unknown
        return None
    if t in _SCALAR_SIZES:
        return _SCALAR_SIZES[t]
    recs = record_sizes() if records is None else records
    key = re.sub(r"^(struct|class|union|enum)\s+", "", t)
    return recs.get(key)


def clang_ast(clang, tu, flags, cl_flags=None):
    if cl_flags is not None:
        cmd = [clang, "--driver-mode=cl", *cl_flags, *INC_CL, tu, "-fsyntax-only",
               "-Xclang", "-ast-dump=json"]
    else:
        cmd = [clang, *MS_FLAGS, *flags, *INC_GCC, tu, "-fsyntax-only",
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


def blank_comments(text):
    """Copy of `text` with // and /* */ comment bodies blanked to spaces (newlines
    kept, so offsets/line numbers are unchanged). Lets DATA_MACRO_RE match only real
    code - a `DATA(0x..)` written inside a COMMENT must not be read as a binding
    (it would steal the next VarDecl and double-bind the address)."""
    out = list(text)
    n, i, st = len(text), 0, "code"
    while i < n:
        c = text[i]
        if st == "code":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    out[i] = " "; i += 1
                continue
            if c == "/" and i + 1 < n and text[i + 1] == "*":
                while i < n and not (text[i] == "*" and i + 1 < n and text[i + 1] == "/"):
                    if text[i] != "\n":
                        out[i] = " "
                    i += 1
                continue
            if c in "\"'":
                st = c
        elif c == "\\":
            i += 2; continue
        elif c == st:
            st = "code"
        i += 1
    return "".join(out)


def data_labels(text, ast, main_file):
    """[(rva, mangledName, qualType)] for each DATA(0x..) macro bound to the AST
    VarDecl just below it (by line). The variable pool and macro sites never cross
    with functions, so a non-matched global cannot steal a function's address.
    """
    off2line = line_index(text)
    var_defs = sorted((off2line(off), mn, qt, dq)
                      for (mn, off, qt, dq) in collect_vars(ast, main_file))
    out = []
    line_no = 1
    for m in re.finditer(r"[^\n]*\n", blank_comments(text)):
        seg = m.group(0)
        dm = DATA_MACRO_RE.search(seg)
        if dm:
            rva = int(dm.group(1), 16)
            cand = next(((mn, qt, dq) for (dl, mn, qt, dq) in var_defs if dl >= line_no),
                        (None, None, None))
            out.append((rva, cand[0], cand[1], cand[2]))
        line_no += 1
    return out


# --- function signatures for Ghidra prototypes (functions.json) ------------
# llvm-undname gives the authoritative signature (return type, calling
# convention, class, parameter TYPES) for every mangled symbol; the source AST
# adds the parameter NAMES, which the mangling does not carry. apply.py turns
# these into typed, named Ghidra prototypes (+ a typed `this`). This is the
# structured replacement for the removed `// engine-label:` JSON `prototype`.
_FUNC_DECL_KINDS = {"FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl",
                    "CXXDestructorDecl", "CXXConversionDecl"}


def param_names_from_ast(ast, main_file):
    """{mangledName: [param-name-or-None, ...]} for function DEFINITIONS in
    main_file. Parameter names live only in the source, not the mangling.
    Main-file-only (like collect_vars) so a header-inlined definition's params
    aren't misattributed."""
    main_real = os.path.realpath(main_file)
    out = {}
    state = {"in_main": True}

    def update_file(node):
        for loc in (node.get("loc"), (node.get("range") or {}).get("begin")):
            if isinstance(loc, dict) and loc.get("file") is not None:
                state["in_main"] = os.path.realpath(loc["file"]) == main_real

    def visit(node):
        if not isinstance(node, dict):
            return
        update_file(node)
        if (state["in_main"] and node.get("kind") in _FUNC_DECL_KINDS
                and node.get("mangledName")):
            inner = node.get("inner") or []
            if any(c.get("kind") == "CompoundStmt" for c in inner):   # has a body
                out[node["mangledName"]] = [
                    (c.get("name") or None)
                    for c in inner if c.get("kind") == "ParmVarDecl"]
        for c in node.get("inner") or []:
            visit(c)
    visit(ast)
    return out


def undname_map(symbols, undname="llvm-undname"):
    """{mangled: demangled} via one llvm-undname call (stdin batch).

    undname echoes each input symbol then its demangling, separated by a blank
    line; key each block by its echoed first line.
    """
    syms = [s for s in dict.fromkeys(symbols) if s]
    if not syms:
        return {}
    res = subprocess.run([undname], input="\n".join(syms) + "\n",
                         capture_output=True, text=True)
    out = {}
    for block in res.stdout.split("\n\n"):
        ls = block.splitlines()
        if len(ls) >= 2:
            out[ls[0].strip()] = ls[1].strip()
    return out


_CC_KW_RE = re.compile(r"__(thiscall|cdecl|stdcall|fastcall)\b")
_ACCESS_RE = re.compile(r"^(?:public|private|protected):\s*")


def _split_top_commas(s):
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "<([":
            depth += 1; cur += ch
        elif ch in ">)]":
            depth -= 1; cur += ch
        elif ch == "," and depth == 0:
            out.append(cur); cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [x.strip() for x in out]


def parse_demangled(dem):
    """llvm-undname output -> signature dict, or None when not a usable function.

    'public: int __thiscall RezMgr::MakeImageKey(void *, char *, void *)' ->
      {qual:'RezMgr::MakeImageKey', cls:'RezMgr', kind:'method', ret:'int',
       cc:'__thiscall', param_types:['void *','char *','void *']}
    """
    if not dem or "(" not in dem or ")" not in dem:
        return None
    m = _CC_KW_RE.search(dem)
    if not m:
        return None                       # no calling convention -> not a function
    cc = "__" + m.group(1)
    pre = _ACCESS_RE.sub("", dem[:m.start()]).strip()
    is_static = "static" in pre.split()
    is_virtual = "virtual" in pre.split()
    ret = re.sub(r"\b(?:static|virtual)\b", "", pre).strip() or "void"
    post = dem[m.end():].strip()
    # params = the final balanced (...) group (handles operator()(...)); qual is
    # everything before it.
    close = post.rfind(")")
    depth, open_idx = 0, -1
    for i in range(close, -1, -1):
        if post[i] == ")":
            depth += 1
        elif post[i] == "(":
            depth -= 1
            if depth == 0:
                open_idx = i
                break
    if open_idx < 0:
        return None
    qual = post[:open_idx].strip()
    param_types = [p for p in _split_top_commas(post[open_idx + 1:close])
                   if p and p != "void"]
    cls = qual.rsplit("::", 1)[0] if "::" in qual else None
    leaf = qual.rsplit("::", 1)[-1]
    if "~" in leaf or "destructor" in leaf or "deleting dtor" in leaf:
        kind = "dtor"
    elif cls and leaf == cls.rsplit("::", 1)[-1]:
        kind = "ctor"
    elif is_static:
        kind = "static"
    elif cls:
        kind = "vfunc" if is_virtual else "method"
    else:
        kind = "free"
    return {"qual": qual, "cls": cls, "kind": kind, "ret": ret, "cc": cc,
            "param_types": param_types}


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


def load_vtable_names(path):
    """name -> (rva, size) for the deterministic ??_7<Class>@@6B@ vtable map
    (config/vtable_names.csv, generated by gruntz.core.vtable_scan)."""
    import csv
    m = {}
    if not Path(path).exists():
        return m
    with open(path) as f:
        for r in csv.reader(f):
            if not r or r[0].strip() in ("", "name") or r[0].lstrip().startswith("#"):
                continue
            try:
                rva = int(r[1], 16)
            except (ValueError, IndexError):
                continue
            size = int(r[2], 16) if len(r) > 2 and r[2] else None
            m[r[0].strip()] = (rva, size)
    return m


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


def write_symbol_names(rows, addr_sites, out, misses=None):
    """Finalize + write symbol_names.csv: the cross-TU duplicate-RVA guard, the
    DATA dedup (keep last per rva, matching synth_pdb), sort, header. Shared by the
    per-TU emit and the --merge step so both apply the same checks."""
    # A function address identifies exactly one function. The SAME symbol at one rva
    # in several units is LEGITIMATE - an inline HEADER member is emitted (as one
    # deduped COMDAT) by every TU that includes the header, so labels.py sees it in
    # each includer's IR. Only DISTINCT names at one rva are a real mistake (two
    # different functions, or a mislabeled rva). Same-name duplicates are collapsed
    # to a single row by the per-rva dedup below.
    dup_addrs = {rva: sites for rva, sites in addr_sites.items()
                 if len({n for _u, n in sites}) > 1}
    if dup_addrs:
        for rva, sites in sorted(dup_addrs.items()):
            where = ", ".join(f"{a} ({b})" for a, b in sites)
            log(f"ERROR duplicate RVA 0x{rva:06x}: {where}")
        log(f"{len(dup_addrs)} conflicting RVA label(s); refusing to write {out}")
        return 1
    # RE-PROLIFERATION GUARD (data): a global consolidated into the trusted
    # `globals` unit (src/Globals.cpp, by gruntz.analysis.consolidate_globals) is
    # the SINGLE DATA() binding for its address. If the SAME (rva, name) DATA also
    # appears in another unit, a matcher re-declared an already-consolidated
    # global - a hard error (mirrors the function dup-RVA guard). Differently-NAMED
    # data at one rva (the engine-global modelling backlog + auto vtable/string
    # symbols) stays keep-last+WARN below, so this never false-fires on those.
    glob_sites = {}
    for rva, name, unit, _size, kind in rows:
        if kind == "data":
            glob_sites.setdefault((rva, name), set()).add(unit)
    reprol = {k: u for k, u in glob_sites.items()
              if GLOBALS_UNIT in u and len(u) > 1}
    if reprol:
        for (rva, name), units in sorted(reprol.items()):
            others = ", ".join(sorted(u for u in units if u != GLOBALS_UNIT))
            log(f"ERROR re-declared consolidated global 0x{rva:06x} {name}: "
                f"in '{GLOBALS_UNIT}' and also {others} - move it back to "
                f"src/Globals.cpp only (it is declared via <Globals.h>)")
        log(f"{len(reprol)} re-proliferated global(s); refusing to write {out}")
        return 1
    rows.sort()
    # Per-rva dedup (keep last; rows are sorted -> deterministic).
    #  DATA: the same `extern` declared in N TUs emits N rows for one rva.
    #  FUNC: an inline HEADER member emitted as one deduped COMDAT by N includer TUs
    #        yields N identical rows -> collapse to one. Distinct func names at one
    #        rva were already rejected above, so this only folds true same-symbol
    #        duplicates. synth_pdb also keys by rva, so one row per rva is required.
    last_data, data_names, last_func = {}, {}, {}
    for row in rows:
        if row[4] == "data":
            data_names.setdefault(row[0], set()).add(row[1])
            last_data[row[0]] = row
        else:
            last_func[row[0]] = row
    for rva, names in sorted(data_names.items()):
        if len(names) > 1:
            log(f"WARN data 0x{rva:06x}: conflicting names {sorted(names)} - kept the last")
    out_rows = list(last_func.values()) + list(last_data.values())
    out_rows.sort()
    out = Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    # `size` is the RVA byte extent (hex); `kind` is func or data (synth_pdb routes
    # data rows to the data-symbol table). See the module docstring.
    lines = ["rva,name,unit,size,kind"]
    for rva, name, unit, size, kind in out_rows:
        size_s = f"0x{size:x}" if size else ""
        lines.append(f"0x{rva:06x},{name},{unit},{size_s},{kind}")
    content = "\n".join(lines) + "\n"
    # WRITE-IF-CHANGED: leave the file (and its mtime) untouched when the content is
    # identical, so ninja's `restat` stops the cascade. A pure code edit recompiles
    # the obj but does not change the labels, so this fragment/csv is byte-identical
    # and merge -> delink -> objdiff are all skipped.
    if not (out.exists() and out.read_text() == content):
        out.write_text(content)
        log(f"wrote {len(out_rows)} label(s) -> {out}")
    else:
        log(f"unchanged ({len(out_rows)} labels) -> {out}")
    for rva, cand, unit, why in (misses or []):
        log(f"  MISS 0x{rva:x} [{unit}] {why}" + (f" (cand {cand})" if cand else ""))
    return 0


def _write_json_if_changed(obj, path, label):
    """Write JSON only when the content changed (mtime-stable for ninja restat)."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    content = json.dumps(obj, indent=1)
    if not (path.exists() and path.read_text() == content):
        path.write_text(content)
        log(f"wrote {len(obj)} {label} -> {path}")
    else:
        log(f"unchanged ({len(obj)} {label}) -> {path}")


def merge_json_fragments(frags, out, label, key="rva"):
    """Combine per-TU JSON list fragments into one sorted list (last per `key`
    wins, matching the CSV's keep-last semantics) and write it. Each fragment is a
    functions.json/globals.json slice from one --tu run. Missing fragments are
    tolerated (a TU may carry no funcs/globals)."""
    by_key = {}
    for frag in frags:
        p = Path(frag)
        if not p.exists():
            continue
        try:
            for d in json.loads(p.read_text() or "[]"):
                by_key[d.get(key)] = d
        except (OSError, json.JSONDecodeError):
            continue
    merged = sorted(by_key.values(), key=lambda d: d.get(key) or "")
    _write_json_if_changed(merged, out, label)
    return merged


def vtbl_labels(repo):
    """[(rva, "??_7<Class>@@6B@")] for every VTBL(Class, 0x..) macro under src/ +
    include/ (comments blanked). The vtable-catalog single source of truth: a
    class's VTBL(...) annotation lives in the class's header, so it is scanned
    tree-wide here (in the once-per-build merge step) rather than per-TU. The
    emitted name is TARGET-side and reloc-masked -> matching-neutral, so it is
    NOT authority-checked. De-duplicated on (rva, name); a genuinely conflicting
    name at one rva is left for write_symbol_names to WARN + keep-last."""
    repo = Path(repo)
    seen, out = set(), []
    for root in (repo / "src", repo / "include"):
        if not root.exists():
            continue
        for path in sorted(list(root.rglob("*.h")) + list(root.rglob("*.cpp"))):
            try:
                text = blank_comments(path.read_text())
            except OSError:
                continue
            for m in VTBL_MACRO_RE.finditer(text):
                rva = int(m.group(2), 16)
                name = f"??_7{m.group(1)}@@6B@"
                if (rva, name) not in seen:
                    seen.add((rva, name))
                    out.append((rva, name))
    return out


LABELS_MANIFEST = REPO / "config/labels_manifest.tsv"


def check_labels_manifest(rows):
    """BUILD-INTEGRITY GATE (fatal): the tree-wide label DENOMINATOR may not shrink.

    The per-TU and empty-fragment gates make it impossible for one unit to contribute
    nothing. This is the last line: a COMMITTED per-unit function count. Any unit whose
    count DROPS - or that disappears entirely - fails the build and is named, with the
    before/after. Growth is fine and is recorded silently.

    Why a committed file: the failure we are guarding against makes the numbers look
    BETTER (fewer functions in the denominator => a higher exact %). Nothing in the build
    itself can notice that, because every metric is computed from the very file that just
    lost the rows. Only a value carried in from the last known-good commit can.

    A deliberate drop (deleting a stub, moving functions between units) is acknowledged
    with GRUNTZ_LABELS_ACK=1, which rewrites the manifest; the commit then RECORDS the
    drop, which is exactly the "explicit acknowledgement" we want in the history.
    """
    cur = {}
    for _rva, _name, unit, _size, kind in rows:
        if kind != "data":
            cur[unit] = cur.get(unit, 0) + 1
    ack = os.environ.get("GRUNTZ_LABELS_ACK") == "1"
    if LABELS_MANIFEST.exists() and not ack:
        prev = {}
        for ln in LABELS_MANIFEST.read_text().splitlines():
            ln = ln.strip()
            if not ln or ln.startswith("#"):
                continue
            u, _, n = ln.partition("\t")
            if n.strip().isdigit():
                prev[u] = int(n)
        drops = [(u, n, cur.get(u, 0)) for u, n in sorted(prev.items())
                 if cur.get(u, 0) < n]
        if drops:
            for u, was, now in drops:
                log(f"ERROR unit '{u}': labelled functions DROPPED {was} -> {now}"
                    + ("  (the unit vanished entirely)" if now == 0 else ""))
            lost = sum(was - now for _u, was, now in drops)
            log(f"{len(drops)} unit(s) lost {lost} labelled function(s) vs "
                f"{LABELS_MANIFEST.relative_to(REPO)}. A shrinking denominator makes every "
                f"metric look BETTER while the tree gets WORSE, so this is fatal. If the "
                f"drop is DELIBERATE, re-run with GRUNTZ_LABELS_ACK=1 to rewrite the "
                f"manifest (and commit it, so the history records the acknowledgement).")
            return 1
    body = "".join(f"{u}\t{n}\n" for u, n in sorted(cur.items()))
    new = ("# labels_manifest.tsv - per-unit labelled-FUNCTION counts, the committed\n"
           "# floor for the build-integrity denominator gate (labels.py). A unit whose\n"
           "# count DROPS fails the build; see check_labels_manifest(). Regenerate a\n"
           "# deliberate drop with GRUNTZ_LABELS_ACK=1.\n"
           f"# units: {len(cur)}  functions: {sum(cur.values())}\n") + body
    if not LABELS_MANIFEST.exists() or LABELS_MANIFEST.read_text() != new:
        LABELS_MANIFEST.write_text(new)
        log(f"labels manifest: {len(cur)} unit(s), {sum(cur.values())} function(s)"
            + ("  [ACK: drop acknowledged]" if ack else ""))
    return 0


def merge_fragments(frags, out, functions_frags=None, functions_out=None,
                    globals_frags=None, globals_out=None):
    """Combine per-TU fragment CSVs into symbol_names.csv, re-applying the cross-TU
    duplicate-RVA guard + DATA dedup. Each fragment is a symbol_names.csv slice
    (rva,name,unit,size,kind) emitted by one --tu run. The per-TU functions.json /
    globals.json fragments are merged the same way (last per rva wins) so apply.py
    sees EVERY unit's signatures/global types, not just the last TU built. VTBL()
    vtable-catalog rows are folded in here too (tree-wide source scan)."""
    import csv as _csv
    rows, addr_sites = [], {}
    empty_frags = []
    for frag in frags:
        n_before = len(rows)
        with open(frag) as f:
            reader = _csv.DictReader(ln for ln in f if not ln.lstrip().startswith("#"))
            for r in reader:
                rva = int(r["rva"], 16)
                size = int(r["size"], 16) if r.get("size") else None
                kind = r.get("kind") or "func"
                rows.append((rva, r["name"], r["unit"], size, kind))
                if kind != "data":
                    addr_sites.setdefault(rva, []).append((r["unit"], r["name"]))
        if len(rows) == n_before:
            empty_frags.append(frag)
    # BUILD-INTEGRITY GATE (fatal): an EMPTY fragment whose TU actually HAS annotations.
    # The per-TU emit now refuses to write one, but ninja CACHES fragments - a fragment
    # left empty by an older, broken label run survives untouched until its TU changes,
    # so the merge must reject it too. This is the last place a unit can silently
    # contribute nothing.
    #
    # A fragment IS allowed to be empty when its TU carries no RVA/DATA/SYMBOL macro at
    # all: the class-metadata-only TUs (FinalVtables.cpp, OrphanClassMeta.cpp) exist
    # purely to host SIZE()/VTBL() annotations, and VTBL rows are folded in below from a
    # tree-wide source scan, not from the per-TU fragment. So consult the SOURCE rather
    # than guessing from the file - the check stays honest and needs no marker file.
    src_of = {u: s for s, u in units_from_toml(REPO / "config/units.toml").items()} \
        if (REPO / "config/units.toml").exists() else {}
    bad = []
    for frag in empty_frags:
        src = src_of.get(Path(frag).stem)
        if src and (REPO / src).exists() and MACRO_RE.search((REPO / src).read_text()):
            bad.append((frag, src))
    if bad:
        for frag, src in bad:
            log(f"ERROR empty label fragment {frag}: {src} CARRIES rva.h annotations but "
                f"labelled nothing, so its functions would silently vanish from {out}. "
                f"(A common cause: RVA() on an INLINE member that is never called - clang "
                f"only annotates functions it actually EMITS, so the label never reaches "
                f"the IR. Move the body out-of-line.)")
        log(f"{len(bad)} empty label fragment(s); refusing to write {out}.")
        return 1
    # VTBL(Class, 0x..) catalog rows (kind=data, cosmetic "vtables" unit - synth_pdb
    # ignores the unit for data symbols, keying the datum rename by rva->name).
    for rva, name in vtbl_labels(REPO):
        rows.append((rva, name, "vtables", None, "data"))
    rc = check_labels_manifest(rows)
    if rc != 0:
        return rc
    rc = write_symbol_names(rows, addr_sites, out)
    if rc != 0:
        return rc
    if functions_out:
        merge_json_fragments(functions_frags or [], functions_out,
                             "function signature(s)")
    if globals_out:
        merge_json_fragments(globals_frags or [], globals_out, "global(s)")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--clang", default=os.environ.get("GRUNTZ_CLANG") or "clang")
    ap.add_argument("--nm", default="llvm-nm")
    ap.add_argument("--tu", action="append", default=[],
                    help="source TU(s) to read annotations from.")
    ap.add_argument("--merge", nargs="+",
                    help="merge mode: combine these per-TU fragment CSVs into "
                         "--out (cross-TU dup guard + DATA dedup); no --tu needed. "
                         "Also merges --merge-functions/--merge-globals JSON "
                         "fragments into --functions-out/--globals-out.")
    ap.add_argument("--merge-functions", nargs="*", default=[],
                    help="per-TU functions.json fragments to merge (merge mode).")
    ap.add_argument("--merge-globals", nargs="*", default=[],
                    help="per-TU globals.json fragments to merge (merge mode).")
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
    ap.add_argument("--functions-out",
                    default=str(REPO / "build/gen/functions.json"),
                    help="per-RVA function signatures (class, return, cc, named "
                         "params) for apply.py's Ghidra prototype enrichment.")
    ap.add_argument("--undname", default="llvm-undname",
                    help="MSVC name demangler for the authoritative signature.")
    ap.add_argument("--globals-out",
                    default=str(REPO / "build/gen/globals.json"),
                    help="per-RVA global/static declared types for apply.py.")
    args = ap.parse_args()

    if args.merge:
        return merge_fragments(
            args.merge, Path(args.out),
            functions_frags=args.merge_functions, functions_out=args.functions_out,
            globals_frags=args.merge_globals, globals_out=args.globals_out)
    if not args.tu:
        ap.error("either --tu (emit) or --merge (combine fragments) is required")

    unit_map = {}
    if not args.unit and Path(args.units_toml).exists():
        unit_map = units_from_toml(args.units_toml)

    compdb = load_compdb(args.compdb) if args.compdb else {}
    label_config = load_label_config(LABEL_CONFIG)
    vtable_names = load_vtable_names(VTABLE_NAMES)

    rows = []          # (rva, name, unit, size, kind)
    misses = []        # (rva, candidate, unit, reason)
    addr_sites = {}    # rva -> [(tu, "fn")] for every function rva, to catch dups
    func_meta = {}     # rva -> {ir_sym, names} for functions.json signatures
    global_meta = {}   # rva -> {name, type, unit} for globals.json (typed data)
    no_ir = []         # TUs whose label pass produced NO IR at all      -> FATAL
    no_rows = []       # TUs that carry rva.h macros but labelled nothing -> FATAL
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
        # FATAL, never `continue`. The label pass is CLANG, not the wine cl that builds
        # the objs, so a TU can compile perfectly under cl and still yield NO IR here -
        # e.g. afxwin*.inl's implicit-int CMenu::operator== which clang rejects and cl
        # accepts (the #undef _AFX_ENABLE_INLINES guard). This used to `continue`, so
        # the TU contributed ZERO labels and every function in it silently vanished from
        # symbol_names.csv while the build still reported SUCCESS. That is the worst kind
        # of bug we can have: every metric we trust (exact count, reloc_fidelity,
        # assert_relocs, link_defects) reads symbol_names.csv, so a silent label drop
        # makes the numbers LOOK BETTER while the tree gets worse (8 units / 211 fns went
        # missing before anyone noticed the denominator move). A TU that compiles MUST
        # contribute; if it cannot, the build stops and says which one.
        ir = emit_ir(args.clang, tu, args.flag, cl_flags)
        if ir is None:
            no_ir.append(tu)
            continue
        rows_before = len(rows)
        # AST: source-only signal - parameter NAMES (for functions.json) and the
        # DATA extern join. One dump, reused by both. Keyed by clang's mangledName,
        # which equals the IR-paired symbol (`ir_sym`), so the join is by rva below.
        ast = clang_ast(args.clang, tu, args.flag, cl_flags)
        ast_param_names = param_names_from_ast(ast, tu) if ast is not None else {}
        for rva, ir_sym, size, override in func_labels_from_ir(ir):
            addr_sites.setdefault(rva, []).append((tu, ir_sym))
            # The IR pairs the annotation with the function's own mangled symbol;
            # an explicit SYMBOL override wins over it when present.
            name = override or ir_sym
            # functions.json signature is derived from the clang mangledName
            # (ir_sym): undname gives the type/cc/class, AST the parameter names.
            func_meta[rva] = {"ir_sym": ir_sym,
                              "names": ast_param_names.get(ir_sym)}
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
            # No source body -> no AST param names; undname still gives the
            # (typed, unnamed) signature for functions.json.
            func_meta[rva] = {"ir_sym": sym, "names": None}
            if obj_syms is None or sym in obj_syms:
                rows.append((rva, sym, unit, size, "func"))
            else:
                misses.append((rva, sym, unit, "@rva-symbol not in base obj"))

        # --- standalone `// @data-symbol:` data (a `??_7` vtable / `??_R*` RTTI
        # datum cl emits for a real polymorphic class; name is deterministic from
        # the class). Authority-checked against the base obj's DATA symbols. ---
        for m in DATA_SYMBOL_RE.finditer(text):
            sym, rva_s, size_s = m.group(1), m.group(2), m.group(3)
            rva = int(rva_s, 16)
            size = (int(size_s, 16) if size_s and size_s.lower().startswith("0x")
                    else int(size_s) if size_s else None)
            sym = resolve_pool_id(sym, all_syms)  # `...$S*` -> the emitted `...$S<n>`
            if all_syms is None or sym in all_syms:
                rows.append((rva, sym, unit, size, "data"))
            else:
                misses.append((rva, sym, unit, "@data-symbol not in base obj"))

        # --- AUTO deterministic vtable names: name ??_7<Class>@@6B@ on the target
        # whenever THIS TU's base obj emits it (the class is real-polymorphic in
        # src). RVA/size from the generated map; no src annotation needed, and
        # nothing is emitted for unconverted classes (no base ??_7 -> no row). ---
        if all_syms:
            for vsym, (vrva, vsize) in vtable_names.items():
                if vsym in all_syms:
                    rows.append((vrva, vsym, unit, vsize, "data"))

        # --- DATA via AST (IR drops the extern's annotation) ---
        if ast is not None and DATA_MACRO_RE.search(text):
            for rva, cand, qtype, dqtype in data_labels(text, ast, tu):
                if cand is None:
                    misses.append((rva, None, unit, "no VarDecl below DATA()"))
                    continue
                # The `globals` unit is trusted (see GLOBALS_UNIT): its externs are
                # never referenced in its own base obj, so bypass the authority
                # check (the names came pre-checked from the matched TUs).
                if obj_syms is None or cand in all_syms or unit == GLOBALS_UNIT:
                    # The declared type is the ONLY authority for a global's extent
                    # (no CodeView length, no retail symbol sizes) - see
                    # sizeof_qualtype. Unprovable types stay None => empty column =>
                    # the consumer falls back to the next-symbol gap, as before.
                    dsize = sizeof_qualtype(qtype, desugared=dqtype)
                    rows.append((rva, cand, unit, dsize, "data"))
                    # remember the declared type so apply.py can type the global
                    global_meta[rva] = {"name": cand, "type": qtype, "unit": unit,
                                        "size": dsize}
                else:
                    misses.append((rva, cand, unit,
                                   "data candidate not in base obj"))

        # A TU that carries rva.h macros MUST label something. If it labelled nothing,
        # the label pass silently dropped it (clang parsed the source but the IR/AST
        # join produced no rows) - the same class of silent hole as `no_ir` above.
        if len(rows) == rows_before:
            no_rows.append(tu)

    # ------------------------------------------------------------------
    # BUILD-INTEGRITY GATE (fatal): a TU that compiles must CONTRIBUTE.
    #
    # The label pass runs clang, the objs are built by wine cl. When clang chokes on
    # something cl accepts, the TU used to be skipped in silence: it still compiled, the
    # build still said SUCCESS, and every one of its functions just... was not in
    # symbol_names.csv. Because every metric we report reads that file, the tree can get
    # WORSE while the numbers get BETTER (a smaller denominator). 8 units / 211 functions
    # went missing exactly this way and it was caught only by someone noticing the
    # denominator move. A green build must MEAN the labels landed - so this is fatal, and
    # it names the units.
    # ------------------------------------------------------------------
    if no_ir or no_rows:
        for tu in no_ir:
            log(f"ERROR {tu}: label pass produced NO IR - the TU compiles under cl but "
                f"clang rejected it, so ALL of its functions would silently vanish from "
                f"{args.out}. (Causes seen: an ORPHAN `DATA(0x..)` with no declaration "
                f"under it - see the orphan-annotation gate below, which usually fires "
                f"first; or an MFC header inline clang rejects - the "
                f"`#undef _AFX_ENABLE_INLINES` guard in docs/build-system.md.)")
        for tu in no_rows:
            log(f"ERROR {tu}: carries rva.h macros but labelled NOTHING - its functions "
                f"would silently vanish from {args.out}.")
        log(f"{len(no_ir) + len(no_rows)} TU(s) contributed no labels; refusing to write "
            f"{args.out}. A TU that compiles MUST contribute.")
        return 1

    # ------------------------------------------------------------------
    # ORPHAN-ANNOTATION GATE. A `DATA(0x..)` / `RVA(..)` line with NO declaration under it
    # is not a no-op: DATA() is a clang annotate attribute, so clang attaches it to
    # whatever declaration comes NEXT and binds THAT name to the orphan's rva. cl expands
    # the macro to nothing, so the unit still compiles and nobody notices.
    #
    # This shipped silently: deleting a declaration but leaving its DATA() line above put
    # THREE rows in symbol_names.csv for ?g_faderHalf@@3MB - its real 0x1f0828 plus
    # 0x2c4490 and 0x2c456c, the OffsetRect and PtInRect IAT slots whose decls had been
    # removed. The reference is reloc-masked, so no metric moved. Thirteen of these were
    # found tree-wide, two of them pre-dating the change that exposed the pattern.
    # (When the orphan lands inside an `extern "C" { }` block it instead leaves an EMPTY
    # block and clang dies with "extraneous closing brace" - that path is the no-IR error
    # above, which is how this was finally caught.)
    # ------------------------------------------------------------------
    orphans = []
    # The invariant, stated once: BETWEEN TWO ANNOTATION LINES THERE MUST BE A DECLARATION.
    # An annotation annotates exactly one declaration, so if the next thing after it (past
    # blanks and comments) is EOF or ANOTHER annotation, the first one is dangling.
    #
    # The blank-line-only check this started as was too weak, and the weakness was live: it
    # missed STACKED orphans, where a removed declaration leaves its DATA() piled on top of
    # the next one's -
    #
    #     DATA(0x002c44a4)      <- decl deleted; dangling
    #     DATA(0x002c44f0)      <- decl deleted; dangling
    #     DATA(0x002c4520)      <- decl deleted; dangling
    #     DATA(0x002c44d8)      <- decl deleted; dangling
    #     DATA(0x0021243c)
    #     char s_UsingCmdDelay[] = "...";
    #
    # - which bound s_UsingCmdDelay to FIVE rvas, four of them import-table slots. Found by
    # auditing symbol_names.csv for one name mapped to several rvas (the g_faderHalf
    # signature); that audit is what this rule now enforces at the source.
    _ANN = re.compile(r"\s*(?:DATA|RVA)\([^)]*\)\s*$")
    for tu in args.tu:
        try:
            lines = Path(tu).read_text(encoding="latin-1").split("\n")
        except OSError:
            continue
        for i, ln in enumerate(lines):
            if not _ANN.match(ln):
                continue
            j = i + 1
            while j < len(lines) and (not lines[j].strip()
                                      or lines[j].lstrip().startswith("//")):
                j += 1
            if j >= len(lines) or _ANN.match(lines[j]):
                orphans.append((tu, i + 1, ln.strip()))
    if orphans:
        for tu, n, txt in orphans:
            log(f"ERROR {tu}:{n}: ORPHAN `{txt}` - no declaration under it. clang will "
                f"attach this annotation to the NEXT declaration and bind that symbol to "
                f"the wrong rva. Delete it, or put its declaration back.")
        log(f"{len(orphans)} orphan annotation(s); refusing to write {args.out}.")
        return 1

    # Finalize + write the CSV via the shared helper (cross-TU dup-RVA guard,
    # DATA keep-last-per-rva dedup, sort, write-if-changed). It sorts `rows` in
    # place but writes its own deduped copy, so `rows` below still holds every row.
    rc = write_symbol_names(rows, addr_sites, Path(args.out), misses)
    if rc != 0:
        return rc

    # --- functions.json: per-RVA signatures for apply.py's Ghidra prototypes ---
    # Join the func rows (authority-checked) with their clang mangledName
    # (func_meta) and demangle once for the authoritative signature; overlay the
    # source AST's parameter names. Vendored/config funcs carry no source
    # signature and are omitted.
    func_rows = [r for r in rows if r[4] == "func" and r[0] in func_meta]
    dem = undname_map([func_meta[r[0]]["ir_sym"] for r in func_rows], args.undname)
    functions = []
    n_named_params = 0
    for rva, name, unit, size, kind in func_rows:
        meta = func_meta[rva]
        sig = parse_demangled(dem.get(meta["ir_sym"], ""))
        if sig is None:
            continue
        names = meta["names"]
        params = []
        for j, ptype in enumerate(sig["param_types"]):
            pname = names[j] if names and j < len(names) else None
            if pname:
                n_named_params += 1
            params.append({"type": ptype, "name": pname})
        functions.append({
            "rva": f"0x{rva:06x}", "name": sig["qual"], "class": sig["cls"],
            "kind": sig["kind"], "ret": sig["ret"], "cc": sig["cc"],
            "params": params, "unit": unit})
    functions.sort(key=lambda d: d["rva"])
    _write_json_if_changed(functions, args.functions_out,
                           f"function signature(s) ({n_named_params} named params)")

    # --- globals.json: the declared C/C++ TYPE of each named global (data row),
    # so apply.py types it in Ghidra (it already names it from symbol_names.csv).
    # write_symbol_names no longer mutates `rows` to dedup data, so dedup here the
    # same way (keep last per rva) to match the rows written to the CSV.
    last_data = {}
    for row in rows:                      # rows is sorted in place by write_symbol_names
        if row[4] == "data":
            last_data[row[0]] = row       # keep last per rva = matches the written CSV
    globals_out = []
    for rva, name, unit, size, kind in sorted(last_data.values()):
        gm = global_meta.get(rva) or {}
        row = {"rva": f"0x{rva:06x}", "name": name,
               "type": gm.get("type") or "", "unit": unit}
        # The type-derived exact extent (sizeof_qualtype); omitted when unprovable
        # so a consumer can tell "no evidence" from a real size.
        if size:
            row["size"] = f"0x{size:x}"
        globals_out.append(row)
    globals_out.sort(key=lambda d: d["rva"])
    n_typed = sum(1 for g in globals_out if g["type"])
    n_sized = sum(1 for g in globals_out if g.get("size"))
    _write_json_if_changed(globals_out, args.globals_out,
                           f"global(s) ({n_typed} typed, {n_sized} sized)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
