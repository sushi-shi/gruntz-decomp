"""gruntz.walls.priors - every prior verdict on a row, before any A/B.

A wall row can already carry a written verdict, and there are TWO independent
stores that hold one.  Screening only the source comment reads half the record:
7 of 17 rows on one lane's worklist carried a `codex_wall_reviews.tsv` review
that nobody saw, and one of them listed the exact A/B that lane was queuing.

  source     the contiguous `//` block directly above the function's `RVA(...)`
             pin - what the matcher who parked it wrote, plus the `@early-stop`
             marker itself.  Re-derive the residue; the prose can be stale.
  review     config/codex_wall_reviews.tsv, keyed by rva AND source hash, so a
             row is reported `current` only when the body has not changed since
             the review was written; `STALE` means the verdict predates an edit.

    gruntz walls priors <rva|mangled|CClass::Member>...
    gruntz walls priors --todo N          the head of the campaign queue
    gruntz walls priors --stdin           tokens from a pipe, one worklist
"""

from __future__ import annotations

import argparse
import re
import sys

from gruntz.core.paths import REPO

ROOTS = ("src", "include")
PIN = re.compile(r"\bRVA(?:_COMPGEN|_DYNINIT)?\s*\(\s*(0x[0-9a-fA-F]+)")
COMMENT = re.compile(r"^\s*(?://|/\*|\*)")


def _pin_sites() -> dict[int, list[tuple[str, int]]]:
    """{rva: [(relative path, 1-based line of the pin)]} over the whole tree."""
    sites: dict[int, list[tuple[str, int]]] = {}
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            rel = str(path.relative_to(REPO))
            for i, line in enumerate(path.read_text(errors="replace").split("\n")):
                m = PIN.search(line)
                if m:
                    sites.setdefault(int(m.group(1), 16), []).append((rel, i + 1))
    return sites


def _comment_above(rel: str, line: int) -> list[str]:
    """The contiguous comment block immediately above a pin, in source order."""
    lines = (REPO / rel).read_text(errors="replace").split("\n")
    out: list[str] = []
    i = line - 2                       # 0-based index of the line above the pin
    while i >= 0 and COMMENT.match(lines[i]):
        out.append(lines[i].strip())
        i -= 1
    return list(reversed(out))


def _targets(a) -> list[str]:
    toks = list(a.target)
    if a.stdin:
        toks += [w for w in sys.stdin.read().split() if w]
    if a.todo:
        from gruntz.walls.inventory import build
        toks += [r["rva"] for r in build(a.unit, todo=True)[:a.todo] if r["rva"]]
    return toks


def report(token: str, sites: dict, reviews: dict, fresh: set[int]) -> bool:
    """Print both stores for one row; True when either carried a verdict."""
    from gruntz.walls.diagnose import _locate
    b, why = _locate(token)
    if b is None:
        print(f"\n{token}\n  [priors] {why}")
        return False
    from gruntz.walls.inventory import baseline_rows, report_scores
    bank = baseline_rows().get(b.rva)
    _p, scores = report_scores()
    cur = scores.get((b.unit, b.name))
    head = f"0x{b.rva:06x}  {b.unit}/{b.name}"
    if cur is not None:
        head += f"   cur {cur:.2f}"
    if bank:
        head += f"  best {bank[0]:.2f}  hist {bank[1]:.2f}"
    print(f"\n{head}")

    found = False
    for rel, line in sites.get(b.rva, ()):
        block = _comment_above(rel, line)
        if block:
            found = True
            print(f"  source   {rel}:{line}")
            for text in block:
                print(f"      {text}")
        else:
            print(f"  source   {rel}:{line}  (no comment above the pin)")
    if b.rva not in sites:
        print("  source   no RVA() pin found in src/ or include/")

    row = reviews.get(b.rva)
    if row is None:
        print("  review   none")
    else:
        found = True
        state = "current" if b.rva in fresh else "STALE (body edited since)"
        print(f"  review   {row['status']}/{row['wall_class']}  [{state}]")
        print(f"      {row['evidence']}")
    return found


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        prog="gruntz walls priors", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("target", nargs="*", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--todo", type=int, metavar="N",
                    help="also screen the first N rows of the campaign queue")
    ap.add_argument("--unit", help="restrict --todo to one unit")
    ap.add_argument("--stdin", action="store_true", help="read tokens from a pipe")
    a = ap.parse_args(argv)
    from gruntz.walls import check_unit
    check_unit(a.unit)
    toks = _targets(a)
    if not toks:
        ap.error("give a target, --stdin, or --todo N")

    from gruntz.walls.reviews import current as current_reviews, load
    sites, reviews = _pin_sites(), load()
    fresh = set(current_reviews())
    hits = sum(report(t, sites, reviews, fresh) for t in toks)
    print(f"\n[priors] {hits}/{len(toks)} row(s) already carry a written verdict")
    return 0


if __name__ == "__main__":
    sys.exit(main())
