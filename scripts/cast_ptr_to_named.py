#!/usr/bin/env python3
"""cast_ptr_to_named.py - convert C-style POINTER casts `(CFoo*)expr` / `(void*)expr` /
`(u8*)expr` to the right C++ NAMED cast via libclang, reusing cast_str/cast_to_static extent
logic. (The two exact char* spellings `(char*)`/`(const char*)` stay with cast_str_to_named.)

Byte-neutrality is BY CONSTRUCTION: a C-style `(T*)x` already resolves to the FIRST valid of
const_cast -> static_cast -> reinterpret_cast, so emitting that SAME named cast lowers to the
identical MSVC5 /O2 code. The classifier reproduces that choice from the operand's real type:
  - const-drop only (same pointee mod cv)                        -> const_cast
  - operand void*  (or target void*)                             -> static_cast (standard conv)
  - operand & target record pointees hierarchy-related (up/down) -> static_cast
  - operand unrelated pointer / int / enum / function            -> reinterpret_cast
  - operand `this`                                    -> routed to the .this.json sink (a
    downcast/cross-cast of `this` is usually a mis-modelled base - REVIEW for real inheritance
    before accepting the named cast; see no-sane-dev-test).
  - cast result used in `+`/`-` pointer arithmetic (offset-cast) -> skipped (model the member).

  collect CACHE.json  |  apply CACHE.json ROOT   (same workflow as cast_str_to_named.py)
"""
from __future__ import annotations
import json, os, sys
import clang.cindex as cidx
from cast_to_static import (REPO, CDB_PATH, _cl_args_for, _matching_paren_close,
                            _skip_ws, _cast_end_offset)

_INTISH = {cidx.TypeKind.INT, cidx.TypeKind.UINT, cidx.TypeKind.LONG, cidx.TypeKind.ULONG,
           cidx.TypeKind.LONGLONG, cidx.TypeKind.ULONGLONG, cidx.TypeKind.SHORT,
           cidx.TypeKind.USHORT, cidx.TypeKind.ENUM, cidx.TypeKind.BOOL,
           cidx.TypeKind.CHAR_S, cidx.TypeKind.CHAR_U, cidx.TypeKind.SCHAR, cidx.TypeKind.UCHAR}
_PTRISH = {cidx.TypeKind.POINTER, cidx.TypeKind.CONSTANTARRAY, cidx.TypeKind.INCOMPLETEARRAY,
           cidx.TypeKind.FUNCTIONPROTO, cidx.TypeKind.FUNCTIONNOPROTO}
_UNWRAP = {cidx.CursorKind.UNEXPOSED_EXPR, cidx.CursorKind.PAREN_EXPR}


def _usr(c):
    try:
        return c.get_usr() if c is not None else None
    except Exception:  # noqa
        return None


def _bases(decl, seen):
    for ch in decl.get_children():
        if ch.kind == cidx.CursorKind.CXX_BASE_SPECIFIER:
            d = ch.type.get_declaration()
            u = _usr(d)
            if d is not None and u and u not in seen:
                seen.add(u)
                yield d
                yield from _bases(d, seen)


def _hier_related(a_decl, b_decl):
    """True if a==b or one is a (transitive) base of the other."""
    au, bu = _usr(a_decl), _usr(b_decl)
    if not au or not bu:
        return False
    if au == bu:
        return True
    for base in _bases(a_decl, set()):
        if _usr(base) == bu:
            return True
    for base in _bases(b_decl, set()):
        if _usr(base) == au:
            return True
    return False


def _unwrap_this(op):
    """Descend through paren/implicit nodes; return True if the operand is `this`."""
    cur = op
    for _ in range(8):
        if cur is None:
            return False
        if cur.kind == cidx.CursorKind.CXX_THIS_EXPR:
            return True
        kids = list(cur.get_children())
        if cur.kind in _UNWRAP and len(kids) == 1:
            cur = kids[0]
        else:
            return False
    return False


def _classify_ptr(node, tgt_type, tgt_const):
    """Return (kind, is_this) where kind in static|reinterpret|const|skip:reason."""
    children = list(node.get_children())
    if not children:
        return ('skip:no-operand', False)
    op = children[-1]
    is_this = _unwrap_this(op)
    try:
        ot = op.type.get_canonical()
    except Exception:  # noqa
        return ('skip:no-type', is_this)
    k = ot.kind
    tgt_pointee = tgt_type.get_pointee() if tgt_type.kind == cidx.TypeKind.POINTER else None
    tgt_is_void = tgt_pointee is not None and tgt_pointee.get_canonical().kind == cidx.TypeKind.VOID
    if k == cidx.TypeKind.POINTER:
        op_pointee = ot.get_pointee()
        opk = op_pointee.get_canonical().kind
        # const-drop only: identical pointee type, operand const, target not
        if (tgt_pointee is not None
                and op_pointee.get_canonical().spelling.replace('const ', '')
                == tgt_pointee.get_canonical().spelling.replace('const ', '')
                and op_pointee.is_const_qualified() and not tgt_const):
            return ('const', is_this)
        if opk == cidx.TypeKind.VOID or tgt_is_void:
            return ('static', is_this)   # void*<->T* is a standard (static) conversion
        if (opk == cidx.TypeKind.RECORD and tgt_pointee is not None
                and tgt_pointee.get_canonical().kind == cidx.TypeKind.RECORD):
            if _hier_related(op_pointee.get_declaration(),
                             tgt_pointee.get_declaration()):
                return ('static', is_this)   # up/down-cast within one hierarchy
        return ('reinterpret', is_this)      # unrelated object pointers
    if k in _PTRISH:                         # array / function operand
        return ('static' if tgt_is_void else 'reinterpret', is_this)
    if k in _INTISH:
        return ('reinterpret', is_this)      # int/enum -> pointer
    return ('skip:kind:' + str(k), is_this)


