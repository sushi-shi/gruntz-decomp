#!/usr/bin/env python3
"""strip_wall_prose.py - delete the REASON prose after every `@early-stop` marker.

WHY. The reason block is a causal story about why a function will not match, and the
corpus is not trustworthy: 1011 markers, and half assert a wall with no measurement
behind it at all. In one session lanes re-derived a dozen of them and were right every
time - three notes reading "no source lever" / "regalloc wall" / "block-layout wall"
turned into EXACT once someone read the disassembly instead of the comment. A note that
says "stop" on no evidence costs a lane a session; the same note deleted costs nothing,
because the disassembly is still there.

Two more reasons the prose is the wrong home:

  * PERCENTAGES ROT. They are a snapshot of OUR state, regenerable from
    `build/objdiff/report.json` in one command - and they go stale silently. One note
    claimed ~95.6% for a function that measured 72.33.
  * SEARCH RECORDS BELONG IN `config/axes/*.json`. A manifest records exactly which
    spellings were ruled out and can be re-run; prose cannot.

WHAT IS DELETED. Only the contiguous `//` comment lines BETWEEN `// @early-stop` and the
next non-comment line (normally the `RVA(...)` pin). Nothing else.

WHAT IS KEPT.
  * The `// @early-stop` marker itself - it is a gated marker (`label_style.py`
    ALLOWED_MARKERS), `stale_walls`/`stale_markers` consume it, and `rg '@early-stop' src`
    is the final-sweep worklist. The marker states a FACT (a complete reconstruction
    parked below 100%); only its justification is being removed.
  * Every comment ABOVE the marker - that is the "what this function does / what this
    field is" prose, which is the durable knowledge.

Deterministic and idempotent, so it can be re-run on main after each lane integration
rather than cherry-picked through a branch (docs/gotchas.md: re-run a tree-wide rewrite,
do not cherry-pick it).

    python -m gruntz.audit.strip_wall_prose --dry-run    # counts + a sample
    python -m gruntz.audit.strip_wall_prose              # apply
"""
import argparse
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
MARKER = "@early-stop"


def _is_marker(line):
    """True only for a comment whose ENTIRE content is the marker."""
    t = line.strip()
    return t.startswith("//") and t[2:].strip() == MARKER


def strip_file(text):
    """Return (new_text, blocks_stripped, lines_removed)."""
    lines = text.splitlines(keepends=True)
    out, i, blocks, removed = [], 0, 0, 0
    while i < len(lines):
        out.append(lines[i])
        # A marker line is a comment whose ENTIRE content is `@early-stop`. Anything
        # else that merely CONTAINS the token is prose - either a wrapped sentence that
        # happens to begin with it ("@early-stop on. Banked for the final sweep.") or a
        # reference to the concept ("the @early-stop here is RETIRED"). Matching those
        # would swallow real documentation, which a dry run caught.
        if not _is_marker(lines[i]):
            i += 1
            continue
        # Swallow the contiguous comment run that follows: it is the reason block.
        # STOP at another bare marker - two notes can sit back to back with no code
        # between them, and swallowing the second would delete a marker (measured: 6
        # markers lost on the first run, which is why this guard exists).
        j = i + 1
        while j < len(lines) and lines[j].lstrip().startswith("//"):
            if _is_marker(lines[j]):
                break
            j += 1
        if j > i + 1:
            blocks += 1
            removed += j - (i + 1)
        i = j
    return "".join(out), blocks, removed


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--root", default=str(REPO / "src"))
    args = ap.parse_args(argv)

    files = sorted(Path(args.root).rglob("*.cpp")) + sorted(Path(args.root).rglob("*.h"))
    tf = tb = tl = 0
    sample = []
    for p in files:
        t = p.read_text(errors="replace")
        if MARKER not in t:
            continue
        new, b, r = strip_file(t)
        if not b:
            continue
        tf += 1
        tb += b
        tl += r
        if len(sample) < 3:
            sample.append((p, t, new))
        if not args.dry_run:
            p.write_text(new)

    verb = "would strip" if args.dry_run else "stripped"
    print(f"strip_wall_prose: {verb} {tb} reason block(s) / {tl} comment line(s) "
          f"across {tf} file(s)")
    if args.dry_run and sample:
        p, old, new = sample[0]
        print(f"\n--- sample: {p.relative_to(REPO)} ---")
        o, n = old.splitlines(), new.splitlines()
        k = next((x for x, ln in enumerate(o) if MARKER in ln), 0)
        print("  BEFORE:")
        for ln in o[max(0, k - 2):k + 6]:
            print("   ", ln)
        m = next((x for x, ln in enumerate(n) if MARKER in ln), 0)
        print("  AFTER:")
        for ln in n[max(0, m - 2):m + 3]:
            print("   ", ln)
    return 0


if __name__ == "__main__":
    sys.exit(main())
