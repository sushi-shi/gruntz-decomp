#!/usr/bin/env python3
"""A full census of BARE integer constants in src/ + include/.

libclang, not a regex: every count here is an AST node, so a number inside a
comment or a string is structurally invisible and a named constant is too (an
enumerator is a DECL_REF_EXPR, never an INTEGER_LITERAL). That makes
"INTEGER_LITERAL" the exact population of bare constants, and the
enumerator-reference count beside it the named half of the same field.

Excluded, and each for a stated reason:
  - label-macro arguments (RVA/DATA/SIZE/...) - those are ADDRESSES and
    sizes recovered from the binary, not constants anyone chose.
  - literals in vendor/ and anything outside src/ + include/.
"""
from __future__ import annotations
import collections, json, re, sys
from pathlib import Path

import clang.cindex as cidx

REPO = Path(__file__).resolve().parents[3]
CDB = REPO / "build/clangd/compile_commands.json"
LABEL_RE = re.compile(r"^\s*(RVA|RVA_COMPGEN|DATA|SIZE|SIZE_UNKNOWN|VTBL_ABSENT)\s*\(")

K = cidx.CursorKind
CMP = {"==", "!=", "<", ">", "<=", ">="}
ARITH = {"+", "-", "*", "/", "%", "<<", ">>", "&", "|", "^"}


def flags_for(entry):
    args = list(entry.get("arguments") or entry["command"].split())
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for a in args[1:]:
        if a == "/c" or a == src or a.endswith(src):
            continue
        out.append(a)
    return out


def rel(cur):
    f = cur.location.file
    if f is None:
        return None
    try:
        r = str(Path(f.name).resolve().relative_to(REPO))
    except ValueError:
        return None
    return r if r.startswith(("src/", "include/")) else None


_text_cache: dict[str, list[str]] = {}


def line_text(path, line):
    if path not in _text_cache:
        try:
            _text_cache[path] = (REPO / path).read_text(errors="replace").splitlines()
        except OSError:
            _text_cache[path] = []
    ls = _text_cache[path]
    return ls[line - 1] if 0 < line <= len(ls) else ""


def op_of(node):
    """The binary/unary operator spelling, via tokens (libclang <16 has no API)."""
    toks = [t.spelling for t in node.get_tokens()]
    kids = list(node.get_children())
    if node.kind == K.BINARY_OPERATOR and len(kids) == 2:
        n = len(list(kids[0].get_tokens()))
        return toks[n] if n < len(toks) else ""
    if node.kind == K.COMPOUND_ASSIGNMENT_OPERATOR and len(kids) == 2:
        n = len(list(kids[0].get_tokens()))
        return toks[n] if n < len(toks) else ""
    return ""


def classify(stack):
    """stack[-1] is the literal; walk outward to the first context that decides."""
    lit = stack[-1]
    for i in range(len(stack) - 2, -1, -1):
        p = stack[i]
        kd = p.kind
        if kd == K.CASE_STMT:
            return "case label"
        if kd in (K.BINARY_OPERATOR, K.COMPOUND_ASSIGNMENT_OPERATOR):
            o = op_of(p)
            if o in CMP:
                return "comparison"
            if o == "=":
                return "assignment"
            if o in ARITH or o.rstrip("=") in ARITH:
                return "arithmetic"
            return "other operator"
        if kd == K.ARRAY_SUBSCRIPT_EXPR:
            return "array index"
        if kd in (K.CALL_EXPR,):
            return "call argument"
        if kd == K.INIT_LIST_EXPR:
            return "aggregate initialiser"
        if kd == K.VAR_DECL:
            return "variable initialiser"
        if kd == K.FIELD_DECL:
            return "field initialiser"
        if kd == K.RETURN_STMT:
            return "return value"
        if kd in (K.IF_STMT, K.WHILE_STMT, K.DO_STMT, K.FOR_STMT):
            return "bare condition"
        if kd == K.UNARY_OPERATOR:
            continue
        if kd in (K.PAREN_EXPR, K.UNEXPOSED_EXPR, K.CSTYLE_CAST_EXPR,
                  K.CXX_STATIC_CAST_EXPR, K.CXX_REINTERPRET_CAST_EXPR,
                  K.CXX_FUNCTIONAL_CAST_EXPR, K.CXX_CONST_CAST_EXPR):
            continue
        if kd == K.ENUM_CONSTANT_DECL:
            return "enumerator value"
        if kd in (K.CONDITIONAL_OPERATOR,):
            return "conditional arm"
    return "other"


