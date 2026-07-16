#!/usr/bin/env python3
"""cast_to_static.py — convert C-style NUMERIC casts to static_cast<T>() via libclang AST.

The cleanliness metric "C-style numeric casts" counts textual occurrences of
`(i8|i16|i32|i64|u8|u16|u32|u64|float|double|char|short|int|long|unsigned)` — i.e. a
single-token arithmetic type in parens. This tool rewrites exactly those, but only when
the AST confirms the token is a real `CStyleCastExpr` (not a `f(int)` param decl, not a
pointer cast `(char*)`, not a macro-def token), and uses the AST cast extent to place the
closing paren at the correct tight-binding boundary:  `(i32)a + b` -> `static_cast<i32>(a) + b`.

Byte-neutral under MSVC5 /O2: static_cast<T>(x) lowers identically to (T)x.

Workflow:
  python3 scripts/cast_to_static.py selftest            # verify range-end convention
  python3 scripts/cast_to_static.py collect  CACHE.json  # parse all src TUs -> cast-site cache
  python3 scripts/cast_to_static.py apply CACHE.json ROOT # rewrite files under ROOT (e.g. src/Gruntz)
"""
from __future__ import annotations

import json
import os
import sys

import clang.cindex as cidx

REPO = "/home/sheep/Projects/gruntz"
CDB_PATH = REPO + "/build/clangd/compile_commands.json"

# The exact single-token types the metric counts. A cast whose parenthesized type text,
# after stripping surrounding whitespace, is one of these -> convert.
TOKENS = {
    "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
    "float", "double", "char", "short", "int", "long", "unsigned",
}

# Operand-type kinds that a static_cast<numeric>(x) accepts: genuine value/numeric
# conversions. A pointer/array/function/member-ptr operand cast to a numeric type is a
# POINTER->INT REINTERPRET, not a value cast - it's fake-view rot (a mis-modelled type,
# per docs/cast-metric-policy.md case 1) and must be fixed by real typing, NOT laundered
# into static_cast/reinterpret_cast. Those we SKIP + report, never rewrite.
_NUMERIC_KINDS = {
    cidx.TypeKind.BOOL, cidx.TypeKind.CHAR_U, cidx.TypeKind.UCHAR,
    cidx.TypeKind.CHAR16, cidx.TypeKind.CHAR32, cidx.TypeKind.USHORT,
    cidx.TypeKind.UINT, cidx.TypeKind.ULONG, cidx.TypeKind.ULONGLONG,
    cidx.TypeKind.UINT128, cidx.TypeKind.CHAR_S, cidx.TypeKind.SCHAR,
    cidx.TypeKind.WCHAR, cidx.TypeKind.SHORT, cidx.TypeKind.INT,
    cidx.TypeKind.LONG, cidx.TypeKind.LONGLONG, cidx.TypeKind.INT128,
    cidx.TypeKind.FLOAT, cidx.TypeKind.DOUBLE, cidx.TypeKind.LONGDOUBLE,
    cidx.TypeKind.ENUM,
}


def _cl_args_for(entry: dict) -> list[str]:
    """Turn a clang-cl compile_commands entry into libclang parse args (cl driver mode)."""
    args = list(entry["arguments"])
    # drop argv[0] (clang-cl), the /c flag, and the source-file positional
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for a in args[1:]:
        if a == "/c":
            continue
        if a == src or a.endswith(src):
            continue
        out.append(a)
    return out


def _matching_paren_close(data: bytes, open_off: int) -> int:
    """Given data[open_off]=='(', return the offset of the matching ')'."""
    assert data[open_off:open_off + 1] == b"(", (open_off, data[open_off:open_off + 4])
    depth = 0
    i = open_off
    n = len(data)
    while i < n:
        c = data[i:i + 1]
        if c == b"(":
            depth += 1
        elif c == b")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unbalanced paren at %d" % open_off)


def _skip_ws(data: bytes, i: int) -> int:
    n = len(data)
    while i < n and data[i:i + 1] in (b" ", b"\t", b"\n", b"\r"):
        i += 1
    return i


def _cast_end_offset(c) -> int:
    """End offset (one-past-last-char) of the whole cast expression, via its last token.

    Robust to the SourceRange.end convention: we take the last token of the cursor and
    use start-of-token + len(spelling)."""
    last = None
    for t in c.get_tokens():
        last = t
    if last is None:
        return c.extent.end.offset
    return last.extent.start.offset + len(last.spelling)


