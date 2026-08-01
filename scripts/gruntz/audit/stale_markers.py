#!/usr/bin/env python3
"""stale_markers.py - which `@early-stop` markers sit on functions that are ALREADY 100%?

`// @early-stop` means "a complete reconstruction parked below 100%", and
`rg '@early-stop' src` is the documented final-sweep worklist. That only works while the
markers are true. They are not self-clearing: when a fix elsewhere in the TU flips a
parked function - an /O2 ripple, a header retype, a sibling's fold - nothing goes back
and removes its marker.

Measured 2026-07-28: ~149 of them sat on functions already scoring 100.0, so the
"worklist" was mostly noise and reading it top-down meant re-opening solved functions.
That is the same failure as a capped listing (see cast_ledger): a worklist nobody can
trust is worse than no worklist.

Method: for each marker, find the next `RVA(addr, size)` below it - that is the function
the marker belongs to - map the address to its mangled name via
`build/gen/symbol_names.csv`, and look the name up in `build/objdiff/report.json`.

  STALE   the function is at 100.0 - the marker is a lie, delete it
  LIVE    below 100.0 - a real park
  UNKNOWN no RVA below the marker, or the symbol is not in the report (reported, never
          auto-deleted: an unmapped marker is exactly where a wrong guess would do harm)

    python -m gruntz.audit.stale_markers            # census + the STALE list
    python -m gruntz.audit.stale_markers --summary  # counts only
    python -m gruntz.audit.stale_markers --max N    # exit 1 if STALE exceeds N
"""
import argparse
import csv
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")
MARKER = re.compile(r"^\s*//\s*@early-stop\b")
# The OWNING function is normally the next plain `RVA(` below the marker. RVA_COMPGEN is
# excluded while that search succeeds: it labels a compiler-generated body (a
# scalar-deleting dtor, a vbase helper) that may merely sit between the marker and its
# real owner. Matching it made the marker on zBitVec::operator= resolve to the adjacent
# ??_GzBitVec dtor. A directly marked RVA_COMPGEN is accepted only as a fallback when no
# plain RVA follows; UserLogicCtorEmit deliberately binds an emitted inline constructor
# that way.
RVA = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)")
RVA_COMPGEN = re.compile(r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)")


def symbol_by_rva():
    """rva -> mangled name, from the generated label map."""
    path = REPO / "build/gen/symbol_names.csv"
    if not path.is_file():
        raise SystemExit("[stale-markers] %s missing - run `gruntz build`" % path)
    out = {}
    with path.open() as fh:
        for row in csv.DictReader(fh):
            addr = row.get("rva") or row.get("address") or row.get("addr")
            name = row.get("symbol") or row.get("name")
            if not addr or not name:
                continue
            try:
                out[int(addr, 16)] = name
            except ValueError:
                continue
    return out


def scores():
    path = REPO / "build/objdiff/report.json"
    if not path.is_file():
        raise SystemExit("[stale-markers] %s missing - run `gruntz build`" % path)
    rep = json.loads(path.read_text())
    out = {}
    for unit in rep["units"]:
        for fn in unit.get("functions", []):
            # report.json OMITS fuzzy_match_percent when it is exactly 0.0 (serde
            # skip-default), so the default here must be 0.0, not 100.0. Defaulting it
            # the other way once silently excluded every 0% function from a worklist.
            out[fn["name"]] = fn.get("fuzzy_match_percent", 0.0)
    return out


def scan():
    syms, pct = symbol_by_rva(), scores()
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
                # the owning function is the next RVA() below the marker
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


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--summary", action="store_true", help="counts only")
    ap.add_argument("--max", type=int, default=None, help="exit 1 if STALE exceeds N")
    a = ap.parse_args()

    stale, live, unknown = scan()
    print("early-stop markers: %d total  |  %d LIVE (a real park)  |  %d STALE (already "
          "100%%)  |  %d unmapped" % (len(stale) + live + len(unknown), live, len(stale),
                                      len(unknown)))

    if not a.summary and stale:
        print("\nSTALE - the function is at 100.0, so the marker is a lie:")
        for rel, ln, name in sorted(stale):
            print("   %s:%d  %s" % (rel, ln, name[:64]))

    if a.max is not None and len(stale) > a.max:
        print("stale-markers: STALE %d exceeds the %d ratchet" % (len(stale), a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
