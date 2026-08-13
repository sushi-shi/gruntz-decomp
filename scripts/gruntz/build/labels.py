#!/usr/bin/env python3
"""labels.py - derive the rva -> mangled-name, unit label map from src/.

Replaces the hand-maintained config/symbol_names.csv. The only manual input is
the address annotation on each matched function/global; everything else is
derived. See docs/build-system.md.

ANNOTATIONS (include/rva.h macros, compiled out under MSVC):

    RVA(0x13dc70, 0x1d)        // a matched FUNCTION: rva + (optional) byte size
    DATA(0x253c70)             // the DATA symbol a matched `extern` global uses

How the map is built per TU:

  1. FUNCTIONS come from LLVM IR. `clang ... -S -emit-llvm` emits
     `@llvm.global.annotations`, which pairs the MANGLED SYMBOL of each annotated
     function DIRECTLY with its annotation string (e.g. "rva:0x13dc70 size:0x1d").
     There is NO positional "nearest definition below the comment" join, so an
     inline header definition can no longer steal a nearby address (the old, and
     fragile, `// @address:` comment path - now gone).

  2. DATA comes from the clang AST. An `extern` declaration is not a definition,
     so clang DROPS its annotation from IR (measured: `used` does not rescue it,
     even when the global is referenced - see include/rva.h). So DATA addresses are
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
     makes it safe. (The old SYMBOL() name-override escape hatch is RETIRED: a
     clang-vs-VC5 mangling mismatch is a modeling bug to fix in source.)

  4. unit comes from config/units.toml via the source path.

A compiler-generated thunk with no source body (a `??_G` scalar-deleting dtor)
cannot hang an RVA() attribute, so it is pinned with a self-contained
`RVA_COMPGEN(<rva>, <size>, <mangled>)` invocation (text-scanned - the name is
given verbatim, so no join and no IR). Every datum, likewise, is a real C++
definition carrying `DATA(<rva>)`, whose name comes from the AST and is
authority-checked against the base obj (`msvc5_data_symbol` resolves the
clang-vs-VC5 spellings, including cl's `$S<n>` file-static decoration).

The DATA analog of RVA_COMPGEN is a MANIFEST, not a macro:
config/retail/compiler-generated-data.tsv. It reaches the data whose real C++
definition sits in a HEADER - a function-local static inside a header inline, and
the `??_B` dynamic-init guard byte cl emits beside it, which has no source
spelling at ALL. DATA() cannot reach either (collect_vars is main-file-only) and
neither can DATA_COMPGEN (it wraps a value expression at a use site). It is a
manifest and not a macro because these data have no owning TU: cl emits each as a
COFF COMMON into every TU that instantiates the inline and the linker merges them,
so any source position would fabricate an owner. Each row is re-proven against
every base obj's COMMON table at every build (see compgen_data_tu), which is what
keeps it from being the retired declaration-only DATA_SYMBOL in a new coat.

VENDORED PATH: vendored C TUs (vendor/zlib-1.0.4/*.c) keep their source PRISTINE -
no labels in the source at all. They are mostly `static`/`local` K&R functions
that clang DROPS from IR when unused, so neither attributes nor a source join can
carry their labels. Instead their rva->symbol map lives in config/retail/zlib_labels.csv
(a static table - the retail binary never changes - generated once) and is emitted
directly, authority-checked against the base obj: no source parsing, no positional
join. A TU is routed to the config path iff it carries NO include/rva.h macro.

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
from pathlib import Path

# The clang MSVC-compat flag set + the IR/compdb helpers live in gruntz.core.ir
# (shared with gruntz.cleanliness.caller_callee).
from gruntz.core.ir import INC_CL, INC_GCC, MS_FLAGS, emit_ir, load_compdb  # noqa: F401
from gruntz.core import vtable_catalog

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

# The single consolidated-globals unit (src/Globals.cpp). Its DATA() rows are
# TRUSTED: the base obj is all unused externs (no symbols), so the authority
# check cannot confirm them - but each name was authority-checked in the matched
# TU it came from before `consolidate_globals` (now scripts/archive/) moved it here.
GLOBALS_UNIT = "globals"

# DATA(0x...) macro invocation - scanned from source text (IR drops extern
# annotations). The address is bound to the AST VarDecl below it.
DATA_MACRO_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
# `RVA_COMPGEN(<rva>, <size>, <mangled>)` - a self-contained function label for a
# compiler-generated body that has NO source definition to hang an RVA() attribute
# on (a deterministic `??_G` scalar-deleting destructor or `??_D` vbase dtor).
# The mangled name is given verbatim, so there is no join and no IR (rva.h; expands
# to nothing under both compilers, read here from source text). size 0 = unknown.
# Keep invocations in RVA order among the TU's RVA() functions.
RVA_COMPGEN_RE = re.compile(
    r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*([^\s,)]+)\s*\)")
# `$E<n>` (x86 `_$E<n>`) dynamic-init/EH helpers have a volatile emission
# ordinal, not a semantic source identity. Their pins live at the OWNER's
# definition as RVA_DYNINIT (gruntz.core.dyninit); the ordinal itself must
# never become a source label.
VOLATILE_ORDINAL_FN_RE = re.compile(r"^_?\$E[0-9]+$")

# Annotation strings carried in @llvm.global.annotations (emitted by include/rva.h).
# shape as RVA(), different storage class, so it must not count toward the
# owning compiland's .text extent. It rides the same annotation and reaches
ANN_RVA_RE = re.compile(r"^rva:(0x[0-9a-fA-F]+)(?:\s+size:(0x[0-9a-fA-F]+|\d+))?$")
# Macro annotation markers (presence of any -> a migrated TU; functions come
# from IR for these). A TU with none is a vendored C TU (config-table path).
# A TU "carries labels" if it invokes ANY rva.h label macro - a pin-only TU (e.g.
# an explicit template-instantiation host whose every function is a
# compiler-emitted COMDAT, like ArraySerialize.cpp's CArray<PLAYLISTINFOSTRUCT*>)
# carries only RVA_COMPGEN rows, and without that alternative it silently fell
# through to the vendored-C path and contributed ZERO rows.
MACRO_RE = re.compile(r"\b(?:RVA|DATA|RVA_COMPGEN|DATA_COMPGEN)\s*\(")

# Static rva->symbol table for vendored C TUs whose source carries no labels.
LABEL_CONFIG = REPO / "config/retail/zlib_labels.csv"

# The compiler-generated DATA pins - the DATA analog of RVA_COMPGEN, but a manifest
# rather than a source macro because these data have no owning TU: cl emits each one
# as a COFF COMMON into EVERY TU that instantiates the header inline, and the linker
# merges them into one bss slot. See the file's own header for the full rationale
# (and why this is not the retired DATA_SYMBOL). Gated by gruntz.audit.compgen_data.
COMPGEN_DATA = REPO / "config/retail/compiler-generated-data.tsv"


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


def func_labels_from_ir(ir):
    """[(rva, mangled, size_or_None)] keyed by the symbol the annotation is
    attached to: the symbol the IR pairs the annotation with IS the function's
    mangled name (no positional join, no name overrides - the retired SYMBOL()
    escape hatch is gone; a name mismatch is a modeling bug to fix in source)."""
    rows = {}            # mangled symbol -> {"rva":..,"size":..}
    for sym, ann in parse_ir_annotations(ir):
        ma = ANN_RVA_RE.match(ann)
        if not ma:
            continue
        size = None
        if ma.group(2):
            s = ma.group(2)
            size = int(s, 16) if s.lower().startswith("0x") else int(s)
        rows.setdefault(sym, {})["rva"] = int(ma.group(1), 16)
        rows[sym].setdefault("size", None)
        if size is not None:
            rows[sym]["size"] = size
    return [(d["rva"], sym, d.get("size")) for sym, d in rows.items()]


def loc_file(loc):
    """The file clang's JSON printer last NAMED for this location, or None.

    clang omits `file` whenever it equals the previously PRINTED one, so the
    reader has to track it exactly the way the printer does. A location inside a
    MACRO EXPANSION has no top-level `file` at all - it carries `spellingLoc`
    (where the macro is written, a header) and `expansionLoc` (where it was USED,
    the .cpp) - and the printer emits the expansion last, so the expansion file is
    what every following node's omitted `file` refers to.

    Reading only the top-level key was silent data loss: put ANY macro-expanded
    declaration above a TU's globals - e.g. a `GZ_ENUM_CONST_BEGIN(...)` bag,
    which expands to a bare `enum {` - and the main-file transition arrives inside
    an `expansionLoc`, the in-main flag never flips, and the whole TU's DATA()
    globals and function parameter names vanish. Measured on
    src/Gruntz/BattlezMapConfig.cpp: 6 globals and 61 param names dropped, the
    six `?g_*@@3..` rows fell out of symbol_names.csv and the delink data
    manifest, and the only symptom was six `MISS ... no VarDecl below DATA()`
    lines that do not fail a build.
    """
    if not isinstance(loc, dict):
        return None
    for key in ("file", "expansionLoc", "spellingLoc"):
        v = loc.get(key)
        if key == "file":
            if v is not None:
                return v
        elif isinstance(v, dict) and v.get("file") is not None:
            return v["file"]
    return None


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
            f = loc_file(loc)
            if f is not None:
                state["in_main"] = os.path.realpath(f) == main_real

    def visit(node):
        if isinstance(node, dict):
            update_file(node)
            # `gruntz_clsmeta_*` are VTBL_ABSENT class-metadata carriers
            # (include/rva.h): file-scope `used` statics that DO carry a
            # mangledName. Skip them so a carrier written between a DATA(...) and
            # its extern can never steal the DATA binding (data_labels picks the
            # first VarDecl below the macro).
            if (state["in_main"] and node.get("kind") == "VarDecl"
                    and "mangledName" in node and not node.get("isImplicit")
                    and not (node.get("name") or "").startswith("gruntz_clsmeta_")):
                loc = node.get("loc") or {}
                off = loc.get("offset")
                if off is not None:
                    qt = (node.get("type") or {}).get("qualType") or ""
                    out.append((node["mangledName"], off, qt))
            for c in node.get("inner", []) or []:
                visit(c)
    visit(ast)
    return out


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


def clang_var_sizes(tu, flags, cl_flags=None):
    """{mangled VarDecl name: exact byte extent} from the current TU.

    This is the DATA extent authority.  libclang lays each declaration out under
    the TU's real i386/MSVC flags, so a record, typedef, array, or enum is sized
    directly from the source being labelled.  Do not route this through the
    whole-tree ``structs.json`` audit cache: that snapshot may be older than this
    per-TU label edge.

    Return None when libclang could not parse the TU cleanly.  An incomplete type
    has a negative ``get_size()`` and is deliberately omitted from the result.
    """
    try:
        import clang.cindex as cidx
    except ImportError as exc:
        log(f"ERROR {tu}: pylibclang unavailable for DATA extents: {exc}")
        return None

    if cl_flags is not None:
        parse_args = ["--driver-mode=cl", "/DGRUNTZ_EMIT_META",
                      *cl_flags, *INC_CL]
    else:
        parse_args = ["-DGRUNTZ_EMIT_META", *MS_FLAGS, *flags, *INC_GCC]
    try:
        parsed = cidx.Index.create().parse(tu, args=parse_args)
    except cidx.LibclangError as exc:
        log(f"ERROR {tu}: pylibclang parse failed for DATA extents: {exc}")
        return None
    errors = [d for d in parsed.diagnostics if d.severity >= cidx.Diagnostic.Error]
    if errors:
        log(f"ERROR {tu}: pylibclang reported DATA-extent parse errors:\n" +
            "\n".join(str(d) for d in errors[:10]))
        return None

    main_real = os.path.realpath(tu)
    sizes = {}
    conflicts = set()
    for cursor in parsed.cursor.walk_preorder():
        if cursor.kind != cidx.CursorKind.VAR_DECL or cursor.location.file is None:
            continue
        if os.path.realpath(cursor.location.file.name) != main_real:
            continue
        name = cursor.mangled_name
        size = cursor.type.get_size()
        if not name or size < 0:
            continue
        if name in sizes and sizes[name] != size:
            conflicts.add(name)
        else:
            sizes[name] = size
    for name in conflicts:
        sizes.pop(name, None)
    return sizes


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


DATACOMPGEN_RE = re.compile(r"\bDATA_COMPGEN\s*\(")
COMPGEN_ADDR_RE = re.compile(r"0x[0-9a-f]{8}$")
_STR_SEG_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')
_FLOAT_RE = re.compile(r"[-+]?(?:\d+\.\d*|\.\d+|\d+[eE][-+]?\d+|\d+\.?\d*[eE][-+]?\d+)"
                       r"([fF]?)$")


def compgen_invocations(text):
    """[(line, [arg, ...])] for each DATA_COMPGEN(...) in comment-blanked TU text.

    The macro sits in EXPRESSION position, so unlike the statement labels it may
    be wrapped by clang-format - the scan is balanced-paren and quote-aware, not
    line-based. Args are split on top-level commas.
    """
    out = []
    for m in DATACOMPGEN_RE.finditer(text):
        i, n = m.end(), len(text)
        depth, j = 1, m.end()
        while j < n and depth:
            c = text[j]
            if c in "\"'":
                q = c
                j += 1
                while j < n and text[j] != q:
                    j += 2 if text[j] == "\\" else 1
            elif c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            j += 1
        body = text[i:j - 1]
        parts, depth, start, k = [], 0, 0, 0
        while k < len(body):
            c = body[k]
            if c in "\"'":
                q = c
                k += 1
                while k < len(body) and body[k] != q:
                    k += 2 if body[k] == "\\" else 1
            elif c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            elif c == "," and depth == 0:
                parts.append(body[start:k])
                start = k + 1
            k += 1
        parts.append(body[start:])
        out.append((text.count("\n", 0, m.start()) + 1,
                    [p.strip() for p in parts]))
    return out


def compgen_value(value_src):
    """('str'|'f32'|'f64', payload bytes) or (None, reason).

    The value spelling is the allocation's type (homm2 contract): `0.0` is an
    f64 pool entry, `0.0f` an f32, a (concatenation of) narrow string literal(s)
    the pooled `??_C@` payload. Anything else - identifiers, wide strings,
    integers (immediates live in code, not data) - is rejected.
    """
    import struct as _struct
    v = value_src.strip()
    if v.startswith('"'):
        segs = _STR_SEG_RE.findall(v)
        if not segs or _STR_SEG_RE.sub("", v).strip():
            return None, "value must be pure narrow string literal(s)"
        try:
            payload = ("".join(segs).encode("latin-1")
                       .decode("unicode_escape").encode("latin-1"))
        except (UnicodeDecodeError, UnicodeEncodeError):
            return None, "unsupported escape in string literal"
        return "str", payload
    m = _FLOAT_RE.fullmatch(v)
    if m:
        num = v[:-1] if m.group(1) else v
        try:
            val = float(num)
        except ValueError:
            return None, "unparsable float constant"
        if m.group(1):
            return "f32", _struct.pack("<f", val)
        return "f64", _struct.pack("<d", val)
    return None, "value is neither a string literal nor a float constant"


def compgen_tu(text, tu, unit, obj_path):
    """[(rva, name, unit, size, vkind, payload)] claims + [errors].

    Every claim is authority-checked against the TU's base obj - the exact
    artifact cl emitted from this source:
      * a string value must equal a `??_C@` COMDAT payload there (cl's own
        spelling for those bytes IS the emitted name - coff_oracle doctrine);
      * a float value's bits must occur in the obj's section bytes (the TU's
        `$T<n>` FP pool); the emitted `$T<rva>` spelling only has to satisfy
        canonicalize's VOLATILE_T so both sides content-address identically.
    """
    from gruntz.build.coff_oracle import _Coff

    claims, errors = [], []
    coff = None
    if obj_path is not None:
        try:
            coff = _Coff(Path(obj_path))
        except Exception as e:
            errors.append((tu, 0, "base obj unreadable for DATA_COMPGEN: %s" % e))
            return claims, errors
    strings = {}
    if coff is not None:
        for idx, value, secnum in coff.iter_symbols():
            nm = coff.sym_name(idx)
            if nm.startswith("??_C@") and secnum >= 1:
                cs = coff.cstring(secnum, value)
                if cs is not None:
                    strings[cs] = nm
    seen_rva = {}
    for line, args_ in compgen_invocations(text):
        where = (tu, line)
        if len(args_) != 2:
            errors.append((*where, "DATA_COMPGEN takes (addr, value); got %d arg(s)"
                           % len(args_)))
            continue
        addr_s, value_src = args_
        if not COMPGEN_ADDR_RE.fullmatch(addr_s):
            errors.append((*where, "address %r not canonical 0x%%08x form" % addr_s))
            continue
        vkind, payload = compgen_value(value_src)
        if vkind is None:
            errors.append((*where, "%s: %s" % (addr_s, payload)))
            continue
        rva = int(addr_s, 16)
        if rva in seen_rva:
            if seen_rva[rva] != (vkind, payload):
                errors.append((*where, "0x%08x claimed twice in this TU with "
                               "different values" % rva))
            continue                    # repeated expansions coalesce
        seen_rva[rva] = (vkind, payload)
        if vkind == "str":
            name = strings.get(payload)
            if coff is not None and name is None:
                errors.append((*where, "%s: string %r is not a pooled ??_C@ literal "
                               "in this TU's base obj" % (addr_s, payload[:40])))
                continue
            if name is None:            # no obj (candidate mode): cannot derive
                continue
            size = len(payload) + 1
        else:
            found = any(payload in coff.buf[rawptr:rawptr + rawsize]
                        for rawptr, rawsize in coff.sections
                        if rawsize) if coff is not None else False
            if coff is not None and not found:
                errors.append((*where, "%s: %s bits %s not present in this TU's "
                               "base obj (FP pool)" % (addr_s, vkind, payload.hex())))
                continue
            if coff is None:
                continue
            name = "$T%d" % rva
            size = len(payload)
        claims.append((rva, name, unit, size, vkind, payload))
    return claims, errors


def data_labels(text, ast, main_file):
    """[(rva, mangledName, qualType)] for each DATA(0x..) macro bound to the AST
    VarDecl just below it (by line). The variable pool and macro sites never cross
    with functions, so a non-matched global cannot steal a function's address.
    """
    off2line = line_index(text)
    var_defs = sorted((off2line(off), mn, qt)
                      for (mn, off, qt) in collect_vars(ast, main_file))
    out = []
    line_no = 1
    for m in re.finditer(r"[^\n]*\n", blank_comments(text)):
        seg = m.group(0)
        dm = DATA_MACRO_RE.search(seg)
        if dm:
            rva = int(dm.group(1), 16)
            cand = next(((mn, qt) for (dl, mn, qt) in var_defs if dl >= line_no),
                        (None, None))
            out.append((rva, cand[0], cand[1]))
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
            f = loc_file(loc)
            if f is not None:
                state["in_main"] = os.path.realpath(f) == main_real

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
    (config/retail/zlib_labels.csv): the labels for vendored C TUs whose source carries
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


def load_compgen_data(path=None):
    """[(rva, size, symbol, emitter)] from config/retail/compiler-generated-data.tsv.

    The compiler-generated DATA pins: a datum cl.exe emits from a definition that
    already exists in the tree, but that neither source-side data device can reach -
    a function-local static inside a HEADER inline (DATA() is main-file-only) and its
    dynamic-init guard byte (no source spelling at all; cl names it `??_B...@<n>`).
    Only the retail ADDRESS is stated here; the symbol and its extent are re-proven
    against the base objs at every build, so a stale or invented row binds nothing.
    """
    out = []
    p = Path(path or COMPGEN_DATA)
    if not p.exists():
        return out
    for ln in p.read_text().splitlines():
        if not ln.strip() or ln.lstrip().startswith("#"):
            continue
        parts = ln.split("\t")
        if len(parts) < 3:
            continue
        rva_s, size_s, sym = parts[0], parts[1], parts[2]
        emitter = parts[3] if len(parts) > 3 else ""
        out.append((int(rva_s, 16), int(size_s, 16), sym, emitter))
    return out


def compgen_data_tu(unit, pins, common_syms, rows, misses):
    """Emit a symbol_names row for each pinned datum THIS TU's base obj emits.

    Authority is the COMMON symbol table of the obj cl.exe just produced: the name
    must be there and the size cl computed must equal the pinned extent. A COMMON is
    a tentative definition, so every emitting TU is an owner (exactly like a folded
    COMDAT); write_symbol_names collapses the copies to one row per rva.
    """
    if common_syms is None:
        return
    for rva, size, sym, _emitter in pins:
        got = common_syms.get(sym)
        if got is None:
            continue
        if got != size:
            misses.append((rva, sym, unit,
                           "compiler-generated data pin says 0x%x but cl emitted "
                           "COMMON size 0x%x" % (size, got)))
            continue
        rows.append((rva, sym, unit, size, "data"))


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


def nm_common_symbols(obj, nm="llvm-nm"):
    """{name: size} for the obj's COFF COMMON symbols (llvm-nm type `C`).

    A COMMON is a TENTATIVE DEFINITION - section 0 with a non-zero Value that holds
    the size - so the linker allocates it and merges every identically-named copy.
    It is neither a plain reference nor a sectioned definition, which is why the
    other two helpers here both mis-classify it: nm_symbols wants T/t/W/w, and
    nm_all_symbols cannot tell it apart from an undefined external.

    cl emits one of these per TU for a function-local static whose enclosing
    function has external linkage - i.e. every `static T x = <init>;` inside a
    header inline - plus its `??_B` dynamic-init guard byte.
    """
    res = subprocess.run([nm, obj], capture_output=True, text=True)
    out = {}
    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[-2] == "C":
            try:
                out[parts[-1]] = int(parts[0], 16)
            except ValueError:
                continue
    return out


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


def ms_c_symbol(candidate, obj_syms):
    """MS x86 C-linkage decoration of a bare `extern "C"` name.

    clang names an `extern "C"` function's IR GlobalValue by its PLAIN source
    identifier (`RezAssertFail`, `WinMain`) - the leading `_` (cdecl) or `_..@<n>`
    (stdcall) decoration the x86 object file carries is applied by the target's
    data-layout at emission, NOT in the GlobalValue name the annotation references.
    The CL base obj (and retail) carry the decorated symbol, so `@llvm...annotations`
    hands us `RezAssertFail` while the obj has `_RezAssertFail`.

    Resolve the bare IR name to the decorated obj symbol so an `extern "C"` function
    needs no name override. Only fires for a bare identifier (no `?` C++ mangling,
    no already-`_`/`@`-decorated form). Returns the obj symbol or None."""
    if not candidate or candidate[0] in "?_@.$" or not obj_syms:
        return None
    cand = "_" + candidate
    if cand in obj_syms:                              # cdecl: `_name`
        return cand
    pat = re.compile(r"^_" + re.escape(candidate) + r"@\d+$")  # stdcall: `_name@N`
    hits = [s for s in obj_syms if pat.match(s)]
    return hits[0] if len(hits) == 1 else None


# clang mangles an ARRAY variable's storage class `Q`, VC5 spells the same thing
# `P`: `?g_guid1@@3QBEB` (clang) == `?g_guid1@@3PBEB` (cl). The digit is the
# access/storage code (`3` = a file-scope variable, `0` = a private static member,
# which is the MFC `Class::_messageEntries[]` message-map case).
CLANG_Q_ARRAY_RE = re.compile(r"@@([0-9])Q")
# cl decorates every INTERNAL-LINKAGE file-scope variable (`static T x;` and, in
# C++, a file-scope `const T x;`) `_x$S<n>` with a per-object CodeView ordinal;
# clang's mangler reports the plain `_x`.
POOL_ID_RE_TMPL = r"^%s\$S[0-9]+$"
# A FUNCTION-LOCAL static mangles `?<var>@?<scope>??<enclosing-fn>@<storage><type>`.
# clang always numbers the scope `?1`; VC5 counts the blocks it has already left,
# so the same variable can be `?4` (GetAmbientId, one `if` above it) - see
# docs/patterns/function-local-static-dynamic-init-guard.md.
LOCAL_STATIC_SCOPE_RE = re.compile(r"^(\?[^@]+@)\?[0-9]+(\?\?.+)$")


def msvc5_data_symbol(candidate, obj_syms):
    """Resolve the known clang-vs-VC5 static-data NAME differences.

    Three mechanical spellings, all AUTHORITY-CHECKED: a rewrite is returned only
    when that exact symbol is present in the compiled object, so nothing is ever
    accepted speculatively.

      1. array storage class `Q` (clang) vs `P` (VC5) - `?s_rb@@3QBDB` ->
         `?s_rb@@3PBDB`, `?g_guid1@@3QBEB` -> `?g_guid1@@3PBEB`, and the MFC
         message-map `@@0QBUAFX_MSGMAP_ENTRY@@B` -> `@@0PBUAFX_MSGMAP_ENTRY@@B`.
      2. internal-linkage file-scope variables, which cl decorates with a
         volatile `$S<n>` CodeView ordinal - `_s_fmtNotFound` ->
         `_s_fmtNotFound$S19047`. Matched by PREFIX (the ordinal renumbers on any
         symbol churn in the TU) and accepted only when exactly one symbol
         matches.
      3. FUNCTION-LOCAL statics (`static T x = <dynamic>;` inside a function),
         where VC5 differs from clang TWICE: it prefixes a `_` on top of the C++
         mangling, and it numbers the scope ordinal by how many blocks it has
         already left where clang always writes `?1`. clang
         `?s_ambientCoin@?1??GetAmbientId@CPlay@@QAEHXZ@4HA` -> cl
         `_?s_ambientCoin@?4??GetAmbientId@CPlay@@QAEHXZ@4HA$S41910`. Both the
         ordinal and the `$S` suffix are wildcarded and the rewrite is accepted
         only when EXACTLY ONE object symbol matches. This is what lets a
         `DATA(rva)` sit on the local static itself instead of on a fabricated
         file-scope stand-in
         (docs/patterns/function-local-static-dynamic-init-guard.md).

    Without these a `DATA(rva)` on a perfectly ordinary `static const char x[]` /
    `const double x` silently binds NOTHING: the label pass reports a MISS and the
    datum stays unnamed, so the delinker never carves it.
    """
    if not candidate or not obj_syms:
        return None
    for cand in dict.fromkeys([CLANG_Q_ARRAY_RE.sub(r"@@\1P", candidate), candidate]):
        if cand != candidate and cand in obj_syms:
            return cand
        pool = re.compile(POOL_ID_RE_TMPL % re.escape(cand))
        hits = [s for s in obj_syms if pool.match(s)]
        if len(hits) == 1:
            return hits[0]
    m = LOCAL_STATIC_SCOPE_RE.match(candidate)
    if m:
        # MSVC encodes the scope ordinal in its usual number form: 1..10 as the
        # single digits `0`..`9`, anything larger as hex digits spelled A..P and
        # terminated by `@`. A static nested deep in a function therefore reads
        # `?BD@??Fn@@...` (0x13 = 19), not `?19??` - wildcarding only the decimal
        # form silently missed every such datum (measured: BuildBootyWalkingGruntz'
        # `buf` at ?BD@, NotifyFortUnderAttack's `s_alert` at ?BA@).
        local = re.compile(
            r"^_?%s\?(?:[0-9]|[A-P]+@)%s(\$S[0-9]+)?$"
            % (re.escape(m.group(1)), re.escape(m.group(2)))
        )
        hits = [s for s in obj_syms if local.match(s)]
        if len(hits) == 1:
            return hits[0]
    return None


#: A reviewed sidecar row that means to BE cl's own spelling is written in the
#: CANONICAL, ordinal-free form (`..._?$S@?1??Fn@@...@4EA$S`) and resolved to
#: cl's current spelling at merge time. Rows that are deliberate ALIASES (the
#: `?s_gruntDirEast_<rva>@@3U...` decorated cells - N copies at N rvas need N
#: distinct names) carry no `$S` at all and are left untouched.
_VERBATIM_CL_NAME = re.compile(r"^_.*\$S[0-9]*$")


def _repair_static_ordinal(name, unit, cache):
    """Resolve a canonical sidecar row to cl's CURRENT `$S<n>` spelling.

    NO CHECKED-IN FILE MAY CARRY A `$S<n>` ORDINAL. It is a per-object CodeView
    counter that renumbers on ANY symbol churn in the TU, so a row pinned as
    `...@4EA$S41135` silently stops naming anything the moment an unrelated edit
    shifts the counter to 41134 - and nothing notices, because the objdiff
    normalizer content-addresses these names by STRIPPING the ordinal, so the
    stale row and the real symbol still pair and the section stays 100.0 while
    the manifest names a symbol that does not exist. Measured: 6 of the 8
    verbatim rows in the sidecar had rotted exactly that way.

    So the sidecar states the ordinal-free name and the build re-derives the
    rest, authority-checked exactly as `msvc5_data_symbol` does it: a substitute
    is accepted only when the ordinal-stripped name matches EXACTLY ONE symbol
    in that unit's base obj. A row matching none is left as written and
    reported - it names a datum cl no longer emits, which is a real modelling
    question, not something to paper over.

    (Emitting the canonical form all the way into `symbol_names.csv` is NOT
    equivalent and is deliberately not done: the delinker needs one name to
    resolve to one extent image-wide, and stripping collides `_kMsToSeconds$S`
    across creditsstate and fader, which hold distinct statics at distinct rvas.)
    """
    if not _VERBATIM_CL_NAME.match(name):
        return name
    if unit not in cache:
        obj = REPO / "build/objdiff/base" / ("%s.obj" % unit)
        cache[unit] = nm_all_symbols(str(obj)) if obj.is_file() else set()
    syms = cache[unit]
    if not syms or name in syms:
        return name
    strip = lambda s: re.sub(r"\$S[0-9]+", "$S", s)      # noqa: E731
    want = strip(name)
    hits = [s for s in syms if strip(s) == want]
    if len(hits) == 1:
        log("static-copy pin re-pointed to cl's current ordinal: %s -> %s [%s]"
            % (name, hits[0], unit))
        return hits[0]
    log("WARN static-copy pin names no symbol in %s.obj: %s (%d ordinal-stripped "
        "match(es))" % (unit, name, len(hits)))
    return name


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
    # `globals` unit (src/Globals.cpp, by the archived consolidate_globals sweep) is
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


def merge_fragments(frags, out, functions_frags=None, functions_out=None,
                    globals_frags=None, globals_out=None):
    """Combine per-TU fragment CSVs into symbol_names.csv, re-applying the cross-TU
    duplicate-RVA guard + DATA dedup. Each fragment is a symbol_names.csv slice
    (rva,name,unit,size,kind) emitted by one --tu run. The per-TU functions.json /
    globals.json fragments are merged the same way (last per rva wins) so apply.py
    sees EVERY unit's signatures/global types, not just the last TU built. The
    manual vtable catalogs are folded in here too."""
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
    # A fragment IS allowed to be empty when its TU carries no rva.h label macro at
    # all. Consult the SOURCE rather
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
    # Manually managed game-vtable rows. synth_pdb ignores the cosmetic unit for
    # data symbols, keying each datum rename by rva->name.
    for row in vtable_catalog.game_rows():
        # NO game-vtable row carries a size here. A vtable's size is evidence, not a
        # reconstructed storage extent: handing it to the data manifest enrolls the
        # extent under this row's `unit`, duplicating the COFF-emitted vtable section
        # that data_manifest.vtable_rows() already enrolls once per EMITTING object
        # (a vtable is a folded COMDAT, so there is rarely just one).
        #
        # The two template rows used to be the exception, because vtable_rows could
        # not spell a specialization's mangled name from its RTTI key and withheld
        # them. It bridges those names through this catalog now, so the exception -
        # and the hand-written `unit` it needed - is retired. That column had already
        # rotted: `movieplayer` was dissolved on 2026-08-06 and its vtable's six
        # relocated words went silently unscored until 2026-08-09.
        rows.append((row["rva"], row["name"], row.get("unit") or "vtables", None,
                     "data"))
    # Library tables (MFC/CRT) a reconstructed base obj references or emits. The
    # SIZE IS LOAD-BEARING AND IS NOT OPTIONAL: it is what enrols the row in the
    # delinker's data manifest, and the enrolled DEFINITION is what lets the
    # delinker name that address at all. Dropping it (tried 2026-08-09, on the
    # theory that NAFXCW owns these so we should only NAME them) did not leave the
    # name in place - every reference to `??_7type_info@@6B@` immediately fell back
    # to `?g_dot@@3PADA + 0xfffffcc0`, 435 ADDEND rows in the data-reloc sieve.
    # The price is that a table our base obj only REFERENCES gets a target-side-only
    # definition in its unit; that is the cost of naming it.
    for row in vtable_catalog.library_rows():
        if row.get("unit"):
            rows.append((row["rva"], row["name"], row["unit"], row["size"], "data"))
    # Reviewed per-TU static-copy pins (header statics: DATA() in a header is
    # ignored, so their retail rvas live in a tracked sidecar - see
    # include/Gruntz/GruntDirStatics.h).
    copies = REPO / "config/static_data_copies.tsv"
    if copies.exists():
        obj_cache = {}
        for ln in copies.read_text().splitlines():
            if not ln.strip() or ln.lstrip().startswith("#"):
                continue
            rva_s, name, unit, size_s, kind = ln.split("\t")
            name = _repair_static_ordinal(name, unit, obj_cache)
            rows.append((int(rva_s, 16), name, unit, int(size_s, 16), kind))
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
    compgen_data = load_compgen_data()

    rows = []          # (rva, name, unit, size, kind)
    misses = []        # (rva, candidate, unit, reason)
    addr_sites = {}    # rva -> [(tu, "fn")] for every function rva, to catch dups
    func_meta = {}     # rva -> {ir_sym, names} for functions.json signatures
    global_meta = {}   # rva -> {name, type, unit} for globals.json (typed data)
    no_ir = []         # TUs whose label pass produced NO IR at all      -> FATAL
    no_rows = []       # TUs that carry rva.h macros but labelled nothing -> FATAL
    no_data_sizes = [] # TUs whose current libclang layout parse failed  -> FATAL
    compgen_claims = []  # (rva, name, unit, size, vkind, payload)
    compgen_errors = []  # (tu, line, reason)                            -> FATAL
    for i, tu in enumerate(args.tu):
        # Comments are BLANKED before any text scan: a doc comment quoting an
        # RVA_COMPGEN(..) invocation must never become a live pin.
        text = blank_comments(Path(tu).read_text())
        if args.unit:
            unit = args.unit[i]
        else:
            rel = str(Path(tu))
            unit = unit_map.get(rel) or unit_map.get("./" + rel) or Path(tu).stem

        have_obj = i < len(args.obj)
        obj_syms = nm_symbols(args.obj[i], args.nm) if have_obj else None
        all_syms = nm_all_symbols(args.obj[i], args.nm) if have_obj else None
        common_syms = nm_common_symbols(args.obj[i], args.nm) if have_obj else None
        cl_flags = compdb.get(os.path.realpath(tu))

        # A TU with no include/rva.h macro is a vendored C TU with pristine source;
        # its rva->symbol map comes from config/retail/zlib_labels.csv (static, emitted
        # directly - no parse, no join). zlib's static/K&R functions drop from IR
        # when unused, so labels can't live in the source; src/ uses the macros.
        if not MACRO_RE.search(text):
            config_tu(unit, label_config.get(unit, []), obj_syms, all_syms,
                      rows, misses, addr_sites)
            continue

        # --- functions via LLVM IR (mangled symbol paired directly) ---
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
        data_sizes = None
        if DATA_MACRO_RE.search(text):
            data_sizes = clang_var_sizes(tu, args.flag, cl_flags)
            if data_sizes is None:
                no_data_sizes.append(tu)
        for rva, ir_sym, size in func_labels_from_ir(ir):
            addr_sites.setdefault(rva, []).append((tu, ir_sym))
            # The IR pairs the annotation with the function's own mangled symbol.
            name = ir_sym
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
            c_sym = ms_c_symbol(name, obj_syms)   # extern "C": clang drops the x86 `_`/`@n`
            if c_sym:
                rows.append((rva, c_sym, unit, size, "func"))
                continue
            # clang's mangledName misses the destructor (it emits the `??_D vbase
            # dtor` variant, not the real `??1`). Resolve the canonical
            # `??1<class>@@...@XZ` directly from the obj's code symbols.
            resolved = plain_dtor_symbol(name, obj_syms)
            if resolved:
                rows.append((rva, resolved, unit, size, "func"))
            else:
                misses.append((rva, name, unit, "candidate not in base obj"))

        # --- RVA_COMPGEN(<rva>, <size>, <mangled>): compiler-generated function
        # pins (no source body for an RVA() to ride) ---
        for m in RVA_COMPGEN_RE.finditer(text):
            rva_s, size_s, sym = m.group(1), m.group(2), m.group(3)
            rva = int(rva_s, 16)
            size = (int(size_s, 16) if size_s.lower().startswith("0x")
                    else int(size_s)) or None  # 0 = size unknown
            addr_sites.setdefault(rva, []).append((tu, sym))
            # No source body -> no AST param names; undname still gives the
            # (typed, unnamed) signature for functions.json.
            func_meta[rva] = {"ir_sym": sym, "names": None}
            if VOLATILE_ORDINAL_FN_RE.match(sym):
                misses.append((
                    rva, sym, unit,
                    "volatile compiler ordinal - pin it at its owner with "
                    "RVA_DYNINIT instead",
                ))
            elif obj_syms is None or sym in obj_syms:
                rows.append((rva, sym, unit, size, "func"))
            else:
                misses.append((rva, sym, unit, "RVA_COMPGEN not in base obj"))

        # --- DATA via AST (IR drops the extern's annotation) ---
        if ast is not None and data_sizes is not None and DATA_MACRO_RE.search(text):
            for rva, cand, qtype in data_labels(text, ast, tu):
                if cand is None:
                    misses.append((rva, None, unit, "no VarDecl below DATA()"))
                    continue
                clang_name = cand
                if all_syms is not None and cand not in all_syms:
                    cand = msvc5_data_symbol(cand, all_syms) or cand
                # The `globals` unit is trusted (see GLOBALS_UNIT): its externs are
                # never referenced in its own base obj, so bypass the authority
                # check (the names came pre-checked from the matched TUs).
                if obj_syms is None or cand in all_syms or unit == GLOBALS_UNIT:
                    # libclang sizes the declaration in THIS TU under the real
                    # i386/MSVC flags. Unprovable/incomplete types stay None.
                    dsize = data_sizes.get(clang_name)
                    rows.append((rva, cand, unit, dsize, "data"))
                    # remember the declared type so apply.py can type the global
                    global_meta[rva] = {"name": cand, "type": qtype, "unit": unit,
                                        "size": dsize}
                else:
                    misses.append((rva, cand, unit,
                                   "data candidate not in base obj"))

        # --- compiler-generated data via DATA_COMPGEN (source parse: the macro
        # sits in EXPRESSION position, so there is no IR/AST carrier to ride -
        # the homm2 strategy, see docs/data-attribution.md) ---
        if DATACOMPGEN_RE.search(text):
            cg_claims, cg_errors = compgen_tu(
                text, tu, unit, args.obj[i] if have_obj else None)
            compgen_claims += cg_claims
            compgen_errors += cg_errors
            for rva, name, cunit, size, _vkind, _payload in cg_claims:
                rows.append((rva, name, cunit, size, "data"))

        # A TU that carries rva.h macros MUST label something. If it labelled nothing,
        # the label pass silently dropped it (clang parsed the source but the IR/AST
        # join produced no rows) - the same class of silent hole as `no_ir` above.
        if len(rows) == rows_before:
            no_rows.append(tu)

        # --- compiler-generated DATA pins (config/retail/compiler-generated-data.tsv):
        # a COMMON cl emitted from a definition that lives in a HEADER, so no source
        # macro can reach it. Deliberately AFTER the no_rows check - these rows are
        # inherited from a shared header, so they must never satisfy the "this TU
        # contributed something" gate on their own. ---
        compgen_data_tu(unit, compgen_data, common_syms, rows, misses)

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
    # ------------------------------------------------------------------
    # DATA_COMPGEN cross-TU gate (fatal). One compiler-generated identity per
    # RVA - EXCEPT identical string payloads: /Gf pooling (implied by /O2) folds
    # the same literal from N TUs onto ONE retail RVA, so N string claims with
    # byte-identical content are the SAME datum and all coalesce onto its one
    # `??_C@` name (docs/string-pooling.md). FP pools never fold (per-TU $T
    # statics), so a numeric RVA claimed by two TUs is always a mis-pin.
    # ------------------------------------------------------------------
    by_rva = {}
    for c in compgen_claims:
        by_rva.setdefault(c[0], []).append(c)
    for rva, group in sorted(by_rva.items()):
        if len(group) == 1:
            continue
        kinds = {c[4] for c in group}
        payloads = {c[5] for c in group}
        if kinds != {"str"} or len(payloads) != 1:
            units = ", ".join(sorted({c[2] for c in group}))
            compgen_errors.append(
                ("<cross-TU>", 0,
                 "0x%08x claimed by [%s] with non-identical or non-string values - "
                 "only byte-identical pooled strings may share an rva" % (rva, units)))
    if compgen_errors:
        for etu, eline, why in compgen_errors:
            log(f"ERROR {etu}:{eline}: DATA_COMPGEN {why}")
        log(f"{len(compgen_errors)} DATA_COMPGEN violation(s); refusing to write "
            f"{args.out}.")
        return 1

    if no_ir or no_rows or no_data_sizes:
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
        for tu in no_data_sizes:
            log(f"ERROR {tu}: pylibclang could not derive DATA extents from the current "
                "TU; refusing to reuse a structs.json snapshot or emit stale sizes.")
        log(f"{len(no_ir) + len(no_rows) + len(no_data_sizes)} TU(s) failed label "
            "generation; refusing to write "
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

    # The reviewed DATA_COMPGEN claims table, one row PER CLAIMING UNIT (unlike
    # symbol_names.csv, whose per-rva dedup keeps one representative row) - the
    # data manifest enrolls a folded literal once per owner from this.
    cg_out = Path(args.out).with_name("data_compgen.csv")
    cg_out.parent.mkdir(parents=True, exist_ok=True)
    cg_lines = ["rva,name,unit,size,kind\n"] + [
        "0x%08x,%s,%s,0x%x,%s\n" % (rva, name, unit, size, vkind)
        for rva, name, unit, size, vkind, _payload in sorted(compgen_claims)]
    cg_text = "".join(cg_lines)
    if not cg_out.exists() or cg_out.read_text() != cg_text:
        cg_out.write_text(cg_text)

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
