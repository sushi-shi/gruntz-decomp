#!/usr/bin/env python3
r"""fuzz.py - drive replay.exe over many mutated entry states, one child each.

    python recomp/replay/fuzz.py build/replay/snap/<tag> \
        --obj build/replay/<unit>.objmod --fn '?Foo@CBar@@QAEHH@Z' \
        --field 0x0f71003c --pool-scan 0x0f710030:0x60 \
        --oob 0xdeadbeef --oob 0

    python recomp/replay/fuzz.py <snapdir> --obj M --fn N --arg 0 --arg 1 \
        --oob 0 --oob 1 --oob -1
      `--arg K` names the K-th incoming STACK argument by its offset from the
      recorded entry esp, so a pure query function whose only observable is its
      return value can be swept over its own argument space with no game launch.

Why a driver at all, and why one process per case
-------------------------------------------------
A mutated entry state can crash the function or fail to terminate - a loop bound
that no longer converges is the normal outcome of flipping a length field, not
an exotic one. wine has no fork, so isolation is a fresh process per case:
replay.exe --spawn already creates one (it has to, to claim the recorded
addresses before its own loader runs) and already carries the watchdog, so a
case that faults comes back as exit 4 and one that hangs as exit 3. Neither
stops the run.

IN-DISTRIBUTION vs OUT-OF-DISTRIBUTION - kept apart, deliberately
----------------------------------------------------------------
A mutation can produce state the program could never actually reach, and a
disagreement there may be two implementations differing on an impossible input
rather than a bug in either. So a value is only IN-DISTRIBUTION if the RECORDED
capture is observed to contain it in the same buffer at the same alignment
(`--pool-scan BASE:LEN`); anything supplied with `--oob` is out-of-distribution
and is reported in its own bucket. The two never blur into one pass/fail number.

The un-mutated case is run first and is the only one with ground truth: it is
checked against the RECORDED exit, and against a re-run of retail (the harness
self-test). Every mutated case can only be checked ours-against-retail.

Coverage
--------
`gruntz sema disasm <rva> --branches` prints the static conditional-branch
inventory; this shells out to it and prints it alongside the results, so the
number of arms a corpus could reach is visible next to the number it did. That
tooling already exists (gruntz.core.branches) and is not reimplemented here.
"""
import argparse
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = next(p for p in HERE.parents if (p / "flake.nix").exists())
sys.path.insert(0, str(HERE))
from snapshot import Snapshot  # noqa: E402

VERDICT = {0: "AGREE", 1: "DISAGREE", 2: "harness error", 3: "TIMEOUT (killed)",
           4: "CRASH (faulted)"}


def run_case(exe, snapdir, obj, fn, sets, timeout_ms, wine_timeout):
    cmd = ["wine", str(exe), snapdir, "--obj", obj, "--fn", fn, "--spawn",
           "--timeout", str(timeout_ms)]
    for addr, val in sets:
        cmd += ["--set", "0x%08x=0x%08x" % (addr, val)]
    env = dict(os.environ)
    env.setdefault("WINEDEBUG", "fixme-all,err-all")
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, env=env,
                           timeout=wine_timeout)
        return p.returncode, p.stdout
    except subprocess.TimeoutExpired:
        return 3, "(the wine process itself exceeded %ds)" % wine_timeout


def effect_of(out):
    m = re.search(r"changed (\d+) byte\(s\) in (\d+) region", out)
    return (int(m.group(1)), int(m.group(2))) if m else (None, None)


def pool_scan(snap, base, length):
    """Every distinct 4-byte value the recorded buffer actually contains, at
    4-byte alignment. That is the observed value population of that buffer -
    not a guess about what the field 'could' hold."""
    b = snap.bytes_at(base, length)
    if not b:
        raise SystemExit("fuzz: %08x+%x is not in the recorded snapshot" % (base, length))
    seen = []
    for i in range(0, len(b) - 3, 4):
        v = struct.unpack_from("<I", b, i)[0]
        if v not in seen:
            seen.append(v)
    return seen


