"""gruntz.retail_labels.source - per-TU source-claim extraction.

    gruntz labels [--unit U ...] [--all] [-j N]

Per TU (ported from the old labels pipeline, mechanisms unchanged):

  RVA(rva, size)   clang IR: @llvm.global.annotations pairs the annotation
                   string DIRECTLY with the function's mangled symbol - no
                   positional join, so an inline header definition can never
                   steal a nearby address.
  DATA(rva)        clang drops extern annotations from IR, so the macro is
                   text-scanned (comments blanked) and bound to the AST
                   VarDecl BELOW it; the exact extent comes from pylibclang.
  RVA_COMPGEN      verbatim regex - the name is given, no join, no IR.
  RVA_DYNINIT      the `$E` owner pins - regex; the pin's owner stands in for
                   the volatile ordinal, so NO authority check applies.
  DATA_COMPGEN     the compiler-generated DATUM pins. The macro expands to its
                   value expression, so no IR/AST carrier survives - it is
                   text-scanned (balanced parens: the macro sits in expression
                   position and clang-format wraps it) and the VALUE is the
                   claim: a narrow string literal is a pooled `??_C@` payload,
                   a float constant an FP-pool slot.

AUTHORITY: a function name is kept iff the TU's base obj defines it in a code
section; a DATA name is kept iff the obj carries it as ANY symbol (a matched
global is only referenced there, so it may be a `U` external). A DATA_COMPGEN
pin is kept iff the TU's own base obj emitted that exact payload - the pooled
literal's `??_C@` name IS cl's spelling for those bytes, an FP slot's `$T<n>`
ordinal is volatile so the constant is proven by its bytes and named for its
rva (the spelling every consumer already uses) - AND the retail image holds
those bytes at the pinned address. Misses are reported, never silently
dropped. A TU that compiles under cl but yields no IR is an ERROR: silently
contributing zero labels shrinks every denominator.

Vendored TUs (no rva.h macro in the source) are SKIPPED - their claims are
the functions_zlib/data_zlib provider tables, not extraction.
"""

from __future__ import annotations

import bisect
import os
import re
import struct

from gruntz.core.coff import Coff
from gruntz.core.paths import BUILD, REPO
from gruntz.core.pe import image
from gruntz.core.tsv import write as write_tsv
from gruntz.retail_labels.fragments import FRAGMENTS, HEADER
from gruntz.manifest import units as manifest_units
from gruntz.tool import clang

BASE_OBJS = BUILD / "objdiff/base"

# Presence test ONLY (never extraction): a TU with no rva.h macro at all is a
# vendored TU whose claims are the functions_zlib/data_zlib tables - skip it.
LABELED_TU_RE = re.compile(r"\b(?:RVA|DATA|RVA_COMPGEN|RVA_DYNINIT|DATA_COMPGEN)\s*\(")
DATA_MACRO_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
RVA_COMPGEN_RE = re.compile(
    r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*([^\s,)]+)\s*\)")
RVA_DYNINIT_RE = re.compile(
    r"\bRVA_DYNINIT\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*([A-Za-z_][A-Za-z0-9_:<>]*)\s*\)")
ANN_RVA_RE = re.compile(r"^rva:(0x[0-9a-fA-F]+)(?:\s+size:(0x[0-9a-fA-F]+|\d+))?$")
ANN_DATA_RE = re.compile(r"^data:(0x[0-9a-fA-F]+)$")

DATA_COMPGEN_RE = re.compile(r"\bDATA_COMPGEN\s*\(")
COMPGEN_ADDR_RE = re.compile(r"0x[0-9a-fA-F]{8}$")
_STR_SEG_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')
_FLOAT_RE = re.compile(r"[-+]?(?:\d+\.\d*|\.\d+|\d+[eE][-+]?\d+"
                       r"|\d+\.?\d*[eE][-+]?\d+)([fF]?)$")
#: cl's floating-point literal pool member spelling. The `<n>` is a per-object
#: counter that renumbers on any TU churn, so it is never stored - the claim
#: names the slot for its rva, which is what the data manifest, the compgen
#: manifest and compare's content-addressing all spell.
FP_POOL_NAME = re.compile(r"^\$T[0-9]+$")

