#!/usr/bin/env python3
"""Queue owned methods and record families for compiler/template recovery.

Uses the real compilation database; candidates are review prompts, not proof.
The complete record/template catalog, function definitions, call/allocation
sites, and actual VC5 COFF emissions are emitted alongside the worklist.
USR and symbol keys join these reports; empty bodies alone do not prove that
a method was compiler-generated. Run against a stable source/build snapshot.
"""
import argparse
import csv
import hashlib
import json
import re
import sys
from concurrent.futures import ProcessPoolExecutor
from functools import lru_cache
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))


def digest(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def stable(value):
    return value.replace(str(ROOT) + '/', '')


def source_tokens(cursor):
    return [t.spelling for t in cursor.get_tokens() if t.kind.name != 'COMMENT']


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def snapshot():
    paths = [p for folder in ('src', 'include') for p in (ROOT / folder).rglob('*')
             if p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inl')]
    paths += list((ROOT / 'build/objdiff/base').glob('*.obj'))
    paths += [ROOT / name for name in ('build/gen/bindings.tsv', 'config/units.toml', 'flake.lock')]
    return {stable(str(p)): file_hash(p) for p in paths if p.is_file()}


def merge_functions(destination, incoming):
    for usr, row in incoming.items():
        site = dict(file=row['file'], line=row['line'], source_hash=row['source_hash'])
        if usr not in destination:
            row['definition_sites'] = [site]
            row['contexts'] = [row.get('context_hash')]
            destination[usr] = row
        else:
            if site not in destination[usr]['definition_sites']:
                destination[usr]['definition_sites'].append(site)
            if row.get('context_hash') not in destination[usr]['contexts']:
                destination[usr]['contexts'].append(row.get('context_hash'))


def merge_records(destination, incoming):
    for usr, row in incoming.items():
        shape = digest([row['size'], row['bases'], row['fields'], row.get('source_hash')])
        if usr not in destination:
            row['observed_shapes'] = [shape]
            destination[usr] = row
        elif shape not in destination[usr]['observed_shapes']:
            destination[usr]['observed_shapes'].append(shape)


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
    ], options=ci.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
    # Spelling tokens alone miss changes inside an invoked macro. Keep the
    # owned macro environment and compile flags in review fingerprints without
    # mistaking TU-specific macro populations for conflicting C++ definitions.
    macros = {c.spelling: source_tokens(c) for c in tu.cursor.get_children()
              if c.kind == kinds.MACRO_DEFINITION and c.location.file and owned(c.location.file.name)}
    context_hash = digest([sorted(macros.items()), [stable(f) for f in flags]])
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
                tokens = source_tokens(body) if body else []
                definition = source_tokens(c)
                parameters = [dict(name=x.spelling, type=x.type.get_canonical().spelling)
                              for x in children if x.kind == kinds.PARM_DECL]
                initializers = [dict(kind=x.kind.name, name=x.displayname,
                    type=x.type.get_canonical().spelling, tokens=source_tokens(x))
                    for x in children if x.kind not in (
                        kinds.PARM_DECL, kinds.COMPOUND_STMT, kinds.ANNOTATE_ATTR)]
                # A macro RVA attribute can give libclang a cross-file extent
                # whose whole-declaration token stream is empty. The body,
                # semantic signature and initializer children remain available.
                source_hash = digest([definition, tokens, c.type.get_canonical().spelling,
                                      parameters, initializers])
                ancestor = c
                templated = False
                while ancestor and ancestor.kind != kinds.TRANSLATION_UNIT:
                    templated |= ancestor.kind in (kinds.CLASS_TEMPLATE,
                        kinds.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION, kinds.FUNCTION_TEMPLATE)
                    ancestor = ancestor.semantic_parent
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
                    definition_tokens=definition, tokens=tokens,
                    source_hash=source_hash, template_context=templated,
                    context_hash=context_hash,
                    parameters=parameters, initializers=initializers,
                    is_virtual=c.is_virtual_method() if c.kind in (
                        kinds.CXX_METHOD, kinds.DESTRUCTOR) else False,
                    is_copy_constructor=c.is_copy_constructor() if c.kind == kinds.CONSTRUCTOR else False,
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
                       size=c.type.get_size(), kind=c.kind.name,
                       source_hash=digest(source_tokens(c)) if owned(path) else None,
                       type_dependencies=[x.type.get_declaration().get_usr() for x in children
                           if x.kind == kinds.CXX_BASE_SPECIFIER] + [
                           x.type.get_canonical().get_declaration().get_usr() for x in children
                           if x.kind == kinds.FIELD_DECL])
            if c.kind in (kinds.CLASS_TEMPLATE, kinds.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION):
                templates[c.get_usr()] = row
            else:
                records[c.get_usr()] = row
        for child in c.get_children():
            walk(child, caller)

    walk(tu.cursor)
    return records, templates, files, errors, functions, uses