def main():
    db = json.loads(CDB.read_text())
    index = cidx.Index.create()
    bare: dict[tuple, tuple] = {}
    named: dict[tuple, str] = {}
    parsed = failed = 0

    for entry in db:
        src = Path(entry["file"])
        try:
            tu = index.parse(str(src), args=flags_for(entry))
        except cidx.TranslationUnitLoadError:
            failed += 1
            continue
        parsed += 1

        stack: list = []

        def walk(node):
            stack.append(node)
            if node.kind == K.INTEGER_LITERAL:
                r = rel(node)
                if r:
                    ln, col = node.location.line, node.location.column
                    if not LABEL_RE.match(line_text(r, ln)):
                        toks = [t.spelling for t in node.get_tokens()]
                        txt = toks[0] if toks else ""
                        bare[(r, ln, col)] = (classify(stack), txt)
            elif node.kind == K.DECL_REF_EXPR:
                d = node.referenced
                if d is not None and d.kind == K.ENUM_CONSTANT_DECL:
                    r = rel(node)
                    if r:
                        named[(r, node.location.line, node.location.column)] = d.spelling
            for c in node.get_children():
                walk(c)
            stack.pop()

        walk(tu.cursor)

    out = REPO / "build/gen/constant_census.json"
    if len(sys.argv) > 1:
        out = Path(sys.argv[1])
    out.parent.mkdir(parents=True, exist_ok=True)
    json.dump(
        {"bare": {"|".join(map(str, k)): v for k, v in bare.items()},
         "named": {"|".join(map(str, k)): v for k, v in named.items()},
         "parsed": parsed, "failed": failed},
        out.open("w"))
    report(bare, named)
    print(f"\nwrote {out}  (parsed {parsed} TU(s), {failed} failed)")


DATA_CTX = {"aggregate initialiser", "enumerator value"}
DECIDE_CTX = {"case label", "comparison", "bare condition", "conditional arm"}


def _val(t):
    try:
        return int(t, 0)
    except ValueError:
        return None


def report(bare, named):
    """The split that matters: most bare literals are not names waiting to
    happen. A macro-expanded literal is ALREADY named (NULL, WM_*, the SDK) -
    libclang reports it at the invocation with no tokens, which is how they are
    told apart here."""
    macro = {k: v for k, v in bare.items() if v[1] == ""}
    real = {k: v for k, v in bare.items() if v[1] != ""}
    buckets = collections.Counter()
    for ctx, txt in real.values():
        n = _val(txt)
        if ctx in DATA_CTX:
            buckets["data tables / enum bodies"] += 1
        elif n in (0, 1):
            buckets["0 or 1 (null, bool, empty, first)"] += 1
        elif ctx in DECIDE_CTX:
            buckets["decision constant >=2  <-- NAMEABLE"] += 1
        elif ctx == "arithmetic":
            buckets["arithmetic (shift, stride, mask)"] += 1
        elif ctx == "array index":
            buckets["array index"] += 1
        else:
            buckets["quantity in a call/assign/return/init"] += 1
    tot = sum(buckets.values())
    assert tot == len(real), (tot, len(real))
    for k, v in buckets.most_common():
        print(f"  {v:7d}  {100 * v / tot:5.1f}%  {k}")
    print(f"  {tot:7d}  100.0%  TOTAL genuinely bare")
    print(f"  {len(macro):7d}          + macro-expanded (already named)")
    print(f"  {len(named):7d}          + enumerator references (named domains)")


if __name__ == "__main__":
    main()
