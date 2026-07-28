#!/usr/bin/env python3
r"""verdict.py - one command, one function, one answer: does OUR code do what
retail's does on the state the real game produced?

    python recomp/replay/verdict.py '?ComputeCellFlags@CMapMgr@@QAEXHHH@Z'
    python recomp/replay/verdict.py 0x00077790 --hit 3 --seconds 110
    python recomp/replay/verdict.py <name> --replay-only        # re-run the diff
    python recomp/replay/verdict.py <name> --control            # the negative one

It chains the four steps that were previously run by hand, and the chaining is
the point: every one of them has an argument that must agree with the others
(the unit that defines the symbol, the rva the capture hooks, the image base the
module was bound against, the mangled name the replay looks up), and a
disagreement between any two is exactly how a harness produces a confident wrong
answer.

    1. objbind.py    bake the unit's object, relocations bound to game_base+rva
    2. capture.py    hook the target's call sites, run the game, snapshot
                     entry/entry-again/exit around one call
    3. replay.exe    restore, run RETAIL, run OURS, diff every writable byte
    4.               keep the snapshots under build/replay/snap/<name>/ so the
                     verdict can be re-run, mutated or fuzzed without another
                     90-second game launch

`--control` re-runs the SAME snapshot with `--set-ours`, which perturbs the OURS
side only and MUST come back DISAGREE. Run it whenever a verdict comes back
green: a comparison that cannot go red is worth nothing, and this harness has
already produced one such - a `--set` parser that wrote its terminator into
argv[] in place, so every fuzz case ran unmutated and reported agreement.
"""
import argparse
import csv
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = next(p for p in HERE.parents if (p / "flake.nix").exists())
OUT = REPO / "build" / "replay"
SNAP = OUT / "snap"


def lookup(who):
    rows = list(csv.DictReader((REPO / "build/gen/symbol_names.csv").open()))
    if who.startswith("0x"):
        rva = int(who, 16)
        hit = [r for r in rows if int(r["rva"], 16) == rva and r["kind"] == "func"]
    else:
        hit = [r for r in rows if r["name"] == who and r["kind"] == "func"]
    if not hit:
        raise SystemExit("verdict: '%s' is not a function in symbol_names.csv" % who)
    if len(hit) > 1:
        raise SystemExit("verdict: '%s' is ambiguous: %s"
                         % (who, [r["name"] for r in hit]))
    r = hit[0]
    return r["name"], int(r["rva"], 16), r["unit"]


def win(p):
    return subprocess.check_output(["winepath", "-w", str(p)], text=True).strip()


def sh(cmd, **kw):
    print("[verdict] $ " + " ".join(str(c) for c in cmd))
    return subprocess.run(cmd, **kw)


