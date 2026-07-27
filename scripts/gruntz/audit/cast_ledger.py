#!/usr/bin/env python3
"""cast_ledger.py - account for EVERY reinterpret_cast still in the tree.

The cast campaign's rule is that a reinterpret_cast is a SYMPTOM: something is
mis-modeled (a member typed i32 that is really a pointer, a byte band standing in
for a struct, a phantom view of a real class). Driving them out means fixing the
declaration, not deleting the cast.

Some casts LOOK forced - by MFC's API, by the Win32 ABI, or by a layout C++ seems
unable to express. Those must at least be *named*, because an unexplained cast is
indistinguishable from un-started work. So this ledger sorts every remaining cast into

  PARKED  - it sits in a recognised seam, or its line carries a reason
  OPEN    - nobody has examined it yet

**PARKED IS NOT DONE** (user ruling, 2026-07-28): "reasons are never trusted and were
always disproven. The only thing they can give is a bit higher confidence. We will
still go through them in the future and most likely all of them will fold."

The track record earns that. Reasons retested in one session: "API-forced:
AFX_MODULE_STATE's layout lives in MFC's PRIVATE afxstat_.h" (false three ways - it
ships in the include set, and the slot is just AfxGetApp()); "byte-forced" BMP header
skips (a pad-view of BITMAPFILEHEADER/BITMAPINFO); "the PROVEN dual-band keep"; a
"PROVEN dual-role slot" that was a pun propping up a wrong owner. None survived.

So the number to drive to zero is the TOTAL, which `reinterpret_casts` already
ratchets. OPEN reaching 0 means every cast has been LOOKED at once - a milestone, not
the finish line. `gruntz.audit.cast_reasons` orders the parked ones for the next pass.

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
    # A cast whose TARGET is a Win32/DirectX SDK pointer typedef (LPPOINT, LPRECT,
    # LPDDCAPS, LPDDENUMCALLBACKA, ...) or a handle type is an API boundary by
    # construction - the SDK chose the type, we only name it at the call.
    ("win32-abi",
     r"<H[A-Z]\w*>|<LP[A-Z]\w*>|<LPARAM>|<WPARAM>|<LRESULT>|<DLGPROC>|<WNDPROC>"),
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
                # A cast written INSIDE a comment is prose, not a site. Three of these
                # were being counted as real casts (StatusBarMgr.h, ImageSets.h,
                # ModeObjInit.cpp), which inflates the very number this tool exists to
                # drive to zero. Strip the line comment before matching; a cast is never
                # split across a `//`.
                for _ in CAST.finditer(line.split("//", 1)[0]):
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

    # Word this so PARKED can never be read as finished: the total is the metric.
    print("cast ledger: %d casts total (the number to drive to 0)  |  %d never examined"
          "  |  %d examined + parked, still expected to fold"
          % (n_forced + n_open, n_open, n_forced))
    for name, n in forced.most_common():
        print("   %6d  %s" % (n, name))

    if not args.summary:
        # Print EVERY file. This listing is the campaign worklist, so a cap does not
        # tidy the output - it silently hides work. A [:40] here left 29 single-site
        # files invisible while the header still said "122 OPEN", and the by-file
        # column summed to 93. Partitioning off that list would have retired the
        # campaign with those sites never looked at.
        print("\nOPEN by file (the campaign worklist - each needs a model fix or a reason):")
        listed = 0
        for f, rows in sorted(openv.items(), key=lambda kv: (-len(kv[1]), str(kv[0]))):
            print("   %4d  %s" % (len(rows), f))
            listed += len(rows)
        assert listed == n_open, "by-file listing (%d) != OPEN total (%d)" % (listed, n_open)

    if args.max is not None and n_open > args.max:
        print("cast-ledger: OPEN %d exceeds the %d ratchet" % (n_open, args.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