def _operand_kind(node):
    """Canonical TypeKind of the cast's value operand (last child), or None."""
    children = list(node.get_children())
    if not children:
        return None
    op = children[-1]
    try:
        return op.type.get_canonical().kind
    except Exception:  # noqa
        return None


def collect_from_tu(tu_file: str, args: list[str], index, sink: dict, skipped: dict):
    tu = index.parse(os.path.join(REPO, tu_file), args=args,
                     options=cidx.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES * 0)
    # cache raw bytes per file we touch (only src/ + include/ under REPO)
    filecache: dict[str, bytes] = {}

    def rawbytes(path: str) -> bytes | None:
        if path in filecache:
            return filecache[path]
        try:
            with open(path, "rb") as f:
                b = f.read()
        except OSError:
            b = None
        filecache[path] = b
        return b

    def visit(node):
        if node.kind == cidx.CursorKind.CSTYLE_CAST_EXPR:
            loc = node.extent.start
            f = loc.file
            if f is not None:
                path = os.path.realpath(f.name)
                rel = os.path.relpath(path, REPO)
                if (rel.startswith("src/") or rel.startswith("include/")):
                    data = rawbytes(path)
                    if data is not None:
                        start = node.extent.start.offset
                        # guard: the byte at start must be '(' of the (T) token
                        if data[start:start + 1] == b"(":
                            try:
                                tclose = _matching_paren_close(data, start)
                            except ValueError:
                                tclose = -1
                            if tclose > start:
                                ttext = data[start + 1:tclose].decode("latin-1").strip()
                                if ttext in TOKENS:
                                    end = _cast_end_offset(node)
                                    subexpr_start = _skip_ws(data, tclose + 1)
                                    if end > subexpr_start:
                                        okind = _operand_kind(node)
                                        rec = {
                                            "type": ttext,
                                            "type_close": tclose,        # offset of ')' of (T)
                                            "subexpr_start": subexpr_start,
                                            "end": end,
                                            "opkind": str(okind),
                                        }
                                        if okind in _NUMERIC_KINDS:
                                            sink.setdefault(rel, {})[start] = rec
                                        else:
                                            # pointer/array/fn/member-ptr/unresolved operand:
                                            # fake-view rot -> fix by real typing, never rewrite
                                            rec["src"] = data[start:end].decode("latin-1")
                                            skipped.setdefault(rel, {})[start] = rec
        for ch in node.get_children():
            visit(ch)

    visit(tu.cursor)
    return tu.diagnostics


def cmd_collect(cache_path: str):
    db = json.load(open(CDB_PATH))
    index = cidx.Index.create()
    sink: dict[str, dict] = {}
    skipped: dict[str, dict] = {}
    tus = [e for e in db if e["file"].startswith("src/")]
    total = len(tus)
    for i, e in enumerate(tus):
        args = _cl_args_for(e)
        try:
            collect_from_tu(e["file"], args, index, sink, skipped)
        except Exception as ex:  # noqa
            sys.stderr.write("PARSE-FAIL %s: %s\n" % (e["file"], ex))
        if (i + 1) % 25 == 0 or i + 1 == total:
            nsites = sum(len(v) for v in sink.values())
            nskip = sum(len(v) for v in skipped.values())
            sys.stderr.write("[%d/%d] files-with-casts=%d convert=%d skip-ptr=%d\n"
                             % (i + 1, total, len(sink), nsites, nskip))
            sys.stderr.flush()
    with open(cache_path, "w") as f:
        json.dump(sink, f, indent=0, sort_keys=True)
    with open(cache_path + ".skipped.json", "w") as f:
        json.dump(skipped, f, indent=1, sort_keys=True)
    nsites = sum(len(v) for v in sink.values())
    nskip = sum(len(v) for v in skipped.values())
    print("collected %d convertible sites across %d files -> %s" % (nsites, len(sink), cache_path))
    print("skipped %d pointer/unresolved-operand sites (fake-view rot) -> %s.skipped.json"
          % (nskip, cache_path))