# clang-vs-VC5 static-data name reconciliation (all authority-checked - a
# rewrite is accepted only when that exact symbol is in the compiled obj):
#   1. array storage class: clang `@@3QB..B` vs VC5 `@@3PB..B`;
#   2. internal-linkage file statics: cl decorates `_x$S<n>` (volatile
#      CodeView ordinal), clang reports plain `_x` - matched by prefix,
#      accepted only when exactly ONE symbol matches;
#   3. function-local statics: cl prefixes `_`, renumbers the scope by blocks
#      LEFT (clang always writes `?1`; MSVC spells 1..10 as digits `0`..`9`
#      and larger as hex `A..P@`), and appends `$S<n>`.
CLANG_Q_ARRAY_RE = re.compile(r"@@([0-9])Q")
POOL_ID_RE_TMPL = r"^_?%s\$S[0-9]+$"
LOCAL_STATIC_SCOPE_RE = re.compile(r"^(\?[^@]+@)\?[0-9]+(\?\?.+)$")


def msvc5_data_symbol(candidate: str, obj_syms: set[str]) -> str | None:
    """cl's own spelling for a clang-proposed data name, or None."""
    if not candidate or not obj_syms:
        return None
    for cand in dict.fromkeys([CLANG_Q_ARRAY_RE.sub(r"@@\1P", candidate),
                               candidate]):
        if cand != candidate and cand in obj_syms:
            return cand
        pool = re.compile(POOL_ID_RE_TMPL % re.escape(cand))
        hits = [s for s in obj_syms if pool.match(s)]
        if len(hits) == 1:
            return hits[0]
    m = LOCAL_STATIC_SCOPE_RE.match(candidate)
    if m:
        local = re.compile(
            r"^_?%s\?(?:[0-9]|[A-P]+@)%s(\$S[0-9]+)?$"
            % (re.escape(m.group(1)), re.escape(m.group(2))))
        hits = [s for s in obj_syms if local.match(s)]
        if len(hits) == 1:
            return hits[0]
    return None

# @llvm.global.annotations tuple + the @.str constants it references.
_STR_DEF_RE = re.compile(r'^(@[\w.$"]+)\s*=.*?\bc"((?:[^"\\]|\\.)*)"', re.M)
_ANN_TUPLE_RE = re.compile(
    r'\{\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,\s*ptr\s+(@(?:"[^"]+"|[\w.$]+))\s*,')


def _unescape_ir_cstr(s: str) -> str:
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


def _ir_symbol_name(ref: str) -> str:
    """`@"?Foo@@..."` / `@_foo` -> the bare symbol (the `\\01` verbatim-name
    prefix stripped, since the base obj's symbol has no such prefix)."""
    ref = ref[1:]
    if ref.startswith('"') and ref.endswith('"'):
        ref = ref[1:-1]
    if ref.startswith("\\01"):
        ref = ref[3:]
    return ref


def ir_claims(ir: str) -> tuple[list[tuple[int, str, int | None]],
                                list[tuple[int, str]]]:
    """(func, data) claims from @llvm.global.annotations - each annotation
    string arrives paired DIRECTLY with the symbol's mangled name. `data:`
    tuples cover every annotated DEFINITION; only extern-only declarations
    drop from IR and need the AST fallback."""
    strings = {m.group(1): _unescape_ir_cstr(m.group(2))
               for m in _STR_DEF_RE.finditer(ir)}
    funcs, datas = [], []
    for line in ir.splitlines():
        if "@llvm.global.annotations" not in line:
            continue
        for sym_ref, str_ref in _ANN_TUPLE_RE.findall(line):
            ann = strings.get(str_ref)
            if ann is None:
                continue
            m = ANN_RVA_RE.match(ann)
            if m:
                size = None
                if m.group(2):
                    v = m.group(2)
                    size = int(v, 16) if v.lower().startswith("0x") else int(v)
                funcs.append((int(m.group(1), 16), _ir_symbol_name(sym_ref), size))
                continue
            m = ANN_DATA_RE.match(ann)
            if m:
                datas.append((int(m.group(1), 16), _ir_symbol_name(sym_ref)))
    return funcs, datas


