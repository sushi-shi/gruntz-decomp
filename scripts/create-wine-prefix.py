#!/usr/bin/env python3
"""create-wine-prefix.py - the local Gruntz GAME environment (play/test).

The sibling of create-toolchain-release.py: a one-shot provisioner, not part
of the build loop (the BUILD prefix is `gruntz tool wine --init`). It REQUIRES
the retail resources folder - the runtime set we cannot distribute:

    python3 scripts/create-wine-prefix.py <resources-dir> [--target ~/gruntz-wine]

and assembles:

    <target>/game/     Gruntz.REZ, GRUNTZ.VRZ, *.FNT, MSS32.DLL, SMACKW32.DLL
                       + GRUNTZ.retail.EXE (the control, from build/exe/);
                       `gruntz link` installs the rebuilt GRUNTZ.EXE here
    <target>/cd/GAME/  the CD check's target: GetGruntzDriveLetter() wants a
                       DRIVE_CDROM drive holding <L>:\\GAME\\GRUNTZ.EXE
    <target>/prefix3/  a dedicated wine prefix, SEPARATE from the build prefix

Prefix doctrine, all MEASURED (2026-08-10, the hand-grown ~/gruntz-wine):
  * NO `Version=win98` key - win98 mode does not bring up the WASAPI/mmdevapi
    path, so no audio driver can initialise; the game runs fine on default.
  * NO `Audio` driver pin and no cached device tree - a pin once froze a stale
    HDMI sink and killed sound; auto-probe follows the current default sink.
  * a 1024x768 virtual desktop, so the game cannot switch the host video mode.
  * D: maps to <target>/cd as a cdrom drive + the Monolith registry key.

Idempotent: existing files (saves, an already-populated game/) are never
overwritten; only missing pieces are filled in.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

#: the retail runtime set the resources folder MUST provide
REQUIRED = ("Gruntz.REZ", "GRUNTZ.VRZ", "LARGE.FNT", "MEDIUM.FNT",
            "SMALL.FNT", "TINY.FNT", "MSS32.DLL", "SMACKW32.DLL")

DEFAULT_TARGET = Path.home() / "gruntz-wine"


def log(msg: str) -> None:
    print(f"[wine-prefix] {msg}")


def die(msg: str) -> None:
    print(f"[wine-prefix] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def find_ci(d: Path, name: str) -> Path | None:
    low = name.lower()
    return next((p for p in d.iterdir() if p.name.lower() == low), None) \
        if d.is_dir() else None


def _wine(prefix: Path, *args: str) -> None:
    env = dict(os.environ, WINEPREFIX=str(prefix),
               WINEDLLOVERRIDES="mscoree,mshtml=", WINEDEBUG="fixme-all")
    subprocess.run(["wine", *args], check=False, env=env,
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)


def setup(resources: Path, target: Path) -> None:
    if not resources.is_dir():
        die(f"resources folder missing: {resources}")
    found = {name: find_ci(resources, name) for name in REQUIRED}
    missing = sorted(name for name, p in found.items() if p is None)
    if missing:
        die("resources folder lacks the retail runtime set: "
            + ", ".join(missing)
            + f"\n(searched {resources}; copy them from a retail Gruntz install)")

    game = target / "game"
    game.mkdir(parents=True, exist_ok=True)
    for _name, src in found.items():
        dst = game / src.name
        if not dst.exists():
            shutil.copy2(src, dst)
            log(f"installed {src.name}")

    # the retail control beside the rebuilt EXE (gruntz link installs that one)
    retail = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
    control = game / "GRUNTZ.retail.EXE"
    if retail.is_file() and not control.exists():
        shutil.copy2(retail, control)
        log("installed GRUNTZ.retail.EXE (the control)")

    # the CD check needs exactly <L>:\GAME\GRUNTZ.EXE on a cdrom-typed drive
    cd_game = target / "cd" / "GAME"
    cd_game.mkdir(parents=True, exist_ok=True)
    if not (cd_game / "GRUNTZ.EXE").exists() and retail.is_file():
        shutil.copy2(retail, cd_game / "GRUNTZ.EXE")
        log("installed cd/GAME/GRUNTZ.EXE (CD-check target)")

    prefix = target / "prefix3"
    if not (prefix / "drive_c").is_dir():
        log("creating game wineprefix (default windows version - NEVER win98, "
            "it kills audio) ...")
        prefix.mkdir(parents=True, exist_ok=True)
        env = dict(os.environ, WINEPREFIX=str(prefix),
                   WINEDLLOVERRIDES="mscoree,mshtml=")
        subprocess.run(["wineboot", "-u"], check=False, env=env,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        _wine(prefix, "reg", "add", r"HKCU\Software\Wine\Explorer",
              "/v", "Desktop", "/d", "Default", "/f")
        _wine(prefix, "reg", "add", r"HKCU\Software\Wine\Explorer\Desktops",
              "/v", "Default", "/d", "1024x768", "/f")

    dos_d = prefix / "dosdevices" / "d:"
    if not dos_d.is_symlink():
        dos_d.parent.mkdir(parents=True, exist_ok=True)
        if dos_d.exists():
            dos_d.unlink()
        dos_d.symlink_to(target / "cd")
        log(f"mapped D: -> {target / 'cd'}")
    _wine(prefix, "reg", "add", r"HKLM\Software\Wine\Drives",
          "/v", "D:", "/d", "cdrom", "/f")
    _wine(prefix, "reg", "add",
          r"HKLM\Software\Monolith Productions\Gruntz\1.0",
          "/v", "CdRom Drive", "/d", "D:\\", "/f")
    subprocess.run(["wineserver", "-w"], check=False,
                   env=dict(os.environ, WINEPREFIX=str(prefix)))
    log(f"ready: {target} (run with WINEPREFIX={prefix})")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("resources", help="retail resources folder "
                                      "(Gruntz.REZ, GRUNTZ.VRZ, fonts, DLLs)")
    ap.add_argument("--target", default=str(DEFAULT_TARGET),
                    help=f"environment root (default {DEFAULT_TARGET})")
    a = ap.parse_args()
    setup(Path(a.resources), Path(a.target))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
