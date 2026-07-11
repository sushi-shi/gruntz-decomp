#!/usr/bin/env python3
"""data_home.py - the DATA-definition recovery metric + per-TU worklist.

Reloc-fidelity proved a global's *reference* binds to the right RVA - but binding
is reloc-MASKED: an `extern T g_x; DATA(rva)` makes the reference resolve while
NOTHING defines the storage. A real reconstruction needs each global DEFINED once,
in its owning TU (the .obj whose .data/.rdata/.bss block holds it), with the DATA()
mark on that definition and a single `extern` in the module header.

This tool measures + drives that. For every code-referenced global in .data/.bss
(the mutable + zero-init globals; .rdata const/vtable/RTTI is a later phase):
  - OWNER: reuse globals_attribute's .reloc xref DB. Referenced from exactly ONE
    unit -> PRIVATE, home the definition there (file-static rule, works for .bss
    too since it needs no RVA-ordering). Referenced from N units -> SHARED (needs a
    contiguity/decl tie-break; reported, not auto-homed).
  - STATUS in src/: scan every DATA(rva) mark and the line under it -
      DEFINED   = the line is a real definition (no `extern`)
      EXTERN    = only extern declaration(s) carry the DATA() (the current default)
      ABSENT    = no DATA() for this rva anywhere
  - SIZE: gap to the next referenced global (upper bound; refine later with Ghidra
    data-symbol sizes). .data initializer bytes are readable; .bss is zero.

METRIC: of the PRIVATE .data/.bss globals, how many are DEFINED in their owning TU.
Drive it to 100%. Worklist groups the undefined privates by owning unit so a worker
can define a whole TU's globals in one pass.

Usage:
  python -m gruntz.analysis.data_home              # summary + top-TU worklist
  python -m gruntz.analysis.data_home --unit foo   # one unit's globals to define
  python -m gruntz.analysis.data_home --json out.json
"""
import argparse
import json
import re
from collections import defaultdict
from pathlib import Path

from gruntz.analysis.xref import REPO, _load
from gruntz.analysis.globals_attribute import build_db, _sec_of, _rva_to_off

DATA_LO = 0x208000                       # start of .data (mutable + .bss tail)
SRC = REPO / "src"
DATA_LINE = re.compile(r"^\s*DATA\(0x([0-9a-fA-F]+)\)")


def scan_src_status():
    """rva -> {'defined': bool, 'externs': int, 'files': set}. A DATA() whose next
    non-blank code line lacks the `extern` keyword is a real definition."""
    st = defaultdict(lambda: {"defined": False, "externs": 0, "files": set()})
    for f in list(SRC.rglob("*.cpp")) + list((REPO / "include").rglob("*.h")):
        try:
            lines = f.read_text(errors="ignore").splitlines()
        except OSError:
            continue
        for i, ln in enumerate(lines):
            m = DATA_LINE.match(ln)
            if not m:
                continue
            rva = int(m.group(1), 16)
            nxt = next((lines[j] for j in range(i + 1, min(i + 4, len(lines)))
                        if lines[j].strip()), "")
            rec = st[rva]
            rec["files"].add(str(f.relative_to(REPO)))
            if re.search(r"\bextern\b", nxt):
                rec["externs"] += 1
            elif nxt.strip():
                rec["defined"] = True
    return st


def analyze():
    d, secs = _load()
    db, names = build_db()                # global_rva -> {unit: refcount}
    src = scan_src_status()
    rvas = sorted(r for r in db if r >= DATA_LO)
    nxt = {rvas[i]: rvas[i + 1] for i in range(len(rvas) - 1)}

    rows = []
    for rva in rvas:
        units = {u: n for u, n in db[rva].items() if u not in ("?", "ghidra")}
        sec = _sec_of(secs, rva) or "?"
        bss = _rva_to_off(secs, rva) is None            # no file bytes -> zero-init
        owner = next(iter(units)) if len(units) == 1 else ""
        kind = ("private" if len(units) == 1 else
                "shared" if len(units) > 1 else "unattributed")
        s = src.get(rva, {"defined": False, "externs": 0, "files": set()})
        rows.append({
            "rva": rva, "sec": ".bss" if bss else sec, "kind": kind,
            "owner": owner, "n_units": len(units),
            "size_ub": (nxt.get(rva, rva + 4) - rva),
            "defined": s["defined"], "externs": s["externs"],
            "n_files": len(s["files"]),
        })
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--unit")
    ap.add_argument("--json")
    a = ap.parse_args()
    rows = analyze()

    priv = [r for r in rows if r["kind"] == "private"]
    defined = [r for r in priv if r["defined"]]
    shared = [r for r in rows if r["kind"] == "shared"]
    unatt = [r for r in rows if r["kind"] == "unattributed"]

    if a.json:
        Path(a.json).write_text(json.dumps(rows, indent=1))
        print(f"wrote {a.json} ({len(rows)} globals)")
        return 0

    if a.unit:
        u = [r for r in priv if r["owner"] == a.unit and not r["defined"]]
        print(f"{a.unit}: {len(u)} undefined private globals to define\n")
        for r in sorted(u, key=lambda r: r["rva"]):
            print(f"  0x{r['rva']:06x} {r['sec']:5} size<={r['size_ub']:>5}  "
                  f"{'scattered x%d' % r['externs'] if r['externs'] else 'ABSENT':<14}")
        return 0

    print("=" * 68)
    print("DATA-DEFINITION HOMING  (.data + .bss code-referenced globals)")
    print("=" * 68)
    print(f"{len(rows)} referenced globals >= 0x{DATA_LO:x}")
    print(f"  PRIVATE (home-able now): {len(priv)}   "
          f"DEFINED in owner: {len(defined)}  ({100*len(defined)//max(1,len(priv))}%)")
    print(f"  SHARED  (need tie-break): {len(shared)}")
    print(f"  UNATTRIBUTED (ref'd only from unmatched code): {len(unatt)}")
    by_sec = defaultdict(int)
    for r in priv:
        by_sec[r["sec"]] += 1
    print(f"  private by section: " + "  ".join(f"{k}={v}" for k, v in by_sec.items()))
    print(f"\nTOP owning TUs by undefined-private-global count (worklist):")
    per = defaultdict(list)
    for r in priv:
        if not r["defined"]:
            per[r["owner"]].append(r)
    for u, gs in sorted(per.items(), key=lambda kv: -len(kv[1]))[:25]:
        bss = sum(1 for g in gs if g["sec"] == ".bss")
        print(f"  {u:<26} {len(gs):>4} undefined  ({bss} .bss / {len(gs)-bss} .data)")
    print(f"\n  -> `python -m gruntz.analysis.data_home --unit <name>` for one TU's list")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
