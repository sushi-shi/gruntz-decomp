#!/usr/bin/env python3
"""Find owned records with SDK-compatible field layouts using the real compdb.

Run in nix develop. Candidates require semantic review: identical storage does
not prove type identity. Also inspect API void* buffers and anonymous overlays,
which can conceal partial SDK copies that have no complete matching layout.
"""
import argparse
import json
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))


def owned(path):
    path = Path(path).resolve()
    return path.is_relative_to(ROOT / 'include') or path.is_relative_to(ROOT / 'src')


def harvest(job):
    import clang.cindex as ci
    from gruntz.tool.clang import inc_cl
    source, flags = job
    records, files, errors = {}, set(), []
    try:
        tu = ci.Index.create().parse(source, args=[
            '--driver-mode=cl',
            *[f for f in flags if f != '-fdelayed-template-parsing'],
            *inc_cl(), '-ferror-limit=0',
        ])
    except Exception as error:
        return records, files, [{'source': source, 'error': str(error)}]
    errors.extend({'source': source, 'error': str(d)} for d in tu.diagnostics
                  if d.severity >= ci.Diagnostic.Error)
    files.add(source)
    files.update(str(Path(i.include.name).resolve()) for i in tu.get_includes()
                 if owned(i.include.name))

    def key(t):
        t = t.get_canonical()
        kind = t.kind.name
        if kind in {'INT', 'LONG', 'SHORT', 'LONGLONG', 'SCHAR', 'CHAR_S'}:
            return f'signed:{t.get_size()}'
        if kind in {'UINT', 'ULONG', 'USHORT', 'ULONGLONG', 'UCHAR', 'CHAR_U'}:
            return f'unsigned:{t.get_size()}'
        if kind == 'POINTER':
            return f'pointer:{t.get_size()}'
        if kind == 'CONSTANTARRAY':
            return f'array:{t.element_count}:{key(t.element_type)}'
        # Deliberately coarse for nested records: a candidate, not a type proof.
        return f'{kind}:{t.get_size()}'

    def walk(c):
        if c.kind in (ci.CursorKind.STRUCT_DECL, ci.CursorKind.CLASS_DECL,
                      ci.CursorKind.UNION_DECL) and c.is_definition() and c.location.file:
            fields = list(c.type.get_fields())
            if len(fields) >= 2 and c.type.get_size() >= 0:
                path = str(Path(c.location.file.name).resolve())
                records[c.get_usr()] = {
                    'name': c.displayname, 'file': path, 'line': c.location.line,
                    'owned': owned(path), 'size': c.type.get_size(),
                    'align': c.type.get_align(), 'kind': c.kind.name,
                    'fields': [{'name': f.spelling, 'type': f.type.spelling,
                                'key': key(f.type), 'offset_bits': f.get_field_offsetof(),
                                'size': f.type.get_size()} for f in fields],
                }
        for child in c.get_children():
            walk(child)

    walk(tu.cursor)
    return records, files, errors


def candidates(records):
    def shape(r):
        return r['size'], r['align'], tuple((f['offset_bits'], f['key']) for f in r['fields'])
    sdk = {}
    for usr, r in records.items():
        if not r['owned']:
            sdk.setdefault(shape(r), []).append(usr)
    return [{'owned': usr, 'sdk': sdk[shape(r)]} for usr, r in sorted(records.items())
            if r['owned'] and shape(r) in sdk]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/audits/sdk-layouts')
    parser.add_argument('--workers', type=int, default=4)
    args = parser.parse_args()
    from gruntz.tool.clang import compdb
    db = compdb()
    jobs = sorted((s, f) for s, f in db.items() if owned(s))
    if not jobs:
        raise SystemExit('No owned translation units in compilation database')
    records, files, errors = {}, set(), []
    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        for n, (rs, fs, es) in enumerate(pool.map(harvest, jobs), 1):
            records.update(rs)
            files.update(fs)
            errors.extend(es)
            if n % 25 == 0 or n == len(jobs):
                print(f'{n}/{len(jobs)} TUs; {len(records)} records; {len(errors)} errors', flush=True)
    all_files = {str(p.resolve()) for folder in ('src', 'include')
                 for p in (ROOT / folder).rglob('*')
                 if p.suffix in ('.c', '.cpp', '.h', '.hpp', '.inl')}
    supplemental = sorted(all_files - files)
    flags = db[str(ROOT / 'src/Gruntz/Grunt.cpp')] + ['/TP']
    for path in supplemental:
        rs, fs, es = harvest((path, flags))
        records.update(rs)
        files.update(fs)
        errors.extend(es)
        print(f'Supplemental {Path(path).relative_to(ROOT)}: {len(es)} errors', flush=True)
    pairs = candidates(records)
    summary = {'translation_units': len(jobs), 'records': len(records),
               'owned_records': sum(r['owned'] for r in records.values()),
               'candidate_records': len(pairs), 'parse_errors': len(errors),
               'supplemental_files': supplemental, 'uncovered_files': sorted(all_files - files)}
    args.output.mkdir(parents=True, exist_ok=True)
    for name, value in [('records', records), ('candidates', pairs),
                        ('parse-errors', errors), ('summary', summary)]:
        (args.output / f'{name}.json').write_text(json.dumps(value, indent=2) + '\n')
    with (args.output / 'candidates.tsv').open('w') as f:
        f.write('owned\tfile\tline\tsize\tsdk_candidates\n')
        for pair in pairs:
            r = records[pair['owned']]
            f.write('\t'.join([r['name'], str(Path(r['file']).relative_to(ROOT)),
                               str(r['line']), str(r['size']),
                               ', '.join(sorted({records[s]['name'] for s in pair['sdk']}))]) + '\n')
    print(json.dumps(summary, indent=2))
    return bool(errors or summary['uncovered_files'])


if __name__ == '__main__':
    raise SystemExit(main())