def blank_comments(text: str) -> str:
    """`text` with // and /* */ bodies blanked (newlines kept) so a macro in
    a COMMENT is never read as a binding."""
    out = list(text)
    n, i, st = len(text), 0, "code"
    while i < n:
        c = text[i]
        if st == "code":
            if c == "/" and i + 1 < n and text[i + 1] == "/":
                while i < n and text[i] != "\n":
                    out[i] = " "
                    i += 1
                continue
            if c == "/" and i + 1 < n and text[i + 1] == "*":
                while i < n and not (text[i] == "*" and i + 1 < n
                                     and text[i + 1] == "/"):
                    if text[i] != "\n":
                        out[i] = " "
                    i += 1
                continue
            if c in "\"'":
                st = c
        elif c == "\\":
            i += 2
            continue
        elif c == st:
            st = "code"
        i += 1
    return "".join(out)


def _skip_quote(text: str, i: int) -> int:
    """Index just past the literal opening at `text[i]` (escapes honoured)."""
    quote, n = text[i], len(text)
    i += 1
    while i < n and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1


def _split_top_level(body: str) -> list[str]:
    parts, depth, start, i = [], 0, 0, 0
    while i < len(body):
        c = body[i]
        if c in "\"'":
            i = _skip_quote(body, i)
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            parts.append(body[start:i])
            start = i + 1
        i += 1
    parts.append(body[start:])
    return [p.strip() for p in parts]


def compgen_invocations(text: str) -> list[tuple[int, list[str]]]:
    """[(line, [arg, ...])] for each DATA_COMPGEN(...) in blanked TU text.

    The macro sits in EXPRESSION position, so unlike the statement labels it
    may be wrapped by clang-format and two may share one line - the scan is
    balanced-paren and quote-aware, never line-based."""
    out = []
    for m in DATA_COMPGEN_RE.finditer(text):
        depth, j, n = 1, m.end(), len(text)
        while j < n and depth:
            c = text[j]
            if c in "\"'":
                j = _skip_quote(text, j)
                continue
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            j += 1
        out.append((text.count("\n", 0, m.start()) + 1,
                    _split_top_level(text[m.end():j - 1])))
    return out


def compgen_value(value_src: str) -> tuple[str | None, bytes | str]:
    """('str'|'f32'|'f64', payload bytes), or (None, reason).

    The value SPELLING is the allocation's type: `0.0` is an f64 pool entry,
    `0.0f` an f32, a (concatenation of) narrow string literal(s) the pooled
    `??_C@` payload. Anything else - identifiers, wide strings, integers
    (immediates live in code, not data) - is rejected."""
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
        try:
            val = float(v[:-1] if m.group(1) else v)
        except ValueError:
            return None, "unparsable float constant"
        if m.group(1):
            return "f32", struct.pack("<f", val)
        return "f64", struct.pack("<d", val)
    return None, "value is neither a narrow string literal nor a float constant"


def obj_literals(obj_path) -> tuple[dict[bytes, str], dict[str, bytes]]:
    """({pooled payload: `??_C@` name}, {`$T<n>`: slot bytes}) of one base obj.

    The delink COFF reader is the tree's only section-payload parser; the
    name-authority reader (core.coff) answers names only, and duplicating the
    section walk here would fork the format knowledge."""
    from gruntz.delink.coffx import Obj
    c = Obj(obj_path)
    strings: dict[bytes, str] = {}
    for idx, value, secnum in c.iter_symbols():
        name = c.sym_name(idx)
        if name.startswith("??_C@") and secnum >= 1:
            payload = c.cstring(secnum, value)
            if payload is not None:
                strings.setdefault(payload, name)
    pool: dict[str, bytes] = {}
    for sec in c.section_table:
        members = c.section_members(sec["index"])
        slots = [(off, name) for off, name, _scl in members
                 if FP_POOL_NAME.fullmatch(name)]
        if not slots:
            continue
        starts = sorted(off for off, _n, _s in members)
        payload = c.section_payload(sec["index"])[:sec["size"]]
        for off, name in slots:      # the slot runs to the next member/section end
            end = next((o for o in starts if o > off), sec["size"])
            pool[name] = payload[off:end]
    return strings, pool


