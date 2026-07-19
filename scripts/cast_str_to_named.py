#!/usr/bin/env python3
"""cast_str_to_named.py - convert C-style `(char*)` / `(const char*)` casts to the right
C++ NAMED cast via libclang, reusing cast_to_static's exact-extent logic.

Classification (docs/cast-metric-policy.md):
  - OFFSET-CAST (the cast result is immediately used in `+`/`-`/`[` pointer arithmetic):
    SKIP + report. That is the banned offset-cast - fix by MODELLING the struct member, never
    launder it into a named cast.
  - const-drop (`(char*)` of a `const char*`/`const char[]`): const_cast<char*>.
  - CString record operand (operator LPCTSTR), a char/void pointer, or a char array:
    static_cast (a value/qualification conversion).
  - int / unrelated-pointer operand: reinterpret_cast (a genuine reinterpret).

  collect CACHE.json  |  apply CACHE.json ROOT   (same workflow as cast_to_static.py)
"""
from __future__ import annotations
import json, os, sys
import clang.cindex as cidx
from cast_to_static import (REPO, CDB_PATH, _cl_args_for, _matching_paren_close,
                            _skip_ws, _cast_end_offset)

# operand kinds that a (const)char* cast treats as a VALUE/qualification -> static_cast
# (a pointer whose pointee is char, a void pointer, a char array, or a CString record).
_CHARPTR_STATIC_POINTEE = {cidx.TypeKind.CHAR_S, cidx.TypeKind.CHAR_U, cidx.TypeKind.SCHAR,
                           cidx.TypeKind.UCHAR, cidx.TypeKind.VOID}

# Pointer-sized numeric-cast targets: `(i32)ptr` etc. is a genuine pointer<->int reinterpret
# (int handle / pointer-as-int storage) -> reinterpret_cast (the goal's directive). Only these
# widths (a `(char)ptr`/`(short)ptr` would be a truncating reinterpret - skip, needs review).
_NUM_PTR_TARGETS = {'i32', 'u32', 'i64', 'u64', 'int', 'unsigned', 'long', 'unsigned int',
                    'unsigned long', 'DWORD', 'LPARAM', 'WPARAM', 'LRESULT', 'INT_PTR'}
_POINTERISH = {cidx.TypeKind.POINTER, cidx.TypeKind.CONSTANTARRAY,
               cidx.TypeKind.INCOMPLETEARRAY, cidx.TypeKind.FUNCTIONPROTO,
               cidx.TypeKind.FUNCTIONNOPROTO, cidx.TypeKind.MEMBERPOINTER}


def _classify(node, tgt_is_const):
    """Return ('static'|'reinterpret'|'const', None) or ('skip', reason)."""
    children = list(node.get_children())
    if not children:
        return ('skip', 'no-operand')
    op = children[-1]
    try:
        ot = op.type.get_canonical()
    except Exception:  # noqa
        return ('skip', 'no-type')
    k = ot.kind
    if k == cidx.TypeKind.POINTER:
        pointee = ot.get_pointee()
        pk = pointee.get_canonical().kind
        if pk in _CHARPTR_STATIC_POINTEE:
            # char*/const char*/void* -> (const)char*: const_cast iff we DROP const.
            if pointee.is_const_qualified() and not tgt_is_const:
                return ('const', None)
            return ('static', None)
        return ('reinterpret', None)  # unrelated pointer -> reinterpret
    if k == cidx.TypeKind.CONSTANTARRAY:
        et = ot.element_type.get_canonical()
        if et.kind in _CHARPTR_STATIC_POINTEE:
            if et.is_const_qualified() and not tgt_is_const:
                return ('const', None)
            return ('static', None)
        return ('reinterpret', None)
    if k == cidx.TypeKind.RECORD:
        return ('static', None)  # CString etc. via operator LPCTSTR
    if k in (cidx.TypeKind.INT, cidx.TypeKind.UINT, cidx.TypeKind.LONG, cidx.TypeKind.ULONG,
             cidx.TypeKind.LONGLONG, cidx.TypeKind.ULONGLONG, cidx.TypeKind.SHORT,
             cidx.TypeKind.USHORT, cidx.TypeKind.ENUM, cidx.TypeKind.BOOL):
        return ('reinterpret', None)  # int -> pointer reinterpret
    return ('skip', 'kind:' + str(k))


def collect_from_tu(tu_file, args, index, sink, skipped):
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
                                if ttext in ('char *', 'char*', 'const char *', 'const char*'):
                                    tgt_const = ttext.startswith('const')
                                    end = _cast_end_offset(node)
                                    sub = _skip_ws(data, tclose + 1)
                                    nxt = _skip_ws(data, end)
                                    if end > sub:
                                        if data[nxt:nxt + 1] in (b'+', b'-', b'['):
                                            rec = {'src': data[start:end].decode('latin-1'),
                                                   'reason': 'offset-cast'}
                                            skipped.setdefault(rel, {})[str(start)] = rec
                                        else:
                                            kind, reason = _classify(node, tgt_const)
                                            if kind == 'skip':
                                                skipped.setdefault(rel, {})[str(start)] = {
                                                    'src': data[start:end].decode('latin-1'),
                                                    'reason': reason}
                                            else:
                                                tt = 'const char*' if tgt_const else 'char*'
                                                sink.setdefault(rel, {})[str(start)] = {
                                                    'kind': kind, 'ttype': tt,
                                                    'type_close': tclose, 'subexpr_start': sub,
                                                    'end': end}
                                elif ttext in _NUM_PTR_TARGETS:
                                    # pointer<->int reinterpret: convert ONLY when the operand is
                                    # a pointer/array/fn VALUE. A numeric operand is a value cast
                                    # (cast_to_static -> static_cast); leave those to that tool.
                                    end = _cast_end_offset(node)
                                    sub = _skip_ws(data, tclose + 1)
                                    if end > sub:
                                        kids = list(node.get_children())
                                        opk = None
                                        if kids:
                                            try:
                                                opk = kids[-1].type.get_canonical().kind
                                            except Exception:  # noqa
                                                opk = None
                                        if opk in _POINTERISH:
                                            sink.setdefault(rel, {})[str(start)] = {
                                                'kind': 'reinterpret', 'ttype': ttext,
                                                'type_close': tclose, 'subexpr_start': sub,
                                                'end': end}
        for ch in node.get_children():
            visit(ch)
    visit(tu.cursor)


def cmd_collect(cache):
    db = json.load(open(CDB_PATH))
    index = cidx.Index.create()
    sink, skipped = {}, {}
    tus = [e for e in db if e['file'].startswith('src/')]
    for i, e in enumerate(tus):
        try:
            collect_from_tu(e['file'], _cl_args_for(e), index, sink, skipped)
        except Exception as ex:  # noqa
            sys.stderr.write('PARSE-FAIL %s: %s\n' % (e['file'], ex))
        if (i + 1) % 40 == 0 or i + 1 == len(tus):
            sys.stderr.write('[%d/%d] convert=%d skip=%d\n' % (
                i + 1, len(tus), sum(len(v) for v in sink.values()),
                sum(len(v) for v in skipped.values())))
    json.dump(sink, open(cache, 'w'), indent=0, sort_keys=True)
    json.dump(skipped, open(cache + '.skipped.json', 'w'), indent=1, sort_keys=True)
    print('convert %d sites, skip %d (offset-casts/other)' % (
        sum(len(v) for v in sink.values()), sum(len(v) for v in skipped.values())))


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
