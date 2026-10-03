"""Export one standalone C++ source project from a committed reconstruction."""

import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tarfile
import tempfile
import tomllib

from gruntz.clean.lexer import clean_source

TEMPLATE = 'scripts/gruntz/clean/project/'
MARKER = '.gruntz-source-export'
BRIDGES = ('__init__.py', 'wine.py', 'cl.py', 'link.py', 'rc.py')


def snapshot(repo, revision, working=False):
    commit = subprocess.check_output(
        ['git', '-C', str(repo), 'rev-parse', '--verify', f'{revision}^{{commit}}'],
        text=True).strip()
    if working:
        names = subprocess.check_output(['git', '-C', str(repo), 'ls-files', '-z'],
                                        text=True).split('\0')
        return commit, {name: (repo / name).read_bytes() for name in names
                        if name and not (repo / name).is_symlink() and (repo / name).is_file()}
    data = subprocess.check_output(['git', '-C', str(repo), 'archive', commit])
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:
        return commit, {entry.name: archive.extractfile(entry).read()
                        for entry in archive if entry.isfile()}


def generate(files):
    manifest = tomllib.loads(files['config/units.toml'].decode())
    units = []
    names = set()
    for unit in manifest['unit']:
        name, source = unit['unit'], unit['source']
        if name in names or not name or any(c not in 'abcdefghijklmnopqrstuvwxyz0123456789_-' for c in name):
            raise ValueError(f'invalid or duplicate unit name: {name}')
        names.add(name)
        if not source.startswith(('src/', 'vendor/')):
            raise ValueError(f'unsupported unit source: {source}')
        units.append({'name': name, 'source': source, 'flags': manifest['flags'][unit['flags']]})
    if not units:
        raise ValueError('empty source manifest')
    sources = {unit['source'] for unit in units}
    sources.update(name for name in files if name.startswith(('src/', 'include/'))
                   and Path(name).suffix.lower() in ('.h', '.hpp', '.inl', '.inc', '.rc'))
    sources.discard('include/rva.h')
    output = {}
    for name in sorted(sources):
        try:
            data = files[name]
            output[name] = (data if name.startswith('vendor/') else
                            clean_source(data.decode('utf-8')).encode('utf-8'))
        except (ValueError, KeyError) as error:
            raise ValueError(f'{name}: {error}') from error
    for name, data in sorted(files.items()):
        if name.startswith(TEMPLATE):
            output[name.removeprefix(TEMPLATE)] = data
        elif name.startswith('src/Gruntz/res/'):
            output[name] = data
        elif name.startswith('vendor/') and (Path(name).suffix.lower() in ('.h', '.hpp', '.inl', '.inc')
                                            or Path(name).name in ('README', 'LICENSE', 'COPYING')):
            output[name] = data
    for name in BRIDGES:
        output[f'scripts/gruntzbuild/tool/{name}'] = files[f'scripts/gruntz/tool/{name}'].replace(
            b'gruntz.', b'gruntzbuild.').replace(b'`gruntz init`', b'`python3 build.py`')
    for name in ('scripts/gruntzbuild/__init__.py', 'scripts/gruntzbuild/core/__init__.py'):
        output[name] = b''
    for name in ('LICENSE', 'nix/toolchain.nix'):
        output[name] = files[name]
    output['build.json'] = (json.dumps({'units': units}, indent=2) + '\n').encode()
    lock = json.loads(files['flake.lock'])
    nixpkgs = lock['nodes'][lock['nodes']['root']['inputs']['nixpkgs']]
    output['flake.lock'] = (json.dumps({'nodes': {'nixpkgs': nixpkgs,
        'root': {'inputs': {'nixpkgs': 'nixpkgs'}}}, 'root': 'root', 'version': lock['version']},
        indent=2) + '\n').encode()
    for name in output:
        path = PurePosixPath(name)
        if path.is_absolute() or any(part in ('..', '.git', 'tests', '__pycache__') for part in path.parts):
            raise ValueError(f'unsafe output name: {name}')
    for name in ('build.py', 'flake.nix', 'scripts/gruntzbuild/core/paths.py',
                 'imports/mss32.c', 'imports/smackw32.c'):
        if name not in output:
            raise ValueError(f'missing export template: {name}')
    return output


def validate_output(repo, requested):
    path = requested.absolute()
    if path.is_symlink() or any(parent.is_symlink() for parent in path.parents):
        raise ValueError('output must not traverse symlinks')
    path, repo = path.resolve(), repo.resolve()
    if path == repo or path in repo.parents:
        raise ValueError('output must not contain the repository')
    if path.is_relative_to(repo) and (not path.is_relative_to(repo / 'build') or path == repo / 'build'):
        raise ValueError('output in this checkout must be a child of build/')
    if path.exists():
        marker = path / MARKER
        if not marker.is_file() or marker.is_symlink():
            raise ValueError('existing output is not a generated directory')
        if any(path.rglob('.git')):
            raise ValueError('output contains a Git repository; export to a fresh directory')
        try:
            metadata = json.loads(marker.read_text())
            if not isinstance(metadata, dict) or metadata.get('generator') != 'gruntz clean':
                raise ValueError('unrecognized output marker')
        except json.JSONDecodeError as error:
            raise ValueError('invalid output marker') from error
    return path


def write_output(repo, requested, files, commit, working=False):
    output = validate_output(repo, requested)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.gruntz-source-', dir=output.parent) as temp:
        staging = Path(temp) / 'project'
        staging.mkdir()
        for name, data in sorted(files.items()):
            path = staging / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        (staging / MARKER).write_text(json.dumps({
            'generator': 'gruntz clean', 'commit': commit, 'working_tree': working}) + '\n')
        if output.exists():
            shutil.rmtree(output)
        staging.rename(output)
    return output


def main(argv=None):
    from gruntz.core.paths import REPO
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=REPO / 'build/clean-source')
    parser.add_argument('--ref', default='HEAD')
    parser.add_argument('--working-tree', action='store_true', help='preview tracked working files, including staged additions')
    parser.add_argument('--verify', action='store_true', help='build the exported project in its own Nix shell')
    args = parser.parse_args(argv)
    try:
        if args.working_tree and args.ref != 'HEAD':
            raise ValueError('--working-tree cannot be combined with --ref')
        commit, inputs = snapshot(REPO, args.ref, args.working_tree)
        files = generate(inputs)
        output = write_output(REPO, args.out, files, commit, args.working_tree)
        digest = hashlib.sha256(b''.join(name.encode() + b'\0' + data
                                         for name, data in sorted(files.items()))).hexdigest()
        print(f'Exported {len(files)} files to {output}\nSource: {commit}\nSHA-256: {digest}', flush=True)
        if args.verify:
            subprocess.run(['nix', 'develop', 'path:.', '-c', 'python3', 'build.py'],
                           cwd=output, check=True)
        return 0
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.error(str(error))