def compgen_claims(text: str, unit: str, obj_path) -> tuple[list[tuple[int, str, int]],
                                                            list[str]]:
    """[(rva, name, size)] DATA_COMPGEN claims + [problems].

    Two independent facts per pin: the claiming TU's own base obj emitted that
    payload (cl's spelling for it is the name), and the retail image holds
    those bytes at the pinned address."""
    strings, pool = obj_literals(obj_path)
    img = image()
    claims, problems, seen = [], [], {}
    for line, args in compgen_invocations(text):
        where = f"{unit}: DATA_COMPGEN at line {line}"
        if len(args) != 2:
            problems.append(f"{where} takes (addr, value); got {len(args)} "
                            f"arg(s) (FATAL)")
            continue
        addr_src, value_src = args
        if not COMPGEN_ADDR_RE.fullmatch(addr_src):
            problems.append(f"{where}: address {addr_src!r} is not the "
                            f"canonical 8-digit 0x form (FATAL)")
            continue
        rva = int(addr_src, 16)
        vkind, payload = compgen_value(value_src)
        if vkind is None:
            problems.append(f"{where}: 0x{rva:06x} {payload} (FATAL)")
            continue
        if rva in seen:                       # repeated expansions coalesce
            if seen[rva] != (vkind, payload):
                problems.append(f"{where}: 0x{rva:06x} is claimed twice in "
                                f"this TU with different values (FATAL)")
            continue
        seen[rva] = (vkind, payload)
        if vkind == "str":
            name = strings.get(payload)
            if name is None:
                problems.append(f"{where}: 0x{rva:06x} {payload[:24]!r} is not "
                                f"a pooled ??_C@ literal in this TU's base obj "
                                f"(FATAL)")
                continue
            payload += b"\0"                  # the datum IS the NUL-terminated one
        else:
            # a padded slot may run past the literal; the bytes still prove it
            if not any(slot[:len(payload)] == payload for slot in pool.values()):
                problems.append(f"{where}: 0x{rva:06x} {vkind} bits "
                                f"{payload.hex()} are in no $T pool slot of "
                                f"this TU's base obj (FATAL)")
                continue
            name = f"$T{rva}"                 # cl's ordinal is volatile, rva is not
        if img.read(rva, len(payload)) != payload:
            problems.append(f"{where}: 0x{rva:06x} retail bytes contradict the "
                            f"pinned value (FATAL)")
            continue
        claims.append((rva, name, len(payload)))
    return claims, problems


def _loc_file(loc) -> str | None:
    """clang's JSON printer omits `file` when unchanged, and a macro-expanded
    location carries it under expansionLoc/spellingLoc (expansion printed
    last). Reading only the top-level key silently loses every DATA() below a
    macro-expanded declaration - the measured BattlezMapConfig trap."""
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


def collect_vars(ast: dict, main_file: str) -> list[tuple[str, int, str]]:
    """[(mangledName, offset, qualType)] for main-file global VarDecls."""
    main_real = os.path.realpath(main_file)
    out: list[tuple[str, int, str]] = []
    state = {"in_main": True}

    def visit(node):
        if not isinstance(node, dict):
            return
        for loc in (node.get("loc"), (node.get("range") or {}).get("begin")):
            f = _loc_file(loc)
            if f is not None:
                state["in_main"] = os.path.realpath(f) == main_real
        if (state["in_main"] and node.get("kind") == "VarDecl"
                and "mangledName" in node and not node.get("isImplicit")):
            off = (node.get("loc") or {}).get("offset")
            if off is not None:
                qt = (node.get("type") or {}).get("qualType") or ""
                out.append((node["mangledName"], off, qt))
        for c in node.get("inner") or []:
            visit(c)

    visit(ast)
    return out


def data_claims(text: str, ast: dict, main_file: str) -> list[tuple[int, str | None, str]]:
    """[(rva, mangledName|None, qualType)] - each DATA(0x..) bound to the
    VarDecl just BELOW it by line."""
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    var_defs = sorted((bisect.bisect_right(starts, off), mn, qt)
                      for (mn, off, qt) in collect_vars(ast, main_file))
    out = []
    for line_no, m in enumerate(blank_comments(text).splitlines(), 1):
        dm = DATA_MACRO_RE.search(m)
        if dm:
            cand = next(((mn, qt) for (dl, mn, qt) in var_defs if dl >= line_no),
                        (None, ""))
            out.append((int(dm.group(1), 16), cand[0], cand[1]))
    return out


