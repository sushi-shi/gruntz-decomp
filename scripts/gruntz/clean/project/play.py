"""Build and start Gruntz using local game data and the real runtime DLLs."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'scripts'))

ASSETS = ('Gruntz.REZ', 'GRUNTZ.VRZ', 'LARGE.FNT', 'MEDIUM.FNT', 'SMALL.FNT', 'TINY.FNT')


def assets_in(directory):
    files = {path.name.lower(): path.resolve() for path in directory.iterdir() if path.is_file()}
    missing = [name for name in ASSETS if name.lower() not in files]
    if missing:
        raise ValueError(f'{directory}: missing {", ".join(missing)}')
    return {name: files[name.lower()] for name in ASSETS}


def install_files(game, assets, executable, dlls):
    """Only replace managed runtime inputs; saves and other player files survive."""
    game.mkdir(parents=True, exist_ok=True)
    for name, source in {**assets, **dlls}.items():
        destination = game / name
        if destination.is_symlink() and destination.resolve() == source.resolve():
            continue
        if destination.exists() or destination.is_symlink():
            if not destination.is_symlink():
                raise ValueError(f'{destination}: refusing to replace a local file')
            destination.unlink()
        destination.symlink_to(source.resolve())
    destination = game / 'GRUNTZ.EXE'
    destination.unlink(missing_ok=True)
    shutil.copy2(executable, destination)


def prepare_prefix(target, executable):
    prefix = target / 'prefix3'
    env = dict(os.environ, WINEPREFIX=str(prefix), WINEDLLOVERRIDES='mscoree,mshtml=')
    if not (prefix / 'drive_c').is_dir():
        prefix.mkdir(parents=True, exist_ok=True)
        subprocess.run(['wineboot', '--init'], env=env, check=True)
    def reg(key, name, value):
        subprocess.run(['wine', 'reg', 'add', key, '/v', name, '/d', value, '/f'],
                       env=env, check=True, stdout=subprocess.DEVNULL)
    try:
        reg(r'HKCU\Software\Wine\Explorer', 'Desktop', 'Default')
        reg(r'HKCU\Software\Wine\Explorer\Desktops', 'Default', '640x480')
        cd = target / 'cd'
        (cd / 'GAME').mkdir(parents=True, exist_ok=True)
        # The drive check only needs GAME/GRUNTZ.EXE to exist on a CDROM drive.
        shutil.copy2(executable, cd / 'GAME/GRUNTZ.EXE')
        drive = prefix / 'dosdevices/d:'
        if drive.is_symlink():
            drive.unlink()
        drive.symlink_to(cd, target_is_directory=True)
        reg(r'HKLM\Software\Wine\Drives', 'D:', 'cdrom')
        reg(r'HKLM\Software\Monolith Productions\Gruntz\1.0', 'CdRom Drive', 'D:\\')
    finally:
        subprocess.run(['wineserver', '-k'], env=env, check=False)
        subprocess.run(['wineserver', '-w'], env=env, check=False)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data', type=Path, help='original game folder (remembered after first use)')
    parser.add_argument('--jobs', type=int, default=4, help='parallel compiler jobs')
    args = parser.parse_args(argv)
    target = ROOT / 'build/game'
    saved = target / 'data-path.txt'
    try:
        directory = args.data
        if directory is None and saved.is_file():
            directory = Path(saved.read_text().strip())
        if directory is None:
            parser.error('first run: nix run path:. -- --data "/path/to/Gruntz"')
        directory = directory.expanduser().resolve()
        assets = assets_in(directory)
        dlls = {name: Path(os.environ[variable]) for name, variable in (
            ('MSS32.DLL', 'GRUNTZ_MSS32'), ('SMACKW32.DLL', 'GRUNTZ_SMACKW32'))}
        for path in dlls.values():
            if not path.is_file():
                raise ValueError(f'runtime DLL missing: {path}')
        subprocess.run([sys.executable, str(ROOT / 'build.py'), '--jobs', str(args.jobs)], check=True)
        executable = ROOT / 'build/GRUNTZ.EXE'
        install_files(target / 'game', assets, executable, dlls)
        prepare_prefix(target, executable)
        saved.write_text(str(directory) + '\n')
        from gruntzbuild.play import write_play_sh
        runner = write_play_sh(target, ROOT)
        return subprocess.run([str(runner)], cwd=ROOT).returncode
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f'Cannot start Gruntz: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
