"""cleanliness_ast.py - the AST (libclang) side of the cleanliness board: metrics that are
SYNTACTIC are counted definitionally here instead of by regex, because the regex versions
drifted three times (param decls counted as numeric casts; POSITION idioms counted as
launder-suspects; fn-param lists counted as string casts - see metric-regexes-drift docs).

What this counts (definitional, per unique (file, offset), src/ + include/ only):
  c-style casts        every CSTYLE_CAST_EXPR - the goal metric; 0 == the named-cast endstate
    .. by target       numeric / char* / class-ptr / other buckets
    .. this-operand    the )this equivalent (operand is CXXThisExpr)
    .. offset-casts    the cast result immediately used in +/- pointer arithmetic (the ban)
  self-casts           operand canonical type == target canonical type (redundant cast)
  void* fields         FieldDecl of type void* (the void* m_ metric, no comment/string noise)
  extern var decls     extern VarDecls without init/definition spelled in a .cpp

Lexical/naming metrics (m_<hex>, Slot/Vfunc placeholders, Unknown ids) STAY regex-based in
match/cleanliness.py - they are name conventions, not syntax. The regex cast metrics remain
there too as the fast --fast-loop ratchet; THIS tool is the slow definitional audit
(~1-2 min full parse; run it before blessing a zero).

Run: python -m gruntz.analysis.cleanliness_ast [--by-file]
"""
from __future__ import annotations
import json
import os
import sys
import collections
import clang.cindex as cidx

REPO = "/home/sheep/Projects/gruntz"
CDB_PATH = REPO + "/build/clangd/compile_commands.json"

_NUMERIC = {cidx.TypeKind.INT, cidx.TypeKind.UINT, cidx.TypeKind.LONG, cidx.TypeKind.ULONG,
            cidx.TypeKind.LONGLONG, cidx.TypeKind.ULONGLONG, cidx.TypeKind.SHORT,
            cidx.TypeKind.USHORT, cidx.TypeKind.CHAR_S, cidx.TypeKind.CHAR_U,
            cidx.TypeKind.SCHAR, cidx.TypeKind.UCHAR, cidx.TypeKind.FLOAT,
            cidx.TypeKind.DOUBLE, cidx.TypeKind.BOOL, cidx.TypeKind.ENUM}


def _cl_args_for(entry):
    """clang-cl driver-mode args (the compdb uses /c, /I...; libclang needs the
    cl driver to understand them - same recipe as scripts/cast_to_static.py)."""
    args = list(entry["arguments"])
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for a in args[1:]:
        if a == "/c" or a == src or a.endswith(src):
            continue
        out.append(a)
    return out


def _rel(cursor):
    f = cursor.extent.start.file
    if f is None:
        return None
    path = os.path.realpath(f.name)
    rel = os.path.relpath(path, REPO)
    if rel.startswith("src/") or rel.startswith("include/"):
        return rel
    return None


def _is_this(op):
    cur = op
    for _ in range(8):
        if cur is None:
            return False
        if cur.kind == cidx.CursorKind.CXX_THIS_EXPR:
            return True
        kids = list(cur.get_children())
        if cur.kind in (cidx.CursorKind.UNEXPOSED_EXPR, cidx.CursorKind.PAREN_EXPR) \
                and len(kids) == 1:
            cur = kids[0]
        else:
            return False
    return False


class Board:
    def __init__(self):
        self.seen = set()          # (rel, offset) dedup across TUs
        self.counts = collections.Counter()
        self.by_file = collections.defaultdict(collections.Counter)
        self.sites = collections.defaultdict(list)

    def add(self, key, rel, off, note=""):
        if (key, rel, off) in self.seen:
            return
        self.seen.add((key, rel, off))
        self.counts[key] += 1
        self.by_file[rel][key] += 1
        if len(self.sites[key]) < 20:
            self.sites[key].append("%s:%s %s" % (rel, off, note))