def extract_unit(unit: str, source: str, compdb: dict) -> tuple[list[list[str]], list[str]]:
    """(fragment rows, problems). Empty rows for a vendored (macro-free) TU."""
    src_path = REPO / source
    text = src_path.read_text(errors="replace")
    if not LABELED_TU_RE.search(blank_comments(text)):
        return [], []

    problems: list[str] = []
    obj = BASE_OBJS / f"{unit}.obj"
    coff = Coff(obj) if obj.is_file() else None
    if coff is None:
        # without the obj the authority check cannot run, and admitting every
        # clang proposal unproven would void the module contract silently
        return [], [f"{unit}: no base obj ({obj.name}) - authority check "
                    f"impossible, extraction refused (FATAL)"]
    if obj.stat().st_mtime < src_path.stat().st_mtime:
        # The authority is cl's OBJECT, so a stale one answers for source that
        # no longer exists: a name this edit introduced reads as "not a symbol
        # in <unit>.obj (dropped)". The graph orders cl before labels; a manual
        # `gruntz labels --unit` does not.
        problems.append(f"{unit}: {obj.name} is OLDER than {src_path.name} - "
                        f"the authority check is reading a stale object; drops "
                        f"below may be artifacts. Run `gruntz build` first.")
    code_names = coff.code_names() if coff else None
    all_names = coff.all_names() if coff else None

    cl_flags = compdb.get(os.path.realpath(str(src_path)))
    rows: list[list[str]] = []

    def emit(rva, size, name, kind, channel, qtype=""):
        rows.append([f"0x{rva:08x}", f"0x{size:x}" if size is not None else "",
                     name, kind, channel, qtype])

    def authorize(name, names_set):
        """clang proposes, cl disposes. A C-linkage name reaches the i386 obj
        with a leading underscore the IR-level name lacks - accept and emit
        the obj's own spelling."""
        if names_set is None or name in names_set:
            return name
        if "_" + name in names_set:
            return "_" + name
        return None

    # functions via IR
    ir = clang.emit_ir(str(src_path), cl_flags)
    if ir is None:
        return rows, [f"{unit}: clang produced no IR - every RVA() label of "
                      f"this TU would silently vanish (FATAL)"]
    ir_funcs, ir_datas = ir_claims(ir)
    for rva, name, size in ir_funcs:
        got = authorize(name, code_names)
        if got is None:
            problems.append(("drop", "func", rva,
                             f"{unit}: RVA(0x{rva:06x}) {name} not a code "
                             f"symbol in {obj.name} (dropped)"))
            continue
        emit(rva, size, got, "func", "src")

    # compiler-generated bodies, name verbatim
    blanked = blank_comments(text)
    for m in RVA_COMPGEN_RE.finditer(blanked):
        rva, size = int(m.group(1), 16), int(m.group(2), 0)
        name = m.group(3)
        if code_names is not None and name not in code_names:
            problems.append(f"{unit}: RVA_COMPGEN {name} not a code symbol "
                            f"in {obj.name} (dropped)")
            continue
        emit(rva, size or None, name, "func", "src_compgen")

    # $E dynamic-init owner pins (no symbol to authority-check by design)
    for m in RVA_DYNINIT_RE.finditer(blanked):
        emit(int(m.group(1), 16), int(m.group(2), 0) or None,
             m.group(3), "func", "src_dyninit")

    # compiler-generated DATA the automatic oracles cannot reach: the pinned
    # value is the claim, cl's own obj the authority (see the module header)
    if DATA_COMPGEN_RE.search(blanked):
        cg_claims, cg_problems = compgen_claims(blanked, unit, obj)
        problems.extend(cg_problems)
        for rva, name, size in cg_claims:
            emit(rva, size, name, "data", "src_data_compgen")

    # data: IR annotations primary (join-free), AST line-join only for the
    # extern-only declarations IR drops; pylibclang gives every extent.
    if DATA_MACRO_RE.search(blanked) or ir_datas:
        sizes = clang.var_sizes(str(src_path), cl_flags) or {}

        def emit_data(rva, name, qtype=""):
            got = authorize(name, all_names)
            if got is None and all_names is not None:
                got = msvc5_data_symbol(name, all_names)
            if got is None:
                problems.append(("drop", "data", rva,
                                 f"{unit}: DATA(0x{rva:06x}) {name} not a "
                                 f"symbol in {obj.name} (dropped)"))
                return
            emit(rva, sizes.get(name), got, "data", "src", qtype)

        covered = set()
        for rva, name in ir_datas:
            covered.add(rva)
            emit_data(rva, name)
        site_count = len(DATA_MACRO_RE.findall(blanked))
        if site_count > len(covered):
            ast = clang.ast_dump(str(src_path), cl_flags)
            if ast is None:
                problems.append(f"{unit}: clang produced no AST - extern "
                                f"DATA() labels of this TU would vanish (FATAL)")
                return rows, problems
            for rva, name, qtype in data_claims(text, ast, str(src_path)):
                if rva in covered:
                    continue
                if name is None:
                    problems.append(("drop", "data", rva,
                                     f"{unit}: DATA(0x{rva:06x}) has no "
                                     f"VarDecl below it (dropped)"))
                    continue
                emit_data(rva, name, qtype)

    return rows, problems


