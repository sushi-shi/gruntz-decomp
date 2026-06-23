#!/usr/bin/env python3
"""List Gruntz functions in the TUs I changed that are <100% (the 1 regression is
among them: it was exact, now isn't). Sorted by fuzzy% desc - a near-100% one is
the prime suspect."""
import json
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
r = json.load(open(REPO / "build/objdiff/report.json"))
units = r["units"] if isinstance(r, dict) and "units" in r else r
TOUCHED = ("UserLogic", "GameObjectCtors", "Grunt", "MapMgr", "TileTrigger", "Wormhole", "GameMode", "GameApp", "GameWnd")

def fpct(f):
    m = f.get("measures") or f
    return (m.get("fuzzy_match_percent") or m.get("match_percent") or 0)

rows = []
for u in units:
    nm = u.get("name", "")
    if not any(t.lower() in nm.lower() for t in TOUCHED):
        continue
    for f in (u.get("functions") or u.get("sections") or []):
        p = fpct(f)
        if 0 < p < 100:
            rows.append((p, nm, f.get("name", "?")))
for p, nm, fn in sorted(rows, reverse=True)[:25]:
    print(f"{p:6.1f}%  {nm:24} {fn[:60]}")
print(f"\n{len(rows)} non-exact funcs in touched Gruntz units")
