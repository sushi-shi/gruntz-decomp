"""Sieve: our base references a DIFFERENT CRT symbol than retail does.

A C function's COFF name carries one decoration underscore, so source `strcmp`
emits `_strcmp`.  A name with TWO leading underscores therefore means the source
itself wrote the underscore form: retail's `__strcmpi` proves the devs typed
`_strcmpi`, not the OLDNAMES alias `strcmpi`.  Where two CRT names are the same
routine (`_stricmp` / `_strcmpi`, `_chkstk` / `_alloca_probe`) the link succeeds
either way, so nothing but this comparison catches the wrong spelling -- and
objdiff scores relocation names, so it costs match percent at every call site.

A count mismatch on a name means one of three things, in rough order of value:
  * we spelled a CRT call differently from retail  (`__stricmp` 2 vs 0)
  * we call a routine retail never calls, or vice versa
  * a construct difference upstream  (`_atexit` counts a file-scope object with
    a destructor; `__alloca_probe` means retail called `_alloca` where we
    declared a large local, `__chkstk` is the large-frame probe)

    python -m gruntz.audit.crt_symbols [--min-delta 1] [--all]
"""

import argparse
import collections
import glob
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
# Names whose absence/presence is a delinker artefact, not a source fact.
IGNORE = {"__except_list"}
C_NAME = re.compile(r"^_{1,2}[a-z][a-z0-9_]*$")
RELOC = re.compile(r"IMAGE_REL_I386_(?:REL32|DIR32)\s+(\S+)")


def _counts(side: str) -> collections.Counter:
    c = collections.Counter()
    for obj in glob.glob(str(REPO / "build" / "objdiff" / side / "*.obj")):
        out = subprocess.run(["llvm-objdump", "-r", obj], capture_output=True, text=True).stdout
        for name in RELOC.findall(out):
            if C_NAME.match(name) and name not in IGNORE:
                c[name] += 1
    return c


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--min-delta", type=int, default=1)
    ap.add_argument("--all", action="store_true", help="also list matching names")
    args = ap.parse_args(argv)

    t, b = _counts("target"), _counts("base")
    rows = []
    for name in sorted(set(t) | set(b)):
        dt, db = t.get(name, 0), b.get(name, 0)
        if args.all or (dt != db and abs(dt - db) >= args.min_delta):
            rows.append((abs(dt - db), name, dt, db))
    rows.sort(reverse=True)

    print(f"crt-symbols: {len(rows)} name(s) where base and target disagree\n")
    print(f"{'target':>7} {'base':>7}  {'delta':>6}  symbol")
    for d, name, dt, db in rows:
        print(f"{dt:>7} {db:>7}  {db - dt:>+6}  {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
