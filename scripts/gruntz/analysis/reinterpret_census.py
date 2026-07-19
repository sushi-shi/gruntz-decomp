"""reinterpret_census.py - the FULL reinterpret_cast inventory (task #8, user goal
2026-07-19: every one of the ~4.6k sites gets a bucket verdict; view-vector buckets
drive to 0, only generic-collection + raw-data keeps stay).

Buckets (definitional, full clang-cl AST like cleanliness_ast):
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

Run: python -m gruntz.analysis.reinterpret_census [--out FILE]
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

        def visit(node):
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
                            counts[b] += 1
                            rows.append((b, "%s:%d" % (rel, off), frm, to))
            for ch in node.get_children():
                visit(ch)

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
