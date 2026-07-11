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
import tomllib
from collections import defaultdict
from pathlib import Path

from gruntz.analysis.xref import REPO, _load
from gruntz.analysis.globals_attribute import build_db, _sec_of, _rva_to_off

DATA_LO = 0x208000                       # start of .data (mutable + .bss tail)
SRC = REPO / "src"
DATA_LINE = re.compile(r"^\s*DATA\(0x([0-9a-fA-F]+)\)")
ARRAY_DIM = re.compile(r"\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]")


def _src_to_unit():
    """src-path -> unit name, from config/units.toml."""
    doc = tomllib.loads((REPO / "config/units.toml").read_text())
    m = {}
    for u in doc.get("unit", []):
        for key in ("source", "sources"):
            v = u.get(key)
            for s in ([v] if isinstance(v, str) else (v or [])):
                m[s] = u["unit"]
    return m


def scan_src_status():
    """rva -> {'def_files': set, 'extern_files': set}. A DATA() whose next non-blank
    code line lacks `extern` is a real DEFINITION; else it's an extern declaration."""
    st = defaultdict(lambda: {"def_files": set(), "extern_files": set()})
    for f in list(SRC.rglob("*.cpp")) + list((REPO / "include").rglob("*.h")):
        try:
            lines = f.read_text(errors="ignore").splitlines()
        except OSError:
            continue
        rel = str(f.relative_to(REPO))
        for i, ln in enumerate(lines):
            m = DATA_LINE.match(ln)
            if not m:
                continue
            rva = int(m.group(1), 16)
            nxt = next((lines[j] for j in range(i + 1, min(i + 4, len(lines)))
                        if lines[j].strip()), "")
            if not nxt.strip():
                continue
            (st[rva]["extern_files"] if re.search(r"\bextern\b", nxt)
             else st[rva]["def_files"]).add(rel)
    return st


def _classify(kind, owner, def_units, ndef):
    """CORRECT / DUP_DEF / WRONG_TU / MISSING_EXTERN / MISSING_ABSENT."""
    if ndef == 0:
        return "missing_extern", None
    if ndef > 1:
        return "dup_def", None
    du = next(iter(def_units))
    if kind == "private" and du != owner:
        return "wrong_tu", du
    return "correct", du


def analyze():
    d, secs = _load()
    db, names = build_db()                # global_rva -> {unit: refcount}
    src = scan_src_status()
    s2u = _src_to_unit()
    rvas = sorted(r for r in db if r >= DATA_LO)
    nxt = {rvas[i]: rvas[i + 1] for i in range(len(rvas) - 1)}

    rows = []
    for rva in rvas:
        units = {u: n for u, n in db[rva].items() if u not in ("?", "ghidra")}
        bss = _rva_to_off(secs, rva) is None            # no file bytes -> zero-init
        owner = next(iter(units)) if len(units) == 1 else ""
        kind = ("private" if len(units) == 1 else
                "shared" if len(units) > 1 else "unattributed")
        s = src.get(rva, {"def_files": set(), "extern_files": set()})
        def_units = {s2u.get(f, "?") for f in s["def_files"]}
        cat, def_unit = _classify(kind, owner, def_units, len(s["def_files"]))
        # absent = not even an extern declares it in src
        if cat == "missing_extern" and not s["extern_files"]:
            cat = "missing_absent"
        rows.append({
            "rva": rva, "sec": ".bss" if bss else (_sec_of(secs, rva) or "?"),
            "kind": kind, "owner": owner, "n_units": len(units),
            "size_ub": (nxt.get(rva, rva + 4) - rva),
            "cat": cat, "def_unit": def_unit,
            "n_def": len(s["def_files"]), "n_extern": len(s["extern_files"]),
        })
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--unit")
    ap.add_argument("--json")
    a = ap.parse_args()
    rows = analyze()

    if a.json:
        Path(a.json).write_text(json.dumps(rows, indent=1))
        print(f"wrote {a.json} ({len(rows)} globals)")
        return 0

    if a.unit:
        u = [r for r in rows if r["owner"] == a.unit and r["cat"] != "correct"]
        print(f"{a.unit}: {len(u)} globals needing a definition\n")
        for r in sorted(u, key=lambda r: r["rva"]):
            note = {"missing_absent": "ABSENT", "missing_extern": "extern x%d" % r["n_extern"],
                    "wrong_tu": "DEFINED IN %s" % r["def_unit"], "dup_def": "DUP x%d" % r["n_def"],
                    }.get(r["cat"], r["cat"])
            print(f"  0x{r['rva']:06x} {r['sec']:5} size<={r['size_ub']:>5}  {note}")
        return 0

    CATS = ["correct", "missing_absent", "missing_extern", "wrong_tu", "dup_def"]
    LABEL = {"correct": "CORRECT (defined once, in owner)",
             "missing_absent": "MISSING - absent (referenced, nothing in src)",
             "missing_extern": "MISSING - extern-only (reloc-masked, no storage)",
             "wrong_tu": "WRONG TU (defined, but not the xref owner)",
             "dup_def": "DUPLICATE (defined in >1 file)"}
    priv = [r for r in rows if r["kind"] == "private"]
    print("=" * 70)
    print("GLOBALS CENSUS  (.data + .bss code-referenced globals)")
    print("=" * 70)
    print(f"{len(rows)} referenced globals >= 0x{DATA_LO:x}   "
          f"[private {len(priv)} | shared {sum(1 for r in rows if r['kind']=='shared')} "
          f"| unattributed {sum(1 for r in rows if r['kind']=='unattributed')}]\n")
    cc = defaultdict(int)
    for r in rows:
        cc[r["cat"]] += 1
    for c in CATS:
        print(f"  {LABEL[c]:<50} {cc[c]:>5}")
    print(f"\n  DEFINED-CORRECTLY: {cc['correct']} / {len(rows)}  "
          f"({100*cc['correct']//max(1,len(rows))}%)   "
          f"defects (missing+wrong+dup): {len(rows)-cc['correct']-cc['missing_absent'] if False else sum(cc[c] for c in CATS[1:])}")
    # the actionable slice: private, owned by a RECOVERED TU, not yet correct
    act = [r for r in priv if r["owner"] not in ("", "(unrecovered)") and r["cat"] != "correct"]
    print(f"\nACTIONABLE now (private, owner is a recovered TU, not correct): {len(act)}")
    per = defaultdict(lambda: defaultdict(int))
    for r in act:
        per[r["owner"]][r["cat"]] += 1
    print("TOP owning TUs (worklist):")
    for u, cats in sorted(per.items(), key=lambda kv: -sum(kv[1].values()))[:22]:
        tot = sum(cats.values())
        brk = " ".join(f"{k.split('_')[-1]}:{v}" for k, v in cats.items())
        print(f"  {u:<24} {tot:>4}   ({brk})")
    print(f"\n  -> `python -m gruntz.analysis.data_home --unit <name>` for one TU's list")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