def safe(name):
    return re.sub(r"[^A-Za-z0-9_.-]", "_", name)[:80]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("who", help="mangled name or 0x<rva>")
    ap.add_argument("--hit", type=int, default=0, help="capture the Nth call")
    ap.add_argument("--seconds", type=int, default=100)
    ap.add_argument("--replay-only", action="store_true",
                    help="reuse the stored snapshots (no game launch)")
    ap.add_argument("--cross", default=None,
                    help="run THIS function on the snapshot stored for another "
                         "one (e.g. Shape7 on Shape1's). Same signature only; "
                         "only [3] OURS-vs-RETAIL is a verdict.")
    ap.add_argument("--control", action="store_true",
                    help="also run the --set-ours negative control")
    ap.add_argument("--set", action="append", default=[],
                    help="mutate the entry state (both sides); see replay.exe")
    ap.add_argument("--timeout", type=int, default=180000)
    ap.add_argument("--cap", default="0x10000000",
                    help="per-phase snapshot buffer cap in the game process. An "
                         "IN-GAME state is several times the menu one; the DLL "
                         "prints the exact requirement when it does not fit.")
    a = ap.parse_args()

    name, rva, unit = lookup(a.who)
    tag = "%s_%08x" % (safe(name), rva)
    snapdir = SNAP / tag
    if a.cross:
        cname, crva, _ = lookup(a.cross)
        if name[name.find("@"):] != cname[cname.find("@"):]:
            raise SystemExit(
                "verdict: --cross refused. '%s' and '%s' do not share a "
                "signature, so the recorded registers and stack are not this "
                "function's arguments." % (name, cname))
        snapdir = SNAP / ("%s_%08x" % (safe(cname), crva))
        a.replay_only = True
    mod = OUT / ("%s.objmod" % unit)
    exe = HERE / "replay.exe"
    if not exe.exists():
        raise SystemExit("verdict: no %s - run recomp/replay/build.sh replay" % exe)

    print("[verdict] %s\n          rva %08x  unit %s" % (name, rva, unit))

    # 1. bake -----------------------------------------------------------------
    r = sh([sys.executable, str(HERE / "objbind.py"), unit, "-o", str(mod),
            "--fn", name])
    if r.returncode:
        return r.returncode

    # 2. capture --------------------------------------------------------------
    if not a.replay_only:
        r = sh([sys.executable, str(HERE / "capture.py"), "--seconds",
                str(a.seconds), "grab", "0x%08x" % rva, "--name", name,
                "--hit", str(a.hit), "--cap", a.cap])
        if r.returncode:
            return r.returncode
        # A zero-byte snapshot is the cap overflow, and it used to sail on into
        # a replay that then failed on an unrelated unpack. Treat "exists" and
        # "has bytes" as the same question, because they are.
        missing = [f for f in ("entry.snap", "exit.snap")
                   if not (OUT / f).exists() or (OUT / f).stat().st_size == 0]
        if missing and (OUT / "capture.log").exists():
            log = (OUT / "capture.log").read_text()
            if "NEEDS" in log or "OVERFLOW" in log:
                print("[verdict] the snapshot did not fit:")
                for ln in log.splitlines():
                    if "NEEDS" in ln or "OVERFLOW" in ln:
                        print("          " + ln)
                return 5
        if missing:
            print("[verdict] NOT REACHED: the session never called %s "
                  "(no %s). This is a property of the play session, not of the "
                  "harness - try a different session, or a different target."
                  % (name, ", ".join(missing)))
            return 3
        snapdir.mkdir(parents=True, exist_ok=True)
        for f in ("entry.snap", "entry2.snap", "exit.snap", "capture.log"):
            if (OUT / f).exists():
                shutil.move(str(OUT / f), str(snapdir / f))
    if not (snapdir / "entry.snap").exists():
        raise SystemExit("verdict: no stored snapshot at %s - drop --replay-only"
                         % snapdir)

    # 3. replay ---------------------------------------------------------------
    base = [str(exe), win(snapdir), "--obj", win(mod), "--fn", name, "--spawn",
            "--timeout", str(a.timeout)]
    if a.cross:
        base += ["--cross"]
    for s in a.set:
        base += ["--set", s]
    r = sh(["wine"] + base)
    rc = r.returncode

    # 4. the negative control -------------------------------------------------
    #
    # Perturb the CALLER'S FRAME, on the OURS side only, and require DISAGREE.
    #
    # entry_esp + 0x40 upwards is above the stack boundary, so it IS compared;
    # it is the caller's live frame, so neither side writes it (a callee's own
    # frame and everything it calls live BELOW entry_esp); and it is not an
    # argument, so the run does not diverge for a behavioural reason. Eight
    # consecutive dwords rather than one, because a single dword could land on
    # an out-parameter that both sides then overwrite - which would make the
    # control quietly pass, i.e. exactly the failure it exists to catch.
    #
    # And then it is CHECKED, not assumed: an AGREE here is reported as a
    # HARNESS FAILURE, because it means this verdict's comparison could not
    # have gone red.
    if a.control:
        import struct
        h = (snapdir / "entry.snap").open("rb").read(256)
        esp = struct.unpack_from("<I", h, 52)[0]      # SnapHeader.entry_esp
        sets = []
        for k in range(8):
            sets += ["--set-ours", "0x%08x=0xdeadbe%02x" % (esp + 0x40 + 4 * k, k)]
        print("\n[verdict] NEGATIVE CONTROL: perturbing 8 dword(s) at %08x on the "
              "OURS side only. This run MUST report DISAGREE." % (esp + 0x40))
        c = subprocess.run(["wine"] + base + sets, capture_output=True, text=True)
        sys.stdout.write(c.stdout)
        sys.stdout.write(c.stderr)
        if "VERDICT: DISAGREE" in c.stdout:
            print("[verdict] control OK - the comparison can go red.")
        else:
            print("[verdict] *** CONTROL FAILED *** the comparison did NOT go red "
                  "under a one-sided perturbation. Treat the verdict above as "
                  "meaningless until this is explained.")
            return 4
    return rc


if __name__ == "__main__":
    sys.exit(main())
