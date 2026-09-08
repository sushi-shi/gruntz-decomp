#!/usr/bin/env python3
"""Inventory owned record families that may conceal template instantiations.

Uses the real compilation database; candidates are review prompts, not proof.
The complete record/template catalog, function definitions, call/allocation
sites, and actual VC5 COFF emissions are emitted alongside the worklist.
USR and symbol keys join these reports; empty bodies alone do not prove that
a method was compiler-generated. Run against a stable source/build snapshot.
"""
import argparse
import json
import re
import sys
from concurrent.futures import ProcessPoolExecutor
from functools import lru_cache
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))


@lru_cache(maxsize=None)
def owned(path):
    p = Path(path).resolve()
    return any(p.is_relative_to(ROOT / d) for d in ('src', 'include'))


@lru_cache(maxsize=1)
def cxx_mangling_api():
    """libclang exposes complete ctor/dtor names outside cindex's wrapper."""
    import ctypes
    import clang.cindex as ci

    class CXString(ctypes.Structure):
        _fields_ = [('data', ctypes.c_void_p), ('flags', ctypes.c_uint)]

    class CXStringSet(ctypes.Structure):
        _fields_ = [('strings', ctypes.POINTER(CXString)), ('count', ctypes.c_uint)]

    # Separate function wrappers preserve cindex's own ctypes signatures.
    lib = ctypes.CDLL(ci.conf.lib._name)
    lib.clang_Cursor_getCXXManglings.argtypes = [ci.Cursor]
    lib.clang_Cursor_getCXXManglings.restype = ctypes.POINTER(CXStringSet)
    lib.clang_getCString.argtypes = [CXString]
    lib.clang_getCString.restype = ctypes.c_char_p
    lib.clang_disposeStringSet.argtypes = [ctypes.POINTER(CXStringSet)]
    lib.clang_disposeStringSet.restype = None
    return lib


def cxx_manglings(cursor):
    lib = cxx_mangling_api()
    result = lib.clang_Cursor_getCXXManglings(cursor)
    if not result:
        return []
    try:
        return [lib.clang_getCString(result.contents.strings[i]).decode()
                for i in range(result.contents.count)]
    finally:
        lib.clang_disposeStringSet(result)


def harvest(job):
    import clang.cindex as ci
    from gruntz.tool.clang import inc_cl
    source, flags = job
    kinds = ci.CursorKind
    tu = ci.Index.create().parse(source, args=[
        '--driver-mode=cl', *[f for f in flags if f != '-fdelayed-template-parsing'],
        *inc_cl(), '/DGRUNTZ_EMIT_META', '-ferror-limit=0',
    ])
    records, templates, functions, uses = {}, {}, {}, []
    files = {source} | {str(Path(i.include.name).resolve()) for i in tu.get_includes()
                        if owned(i.include.name)}
    errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]

    function_kinds = (kinds.FUNCTION_DECL, kinds.CXX_METHOD, kinds.CONSTRUCTOR,
                      kinds.DESTRUCTOR, kinds.FUNCTION_TEMPLATE)

    def walk(c, caller=None):
        if (c.kind in (*function_kinds, kinds.CALL_EXPR, kinds.CXX_NEW_EXPR)
                and c.location.file and owned(c.location.file.name)):
            site = dict(file=str(Path(c.location.file.name).resolve()), line=c.location.line)
            if c.kind in function_kinds and c.is_definition():
                caller = c.get_usr()
                children = list(c.get_children())
                body = next((x for x in children if x.kind == kinds.COMPOUND_STMT), None)
                tokens = [t.spelling for t in body.get_tokens()] if body else []
                symbol = c.mangled_name
                variants = cxx_manglings(c) if symbol and c.kind in (
                    kinds.CONSTRUCTOR, kinds.DESTRUCTOR) else []
                functions[caller] = dict(
                    usr=caller, name=c.displayname, owner=c.semantic_parent.displayname,
                    owner_usr=c.semantic_parent.get_usr(), kind=c.kind.name,
                    clang_symbol=symbol, symbol=variants[0] if variants else symbol,
                    variants=variants, annotations=[x.spelling for x in children
                        if x.kind == kinds.ANNOTATE_ATTR],
                    empty_body=tokens == ['{', '}'],
                    body_tokens=len(tokens), **site)
            if c.kind in (kinds.CALL_EXPR, kinds.CXX_NEW_EXPR):
                ref = c.referenced
                uses.append(dict(caller=caller, kind=c.kind.name,
                    type=c.type.get_canonical().spelling,
                    callee=ref.get_usr() if ref else None,
                    name=ref.displayname if ref else c.displayname, **site))
        if (c.kind in (kinds.CLASS_DECL, kinds.STRUCT_DECL, kinds.CLASS_TEMPLATE,
                       kinds.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION)
                and c.is_definition() and c.location.file):
            path = str(Path(c.location.file.name).resolve())
            children = list(c.get_children())
            bases = [x.type.spelling for x in children if x.kind == kinds.CXX_BASE_SPECIFIER]
            methods = [x.spelling for x in children if x.kind in (
                kinds.CXX_METHOD, kinds.CONSTRUCTOR, kinds.DESTRUCTOR)]
            fields = [{'name': x.spelling, 'type': x.type.get_canonical().spelling,
                       'offset': x.get_field_offsetof(), 'size': x.type.get_size()}
                      for x in children if x.kind == kinds.FIELD_DECL]
            row = dict(name=c.displayname, file=path, line=c.location.line,
                       owned=owned(path), bases=bases, methods=methods, fields=fields,
                       size=c.type.get_size(), kind=c.kind.name)
            if c.kind in (kinds.CLASS_TEMPLATE, kinds.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION):
                templates[c.get_usr()] = row
            else:
                records[c.get_usr()] = row
        for child in c.get_children():
            walk(child, caller)

    walk(tu.cursor)
    return records, templates, files, errors, functions, uses