MACRO_SITE_RE = re.compile(
    r"\b(RVA_COMPGEN|RVA_DYNINIT|DATA_COMPGEN|RVA|DATA)\s*\(\s*(0x[0-9a-fA-F]+)")


def sweep_sites() -> dict[str, dict[int, str]]:
    """Tree-wide macro-site census over src/ + include/ (comments blanked,
    rva.h's own #defines excluded): {macro: {rva: 'file:line'}}.

    The completeness oracle: every site must be accounted for by a fragment
    or a stated doctrine rule - a macro that neither extraction nor doctrine
    reaches is a silently lost label, the old tree's worst failure class."""
    out: dict[str, dict[int, list[str]]] = {}
    for base in ("src", "include"):
        root = REPO / base
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".cpp", ".h") or path.name == "rva.h":
                continue
            text = blank_comments(path.read_text(errors="replace"))
            for m in MACRO_SITE_RE.finditer(text):        # whole-file: a macro
                lineno = text.count("\n", 0, m.start()) + 1   # may span lines
                out.setdefault(m.group(1), {}).setdefault(
                    int(m.group(2), 16), []).append(
                    f"{path.relative_to(REPO)}:{lineno}")
    return out


def check_completeness(explained: set[tuple[str, int]] = frozenset()) -> list[str]:
    """The oracle over extraction: every site accounted for, loudly.
    `explained` (kind, rva) keys already have a louder same-run drop report;
    the sweep stays silent on those instead of restating one root cause
    twice. Keys carry the KIND so a func claim can never excuse a lost DATA
    site at the same rva (RVA and DATA share the `src` channel)."""
    from gruntz.retail_labels.fragments import all_claims
    sites = sweep_sites()
    have: dict[tuple[str, str], set[int]] = {}
    for c in all_claims():
        have.setdefault((c.channel, c.kind), set()).add(c.rva)
    problems = []
    checks = [("RVA", "src", "func"), ("RVA_COMPGEN", "src_compgen", "func"),
              ("RVA_DYNINIT", "src_dyninit", "func"), ("DATA", "src", "data"),
              ("DATA_COMPGEN", "src_data_compgen", "data")]
    for macro, channel, kind in checks:
        for rva, wheres in sorted(sites.get(macro, {}).items()):
            if macro == "DATA" and all(".h:" in w for w in wheres):
                problems.append(f"DATA(0x{rva:06x}) at {wheres[0]} is in a "
                                f"HEADER - extraction ignores it by design; a "
                                f"header static belongs in data_compgen.tsv "
                                f"(FATAL)")
                continue
            if rva not in have.get((channel, kind), set()) \
                    and (kind, rva) not in explained:
                problems.append(f"{macro}(0x{rva:06x}) at {wheres[0]} is in "
                                f"NO fragment - silently lost label (FATAL)")
            if len(wheres) > 1 and macro != "RVA":
                problems.append(f"{macro}(0x{rva:06x}) appears at "
                                f"{len(wheres)} sites ({wheres[0]} ...) - "
                                f"stacked/duplicated macro")
    return problems


