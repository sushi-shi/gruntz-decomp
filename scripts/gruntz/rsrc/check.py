"""gruntz.rsrc.check - THE GATE: src/Gruntz/Gruntz.rc == retail's .rsrc.

    python -m gruntz.rsrc.check [--exe PE] [--out RES]

Compiles the tracked .rc with the era RC.EXE (gruntz.tool.rc) and
byte-compares EVERY compiled resource against what the retail PE's .rsrc
actually carries: type, name, language, payload bytes, and payload order,
with total coverage in both directions (nothing missing, nothing extra).
The retail image is the only oracle - no extracted bytes, no manifest.

The compiled .res is left at build/gen/gruntz.res (override with --out); it
is the container `gruntz link --res` consumes for the candidate's .rsrc.
"""

from __future__ import annotations

import sys
from pathlib import Path

from gruntz.core.paths import REPO, SRC
from gruntz.core.pe import Pe
from gruntz.rsrc.res import read_pe_rsrc, read_res, rt
from gruntz.tool import ToolError
from gruntz.tool import rc as rc_tool

RC_FILE = SRC / "Gruntz/Gruntz.rc"
RES_OUT = REPO / "build/gen/gruntz.res"


def check(exe: Path | str | None = None, out: Path | str = RES_OUT) -> int:
    try:
        rc_tool.compile(RC_FILE, out)
    except ToolError as e:
        print(f"[rsrc] check FAILED: {e}", file=sys.stderr)
        return 1
    retail = read_pe_rsrc(Pe(exe))
    ours = read_res(out)

    problems: list[str] = []
    for t, n, _lg, cp, _d in retail:
        if cp:
            problems.append(f"retail {rt(t)} {n!r} has codepage {cp} - unmodeled")
    rk = {(t, n, lg): d for t, n, lg, cp, d in retail}
    ck = {(t, n, lg): d for t, n, lg, d in ours}
    if len(rk) != len(retail):
        problems.append("retail carries duplicate (type, name, lang) keys")
    if len(ck) != len(ours):
        problems.append(f"{RC_FILE.name} compiles duplicate (type, name, lang) keys")
    for k in (kk for kk in rk if kk not in ck):
        problems.append(f"MISSING from {RC_FILE.name}: {rt(k[0])} {k[1]!r} lang {k[2]}")
    for k in (kk for kk in ck if kk not in rk):
        problems.append(f"EXTRA (not in retail): {rt(k[0])} {k[1]!r} lang {k[2]}")
    for k in (kk for kk in rk if kk in ck and ck[kk] != rk[kk]):
        a, b = rk[k], ck[k]
        if len(a) != len(b):
            problems.append(f"{rt(k[0])} {k[1]!r}: compiled {len(b)} B, "
                            f"retail {len(a)} B")
        else:
            at = next(i for i in range(len(a)) if a[i] != b[i])
            problems.append(f"{rt(k[0])} {k[1]!r}: byte mismatch at +{at:#x} "
                            f"({b[at]:#04x} != retail {a[at]:#04x})")
    if [r[:3] for r in retail] != [r[:3] for r in ours]:
        problems.append(".rc statement order diverges from retail payload order")

    if problems:
        for pr in problems:
            print(f"[rsrc] check: {pr}", file=sys.stderr)
        return 1
    tot = sum(len(d) for _t, _n, _lg, _cp, d in retail)
    print(f"[rsrc] check OK: {len(ours)}/{len(retail)} resources compiled from "
          f"{RC_FILE.relative_to(REPO)} byte-identical to the retail .rsrc "
          f"({tot:,} B payload), order preserved")
    return 0


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", help="PE to compare against (default: retail)")
    ap.add_argument("--out", type=Path, default=RES_OUT,
                    help="where to leave the compiled .res")
    a = ap.parse_args()
    return check(a.exe, a.out)


if __name__ == "__main__":
    raise SystemExit(main())
