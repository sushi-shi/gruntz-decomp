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

AUTHORITY: a function name is kept iff the TU's base obj defines it in a code
section; a DATA name is kept iff the obj carries it as ANY symbol (a matched
global is only referenced there, so it may be a `U` external). Misses are
reported, never silently dropped. A TU that compiles under cl but yields no
IR is an ERROR: silently contributing zero labels shrinks every denominator.

Vendored TUs (no rva.h macro in the source) are SKIPPED - their claims are
the functions_zlib/data_zlib provider tables, not extraction.
"""

from __future__ import annotations

import bisect
import os
import re

from gruntz.core.coff import Coff
from gruntz.core.paths import BUILD, REPO
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
    r"\s*([A-Za-z_][A-Za-z0-9_:]*)\s*\)")
ANN_RVA_RE = re.compile(r"^rva:(0x[0-9a-fA-F]+)(?:\s+size:(0x[0-9a-fA-F]+|\d+))?$")

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
POOL_ID_RE_TMPL = r"^%s\$S[0-9]+$"
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


def ir_func_claims(ir: str) -> list[tuple[int, str, int | None]]:
    """[(rva, mangled, size|None)] from @llvm.global.annotations."""
    strings = {m.group(1): _unescape_ir_cstr(m.group(2))
               for m in _STR_DEF_RE.finditer(ir)}
    out = []
    for line in ir.splitlines():
        if "@llvm.global.annotations" not in line:
            continue
        for sym_ref, str_ref in _ANN_TUPLE_RE.findall(line):
            ann = strings.get(str_ref)
            m = ANN_RVA_RE.match(ann) if ann is not None else None
            if not m:
                continue
            size = None
            if m.group(2):
                s = m.group(2)
                size = int(s, 16) if s.lower().startswith("0x") else int(s)
            out.append((int(m.group(1), 16), _ir_symbol_name(sym_ref), size))
    return out


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
    for rva, name, size in ir_func_claims(ir):
        got = authorize(name, code_names)
        if got is None:
            problems.append(f"{unit}: RVA(0x{rva:06x}) {name} not a code "
                            f"symbol in {obj.name} (dropped)")
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

    # data via the AST join + pylibclang extents
    if DATA_MACRO_RE.search(blanked):
        ast = clang.ast_dump(str(src_path), cl_flags)
        if ast is None:
            problems.append(f"{unit}: clang produced no AST - DATA() labels "
                            f"of this TU would vanish (FATAL)")
            return rows, problems
        sizes = clang.var_sizes(str(src_path), cl_flags) or {}
        for rva, name, qtype in data_claims(text, ast, str(src_path)):
            if name is None:
                problems.append(f"{unit}: DATA(0x{rva:06x}) has no VarDecl "
                                f"below it (dropped)")
                continue
            got = authorize(name, all_names)
            if got is None and all_names is not None:
                got = msvc5_data_symbol(name, all_names)
            if got is None:
                problems.append(f"{unit}: DATA(0x{rva:06x}) {name} not a "
                                f"symbol in {obj.name} (dropped)")
                continue
            emit(rva, sizes.get(name), got, "data", "src", qtype)

    return rows, problems


MACRO_SITE_RE = re.compile(
    r"\b(RVA_COMPGEN|RVA_DYNINIT|DATA_COMPGEN|RVA|DATA)\s*\(\s*(0x[0-9a-fA-F]+)")


def sweep_sites() -> dict[str, dict[int, str]]:
    """Tree-wide macro-site census over src/ + include/ (comments blanked,
    rva.h's own #defines excluded): {macro: {rva: 'file:line'}}.

    The completeness oracle: every site must be accounted for by a fragment
    or a stated doctrine rule - a macro that neither extraction nor doctrine
    reaches is a silently lost label, the old tree's worst failure class."""
    out: dict[str, dict[int, str]] = {}
    for base in ("src", "include"):
        root = REPO / base
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".cpp", ".h") or path.name == "rva.h":
                continue
            text = blank_comments(path.read_text(errors="replace"))
            for lineno, line in enumerate(text.splitlines(), 1):
                for m in MACRO_SITE_RE.finditer(line):
                    out.setdefault(m.group(1), {}).setdefault(
                        int(m.group(2), 16),
                        f"{path.relative_to(REPO)}:{lineno}")
    return out


def check_completeness() -> list[str]:
    """Compare the tree-wide site census against the written fragments."""
    from gruntz.retail_labels.fragments import all_claims
    sites = sweep_sites()
    have: dict[str, set[int]] = {}
    for c in all_claims():
        have.setdefault(c.channel, set()).add(c.rva)
    problems = []
    checks = [("RVA", "src"), ("RVA_COMPGEN", "src_compgen"),
              ("RVA_DYNINIT", "src_dyninit")]
    for macro, channel in checks:
        for rva, where in sorted(sites.get(macro, {}).items()):
            if rva not in have.get(channel, set()):
                problems.append(f"{macro}(0x{rva:06x}) at {where} is in NO "
                                f"fragment - silently lost label (FATAL)")
    # header DATA() is a silent no-op by extraction doctrine - flag it loud.
    for rva, where in sorted(sites.get("DATA", {}).items()):
        if where.startswith("include/") or where.endswith(".h") \
                or (".h:" in where):
            problems.append(f"DATA(0x{rva:06x}) at {where} is in a HEADER - "
                            f"extraction ignores it by design; a header "
                            f"static belongs in data_compgen.tsv (FATAL)")
    return problems


def run(only_units: list[str] | None = None, jobs: int = os.cpu_count() or 4):
    """Extract fragments; returns (changed, problems). Fragment writes are
    content-idempotent so unchanged TUs never dirty downstream edges."""
    from concurrent.futures import ThreadPoolExecutor

    db = clang.compdb()
    todo = [u for u in manifest_units()
            if only_units is None or u["unit"] in only_units]
    changed, problems = [], []

    def one(u):
        rows, probs = extract_unit(u["unit"], u["source"], db)
        if rows or probs:
            pass
        banner = [f"# GENERATED claim fragment for unit {u['unit']} - the "
                  f"macros in {u['source']} are the storage; do not edit."]
        did = write_tsv(FRAGMENTS / f"{u['unit']}.tsv", banner, HEADER, rows)
        return u["unit"], did, probs

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for unit, did, probs in pool.map(one, todo):
            if did:
                changed.append(unit)
            problems.extend(probs)
    if only_units is None:
        problems.extend(check_completeness())
    return changed, problems


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--unit", action="append", help="extract one unit (repeatable)")
    ap.add_argument("--all", action="store_true")
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
