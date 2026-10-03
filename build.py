"""Compile and link the standalone Win32 game; never launch it."""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'scripts'))

from gruntzbuild.tool import ToolError, cl, link, rc
from gruntzbuild.tool.wine import init_prefix, shutdown_wineserver, verify_prefix, winepath

LIBRARIES = [
    'nafxcw.lib', 'libcmt.lib', 'kernel32.lib', 'user32.lib', 'gdi32.lib',
    'advapi32.lib', 'comctl32.lib', 'mss32.lib', 'winmm.lib', 'dplayx.lib',
    'smackw32.lib', 'version.lib', 'winspool.lib', 'comdlg32.lib',
    'shell32.lib', 'dinput.lib', 'dsound.lib', 'ddraw.lib', 'dxguid.lib',
]


def build_import_library(stem, directory):
    """Discard the build-time DLL even on failure; the game uses the real DLL."""
    library = directory / f'{stem}.lib'
    with tempfile.TemporaryDirectory(prefix=f'{stem}-', dir=directory) as temp:
        scratch = Path(temp)
        obj = scratch / f'{stem}.obj'
        dll = scratch / f'{stem}.dll'
        generated = scratch / f'{stem}.lib'
        cl.compile(ROOT / 'imports' / f'{stem}.c', obj, ['/nologo', '/c'])
        link.link(['/NOLOGO', '/DLL', '/NOENTRY', f'/OUT:{winepath(dll)}',
                   f'/IMPLIB:{winepath(generated)}', winepath(obj)],
                  cwd=scratch, expect=[generated, dll])
        generated.replace(library)
    return library


def shared_fingerprint():
    """Conservative header closure: a changed header invalidates every object."""
    digest = hashlib.sha256()
    paths = [ROOT / 'build.py', ROOT / 'build.json']
    for directory in ('src', 'include', 'vendor', 'scripts'):
        paths.extend(path for path in (ROOT / directory).rglob('*')
                     if path.suffix.lower() in ('.h', '.hpp', '.inl', '.inc', '.py'))
    for path in sorted(paths):
        digest.update(str(path.relative_to(ROOT)).encode() + b'\0' + path.read_bytes())
    for variable in ('MSVC_DIR', 'DXSDK_DIR'):
        digest.update(os.environ[variable].encode() + b'\0')
    return digest.digest()


def compile_unit(unit, directory, shared):
    source = ROOT / unit['source']
    obj = directory / (unit['name'] + '.obj')
    stamp = obj.with_suffix('.sha256')
    expected = hashlib.sha256(shared + source.read_bytes()
                              + json.dumps(unit, sort_keys=True).encode()).hexdigest()
    if obj.is_file() and stamp.is_file() and stamp.read_text() == expected:
        return obj, False
    stamp.unlink(missing_ok=True)
    cl.compile(source, obj, unit['flags'])
    stamp.write_text(expected)
    return obj, True


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs', type=int, default=4)
    args = parser.parse_args(argv)
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    os.chdir(ROOT)
    # Always isolate the project from the caller's matching/build Wine prefix.
    os.environ['WINEPREFIX'] = str(ROOT / 'build/wineprefix')
    os.environ.setdefault('WINEDLLOVERRIDES', 'mscoree,mshtml=')
    build = ROOT / 'build'
    objects = build / 'obj'
    libraries = build / 'lib'
    objects.mkdir(parents=True, exist_ok=True)
    libraries.mkdir(parents=True, exist_ok=True)
    try:
        init_prefix()
        verify_prefix()
        units = json.loads((ROOT / 'build.json').read_text())['units']
        shared = shared_fingerprint()
        compiled = 0
        failures = []
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            jobs = [pool.submit(compile_unit, unit, objects, shared) for unit in units]
            for count, future in enumerate(as_completed(jobs), 1):
                try:
                    obj, changed = future.result()
                except ToolError as error:
                    failures.append(str(error))
                    print(error, file=sys.stderr, flush=True)
                    continue
                compiled += changed
                if changed:
                    print(f'[{count}/{len(units)}] {obj.stem}', flush=True)
        print(f'Compiled {compiled} of {len(units)} units', flush=True)
        if failures:
            (build / 'compile-errors.log').write_text('\n\n'.join(failures) + '\n')
            return 1
        imports = {f'{stem}.lib': build_import_library(stem, libraries)
                   for stem in ('mss32', 'smackw32')}
        resource = build / 'Gruntz.res'
        rc.compile(ROOT / 'src/Gruntz/Gruntz.rc', resource)
        executable = build / 'GRUNTZ.EXE'
        response = build / 'link.rsp'
        arguments = ['/NOLOGO', '/SUBSYSTEM:WINDOWS', '/ENTRY:WinMainCRTStartup',
                     '/INCREMENTAL:NO', '/FIXED:NO', f'/OUT:"{winepath(executable)}"']
        arguments += [f'"{winepath(imports[name])}"' if name in imports else name
                      for name in LIBRARIES]
        arguments += [f'"{winepath(objects / (unit["name"] + ".obj"))}"' for unit in units]
        arguments.append(f'"{winepath(resource)}"')
        response.write_text('\n'.join(arguments) + '\n')
        output = link.link([f'@{winepath(response)}'], cwd=build, expect=[executable])
        (build / 'link.log').write_text(output)
        if any(code in output for code in ('LNK4006', 'LNK2005', 'LNK2001', 'LNK2019')):
            raise ToolError(f'link reported duplicate or unresolved symbols:\n{output}')
        print(executable)
        return 0
    except (ToolError, OSError, KeyError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1
    finally:
        shutdown_wineserver()


if __name__ == '__main__':
    raise SystemExit(main())
