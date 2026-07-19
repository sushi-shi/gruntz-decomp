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

# void* fields where void* IS the recovered truth - each entry carries its proof.
# These count under "void* fields (documented keeps)", not the actionable metric.
_VOID_KEEPS = {
    # retail's own Build signature passes these as void* (11-arg __thiscall) - typing
    # them would contradict the recovered retail shape (SymTab.h doc block).
    ("CSymLeafBuilder", "m_record"), ("CSymLeafBuilder", "m_typeTag"),
    ("CSymLeafBuilder", "m_sourceStream"), ("CSymLeafBuilder", "m_valueBuf"),
    # identity cookie by design: heterogeneous object addresses used as reap keys
    # (SoundVoiceList.h "the owning buffer's address as their reap key").
    ("DSoundElem", "m_key"),
    # setter TU unmatched - typing would be a guess (Font.h doc block; unblocks when
    # the setter TU is reconstructed).
    ("FontRenderer", "m_surface"), ("FontRenderer", "m_clip"),
    # the bute-tree VALUE domain is a PROVEN variant container: g_buteTree stores int
    # ids, but ButeMgr's instances store CButeTree* sub-trees and heap CButeValue*
    # records (Find->Find chains, m_teardown + operator delete on m_value) - the i32
    # retype was TESTED 2026-07-19 and refuted by those instances. void* is the
    # container's true value type; members holding its values inherit it.
    ("CButeTreeNode", "m_value"), ("AnimWorkerObj", "m_1c"),
    ("CAnimLookupNode", "m_1c"), ("CUserLogic", "m_prevAnimSetNode"),
    ("CButeValue", "pValue"), ("TypeKeyRec", "m_4"),
    ("CHashElement", "m_record"),  # the generic hash payload (typed per-table at use)
    ("CObjNode", "m_base"),        # the intrusive-list element view slot (vptr-or-data)
    # RenderFrame's RETAIL signature is void* x4 (CImage.h 0x153790) - the ctx slot
    # inherits the ABI type.
    ("CStatzDrawable", "m_14"),
    # multi-type list/bucket payloads (proven variant at their use sites) + generic
    # container value slots + caller-unfound records - each flagged in its header.
    ("TtcNode", "m_data"), ("CObjListNode", "m_data"), ("MapElemB", "m_0"),
    ("CTrieNode", "m_10"), ("TypeKeyRec", "m_4"), ("CKeyFinder", "m_owner"),
    ("CParseSource", "m_entry"), ("CGMSound", "m_14"),
    # boundary-drain scaffolding hosts (*Views.h holding pens) - the fields die with
    # the drain campaign's identity folds, not by per-field typing.
    ("Iter118330", "pos"), ("CUserLogicOOL", "m_3c"), ("Cdb200", "m_8"),
    ("CRect118", "m_0"), ("Cb151d20", "m_1c"),
}

def _usr(d):
    try:
        return d.get_usr()
    except Exception:  # noqa
        return None


def _bases(decl, seen):
    d = decl.get_definition() or decl
    for ch in d.get_children():
        if ch.kind == cidx.CursorKind.CXX_BASE_SPECIFIER:
            d = ch.type.get_declaration()
            u = _usr(d)
            if d is not None and u and u not in seen:
                seen.add(u)
                yield d
                yield from _bases(d, seen)


def _hier_related(a, b):
    a = a.get_definition() or a
    b = b.get_definition() or b
    au, bu = _usr(a), _usr(b)
    if not au or not bu:
        return False
    if au == bu:
        return True
    for base in _bases(a, set()):
        if _usr(base) == bu:
            return True
    for base in _bases(b, set()):
        if _usr(base) == au:
            return True
    return False


# Generic-collection element types whose casts are the ALLOWED keeps (user goal
# 2026-07-19): POSITION cookies and MFC list-node internals - by-contract opaque.
_COLL_TYPES = {"__POSITION"}


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
        if len(self.sites[key]) < 400:
            self.sites[key].append("%s:%s %s" % (rel, off, note))


def scan_tu(tu, board):
    stack = []

    def visit(node, parent):
        rel = None
        if node.kind == cidx.CursorKind.CSTYLE_CAST_EXPR:
            rel = _rel(node)
            # Raw-byte test: if the file byte at the cast's expansion offset is not '(',
            # the cast is not SPELLED there - it lives inside a macro body. Our own macro
            # defs are all named (textually verified), so what remains is vendor/SDK/CRT
            # macro internals (DDERR_*, DSERR_*, mmioFOURCC, RT_*) - counted separately.
            if rel:
                try:
                    raw = open(os.path.join(REPO, rel), 'rb').read()
                except OSError:
                    raw = b''
                off0 = node.extent.start.offset
                if raw[off0:off0 + 1] != b'(':
                    board.add("vendor-macro casts", rel, off0)
                    rel = None
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
        elif node.kind == cidx.CursorKind.CXX_REINTERPRET_CAST_EXPR:
            rel = _rel(node)
            if rel:
                tgt = node.type.get_canonical()
                kids = list(node.get_children())
                op = kids[-1] if kids else None
                if op is not None and tgt.kind == cidx.TypeKind.POINTER:
                    tp = tgt.get_pointee().get_canonical()
                    ot = op.type.get_canonical()
                    if tp.kind == cidx.TypeKind.RECORD and ot.kind == cidx.TypeKind.POINTER:
                        opp = ot.get_pointee().get_canonical()
                        if opp.kind == cidx.TypeKind.RECORD:
                            td, od = tp.get_declaration(), opp.get_declaration()
                            tn, on = td.spelling, od.spelling
                            if tn in _COLL_TYPES or on in _COLL_TYPES:
                                board.add("cross-casts (collection keeps)", rel,
                                          node.extent.start.offset)
                            elif not _hier_related(td, od):
                                board.add("cross-class reinterpret casts", rel,
                                          node.extent.start.offset,
                                          "%s -> %s" % (on[:24], tn[:24]))
        elif node.kind == cidx.CursorKind.FIELD_DECL:
            t = node.type.get_canonical()
            # judge the SPELLED type: a named typedef (HANDLE, LPVOID-free spellings)
            # IS a typed member; only literal void* spellings count.
            if t.kind == cidx.TypeKind.POINTER \
                    and t.get_pointee().get_canonical().kind == cidx.TypeKind.VOID \
                    and node.type.spelling.replace(' ', '') in ('void*', 'constvoid*'):
                rel = _rel(node)
                if rel:
                    cls = node.semantic_parent.spelling if node.semantic_parent else '?'
                    key = "void* fields (documented keeps)" \
                        if (cls, node.spelling) in _VOID_KEEPS else "void* fields"
                    board.add(key, rel, node.extent.start.offset,
                              cls + "::" + node.spelling)
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
    order = ["cross-class reinterpret casts", "cross-casts (collection keeps)",
             "c-style casts", "vendor-macro casts", "  target numeric", "  target char*", "  target class*",
             "  target other-ptr", "  operand this", "offset-casts", "self-casts",
             "void* fields", "void* fields (documented keeps)", "extern var decls (.cpp)"]
    for k in order:
        print("%-26s %5d" % (k, board.counts.get(k, 0)))
    for k in order:
        if board.counts.get(k) and k.strip() != "void* fields" \
                and k != "extern var decls (.cpp)" \
                and k != "cross-casts (collection keeps)":
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
