"""reinterpret_census.py - the FULL reinterpret_cast inventory (task #8, user goal
2026-07-19: every one of the ~4.6k sites gets a bucket verdict; view-vector buckets
drive to 0, only generic-collection + raw-data keeps stay).

Buckets (definitional, full clang-cl AST like the archived cleanliness_ast):
  cross-class    record* -> unrelated record*   (the VIEW VECTOR - drive to 0)
  hier-related   record* -> base/derived         (should be static_cast - convert)
  collection     POSITION / MFC node internals   (allowed keep)
  from-void      void* -> T*                     (container/opaque payloads - mostly keeps;
                                                  the operand's ORIGIN decides, see sites)
  to-void        T* -> void*/void**              (ABI slots)
  char-arith     char*/u8* source or target      (SUSPECT: array-walks with record strides
                                                  hide typed arrays - see the RasterVtx 0x1c
                                                  walks; each needs a stride check)
  fn-ptr         function-pointer type involved  (SUSPECT: manual virtual dispatch - see the
                                                  raster slot-0x80/4 calls; model as virtuals)
  handle         HWND/HANDLE-family target       (SUSPECT: wrong-typed globals/args - see the
                                                  g_previewMgr/hDlg fix, commit 177c74ca2)
  int-ptr        integer <-> pointer             (cookies/ids; check each origin)
  other          everything else

Run: python -m gruntz.audit.reinterpret_census [--out FILE]
Writes the per-site TSV worklist (bucket, file:offset, from -> to) to
build/gen/reinterpret_census.tsv by default; prints the bucket totals.
"""
from __future__ import annotations
import json
import os
import sys
import collections
import clang.cindex as cidx

REPO = "/home/sheep/Projects/gruntz"
CDB_PATH = REPO + "/build/clangd/compile_commands.json"
OUT_DEFAULT = REPO + "/build/gen/reinterpret_census.tsv"

_COLL = {"__POSITION"}
# Runtime-generic container classes: their storage fields (m_alloc/m_base/m_cur/
# m_spare) hold stride-typed rows - the stride is a RUNTIME field (_zvec +0x18), so
# no compile-time type exists for the rows and every facet cast off them is the
# generic-collection keep the goal allows (like POSITION). zDArray<T> instances
# exist in retail RTTI, but the multi-role singletons (g_typeColl/CActReg cells)
# are used at several strides - by construction generic.
_ZVEC_FAMILY = {"_zvec", "_zdvec", "zDArray", "CActReg"}
_ZVEC_FIELDS = {"m_alloc", "m_base", "m_cur", "m_spare"}


def _zvec_origin(op, tainted=None):
    """True if the operand expression reads a _zvec-family storage field, or (one
    dataflow hop) a local whose initializer/assignment did (`tainted` = the USR set
    of such locals, collected per function body)."""
    stack = [op]
    depth = 0
    while stack and depth < 400:
        depth += 1
        n = stack.pop()
        if n.kind == cidx.CursorKind.MEMBER_REF_EXPR and n.spelling in _ZVEC_FIELDS:
            d = n.referenced
            par = d.semantic_parent if d is not None else None
            if par is not None and par.spelling in _ZVEC_FAMILY:
                return True
        if tainted and n.kind == cidx.CursorKind.DECL_REF_EXPR:
            d = n.referenced
            if d is not None and d.get_usr() in tainted:
                return True
        stack.extend(n.get_children())
    return False


def _collect_tainted(fn_body):
    """USRs of locals initialized or assigned from _zvec storage fields."""
    out = set()
    stack = [fn_body]
    depth = 0
    while stack and depth < 20000:
        depth += 1
        n = stack.pop()
        if n.kind == cidx.CursorKind.VAR_DECL:
            kids = list(n.get_children())
            if kids and _zvec_origin(kids[-1]):
                out.add(n.get_usr())
        elif n.kind == cidx.CursorKind.BINARY_OPERATOR:
            kids = list(n.get_children())
            if len(kids) == 2 and kids[0].kind == cidx.CursorKind.DECL_REF_EXPR \
                    and _zvec_origin(kids[1]):
                d = kids[0].referenced
                if d is not None and d.kind == cidx.CursorKind.VAR_DECL:
                    out.add(d.get_usr())
        stack.extend(n.get_children())
    return out
_HANDLE = {"HWND__", "HINSTANCE__", "HANDLE", "HDC__", "HBITMAP__", "HBRUSH__",
           "HICON__", "HMENU__", "HKEY__", "HFONT__", "HPALETTE__", "HRGN__"}


def _cl_args_for(entry):
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


_CHARLIKE = {cidx.TypeKind.CHAR_S, cidx.TypeKind.CHAR_U, cidx.TypeKind.SCHAR,
             cidx.TypeKind.UCHAR}
_INTLIKE = {cidx.TypeKind.INT, cidx.TypeKind.UINT, cidx.TypeKind.LONG,
            cidx.TypeKind.ULONG, cidx.TypeKind.SHORT, cidx.TypeKind.USHORT,
            cidx.TypeKind.LONGLONG, cidx.TypeKind.ULONGLONG}


