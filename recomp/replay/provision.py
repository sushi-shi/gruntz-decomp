#!/usr/bin/env python3
r"""provision.py - stand retail GRUNTZ.EXE up under wine so a capture can run.

This is a thin, purpose-built driver over the machinery that already exists in
`scripts/archive/dynamic-trace/provision_trace.py`. That tool was retired for
CLASS ATTRIBUTION (static `vtable_scan` beats it) - but its *asset provisioning*
is orthogonal to why it was retired, and it works, so we reuse it rather than
re-deriving the archive.org URLs, the CD-check tree and the software-GL wine env.

What we reuse, verbatim, by importing the archived module:

    fetch_retail_assets()   the ~168 MB REZ/VRZ/fonts/DLLs fetch from the ISO
    provision_cd()          the D:\GAME\GRUNTZ.EXE tree the CD check scans for
    provision_wineprefix()  a play prefix: virtual desktop, D:=cdrom, 32-bit
                            software GL (llvmpipe) - all measured host fixes
    play()                  launch under that env

What we do NOT reuse: the Frida gadget fetch, the Ghidra cc dump and the JS
trace-script generation. Capture here is our own MSVC-built DLL (see capture.c),
so none of that applies.

    python recomp/replay/provision.py                 # provision only
    python recomp/replay/provision.py --play --play-timeout 90

The injected DLL is placed as SFMAN32.DLL, exactly the load hook the archived
tracer used: `SFManager_SelectBestDevice` (RVA 0x0f8970) LoadLibraryA's it
unconditionally at its head, and is called once from CGruntzMgr::Run+0x9c8. So
DllMain runs mid-init, before the main loop. Everything after that point -
which is the whole game - is hookable; anything strictly before it is not.
"""
import argparse
import importlib.util
import os
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = next(p for p in HERE.parents if (p / "flake.nix").exists())
ARCHIVED = REPO / "scripts" / "archive" / "dynamic-trace" / "provision_trace.py"


def _archived():
    """Import the archived provisioner as a module (it is not on the package path)."""
    spec = importlib.util.spec_from_file_location("provision_trace", ARCHIVED)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    if mod.REPO != REPO:
        # The module resolves REPO by walking up from its own file, so a worktree
        # checkout resolves to that worktree. Assert it rather than trust it.
        raise SystemExit("provision: archived module resolved REPO=%s, expected %s"
                         % (mod.REPO, REPO))
    return mod


def provision(dll=None, mode="retail"):
    pt = _archived()
    game_dir = REPO / "build" / "game" / mode
    game_dir.mkdir(parents=True, exist_ok=True)

    if mode == "retail":
        asset_dir = pt.fetch_retail_assets(REPO / "build" / "game" / "retail-assets")
        exe_src = REPO / "build" / "exe" / "GRUNTZ.EXE"
        if not exe_src.exists():
            raise SystemExit("provision: no %s - run `gruntz init` first" % exe_src)
        for a in ("Gruntz.REZ", "GRUNTZ.VRZ", "MSS32.DLL", "SMACKW32.DLL",
                  "LARGE.FNT", "MEDIUM.FNT", "SMALL.FNT", "TINY.FNT"):
            pt._link_or_copy(asset_dir / a, game_dir / a)
        pt._link_or_copy(exe_src, game_dir / "GRUNTZ.EXE")
        game_exe = "GRUNTZ.EXE"
    else:
        raise SystemExit("provision: only mode=retail is supported")

    # The capture DLL masquerades as SFMAN32.DLL. Unlike the Frida gadget it has
    # no side-car config, so a symlink would be fine - but copy anyway so the
    # game dir is self-contained and a rebuilt DLL is picked up explicitly.
    if dll is not None:
        dst = game_dir / "SFMAN32.DLL"
        if dst.exists() or dst.is_symlink():
            dst.unlink()
        shutil.copyfile(dll, dst)
        print("[provision] capture DLL -> %s" % dst)
    elif not (game_dir / "SFMAN32.DLL").exists():
        print("[provision] note: no SFMAN32.DLL in %s - the game will run "
              "un-instrumented (LoadLibraryA simply fails)." % game_dir)

    cd_dir = pt.provision_cd(mode, game_exe)
    env = pt.provision_wineprefix(cd_dir)
    print("[provision] ready: (cd %s && wine %s)" % (game_dir, game_exe))
    return pt, env, game_dir, game_exe


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--dll", help="capture DLL to install as SFMAN32.DLL")
    ap.add_argument("--play", action="store_true", help="launch the game after provisioning")
    ap.add_argument("--play-timeout", type=int, default=None,
                    help="kill the game after N seconds (non-interactive smoke)")
    a = ap.parse_args()
    pt, env, game_dir, game_exe = provision(a.dll)
    if a.play:
        pt.play(env, game_dir, game_exe, timeout=a.play_timeout)
    return 0


if __name__ == "__main__":
    sys.exit(main())