def run(only_units: list[str] | None = None, jobs: int = os.cpu_count() or 4):
    """Extract fragments; returns (changed, problems). Fragment writes are
    content-idempotent so unchanged TUs never dirty downstream edges."""
    from concurrent.futures import ThreadPoolExecutor

    db = clang.compdb()
    units = manifest_units()
    if only_units is not None:
        known = {u["unit"] for u in units}
        for name in only_units:
            if name not in known:
                stem = os.path.splitext(os.path.basename(name))[0]
                stems = {stem.lower(), name.lower()}
                hint = sorted(k for k in known if k in stems)
                raise SystemExit(
                    f"[extract] unknown unit {name!r}"
                    + (f" - did you mean {hint[0]!r}?" if hint else
                       " - units are manifest stems, e.g. 'cimage'"))
    todo = [u for u in units
            if only_units is None or u["unit"] in only_units]
    changed, problems = [], []

    def one(u):
        rows, probs = extract_unit(u["unit"], u["source"], db)
        if any(isinstance(pr, str) and "FATAL" in pr for pr in probs):
            # never replace a good cached fragment with a truncated one
            return u["unit"], None, probs
        banner = [f"# GENERATED claim fragment for unit {u['unit']} - the "
                  f"macros in {u['source']} are the storage; do not edit."]
        did = write_tsv(FRAGMENTS / f"{u['unit']}.tsv", banner, HEADER, rows)
        return u["unit"], did, probs

    refused: set[str] = set()
    raw: list = []
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for unit, did, probs in pool.map(one, todo):
            if did:
                changed.append(unit)
            if did is None:
                refused.add(unit)
            raw.extend(probs)
    # a per-TU drop is benign iff ANOTHER unit's fragment claims the same
    # (kind, rva) - header inlines emit everywhere, cl materializes in one
    # TU. A fragment this run REFUSED to rewrite is stale evidence and does
    # not vote. A (kind, rva) dropped by every unit is a lost label.
    from gruntz.retail_labels.fragments import all_claims
    claimed = {(c.kind, c.rva) for c in all_claims() if c.unit not in refused}
    drops: dict[tuple[str, int], list[str]] = {}
    for pr in raw:
        if isinstance(pr, tuple):
            drops.setdefault((pr[1], pr[2]), []).append(pr[3])
        else:
            problems.append(pr)
    explained: set[tuple[str, int]] = set()
    forgiven = 0
    for key, msgs in sorted(drops.items()):
        if key in claimed:
            forgiven += len(msgs)
            continue
        explained.add(key)
        tail = " and NO other unit claims the rva"
        if len(msgs) > 1:
            units = ", ".join(sorted(m.split(":", 1)[0] for m in msgs))
            tail += f" - {len(msgs)} units dropped it ({units})"
        if only_units is None:
            problems.append(msgs[0] + tail + " (FATAL)")
        else:
            # a per-unit run cannot adjudicate a tree-wide census question:
            # one unhomed annotation must not fail every including unit's
            # graph edge. The --all sweep owns the FATAL.
            problems.append(msgs[0] + tail +
                            " (unhomed - the tree-wide sweep adjudicates)")
    if only_units is None:
        problems.extend(check_completeness(explained))
    elif forgiven and os.environ.get("GRUNTZ_VERBOSE"):
        problems.append(f"note: {forgiven} drop(s) forgiven by CACHED "
                        f"fragments of units this run did not extract; a "
                        f"full --all run re-proves them")
    return changed, problems


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(
        prog="gruntz labels", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append", help="extract one unit (repeatable)")
    ap.add_argument("--all", action="store_true",
                    help="extract every unit in the census")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 4)
    a = ap.parse_args()
    if not a.unit and not a.all:
        ap.error("pick --unit U or --all")
    changed, problems = run(a.unit if not a.all else None, a.jobs)
    for p in problems:
        print(f"[extract] {p}", file=sys.stderr)
    print(f"[extract] {len(changed)} fragment(s) changed")
    return 1 if any("FATAL" in p for p in problems) else 0


if __name__ == "__main__":
    raise SystemExit(main())
