"""gruntz.walls.stale_markers - `@early-stop` markers on 100% functions.

`// @early-stop` means "a complete reconstruction parked below 100%", and
grepping it is the final-sweep worklist - which only works while the markers
are TRUE. They are not self-clearing: a fix elsewhere flips a parked
function and nothing removes its marker. A marker on a function already at
100.0 is a lie; a worklist nobody can trust is worse than none.

Method: each marker owns the next plain RVA() below it (RVA_COMPGEN only as
a fallback - it labels an adjacent compiler-generated body); the address
joins the Model for the mangled name, the name joins the compare report.

    gruntz walls stale-markers [--summary] [--max N]
"""

from __future__ import annotations

import re

from gruntz.core.paths import REPO

ROOTS = ("src", "include")
MARKER = re.compile(r"^\s*//\s*@early-stop\b")
RVA = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)")
RVA_COMPGEN = re.compile(r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)")


def scan():
    from gruntz.model import resolve
    from gruntz.walls.inventory import report_scores
    syms = {b.rva: b.name for b in resolve().functions if b.name}
    _path, sc = report_scores()
    pct = {}
    for (_u, name), p in sc.items():
        pct.setdefault(name, p)
    stale, live, unknown = [], 0, []
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            lines = path.read_text(errors="replace").split("\n")
            for i, line in enumerate(lines):
                if not MARKER.match(line):
                    continue
                rel = str(path.relative_to(REPO))
                addr = None
                for j in range(i + 1, min(i + 40, len(lines))):
                    m = RVA.search(lines[j])
                    if m:
                        addr = int(m.group(1), 16)
                        break
                if addr is None:
                    for j in range(i + 1, min(i + 4, len(lines))):
                        m = RVA_COMPGEN.search(lines[j])
                        if m:
                            addr = int(m.group(1), 16)
                            break
                name = syms.get(addr) if addr is not None else None
                if name is None or name not in pct:
                    unknown.append((rel, i + 1, addr))
                elif pct[name] >= 100.0:
                    stale.append((rel, i + 1, name))
                else:
                    live += 1
    return stale, live, unknown


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="gruntz walls stale-markers",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--summary", action="store_true")
    ap.add_argument("--max", type=int, default=None)
    a = ap.parse_args(argv)
    stale, live, unknown = scan()
    print(f"early-stop markers: {len(stale) + live + len(unknown)} total  |  "
          f"{live} LIVE (a real park)  |  {len(stale)} STALE (already 100%)  "
          f"|  {len(unknown)} unmapped")
    if not a.summary and stale:
        print("\nSTALE - the function is at 100.0, so the marker is a lie:")
        for rel, ln, name in sorted(stale):
            print(f"   {rel}:{ln}  {name[:64]}")
    if not a.summary and unknown:
        print("\nUNMAPPED (reported, never auto-deleted):")
        for rel, ln, addr in sorted(unknown)[:20]:
            print(f"   {rel}:{ln}  "
                  f"{'0x%06x' % addr if addr is not None else '(no RVA below)'}")
        if len(unknown) > 20:
            print(f"   ... {len(unknown) - 20} more")
    if a.max is not None and len(stale) > a.max:
        print(f"stale-markers: STALE {len(stale)} exceeds the {a.max} ratchet")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
