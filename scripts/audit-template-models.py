#!/usr/bin/env python3
"""Inventory owned record families that may conceal template instantiations.

Uses the real compilation database; candidates are review prompts, not proof.
The complete record and template catalog is emitted alongside the worklist.
"""
import argparse
import json
import re
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))


def owned(path):
    p = Path(path).resolve()
    return any(p.is_relative_to(ROOT / d) for d in ('src', 'include'))


def harvest(job):
    import clang.cindex as ci
    from gruntz.tool.clang import inc_cl
    source, flags = job
    kinds = ci.CursorKind
    tu = ci.Index.create().parse(source, args=[
        '--driver-mode=cl', *[f for f in flags if f != '-fdelayed-template-parsing'],
        *inc_cl(), '-ferror-limit=0',
    ])
    records, templates = {}, {}
    files = {source} | {str(Path(i.include.name).resolve()) for i in tu.get_includes()
                        if owned(i.include.name)}
    errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]

    def walk(c):
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
            walk(child)

    walk(tu.cursor)
    return records, templates, files, errors


def main():
    from gruntz.tool.clang import compdb
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/audits/template-models')
    args = parser.parse_args()
    db = compdb()
    jobs = sorted((s, f) for s, f in db.items() if owned(s))
    records, templates, files, errors = {}, {}, set(), []
    if not jobs:
        raise SystemExit('No owned translation units in compilation database')
    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        for n, (rs, ts, fs, es) in enumerate(pool.map(harvest, jobs), 1):
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
        rs, ts, fs, es = harvest((path, db[str(ROOT / 'src/Gruntz/Grunt.cpp')] + ['/TP']))
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
    summary = dict(translation_units=len(jobs), records=len(records),
                   owned_records=sum(r['owned'] for r in records.values()),
                   template_declarations=len(templates), candidates=len(candidates),
                   parse_errors=len(errors), supplemental_files=supplemental,
                   uncovered_files=sorted(all_files - files))
    args.output.mkdir(parents=True, exist_ok=True)
    for name, value in [('records', records), ('templates', templates),
                        ('candidates', candidates), ('parse-errors', errors), ('summary', summary)]:
        (args.output / f'{name}.json').write_text(json.dumps(value, indent=2) + '\n')
    with (args.output / 'candidates.tsv').open('w') as out:
        out.write('name\tfile\tline\tsize\tbases\treasons\n')
        for row in candidates:
            out.write('\t'.join((row['name'], str(Path(row['file']).relative_to(ROOT)),
                                 str(row['line']), str(row['size']), ', '.join(row['bases']),
                                 ', '.join(row['reasons']))) + '\n')
    print(json.dumps(summary, indent=2))
    return bool(errors or summary['uncovered_files'])


if __name__ == '__main__':
    raise SystemExit(main())
