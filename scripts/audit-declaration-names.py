#!/usr/bin/env python3
"""Dump source declaration names using libclang and the repository compdb.

Run: nix develop -c python3 scripts/audit-declaration-names.py --tracked-only
Generated output is an audit snapshot, not a matching ledger.
"""
import argparse
import csv
import json
import sys
import subprocess
from collections import Counter
from concurrent.futures import ProcessPoolExecutor
from functools import cache
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PARSER = argparse.ArgumentParser(description=__doc__)
PARSER.add_argument('--output', type=Path, default=ROOT / 'build/audits/declaration-names')
PARSER.add_argument('--workers', type=int, default=4)
PARSER.add_argument('--tracked-only', action='store_true', help='Exclude untracked source/header files')
ARGS = PARSER.parse_args()
OUT = ARGS.output.resolve()
TRACKED = set(subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', 'src', 'include'], text=True).splitlines())
sys.path.insert(0, str(ROOT / 'scripts'))


@cache
def owned(path):
    p = Path(path).resolve()
    return ((p.is_relative_to(ROOT / 'src') or p.is_relative_to(ROOT / 'include'))
            and (not ARGS.tracked_only or str(p.relative_to(ROOT)) in TRACKED))


def harvest(item):
    import clang.cindex as ci
    from gruntz.tool.clang import inc_cl
    source, flags = item
    # Parse template bodies too: delayed parsing can hide unused template locals.
    flags = [f for f in flags if f != '-fdelayed-template-parsing']
    args = ['--driver-mode=cl', *flags, *inc_cl(), '-ferror-limit=0']
    rows, files, diagnostics = [], set(), []
    try:
        tu = ci.Index.create().parse(source, args=args)
    except Exception as e:
        return rows, files, [{'source': source, 'error': str(e)}]
    for d in tu.diagnostics:
        if d.severity >= ci.Diagnostic.Error:
            diagnostics.append({'source': source, 'error': str(d)})
    files.add(source)
    files.update(str(Path(i.include.name).resolve()) for i in tu.get_includes()
                 if owned(i.include.name))
    K = ci.CursorKind
    functions = {K.FUNCTION_DECL, K.CXX_METHOD, K.CONSTRUCTOR, K.DESTRUCTOR,
                 K.CONVERSION_FUNCTION, K.FUNCTION_TEMPLATE}
    records = {K.CLASS_DECL, K.STRUCT_DECL, K.UNION_DECL,
               K.CLASS_TEMPLATE, K.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION}
    wanted = functions | {K.FIELD_DECL, K.VAR_DECL, K.PARM_DECL}

    def scope(c):
        names = []
        p = c.semantic_parent
        while p and p.kind != K.TRANSLATION_UNIT:
            if p.spelling:
                names.append(p.displayname or p.spelling)
            p = p.semantic_parent
        return '::'.join(reversed(names))

    def walk(node, in_function=False, enclosing=''):
        loc = node.location
        if loc.file and not owned(loc.file.name):
            return
        is_function = node.kind in functions
        if node.kind in wanted and loc.file:
            file = str(Path(loc.file.name).resolve().relative_to(ROOT))
            parent = node.semantic_parent
            if is_function:
                kind = 'function'
            elif node.kind == K.FIELD_DECL:
                kind = 'field'
            elif node.kind == K.PARM_DECL:
                kind = 'argument'
            elif parent and parent.kind in records:
                kind = 'static_member'
            else:
                kind = 'local' if in_function else 'global'
            owner = scope(node) or enclosing
            rows.append((kind, node.spelling, node.type.spelling, owner,
                         node.storage_class.name, file, loc.line, loc.column,
                         node.kind.name, node.get_usr(), node.is_definition(), node.linkage.name))
        for child in node.get_children():
            walk(child, in_function or is_function,
                 scope(node) + '::' + node.displayname if is_function else enclosing)

    walk(tu.cursor)
    return rows, files, diagnostics


def main():
    from gruntz.tool.clang import compdb
    db = compdb()
    jobs = sorted((src, flags) for src, flags in db.items() if owned(src))
    if not jobs:
        raise SystemExit('No owned translation units in compilation database')
    OUT.mkdir(parents=True, exist_ok=True)
    all_files = {str(p.resolve()) for root in ('src', 'include')
                 for p in (ROOT / root).rglob('*')
                 if p.suffix in ('.cpp', '.c', '.h', '.hpp', '.inl') and owned(str(p))}
    rows, files, diagnostics = set(), set(), []
    with ProcessPoolExecutor(max_workers=ARGS.workers) as pool:
        for n, (found, included, errors) in enumerate(pool.map(harvest, jobs), 1):
            rows.update(found)
            files.update(included)
            diagnostics.extend(errors)
            if n % 25 == 0 or n == len(jobs):
                print(f'{n}/{len(jobs)} TUs; {len(rows)} declarations; '
                      f'{len(diagnostics)} parse errors', flush=True)
    supplemental = sorted(all_files - files)
    header_flags = db[str(ROOT / 'src/Gruntz/Grunt.cpp')] + ['/TP']
    for header in supplemental:
        found, included, errors = harvest((header, header_flags))
        rows.update(found)
        files.update(included)
        diagnostics.extend(errors)
        print(f'Supplemental {Path(header).relative_to(ROOT)}: {len(errors)} errors', flush=True)
    rows = sorted(rows)
    columns = ['kind', 'name', 'type', 'scope', 'storage', 'file', 'line',
               'column', 'cursor_kind', 'usr', 'definition', 'linkage']
    with (OUT / 'declarations.tsv').open('w') as f:
        w = csv.writer(f, delimiter='\t')
        w.writerow(columns)
        w.writerows(rows)
    violations = []
    for row in rows:
        kind, name, _, _, storage = row[:5]
        static = kind == 'static_member' or storage == 'STATIC' or (kind == 'global' and row[-1] == 'INTERNAL')
        prefix = ('s_' if static else 'm_' if kind == 'field' else 'g_' if kind == 'global' else '') if kind != 'function' else ''
        if name and prefix and not name.startswith(prefix):
            violations.append((*row, prefix))
    with (OUT / 'prefix-violations.tsv').open('w') as f:
        w = csv.writer(f, delimiter='\t')
        w.writerow([*columns, 'required_prefix'])
        w.writerows(violations)
    groups = {'all': rows}
    for kind in sorted({r[0] for r in rows}):
        groups[kind] = [r for r in rows if r[0] == kind]
    groups['static'] = [r for r in rows if r[4] == 'STATIC' or r[0] == 'static_member'
                        or (r[0] == 'global' and r[-1] == 'INTERNAL')]
    for kind, entries in groups.items():
        names = sorted({r[1] for r in entries if r[1]})
        (OUT / f'{kind}.unique.txt').write_text(''.join(n + '\n' for n in names))
    summary = {
        'translation_units': len(jobs), 'declarations': len(rows),
        'tracked_only': ARGS.tracked_only,
        'prefix_violation_declarations': len(violations),
        'supplemental_files': [str(Path(p).relative_to(ROOT)) for p in supplemental],
        'declarations_by_kind': dict(Counter(r[0] for r in rows)),
        'unique_names_by_group': {k: len({r[1] for r in v if r[1]})
                                  for k, v in groups.items()},
        'unnamed_by_kind': dict(Counter(r[0] for r in rows if not r[1])),
        'parse_errors': diagnostics,
        'uncovered_files': sorted(str(Path(p).relative_to(ROOT)) for p in all_files - files),
        'limitations': ['Active preprocessor configuration only; vendor/SDK declarations excluded.',
                        'Otherwise-unincluded headers parsed individually with Grunt.cpp flags and /TP; see supplemental_files.',
                        'Rows deduplicated across TUs by complete declaration record; prototypes and definitions remain separate.',
                        'Unnamed declarations remain in TSV but not unique name lists.',
                        'Static list includes explicit statics, class static data, and internal-linkage globals; function prefixes are unchecked.'],
    }
    (OUT / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    print(json.dumps({k: v for k, v in summary.items() if k != 'parse_errors'}, indent=2))
    return 1 if diagnostics or violations else 0


if __name__ == '__main__':
    raise SystemExit(main())
