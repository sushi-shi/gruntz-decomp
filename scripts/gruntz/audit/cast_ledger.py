#!/usr/bin/env python3
"""cast_ledger.py - account for EVERY reinterpret_cast still in the tree.

The cast campaign's rule is that a reinterpret_cast is a SYMPTOM: something is
mis-modeled (a member typed i32 that is really a pointer, a byte band standing in
for a struct, a phantom view of a real class). Driving them out means fixing the
declaration, not deleting the cast.

Some casts are nonetheless FORCED - by MFC's API, by the Win32 ABI, or by a layout
that C++ inheritance genuinely cannot express. Those are fine, but they must be
*named*, not merely tolerated: an unexplained cast is indistinguishable from
un-started work. So this ledger sorts every remaining cast into

  FORCED  - it sits in a recognised seam, or its line carries a reason
  OPEN    - nobody has explained it yet; it is the campaign's worklist

and prints the OPEN ones grouped by file so the next pass has a work list rather
than a number.  A cast counts as explained when the seam it lives in matches one
of the FORCED patterns below, or when its own line (or the line above it) mentions
one of the reason keywords - which is what every seam this campaign added does.

  python -m gruntz.audit.cast_ledger              # summary + the OPEN worklist
  python -m gruntz.audit.cast_ledger --summary    # just the bucket counts
  python -m gruntz.audit.cast_ledger --max N      # gate: exit 1 if OPEN exceeds N
"""
import argparse
import collections
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")

# A cast matching one of these is structurally forced; the label says why.
FORCED = [
    ("mfc-position",
     r"GetHeadPosition\(\)|GetStartPosition\(\)|<POSITION>|m_posCache"),
    ("mfc-voidref-out", r"void\s*\*\s*&"),
    ("win32-abi",
     r"<H[A-Z]\w*>|<LPARAM>|<WPARAM>|<LPDWORD>|<LRESULT>|<DLGPROC>|<WNDPROC>"),
    ("i64-halves-pun",
     r"<i64\s*\*>\s*\(\s*&|<u64\s*\*>\s*\(\s*&"),
]

# ...and a cast whose own line (or the line above) says one of these is explained.
# The reason VOCABULARY is deliberately small and closed - a cast is accounted for
# only when its lines say, in one of these terms, why it cannot be modelled away:
#   language-forced / API-forced / forced by   - C++ or an API leaves no alternative
#   byte-forced / byte-evidenced / no reloc / bare imm - the target's bytes require it
#   one seam / at one seam / the pun / overlay - it IS the single named boundary
#   faithful                                    - it is what the devs wrote (proved)
#   PROVEN / proven                             - the shape was established from bytes
#   @identity-TODO                              - an open identity, evidence recorded
REASON = re.compile(
    r"language-forced|API-forced|forced by|byte-forced|byte-evidenced|no reloc|"
    r"bare imm|one seam|at one seam|the pun|overlay|faithful|PROVEN|proven|"
    r"@identity-TODO",
    re.I,
)

CAST = re.compile(r"reinterpret_cast\s*<")


def scan():
    forced = collections.Counter()
    openv = collections.defaultdict(list)
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            lines = path.read_text(errors="replace").split("\n")
            for i, line in enumerate(lines):
                for _ in CAST.finditer(line):
                    ctx = " ".join(lines[max(0, i - 3):i + 2])
                    label = None
                    for name, pat in FORCED:
                        if re.search(pat, line) or re.search(pat, ctx):
                            label = name
                            break
                    if label is None and REASON.search(ctx):
                        label = "explained-seam"
                    if label:
                        forced[label] += 1
                    else:
                        openv[str(path.relative_to(REPO))].append(
                            (i + 1, line.strip()[:96]))
    return forced, openv


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--summary", action="store_true", help="bucket counts only")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 when the OPEN count exceeds N (ratchet)")
    args = ap.parse_args()

    forced, openv = scan()
    n_open = sum(len(v) for v in openv.values())
    n_forced = sum(forced.values())

    print("cast ledger: %d total  |  %d accounted for  |  %d OPEN"
          % (n_forced + n_open, n_forced, n_open))
    for name, n in forced.most_common():
        print("   %6d  %s" % (n, name))

    if not args.summary:
        print("\nOPEN by file (the campaign worklist - each needs a model fix or a reason):")
        for f, rows in sorted(openv.items(), key=lambda kv: -len(kv[1]))[:40]:
            print("   %4d  %s" % (len(rows), f))

    if args.max is not None and n_open > args.max:
        print("cast-ledger: OPEN %d exceeds the %d ratchet" % (n_open, args.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