def review_queue(records, templates, functions, candidates, emissions, bindings, uses, reviews):
    """Keep the exhaustive fallback; signals rank work, never certify absence."""
    catalog = {**records, **templates}
    candidate_usrs = {r['usr'] for r in candidates}

    def owner_hash(usr, seen=None):
        seen = set() if seen is None else seen
        if usr in seen or usr not in catalog:
            return None
        seen.add(usr)
        row = catalog[usr]
        return digest([row.get('source_hash'), row['bases'], row['fields'],
                       sorted(row.get('observed_shapes', [])),
                       [owner_hash(u, seen) for u in row.get('type_dependencies', []) if u]])

    incoming = {}
    for use in uses:
        if use.get('callee'):
            incoming.setdefault(use['callee'], set()).add(use.get('caller'))
    queue = []
    for usr, row in sorted(functions.items()):
        signals = []
        if row['kind'] in ('CONSTRUCTOR', 'DESTRUCTOR'):
            signals.append('special-member')
        if row.get('is_copy_constructor') or row['name'].startswith('operator='):
            signals.append('copy-or-assignment')
        if row['empty_body']:
            signals.append('empty-body')
        if row['owner_usr'] in candidate_usrs:
            signals.append('collection-owner')
        definition = row['definition_tokens'] + [token for initializer in row.get('initializers', [])
                                                for token in initializer['tokens']]
        tokens = row['tokens']
        definitions = sorted({site['source_hash'] for site in row.get('definition_sites', [])})
        if len(definitions) > 1:
            signals.append('multiple-definition-forms')
        if len(catalog.get(row['owner_usr'], {}).get('observed_shapes', [])) > 1:
            signals.append('conflicting-owner-shapes')
        if any(re.search(r'InlineBase|INLINE_BASE|InlineSeed|INLINE_SEED|NO_SEED|BASE_CALL|ForceEmit|Realize', t) for t in definition):
            signals.append('emission-selector')
        if any(t in tokens for t in ('for', 'while')) and any(
                t in tokens for t in ('new', 'delete', 'memcpy', 'memmove', 'memset')):
            signals.append('storage-loop')
        text = ' '.join(tokens)
        if re.search(r'(?:->|\.|::)\s*(?:\w+\s*::\s*)?~\s*\w+\s*\(', text) or re.search(
                r'(?:->|\.)\s*(\w+)\s*::\s*\1\s*\(', text) or re.search(r'\bnew\s*\(', text):
            signals.append('explicit-lifetime')
        priority = (0 if any(s in signals for s in ('emission-selector', 'explicit-lifetime',
                        'multiple-definition-forms', 'conflicting-owner-shapes')) else
                    1 if 'copy-or-assignment' in signals or 'empty-body' in signals and 'special-member' in signals else
                    2 if 'collection-owner' in signals else 3 if 'special-member' in signals else
                    4 if signals else 9)
        fingerprint = digest([row['source_hash'], definitions,
            sorted(row.get('contexts', [])), owner_hash(row['owner_usr'])])
        queue.append(dict(key='method:' + stable(usr), category='source-definition',
            name=stable(row['owner']) + '::' + row['name'], file=stable(row['file']), line=row['line'],
            rvas=','.join(b['rva'] for b in row['bindings']), fingerprint=fingerprint,
            priority=priority, signals=','.join(signals) or 'exhaustive-fallback',
            status='template-source' if row.get('template_context') else 'pending',
            evidence='', callers=len(incoming.get(usr, set()))))
    for row in candidates:
        queue.append(dict(key='owner:' + stable(row['usr']), category='record-family',
            name=row['name'], file=stable(row['file']), line=row['line'], rvas='',
            fingerprint=owner_hash(row['usr']), priority=2, signals=','.join(row['reasons']),
            status='pending', evidence='', callers=''))
    # This independent surface catches generated/deleting members absent from
    # the AST, rather than silently dropping them from the search universe.
    source_symbols = {s for row in functions.values() for s in [row['symbol'], *row['variants']]}
    for symbol, sites in sorted(emissions.items()):
        if symbol in source_symbols or not symbol.startswith(('??0', '??1', '??4', '??_')) and '@?$' not in symbol:
            continue
        queue.append(dict(key='emission:' + symbol, category='emission-without-source-body',
            name=symbol, file=','.join(e['unit'] for e in sites), line='',
            rvas=','.join(b['rva'] for b in bindings.get(symbol, [])),
            fingerprint=digest([symbol, [(e['unit'], e.get('code_hash')) for e in sites]]),
            priority=8, signals='COFF-special-or-template-member',
            status='emitted-without-owned-body', evidence='Actual VC5 code symbol; no joined owned AST definition. This alone does not prove an implicit method.', callers=''))
    keys = {r['key'] for r in queue}
    issues = []
    dispositions = {'retained-authored', 'recovered-template', 'recovered-implicit', 'open-model-conflict'}
    for row in queue:
        review = reviews.get(row['key'])
        if not review:
            continue
        if review['disposition'] not in dispositions or not review['evidence'].strip():
            issues.append('Invalid review: ' + row['key'])
            continue
        if review['fingerprint'] != row['fingerprint']:
            row['status'] = 'stale-review'
        else:
            row['status'] = review['disposition']
        row['evidence'] = review['evidence']
    issues.extend('Orphan review (reconcile removed/renamed method): ' + k for k in reviews.keys() - keys)
    return sorted(queue, key=lambda r: (r['priority'], r['category'], r['name'], r['key'])), issues