def _accept_target(ttext):
    """A pointer target we own: ends with '*', not the two char* spellings cast_str handles."""
    t = ttext.strip()
    if not t.endswith('*'):
        return False
    if t in ('char *', 'char*', 'const char *', 'const char*'):
        return False
    return True


def collect_from_tu(tu_file, args, index, sink, this_sink, skipped):
    tu = index.parse(os.path.join(REPO, tu_file), args=args)
    filecache = {}

    def rawbytes(path):
        if path not in filecache:
            try:
                filecache[path] = open(path, 'rb').read()
            except OSError:
                filecache[path] = None
        return filecache[path]

    def visit(node):
        if node.kind == cidx.CursorKind.CSTYLE_CAST_EXPR:
            loc = node.extent.start
            f = loc.file
            if f is not None:
                path = os.path.realpath(f.name)
                rel = os.path.relpath(path, REPO)
                if rel.startswith('src/') or rel.startswith('include/'):
                    data = rawbytes(path)
                    if data is not None:
                        start = node.extent.start.offset
                        if data[start:start + 1] == b'(':
                            try:
                                tclose = _matching_paren_close(data, start)
                            except ValueError:
                                tclose = -1
                            if tclose > start:
                                ttext = data[start + 1:tclose].decode('latin-1').strip()
                                if _accept_target(ttext):
                                    tgt_const = ttext.startswith('const')
                                    end = _cast_end_offset(node)
                                    sub = _skip_ws(data, tclose + 1)
                                    nxt = _skip_ws(data, end)
                                    if end > sub:
                                        if data[nxt:nxt + 1] in (b'+', b'-'):
                                            skipped.setdefault(rel, {})[str(start)] = {
                                                'src': data[start:end].decode('latin-1'),
                                                'reason': 'offset-cast'}
                                        else:
                                            kind, is_this = _classify_ptr(
                                                node, node.type.get_canonical(), tgt_const)
                                            rec = {'src': data[start:end].decode('latin-1'),
                                                   'kind': kind, 'ttype': ttext,
                                                   'subexpr_start': sub, 'end': end}
                                            if kind.startswith('skip'):
                                                skipped.setdefault(rel, {})[str(start)] = rec
                                            elif is_this:
                                                this_sink.setdefault(rel, {})[str(start)] = rec
                                            else:
                                                sink.setdefault(rel, {})[str(start)] = rec
        for ch in node.get_children():
            visit(ch)
    visit(tu.cursor)


def cmd_collect(cache):
    db = json.load(open(CDB_PATH))
    index = cidx.Index.create()
    sink, this_sink, skipped = {}, {}, {}
    tus = [e for e in db if e['file'].startswith('src/')]
    for i, e in enumerate(tus):
        try:
            collect_from_tu(e['file'], _cl_args_for(e), index, sink, this_sink, skipped)
        except Exception as ex:  # noqa
            sys.stderr.write('PARSE-FAIL %s: %s\n' % (e['file'], ex))
        if (i + 1) % 40 == 0 or i + 1 == len(tus):
            sys.stderr.write('[%d/%d] member=%d this=%d skip=%d\n' % (
                i + 1, len(tus), sum(len(v) for v in sink.values()),
                sum(len(v) for v in this_sink.values()),
                sum(len(v) for v in skipped.values())))
    json.dump(sink, open(cache, 'w'), indent=0, sort_keys=True)
    json.dump(this_sink, open(cache + '.this.json', 'w'), indent=1, sort_keys=True)
    json.dump(skipped, open(cache + '.skipped.json', 'w'), indent=1, sort_keys=True)
    print('member casts %d, this casts %d, skipped %d' % (
        sum(len(v) for v in sink.values()), sum(len(v) for v in this_sink.values()),
        sum(len(v) for v in skipped.values())))


def cmd_apply(cache, root):
    sink = json.load(open(cache))
    root = root.rstrip('/')
    nf = ns = 0
    for rel in sorted(sink):
        if not (rel == root or rel.startswith(root + '/')):
            continue
        path = os.path.join(REPO, rel)
        data = open(path, 'rb').read()
        edits = []
        for s, info in sink[rel].items():
            start = int(s)
            if data[start:start + 1] != b'(':
                continue
            kw = {'static': 'static_cast', 'reinterpret': 'reinterpret_cast',
                  'const': 'const_cast'}[info['kind']]
            repl = ('%s<%s>(' % (kw, info['ttype'])).encode('latin-1')
            edits.append((start, info['subexpr_start'] - start, repl))
            edits.append((info['end'], 0, b')'))
        if not edits:
            continue
        edits.sort(key=lambda e: (e[0], e[1]), reverse=True)
        for pos, dl, ins in edits:
            data = data[:pos] + ins + data[pos + dl:]
        open(path, 'wb').write(data)
        nf += 1
        ns += len(sink[rel])
    print('apply %s: %d sites in %d files' % (root, ns, nf))


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(__doc__)
    elif sys.argv[1] == 'collect':
        cmd_collect(sys.argv[2])
    elif sys.argv[1] == 'apply':
        cmd_apply(sys.argv[2], sys.argv[3])
    else:
        print('unknown cmd')