def branches_for(rva):
    try:
        p = subprocess.run(["gruntz", "sema", "disasm", "0x%08x" % rva, "--branches"],
                           capture_output=True, text=True, cwd=str(REPO), timeout=180)
        return p.stdout.strip()
    except Exception as e:  # the inventory is context, not a dependency
        return "(gruntz sema disasm --branches unavailable: %s)" % e


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("snapdir")
    ap.add_argument("--obj", required=True, help="the .objmod from objbind.py")
    ap.add_argument("--fn", required=True, help="the mangled symbol to run as OURS")
    ap.add_argument("--arg", type=int, action="append", default=[],
                    help="mutate the K-th incoming stack argument (entry esp + 4 "
                         "+ 4K). Repeatable; combines with --oob.")
    ap.add_argument("--exe", default=str(HERE / "replay.exe"))
    ap.add_argument("--field", type=lambda s: int(s, 0), action="append", default=[],
                    help="dword address to mutate (repeatable)")
    ap.add_argument("--pool-scan", action="append", default=[],
                    help="BASE:LEN - take in-distribution values from this recorded "
                         "buffer (repeatable, paired with --field by position)")
    ap.add_argument("--oob", type=lambda s: int(s, 0), action="append", default=[],
                    help="an OUT-OF-DISTRIBUTION value to try in every field")
    ap.add_argument("--max-pool", type=int, default=12)
    ap.add_argument("--timeout-ms", type=int, default=20000)
    ap.add_argument("--wine-timeout", type=int, default=180)
    a = ap.parse_args()

    snapdir = Path(a.snapdir).resolve()
    snap = Snapshot(snapdir / "entry.snap")
    win_snapdir = subprocess.check_output(["winepath", "-w", str(snapdir)],
                                          text=True).strip()
    win_obj = subprocess.check_output(["winepath", "-w", str(Path(a.obj).resolve())],
                                      text=True).strip()
    for k in a.arg:
        a.field.append(snap.h["entry_esp"] + 4 + 4 * k)

    print("fuzz: %s  rva=%08x" % (snap.name, snap.h["target_rva"]))
    print("\nstatic branch inventory (what a corpus COULD reach):")
    print("  " + branches_for(snap.h["target_rva"]).replace("\n", "\n  "))

    cases = []  # (label, kind, sets)
    cases.append(("recorded (un-mutated)", "GROUND TRUTH", []))
    for i, field in enumerate(a.field):
        if i < len(a.pool_scan):
            base, length = a.pool_scan[i].split(":")
            pool = pool_scan(snap, int(base, 0), int(length, 0))[:a.max_pool]
            cur = struct.unpack_from("<I", snap.bytes_at(field, 4))[0]
            for v in pool:
                if v == cur:
                    continue
                cases.append(("[%08x] = %08x" % (field, v), "in-distribution",
                              [(field, v)]))
        for v in a.oob:
            cases.append(("[%08x] = %08x" % (field, v), "out-of-distribution",
                          [(field, v)]))

    print("\n%-28s %-20s %-16s %s" % ("CASE", "KIND", "VERDICT", "EFFECT"))
    print("-" * 92)
    tally = {}
    for label, kind, sets in cases:
        rc, out = run_case(a.exe, win_snapdir, win_obj, a.fn, sets, a.timeout_ms,
                           a.wine_timeout)
        n, r = effect_of(out)
        eff = "-" if n is None else "%d byte(s) / %d region(s)" % (n, r)
        print("%-28s %-20s %-16s %s" % (label, kind, VERDICT.get(rc, "rc=%d" % rc), eff))
        tally.setdefault(kind, []).append((VERDICT.get(rc, str(rc)), label, out))

    print()
    for kind, rows in tally.items():
        bad = [r for r in rows if r[0] == "DISAGREE"]
        skipped = [r for r in rows if r[0] in ("TIMEOUT (killed)", "CRASH (faulted)")]
        print("%-20s %d case(s): %d disagree, %d skipped (crash/hang)"
              % (kind, len(rows), len(bad), len(skipped)))
        for _, label, out in bad:
            print("   DISAGREE on %s%s" % (label,
                  "   <-- SPECULATIVE: the program may never reach this state"
                  if kind == "out-of-distribution" else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
