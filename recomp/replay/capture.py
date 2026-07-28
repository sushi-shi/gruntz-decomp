#!/usr/bin/env python3
r"""capture.py - drive the record half: configure the DLL, run the game, collect.

    python recomp/replay/capture.py census                     # address-space map
    python recomp/replay/capture.py probe  --seconds 90        # which candidates fire
    python recomp/replay/capture.py grab 0x000f9280 --name '?MakeButeSectionKey@@YAHPADPBD1@Z'

`grab` finds the target's direct call sites with hookgen, writes capture.cfg,
installs recomp/replay/SFMAN32.DLL into the game directory, launches the game
for `--seconds`, and leaves `entry.snap` / `exit.snap` in build/replay/.

Everything the game writes lands in build/replay/ (gitignored). The game itself
needs the retail assets, which provision.py fetches - let it.
"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = next(p for p in HERE.parents if (p / "flake.nix").exists())
OUT = REPO / "build" / "replay"
GAME = REPO / "build" / "game" / "retail"
DLL = HERE / "SFMAN32.DLL"

sys.path.insert(0, str(HERE))
import hookgen  # noqa: E402


def win_path(p, prefix):
    return subprocess.check_output(
        ["winepath", "-w", str(p)],
        env={**dict(__import__("os").environ), "WINEPREFIX": str(prefix)},
        text=True).strip()


def write_cfg(lines):
    GAME.mkdir(parents=True, exist_ok=True)
    (GAME / "capture.cfg").write_text("\n".join(lines) + "\n")
    print("[capture] capture.cfg:\n  " + "\n  ".join(lines[:8])
          + ("\n  ... (%d lines)" % len(lines) if len(lines) > 8 else ""))


def run_game(seconds):
    sys.path.insert(0, str(HERE))
    import provision
    pt, env, game_dir, game_exe = provision.provision(DLL if DLL.exists() else None)
    pt.play(env, game_dir, game_exe, timeout=seconds)


def cmd_census(a):
    OUT.mkdir(parents=True, exist_ok=True)
    prefix = REPO / "build" / "wineprefix-game"
    write_cfg(["mode=census", "out=" + win_path(OUT, prefix)])
    run_game(a.seconds)
    c = OUT / "census.txt"
    print(c.read_text().splitlines()[-1] if c.exists() else "[capture] no census.txt")


def cmd_probe(a):
    OUT.mkdir(parents=True, exist_ok=True)
    prefix = REPO / "build" / "wineprefix-game"
    tmpl = OUT / "probe.cfg.tmpl"
    if not tmpl.exists():
        raise SystemExit("capture: no %s - generate the candidate list first" % tmpl)
    lines = [ln.replace("out=OUTDIR", "out=" + win_path(OUT, prefix))
             for ln in tmpl.read_text().splitlines()]
    write_cfg(lines)
    run_game(a.seconds)
    hits = OUT / "hits.txt"
    if not hits.exists():
        raise SystemExit("capture: no hits.txt - the DLL did not arm (see capture.log)")
    rows = [ln.split() for ln in hits.read_text().splitlines() if not ln.startswith("#")]
    rows = [(int(r[0], 16), int(r[1])) for r in rows if len(r) == 2]
    live = sorted([r for r in rows if r[1]], key=lambda r: -r[1])
    print("[capture] %d of %d candidates were called:" % (len(live), len(rows)))
    for rva, n in live:
        print("   0x%08x  %8d" % (rva, n))


def cmd_grab(a):
    OUT.mkdir(parents=True, exist_ok=True)
    prefix = REPO / "build" / "wineprefix-game"
    img = hookgen.Image(REPO / "build" / "exe" / "GRUNTZ.EXE")
    target = int(a.target, 0)
    th, sites = hookgen.sites_for(img, target)
    if not sites:
        raise SystemExit("capture: 0x%08x has no direct call site to patch" % target)
    for f in ("entry.snap", "exit.snap", "capture.log"):
        (OUT / f).unlink(missing_ok=True)
    lines = ["mode=capture",
             "out=" + win_path(OUT, prefix),
             "target=0x%08x" % target,
             "name=" + (a.name or ("rva_%08x" % target)),
             "hit=%d" % a.hit,
             "full=%d" % (1 if a.full else 0),
             "cap=0x%x" % a.cap]
    lines += ["site=0x%08x" % s for s in sites]
    write_cfg(lines)
    run_game(a.seconds)
    log = OUT / "capture.log"
    if log.exists():
        print("[capture] capture.log:\n" + log.read_text())
    for f in ("entry.snap", "exit.snap"):
        p = OUT / f
        print("[capture] %-12s %s" % (f, ("%d bytes" % p.stat().st_size)
                                      if p.exists() else "MISSING"))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--seconds", type=int, default=75, help="play-session length")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("census").set_defaults(fn=cmd_census)
    sub.add_parser("probe").set_defaults(fn=cmd_probe)
    g = sub.add_parser("grab")
    g.add_argument("target")
    g.add_argument("--name", default=None)
    g.add_argument("--hit", type=int, default=0)
    g.add_argument("--full", action="store_true", help="tier 2: every committed region")
    g.add_argument("--cap", type=lambda s: int(s, 0), default=0x4000000)
    g.set_defaults(fn=cmd_grab)
    a = ap.parse_args()
    if not DLL.exists():
        raise SystemExit("capture: no %s - run recomp/replay/build.sh capture" % DLL)
    return a.fn(a) or 0


if __name__ == "__main__":
    sys.exit(main())