def _classify(tgt, opt):
    """(bucket, from-spelling, to-spelling) for canonical target/operand types."""
    tn, on = tgt.spelling, opt.spelling

    def pointee(t):
        return t.get_pointee().get_canonical() if t.kind == cidx.TypeKind.POINTER else None

    tp, op = pointee(tgt), pointee(opt)
    if (tp and tp.kind == cidx.TypeKind.FUNCTIONPROTO) or \
       (op and op.kind == cidx.TypeKind.FUNCTIONPROTO):
        return "fn-ptr", on, tn
    if tgt.kind in _INTLIKE or opt.kind in _INTLIKE:
        return "int-ptr", on, tn
    if tp is not None and tp.kind == cidx.TypeKind.RECORD and tp.get_declaration().spelling in _HANDLE:
        return "handle", on, tn
    if (tp and tp.kind in _CHARLIKE) or (op and op.kind in _CHARLIKE):
        return "char-arith", on, tn
    if op is not None and op.kind == cidx.TypeKind.VOID:
        return "from-void", on, tn
    if tp is not None and tp.kind == cidx.TypeKind.VOID:
        return "to-void", on, tn
    if tp is not None and op is not None and tp.kind == cidx.TypeKind.RECORD \
            and op.kind == cidx.TypeKind.RECORD:
        td, od = tp.get_declaration(), op.get_declaration()
        if td.spelling in _COLL or od.spelling in _COLL:
            return "collection", on, tn
        if _hier_related(td, od):
            return "hier-related", on, tn
        if td.get_definition() is None or od.get_definition() is None:
            return "cross-incomplete", on, tn
        return "cross-class", on, tn
    return "other", on, tn


def main():
    out_path = OUT_DEFAULT
    if "--out" in sys.argv:
        out_path = sys.argv[sys.argv.index("--out") + 1]
    db = json.load(open(CDB_PATH))
    index = cidx.Index.create()
    seen = set()
    counts = collections.Counter()
    rows = []
    tus = [e for e in db if e["file"].startswith("src/")]
    for i, e in enumerate(tus):
        try:
            tu = index.parse(os.path.join(REPO, e["file"]), args=_cl_args_for(e))
        except Exception as ex:  # noqa
            sys.stderr.write("PARSE-FAIL %s: %s\n" % (e["file"], ex))
            continue

        def visit(node, parent=None, tainted=None):
            if node.kind in (cidx.CursorKind.FUNCTION_DECL, cidx.CursorKind.CXX_METHOD,
                             cidx.CursorKind.CONSTRUCTOR, cidx.CursorKind.DESTRUCTOR) \
                    and node.is_definition():
                tainted = _collect_tainted(node)
            if node.kind == cidx.CursorKind.CXX_REINTERPRET_CAST_EXPR:
                rel = _rel(node)
                if rel:
                    off = node.extent.start.offset
                    if (rel, off) not in seen:
                        seen.add((rel, off))
                        kids = list(node.get_children())
                        op = kids[-1] if kids else None
                        if op is not None:
                            b, frm, to = _classify(node.type.get_canonical(),
                                                   op.type.get_canonical())
                            if b in ("cross-class", "char-walk", "char-io", "other", "int-ptr") \
                                    and _zvec_origin(op, tainted):
                                b = "collection"  # _zvec runtime-stride storage origin
                            if b == "char-arith":
                                # SPLIT: does the cast participate in pointer math?
                                # (result under +/-/[] => an array-walk SUSPECT; the
                                # operand itself being an arith expr counts too - the
                                # (T*)((char*)p + off) shape.)
                                suspect = False
                                if parent is not None and parent.kind in (
                                        cidx.CursorKind.BINARY_OPERATOR,
                                        cidx.CursorKind.ARRAY_SUBSCRIPT_EXPR):
                                    toks = [t.spelling for t in parent.get_tokens()][:24]
                                    if '+' in toks or '-' in toks or '[' in toks:
                                        suspect = True
                                if not suspect and op is not None:
                                    ok = list(op.get_children())
                                    inner = op
                                    while ok and inner.kind in (
                                            cidx.CursorKind.UNEXPOSED_EXPR,
                                            cidx.CursorKind.PAREN_EXPR):
                                        inner = ok[0]
                                        ok = list(inner.get_children())
                                    if inner.kind == cidx.CursorKind.BINARY_OPERATOR:
                                        toks = [t.spelling for t in inner.get_tokens()][:24]
                                        if '+' in toks or '-' in toks:
                                            suspect = True
                                b = "char-walk" if suspect else "char-io"
                            counts[b] += 1
                            rows.append((b, "%s:%d" % (rel, off), frm, to))
            for ch in node.get_children():
                visit(ch, node, tainted)

        visit(tu.cursor)
        if (i + 1) % 40 == 0 or i + 1 == len(tus):
            sys.stderr.write("[%d/%d]\n" % (i + 1, len(tus)))
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w") as f:
        f.write("bucket\tsite\tfrom\tto\n")
        for r in sorted(rows):
            f.write("\t".join(r) + "\n")
    print("=== reinterpret_cast census (%d sites) ===" % sum(counts.values()))
    for k, n in counts.most_common():
        print("%-14s %5d" % (k, n))
    print("worklist:", os.path.relpath(out_path, REPO))


if __name__ == "__main__":
    main()