def main():
    from gruntz.tool.clang import compdb
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/audits/template-models')
    args = parser.parse_args()
    db = compdb()
    jobs = sorted((s, f) for s, f in db.items() if owned(s))
    records, templates, files, errors = {}, {}, set(), []
    functions, uses = {}, {}
    if not jobs:
        raise SystemExit('No owned translation units in compilation database')
    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        for n, (rs, ts, fs, es, fns, us) in enumerate(pool.map(harvest, jobs), 1):
            functions.update(fns)
            uses.update((json.dumps(u, sort_keys=True), u) for u in us)
            records.update(rs)
            templates.update(ts)
            files.update(fs)
            errors.extend(es)
            if n % 25 == 0 or n == len(jobs):
                print(f'{n}/{len(jobs)} TUs; {len(records)} records; {len(errors)} errors', flush=True)
    all_files = {str(p.resolve()) for folder in ('src', 'include')
                 for p in (ROOT / folder).rglob('*')
                 if p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inl')}
    supplemental = sorted(all_files - files)
    for path in supplemental:
        rs, ts, fs, es, fns, us = harvest((path, db[str(ROOT / 'src/Gruntz/Grunt.cpp')] + ['/TP']))
        functions.update(fns)
        uses.update((json.dumps(u, sort_keys=True), u) for u in us)
        records.update(rs)
        templates.update(ts)
        files.update(fs)
        errors.extend(es)
    template_bases = {b for t in templates.values() for b in t['bases']}
    collection_word = re.compile(r'Array|Vector|Vec|List|Coll|Table|Pool|Hash|Tree|Stack|Queue|Bucket', re.I)
    def shape(row):
        return (row['size'], tuple(row['bases']),
                tuple((f['offset'], f['size'], '*' in f['type'], '[' in f['type'])
                      for f in row['fields']))
    instance_shapes = {shape(r) for r in records.values()
                       if '<' in r['name'] and r['fields'] and r['size'] >= 0}
    candidates = []
    for usr, row in sorted(records.items()):
        if not row['owned'] or '<' in row['name']:
            continue
        reasons = []
        if any(b in template_bases for b in row['bases']):
            reasons.append('shares-template-base')
        if any('<' in b for b in row['bases']):
            reasons.append('derived-template-instance')
        if collection_word.search(row['name']):
            reasons.append('collection-name')
        if not row['fields'] and any(collection_word.search(b) for b in row['bases']):
            reasons.append('fieldless-collection-subclass')
        if row['fields'] and shape(row) in instance_shapes:
            reasons.append('matches-instantiated-layout')
        if row['methods'] and any(re.search(r'count|size|capacity|grow|stride', f['name'], re.I)
                                  for f in row['fields']) and any(
                '*' in f['type'] or '[' in f['type'] for f in row['fields']):
            reasons.append('collection-storage')
        if reasons:
            candidates.append(dict(usr=usr, reasons=reasons, **row))
    # Join source declarations to actual VC5 emission, including synthetic
    # deleting destructors and adjustment thunks that have no AST body.
    from gruntz.core.coff import Coff, IMAGE_SCN_CNT_CODE
    emissions = {}
    for path in sorted((ROOT / 'build/objdiff/base').glob('*.obj')):
        obj = Coff(path)
        for name, value, section, storage in obj.symbols:
            if storage == 2 and 1 <= section <= len(obj.section_chars) and (
                    obj.section_chars[section - 1] & IMAGE_SCN_CNT_CODE):
                emissions.setdefault(name, []).append(dict(unit=path.stem, offset=value))
    bindings = {}
    binding_path = ROOT / 'build/gen/bindings.tsv'
    if binding_path.exists():
        import csv
        with binding_path.open() as f:
            for row in csv.DictReader((ln for ln in f if not ln.startswith('#')), delimiter='\t'):
                if row['name'] and row['space'] == 'text':
                    bindings.setdefault(row['name'], []).append(row)
    by_rva = {b['rva']: b for rows in bindings.values() for b in rows}
    for row in functions.values():
        # Source RVA attributes also resolve aliases to the actual owner claim.
        claimed = []
        for annotation in row['annotations']:
            match = re.search(r'rva:(0x[0-9a-fA-F]+)', annotation)
            if match:
                binding = by_rva.get(f"0x{int(match[1], 16):08x}")
                if binding:
                    claimed.append(binding)
        row['bindings'] = claimed or bindings.get(row['symbol'], [])
        if len(claimed) == 1:
            row['symbol'] = claimed[0]['name']
        row['emissions'] = emissions.get(row['symbol'], [])
    methods = [r for r in functions.values() if r['empty_body'] or any(
        b.get('channel') == 'src' for b in r['bindings']) or r['annotations']]
    summary = dict(translation_units=len(jobs), records=len(records),
                   owned_records=sum(r['owned'] for r in records.values()),
                   template_declarations=len(templates), candidates=len(candidates),
                   functions=len(functions), uses=len(uses),
                   manual_method_candidates=len(methods), emitted_symbols=len(emissions),
                   parse_errors=len(errors), supplemental_files=supplemental,
                   uncovered_files=sorted(all_files - files))
    args.output.mkdir(parents=True, exist_ok=True)
    for name, value in [('records', records), ('templates', templates),
                        ('candidates', candidates), ('parse-errors', errors), ('summary', summary),
                        ('functions', functions), ('uses', list(uses.values())),
                        ('manual-methods', methods), ('emissions', emissions)]:
        (args.output / f'{name}.json').write_text(json.dumps(value, indent=2) + '\n')
    with (args.output / 'candidates.tsv').open('w') as out:
        out.write('name\tfile\tline\tsize\tbases\treasons\n')
        for row in candidates:
            out.write('\t'.join((row['name'], str(Path(row['file']).relative_to(ROOT)),
                                 str(row['line']), str(row['size']), ', '.join(row['bases']),
                                 ', '.join(row['reasons']))) + '\n')
    with (args.output / 'manual-methods.tsv').open('w') as out:
        out.write('name\towner\tfile\tline\tkind\tempty_body\trvas\temitted_units\tusr\n')
        for row in sorted(methods, key=lambda r: (r['file'], r['line'])):
            out.write('\t'.join((row['name'], row['owner'],
                str(Path(row['file']).relative_to(ROOT)), str(row['line']),
                row['kind'], str(row['empty_body']),
                ','.join(b['rva'] for b in row['bindings']),
                ','.join(e['unit'] for e in row['emissions']), row['usr'])) + '\n')
    with (args.output / 'emitted-methods.tsv').open('w') as out:
        out.write('symbol\tcategory\tunits\trvas\n')
        for symbol, sites in sorted(emissions.items()):
            category = ('deleting-destructor' if symbol.startswith(('??_G', '??_E'))
                else 'constructor' if symbol.startswith('??0')
                else 'destructor' if symbol.startswith('??1')
                else 'vcall-thunk' if symbol.startswith('??_9')
                else 'template-member' if '@?$' in symbol else 'function')
            out.write('\t'.join((symbol, category, ','.join(e['unit'] for e in sites),
                ','.join(b['rva'] for b in bindings.get(symbol, [])))) + '\n')
    print(json.dumps(summary, indent=2))
    return bool(errors or summary['uncovered_files'])


if __name__ == '__main__':
    raise SystemExit(main())
