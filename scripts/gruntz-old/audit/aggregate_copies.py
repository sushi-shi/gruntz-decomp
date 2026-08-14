#!/usr/bin/env python3
"""aggregate_copies.py - who copies a whole OBJECT that the other side spells as fields?

A `rep movsd` / `rep movsb` is cl's whole-aggregate copy. At /O2 it only survives when
the compiler could NOT scalar-replace the object - i.e. the SOURCE declared an object and
assigned it by value. So a count mismatch between our object and retail's is a MODELLING
statement, not a scheduling one:

  target > base   retail copied a struct we transcribed as loose scalars. Declare the
                  object and assign it (`RezElem40 rec = m_pData[i];`).
  base > target   WE invented a by-value copy retail does not make - usually a local
                  aggregate that should be a reference, or a member read through a copy.

Found on `CFaderMesh::RenderFrame` 0x17ef00: a 40-byte `rep movsd` into a stack local at
the loop head was `RezElem40 rec = m_meshBuf.m_pData[i];`, where the reconstruction had a
pointer walk. Modelling it - plus the RECTs the record contains, as struct copies rather
than brace-init from eight scalars - took that function 35.95 -> 90.88.

WHY THE COUNT IS THE RIGHT SIGNAL, and its two traps: match the MNEMONIC exactly. An
early cut of this sieve tested `"movs" in mnemonic` and matched `movswl`/`movsbl`
sign-extension loads, which have nothing to do with aggregates - it reported 13 hits, 12
of them noise.  The REP prefix is also mandatory: function contributions can carry inline
switch tables, and `0xa4` in `CGruntzMgr::HandleCommand`'s table linearly decodes as a
plain `movsb`.  It is data, not a one-byte aggregate copy. `rep movs[bwl]` only.

    python -m gruntz.audit.aggregate_copies              # whole tree
    python -m gruntz.audit.aggregate_copies --unit fader
    python -m gruntz.audit.aggregate_copies --max N      # exit 1 if hits exceed N
"""
import argparse
import json
import re
import sys
from pathlib import Path

from gruntz.core.branches import decode, obj_paths

REPO = Path(__file__).resolve().parents[3]
REPORT = REPO / "build" / "objdiff" / "report.json"
# The whole-aggregate copy, and nothing else. `movswl`/`movsbl` are sign-extends.
REP_MOVS = re.compile(r"^rep\s+movs[bwl]?$")


def scan(unit_filter=None):
    rep = json.loads(REPORT.read_text())
    hits = []
    for u in rep.get("units") or []:
        unit = u.get("name")
        if unit_filter and unit != unit_filter:
            continue
        sub = [f for f in (u.get("functions") or [])
               if float(f.get("fuzzy_match_percent") or 0.0) < 100.0]
        if not sub:
            continue
        bobj, tobj = obj_paths(unit)
        if not (bobj.is_file() and tobj.is_file()):
            continue
        bs, ts = decode(bobj), decode(tobj)
        for f in sub:
            name = f.get("name")
            bi, ti = bs.get(name), ts.get(name)
            if not bi or not ti:
                continue
            n_b = sum(1 for i in bi if REP_MOVS.match(i[1].strip()))
            n_t = sum(1 for i in ti if REP_MOVS.match(i[1].strip()))
            if n_b != n_t:
                hits.append((n_t - n_b, unit,
                             float(f.get("fuzzy_match_percent") or 0.0), n_b, n_t, name))
    hits.sort(reverse=True)
    return hits


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit")
    ap.add_argument("--max", type=int, default=None)
    a = ap.parse_args()

    hits = scan(a.unit)
    for d, unit, pct, nb, nt, name in hits:
        role = "retail copies an object we spell as fields" if d > 0 else \
               "we copy an object retail does not"
        print("%+d  %-18s %6.2f  base %d / target %d  %-46s  %s"
              % (d, unit, pct, nb, nt, role, name[:64]))
    print("aggregate-copy mismatches: %d" % len(hits))
    if a.max is not None and len(hits) > a.max:
        print("aggregate_copies: %d exceeds the %d ratchet" % (len(hits), a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
