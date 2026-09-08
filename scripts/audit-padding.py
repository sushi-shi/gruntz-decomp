#!/usr/bin/env python3
"""Inventory suspected padding with MSVC-target clang layouts and removal probes.

Run in nix develop. Output is evidence to review, not permission to delete fields.
"""
import csv
import json
import re
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))
OUT = ROOT / 'build/audits/padding'
SUSPECT = re.compile(r'(?:pad|reserved|unknown|^m_p[0-9a-f]+$)', re.I)


def harvest(job):
    import clang.cindex as ci
    from gruntz.tool.clang import inc_cl
    source, flags = job
    args = ['--driver-mode=cl', *[f for f in flags if f != '-fdelayed-template-parsing'], *inc_cl(), '-ferror-limit=0']
    tu = ci.Index.create().parse(source, args=args)
    errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
    records, refs = {}, {}
    def walk(c):
        if c.location.file:
            path = Path(c.location.file.name).resolve()
            if not any(path.is_relative_to(ROOT / x) for x in ('src', 'include')):
                return
        if c.kind == ci.CursorKind.FIELD_DECL and SUSPECT.search(c.spelling):
            owner = c.semantic_parent
            key = owner.get_usr()
            if key not in records:
                fields = []
                for f in owner.type.get_fields():
                    fields.append(dict(name=f.spelling, usr=f.get_usr(), type=f.type.spelling,
                        offset=f.get_field_offsetof() // 8, size=f.type.get_size(), align=f.type.get_align(),
                        file=str(Path(f.location.file.name).resolve().relative_to(ROOT)), line=f.location.line,
                        start=f.extent.start.offset, end=f.extent.end.offset))
                records[key] = dict(owner=owner.spelling, usr=key, size=owner.type.get_size(),
                    align=owner.type.get_align(), fields=fields, source=source, flags=flags, errors=errors)
        if c.kind == ci.CursorKind.MEMBER_REF_EXPR and c.referenced and SUSPECT.search(c.referenced.spelling):
            refs.setdefault(c.referenced.get_usr(), []).append(f'{Path(c.location.file.name).resolve().relative_to(ROOT)}:{c.location.line}')
        for child in c.get_children():
            walk(child)
    walk(tu.cursor)
    return records, refs, errors


def probe(job):
    import clang.cindex as ci
    from gruntz.tool.clang import inc_cl
    record, field = job
    path = ROOT / field['file']
    original = path.read_bytes()
    # Clang field extents exclude the semicolon; only remove standalone declarations.
    end = field['end']
    while end < len(original) and original[end:end+1] in (b' ', b'\t'):
        end += 1
    if original[end:end+1] != b';':
        return field['usr'], dict(result='not-standalone')
    changed = original[:field['start']] + original[end+1:]
    args = ['--driver-mode=cl', *[f for f in record['flags'] if f != '-fdelayed-template-parsing'], *inc_cl(), '-ferror-limit=0']
    tu = ci.Index.create().parse(record['source'], args=args, unsaved_files=[(str(path), changed.decode())])
    errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
    found = None
    def walk(c):
        nonlocal found
        if c.get_usr() == record['usr'] and c.is_definition():
            found = dict(size=c.type.get_size(), align=c.type.get_align(),
                fields={f.get_usr(): f.get_field_offsetof() // 8 for f in c.type.get_fields()})
            return
        if c.location.file and not Path(c.location.file.name).resolve().is_relative_to(ROOT):
            return
        for ch in c.get_children():
            walk(ch)
    walk(tu.cursor)
    expected = {f['usr']: f['offset'] for f in record['fields'] if f['usr'] != field['usr']}
    unchanged = found and found['size'] == record['size'] and found['align'] == record['align'] and found['fields'] == expected
    return field['usr'], dict(result='layout-identical' if unchanged and not errors else 'parse-error' if errors else 'layout-changes', after=found, errors=errors)


def main():
    from gruntz.tool.clang import compdb
    OUT.mkdir(parents=True, exist_ok=True)
    db = compdb()
    jobs = sorted((src, flags) for src, flags in db.items() if Path(src).is_relative_to(ROOT / 'src'))
    records, refs, errors = {}, {}, []
    with ProcessPoolExecutor(max_workers=4) as pool:
        for n, (rs, rf, es) in enumerate(pool.map(harvest, jobs), 1):
            for key, val in rs.items():
                if key not in records or (records[key]['errors'] and not val['errors']):
                    records[key] = val
            for key, val in rf.items():
                refs.setdefault(key, set()).update(val)
            errors.extend(es)
            if n % 25 == 0: print(f'Harvest {n}/{len(jobs)}', flush=True)
    covered = {f['file'] for r in records.values() for f in r['fields']}
    # Include orphan headers containing suspect declarations as explicit audit inputs.
    flags = db[str(ROOT / 'src/Gruntz/Grunt.cpp')] + ['/TP']
    for path in sorted((ROOT / 'include').rglob('*.h')):
        if str(path.relative_to(ROOT)) not in covered and SUSPECT.search(path.read_text()):
            rs, rf, es = harvest((str(path), flags))
            for key, val in rs.items(): records.setdefault(key, val)
            for key, val in rf.items(): refs.setdefault(key, set()).update(val)
            errors.extend(es)
    candidates = [(r, f) for r in records.values() for f in r['fields'] if SUSPECT.search(f['name'])]
    (OUT / 'records.json').write_text(json.dumps(list(records.values()), indent=2))
    (OUT / 'references.json').write_text(json.dumps({k: sorted(v) for k,v in refs.items()}, indent=2))
    probes = {}
    with ProcessPoolExecutor(max_workers=4) as pool:
        for n, (key, result) in enumerate(pool.map(probe, candidates), 1):
            probes[key] = result
            if n % 25 == 0: print(f'Probe {n}/{len(candidates)}', flush=True)
    (OUT / 'probes.json').write_text(json.dumps(probes, indent=2))
    rows = []
    for r, f in candidates:
        idx = r['fields'].index(f)
        rows.append(dict(file=f['file'], line=f['line'], owner=r['owner'], member=f['name'], type=f['type'],
            offset=hex(f['offset']), bytes=f['size'], member_alignment=f['align'], owner_size=hex(r['size']), owner_alignment=r['align'],
            previous=r['fields'][idx-1]['name'] if idx else '', next=r['fields'][idx+1]['name'] if idx+1<len(r['fields']) else '',
            reference_sites=len(refs.get(f['usr'], [])), removal=probes[f['usr']]['result']))
    rows.sort(key=lambda x:(x['file'], x['line']))
    with (OUT / 'inventory.tsv').open('w') as out:
        w=csv.DictWriter(out, fieldnames=list(rows[0]), delimiter='\t', lineterminator='\n');w.writeheader();w.writerows(rows)
    from collections import Counter
    summary=dict(candidates=len(rows), headers=len({r['file'] for r in rows}), removal_results=dict(Counter(r['removal'] for r in rows)), parse_errors=sorted(set(errors)))
    (OUT / 'summary.json').write_text(json.dumps(summary, indent=2))
    print(json.dumps(summary, indent=2), flush=True)

if __name__ == '__main__':
    main()