def write_queue(output, reviews_path):
    def read(name):
        return json.loads((output / (name + '.json')).read_text())
    functions = read('functions')
    if any('source_hash' not in row for row in functions.values()):
        raise SystemExit('Census predates fingerprints; rerun the full AST harvest')
    if read('snapshot') != snapshot():
        raise SystemExit('Source, bindings or VC5 objects changed since census; rerun the full AST harvest')
    reviews = {}
    if reviews_path.exists():
        with reviews_path.open() as f:
            for row in csv.DictReader(f, delimiter='\t'):
                if row['key'] in reviews:
                    raise SystemExit('Duplicate review key: ' + row['key'])
                reviews[row['key']] = row
    queue, issues = review_queue(read('records'), read('templates'), functions, read('candidates'),
        read('emissions'), read('bindings'), read('uses'), reviews)
    (output / 'review-queue.json').write_text(json.dumps(queue, indent=2) + '\n')
    with (output / 'review-queue.tsv').open('w') as out:
        writer = csv.DictWriter(out, fieldnames=list(queue[0]), delimiter='\t')
        writer.writeheader()
        writer.writerows(queue)
    from collections import Counter
    summary = dict(rows=len(queue), statuses=dict(Counter(r['status'] for r in queue)),
                   categories=dict(Counter(r['category'] for r in queue)), review_issues=issues)
    (output / 'review-summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps(summary, indent=2))
    return bool(issues)


def main():
    from gruntz.tool.clang import compdb
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/audits/template-models')
    parser.add_argument('--reviews', type=Path, default=ROOT / 'docs/compiler-method-review.tsv')
    parser.add_argument('--queue-only', action='store_true', help='Apply reviews to an existing fingerprinted census')
    args = parser.parse_args()
    if args.queue_only:
        return write_queue(args.output, args.reviews)
    initial_snapshot = snapshot()
    db = compdb()
    jobs = sorted((s, f) for s, f in db.items() if owned(s))
    records, templates, files, errors = {}, {}, set(), []
    functions, uses = {}, {}
    if not jobs:
        raise SystemExit('No owned translation units in compilation database')
    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        for n, (rs, ts, fs, es, fns, us) in enumerate(pool.map(harvest, jobs), 1):
            merge_functions(functions, fns)
            uses.update((json.dumps(u, sort_keys=True), u) for u in us)
            merge_records(records, rs)
            merge_records(templates, ts)
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
        merge_functions(functions, fns)
        uses.update((json.dumps(u, sort_keys=True), u) for u in us)
        merge_records(records, rs)
        merge_records(templates, ts)
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
        if any(re.search(r'count', f['name'], re.I) for f in row['fields']) and any(
                re.search(r'avg|average|sum', f['name'], re.I) for f in row['fields']):
            reasons.append('scalar-accumulator')
        if any('<' in f['type'] for f in row['fields']):
            reasons.append('contains-template-storage')
        if reasons:
            candidates.append(dict(usr=usr, reasons=reasons, **row))
    # Join source declarations to actual VC5 emission, including synthetic
    # deleting destructors and adjustment thunks that have no AST body.
    from gruntz.core.coff import Coff, IMAGE_SCN_CNT_CODE
    from gruntz.delink.coffx import Obj
    from gruntz.walls.diagnose import _find_function
    emissions = {}
    for path in sorted((ROOT / 'build/objdiff/base').glob('*.obj')):
        obj = Coff(path)
        full_obj = None
        for name, value, section, storage in obj.symbols:
            if storage == 2 and 1 <= section <= len(obj.section_chars) and (
                    obj.section_chars[section - 1] & IMAGE_SCN_CNT_CODE):
                site = dict(unit=path.stem, offset=value)
                if name.startswith(('??0', '??1', '??4', '??_')) or '@?$' in name:
                    if full_obj is None:
                        full_obj = Obj(str(path))
                    body, relocations, size = _find_function(full_obj, name)
                    site['code_hash'] = digest([body.hex(), sorted(relocations.items()), size])
                emissions.setdefault(name, []).append(site)
    bindings = {}
    binding_path = ROOT / 'build/gen/bindings.tsv'
    if binding_path.exists():
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
    if snapshot() != initial_snapshot:
        raise SystemExit('Source, bindings or VC5 objects changed during census; retry on a stable build')
    for name, value in [('records', records), ('templates', templates),
                        ('candidates', candidates), ('parse-errors', errors), ('summary', summary),
                        ('functions', functions), ('uses', list(uses.values())),
                        ('manual-methods', methods), ('emissions', emissions), ('bindings', bindings),
                        ('snapshot', initial_snapshot)]:
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
    queue_errors = write_queue(args.output, args.reviews)
    return bool(errors or summary['uncovered_files'] or queue_errors)


if __name__ == '__main__':
    raise SystemExit(main())