def scan_tu(tu, board):
    stack = []

    def visit(node, parent):
        rel = None
        if node.kind == cidx.CursorKind.CSTYLE_CAST_EXPR:
            rel = _rel(node)
            if rel:
                off = node.extent.start.offset
                board.add("c-style casts", rel, off)
                tgt = node.type.get_canonical()
                kids = list(node.get_children())
                op = kids[-1] if kids else None
                if tgt.kind == cidx.TypeKind.POINTER:
                    pk = tgt.get_pointee().get_canonical().kind
                    if pk in (cidx.TypeKind.CHAR_S, cidx.TypeKind.CHAR_U):
                        board.add("  target char*", rel, off)
                    elif pk == cidx.TypeKind.RECORD:
                        board.add("  target class*", rel, off)
                    else:
                        board.add("  target other-ptr", rel, off)
                elif tgt.kind in _NUMERIC:
                    board.add("  target numeric", rel, off)
                if op is not None and _is_this(op):
                    board.add("  operand this", rel, off)
                if op is not None:
                    try:
                        if op.type.get_canonical().spelling == tgt.spelling:
                            board.add("self-casts", rel, off,
                                      "(%s)" % tgt.spelling[:40])
                    except Exception:  # noqa
                        pass
                if parent is not None and parent.kind == cidx.CursorKind.BINARY_OPERATOR:
                    pk = list(parent.get_children())
                    if pk and pk[0].extent.start.offset == node.extent.start.offset:
                        toks = [t.spelling for t in parent.get_tokens()]
                        if "+" in toks or "-" in toks:
                            board.add("offset-casts", rel, off)
        elif node.kind == cidx.CursorKind.FIELD_DECL:
            t = node.type.get_canonical()
            if t.kind == cidx.TypeKind.POINTER \
                    and t.get_pointee().get_canonical().kind == cidx.TypeKind.VOID:
                rel = _rel(node)
                if rel:
                    board.add("void* fields", rel, node.extent.start.offset,
                              node.spelling)
        elif node.kind == cidx.CursorKind.VAR_DECL:
            if node.storage_class == cidx.StorageClass.EXTERN \
                    and not node.is_definition():
                rel = _rel(node)
                if rel and rel.endswith(".cpp"):
                    board.add("extern var decls (.cpp)", rel,
                              node.extent.start.offset, node.spelling)
        for ch in node.get_children():
            visit(ch, node)

    visit(tu.cursor, None)


def main():
    by_file = "--by-file" in sys.argv
    db = json.load(open(CDB_PATH))
    index = cidx.Index.create()
    board = Board()
    tus = [e for e in db if e["file"].startswith("src/")]
    for i, e in enumerate(tus):
        try:
            tu = index.parse(os.path.join(REPO, e["file"]), args=_cl_args_for(e))
            scan_tu(tu, board)
        except Exception as ex:  # noqa
            sys.stderr.write("PARSE-FAIL %s: %s\n" % (e["file"], ex))
        if (i + 1) % 40 == 0 or i + 1 == len(tus):
            sys.stderr.write("[%d/%d]\n" % (i + 1, len(tus)))
    print("=== cleanliness (AST, definitional) ===")
    order = ["c-style casts", "  target numeric", "  target char*", "  target class*",
             "  target other-ptr", "  operand this", "offset-casts", "self-casts",
             "void* fields", "extern var decls (.cpp)"]
    for k in order:
        print("%-26s %5d" % (k, board.counts.get(k, 0)))
    for k in order:
        if board.counts.get(k) and k.strip() != "void* fields" \
                and k != "extern var decls (.cpp)":
            print("--- %s sites (first %d) ---" % (k, len(board.sites[k])))
            for s in board.sites[k]:
                print("   ", s)
    if by_file:
        print("=== by file (c-style casts) ===")
        rank = sorted(board.by_file.items(),
                      key=lambda kv: -kv[1].get("c-style casts", 0))
        for rel, c in rank[:20]:
            if c.get("c-style casts"):
                print("%5d  %s" % (c["c-style casts"], rel))


if __name__ == "__main__":
    main()