def cmd_apply(cache_path: str, root: str):
    sink = json.load(open(cache_path))
    root = root.rstrip("/")
    changed_files = 0
    changed_sites = 0
    for rel in sorted(sink):
        if not (rel == root or rel.startswith(root + "/")):
            continue
        sites = sink[rel]  # {start_off(str): {...}}
        path = os.path.join(REPO, rel)
        with open(path, "rb") as f:
            data = f.read()
        # build edit list; apply descending by offset so earlier offsets are stable
        edits = []  # (pos, del_len, insert_bytes)
        for start_str, info in sites.items():
            start = int(start_str)
            tclose = info["type_close"]
            end = info["end"]
            ttype = info["type"]
            subexpr_start = info["subexpr_start"]
            # re-verify against current bytes (defensive; the file must be unmodified)
            if data[start:start + 1] != b"(":
                sys.stderr.write("SKIP %s@%d: not '(' (got %r)\n"
                                 % (rel, start, data[start:start + 4]))
                continue
            inner = data[start + 1:tclose].decode("latin-1").strip()
            if inner != ttype:
                sys.stderr.write("SKIP %s@%d: type text drift %r!=%r\n"
                                 % (rel, start, inner, ttype))
                continue
            # replace [start, subexpr_start) -> "static_cast<T>("
            repl = ("static_cast<%s>(" % ttype).encode("latin-1")
            edits.append((start, subexpr_start - start, repl))
            # insert ")" at end
            edits.append((end, 0, b")"))
        if not edits:
            continue
        # apply: sort by pos desc; for equal pos, insertions (del_len 0) keep any order
        edits.sort(key=lambda e: (e[0], e[1]), reverse=True)
        for pos, dlen, ins in edits:
            data = data[:pos] + ins + data[pos + dlen:]
        with open(path, "wb") as f:
            f.write(data)
        changed_files += 1
        changed_sites += len(sites)
    print("apply root=%s: rewrote %d sites in %d files" % (root, changed_sites, changed_files))


SELFTEST_SRC = b"""
typedef int i32;
typedef unsigned u32;
int g(int);
int f(int a, int b, double d) {
    int x = (i32)a + b;
    int y = (i32)(a + b);
    unsigned z = (u32)a;
    double w = (double)(u32)a;
    int q = (int)d;
    int r = g(3);
    char c = (char)(a + 1);
    return x + y + z + (int)w + q + r + c;
}
"""


def cmd_selftest():
    index = cidx.Index.create()
    tu = index.parse("t.cpp", args=["--driver-mode=cl", "/TP"],
                     unsaved_files=[("t.cpp", SELFTEST_SRC.decode())])
    data = SELFTEST_SRC
    found = []

    def visit(node):
        if node.kind == cidx.CursorKind.CSTYLE_CAST_EXPR:
            start = node.extent.start.offset
            tclose = _matching_paren_close(data, start)
            ttext = data[start + 1:tclose].decode().strip()
            end = _cast_end_offset(node)
            sub = _skip_ws(data, tclose + 1)
            found.append((start, ttext, sub, end,
                          data[start:end].decode()))
        for ch in node.get_children():
            visit(ch)

    visit(tu.cursor)
    print("diagnostics:")
    for d in tu.diagnostics:
        print("  ", d)
    print("casts found (start, type, subexpr_start, end, source-span):")
    for s, t, sub, e, span in sorted(found):
        # simulate rewrite of a single isolated span
        print("  %-4d %-7s sub=%d end=%d  span=%r" % (s, t, sub, e, span))
    # simulate full-file rewrite
    edits = []
    for s, t, sub, e, _ in found:
        tclose = _matching_paren_close(data, s)
        edits.append((s, sub - s, ("static_cast<%s>(" % t).encode()))
        edits.append((e, 0, b")"))
    edits.sort(key=lambda x: (x[0], x[1]), reverse=True)
    out = data
    for pos, dlen, ins in edits:
        out = out[:pos] + ins + out[pos + dlen:]
    print("\n=== rewritten ===")
    print(out.decode())


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    cmd = sys.argv[1]
    if cmd == "selftest":
        cmd_selftest()
    elif cmd == "collect":
        cmd_collect(sys.argv[2])
    elif cmd == "apply":
        cmd_apply(sys.argv[2], sys.argv[3])
    else:
        print("unknown cmd", cmd)
        sys.exit(2)


if __name__ == "__main__":
    main()
