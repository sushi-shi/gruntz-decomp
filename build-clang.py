"""Compile the standalone source with the existing Clang Windows target; never launch."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent

def build_lowercase_mirror(real: Path, mirror: Path) -> Path:
    """Recursive lowercase-symlink mirror of `real`, so <string.h> resolves.

    Every FOO.H under `real` gets a lowercase symlink `foo.h` (to the real,
    ABSOLUTE path) under `mirror`, preserving (lowercased) subdir structure.
    Rebuilt only when `real` changes (a `.src` marker guards it) so a
    toolchain bump does not leave dangling symlinks.
    """
    marker = mirror.parent / (mirror.name + ".src")
    if mirror.is_dir() and marker.is_file() and marker.read_text() == str(real):
        return mirror
    if mirror.exists():
        shutil.rmtree(mirror)
    for root, _dirs, files in os.walk(real):
        rel = os.path.relpath(root, real)
        low = mirror if rel == "." else mirror / rel.lower()
        low.mkdir(parents=True, exist_ok=True)
        for fn in files:
            link = low / fn.lower()
            if not link.exists():
                link.symlink_to(os.path.join(root, fn))
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(str(real))
    return mirror

def adapt_mfc_message_macros(real: Path, mirror: Path) -> None:
    """Qualify VC5's implicit &OnFoo in the CLANG-ONLY SDK mirror.

    VC5 accepts the enclosing class's member name without qualification;
    Clang does not. Keep every SDK message/signature/cast token intact and
    supply the class via MFC_MESSAGE_MAP_CLASS. Never modify the pinned SDK
    or the headers read by the matching compiler.
    """
    original = (real / "AFXMSG_.H").read_text()
    adapted, count = re.subn(r"&(?=On[A-Z]\w*\b)", "&MfcMessageMapClass::", original)
    if not count:
        raise RuntimeError("AFXMSG_.H has no expected VC5 member-pointer expressions")
    dest = mirror / "afxmsg_.h"
    if dest.is_symlink():
        dest.unlink()
    if not dest.exists() or dest.read_text() != adapted:
        dest.write_text(adapted)

def adapt_legacy_stl(real, mirror):
    """Correct pre-standard declaration syntax in Clang's disposable SDK copy."""
    names = ('iosfwd', 'xmemory', 'xlocale', 'vector', 'utility',
             'streambuf', 'ios', 'ostream', 'istream')
    for name in names:
        text = (real / name.upper()).read_text()
        text = re.sub(r'(?m)^(struct|class) (_CRTIMP )?(char_traits<[^>]+>|allocator<void>|codecvt<wchar_t, char, mbstate_t>|ctype<char>|vector<_Bool, _Bool_allocator>)',
                      r'template<> \1 \2\3', text)
        if name in ('utility', 'streambuf', 'ios', 'ostream', 'istream'):
            text = text.replace('class _Tr = char_traits<_E>', 'class _Tr')
        text = text.replace('flags() & unitbuf', 'flags() & ios_base::unitbuf')
        text = text.replace('flags() & skipws', 'flags() & ios_base::skipws')
        dest = mirror / name
        if dest.is_symlink(): dest.unlink()
        if not dest.exists() or dest.read_text() != text: dest.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs', type=int, default=8)
    parser.add_argument('--syntax-only', action='store_true')
    parser.add_argument('units', nargs='*')
    args = parser.parse_args()
    if args.jobs < 1: parser.error('--jobs must be positive')
    build = ROOT / 'build/clang'
    build.mkdir(parents=True, exist_ok=True)
    msvc = Path(os.environ['MSVC_DIR']) / 'include'
    dx = Path(os.environ['DXSDK_DIR']) / 'Include'
    msvc_low = build_lowercase_mirror(msvc, build / 'include/msvc')
    dx_low = build_lowercase_mirror(dx, build / 'include/dx')
    adapt_mfc_message_macros(msvc, msvc_low)
    adapt_legacy_stl(msvc, msvc_low)
    compiler = os.environ.get('GRUNTZ_CLANG', 'clang')
    flags = ['--driver-mode=cl', '/c', '--target=i386-pc-windows-msvc',
             '-fms-compatibility-version=11.00', '-fms-extensions', '-fdelayed-template-parsing',
             '-Wno-address-of-temporary', '-Wno-c++11-narrowing', '-Wno-writable-strings',
             '-Wno-nonportable-include-path', '-ferror-limit=0', '/D_X86_', '/DWIN32', '/D_WINDOWS', '/D_MBCS',
             '/O2', '/EHsc', '/GR', '/I' + str(ROOT / 'include')]
    for directory in (dx_low, msvc_low, dx, msvc):
        flags += ['/imsvc', str(directory)]
    for directory in sorted((ROOT / 'vendor').iterdir()):
        if directory.is_dir(): flags += ['/I' + str(directory)]
    units = json.loads((ROOT / 'build.json').read_text())['units']
    if args.units:
        unknown = set(args.units) - {u['name'] for u in units}
        if unknown: parser.error('unknown units: ' + ', '.join(sorted(unknown)))
        units = [u for u in units if u['name'] in args.units]
    entries = []
    for unit in units:
        cmd = [compiler, *flags, str(ROOT / unit['source']), '/Fo' + str(build / (unit['name'] + '.obj'))]
        if args.syntax_only: cmd += ['-fsyntax-only']
        entries.append((unit,cmd))
    (build / 'compile_commands.json').write_text(json.dumps([
        {'directory': str(ROOT), 'file': str(ROOT / u['source']), 'arguments': cmd}
        for u,cmd in entries], indent=2) + '\n')
    def compile(entry):
        unit,cmd = entry
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        (build / (unit['name'] + '.log')).write_text(result.stdout + result.stderr)
        return unit['name'],result.returncode
    failures=[]
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for name,rc in pool.map(compile, entries):
            if rc:
                failures.append(name)
                print('FAIL', name, flush=True)
    print(f'{len(entries)-len(failures)}/{len(entries)} Clang units passed')
    (build / 'failures.json').write_text(json.dumps(failures))
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
