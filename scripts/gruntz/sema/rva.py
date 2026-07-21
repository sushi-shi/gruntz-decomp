"""gruntz.sema.rva - `gruntz sema rva`: one-shot address dossier.

Joins symbol_names.csv, library_labels.csv, Ghidra functions.csv and the
objdiff report.json - pure lookups, no analysis. A vtable-slot RVA (an ILT
jmp-thunk) is chased to the body and THAT is reported, so `sema rva <slot>`
and nvim's vg resolve the method.
"""
import json
import sys

from gruntz.sema._common import (GEN_NAMES, GHIDRA_FUNCTIONS, REPO, REPORT,
                                 csv_find, die, src_loc_of)


def run(args) -> None:
    try:
        rva = int(args.addr, 16)
    except ValueError:
        die(f"'{args.addr}' is not a hex RVA (e.g. 0x00080850)")
    print(f"RVA 0x{rva:08x}")

    claim = csv_find(GEN_NAMES, rva)
    def_rva, via = rva, None
    if not claim:
        from gruntz.analysis import vtable_scan as vs
        body = vs.chase_thunk(rva)
        if body is not None:
            c2 = csv_find(GEN_NAMES, body)
            if c2:
                claim, def_rva, via = c2, body, body
    if claim:
        via_s = f"  (via ILT thunk -> 0x{via:08x})" if via is not None else ""
        print(f"  src claim : {claim['name']}  [{claim['unit']}] "
              f"({claim.get('kind', '?')}){via_s}")
        loc = src_loc_of(def_rva)
        if loc:
            print(f"  src loc   : {loc[0]}:{loc[1]}")
    else:
        print("  src claim : (none - not reconstructed under src/)")

    librow = csv_find(REPO / "config" / "library_labels.csv", def_rva)
    if librow:
        print(f"  library   : {librow['name']}  {librow['lib']} / "
              f"{librow['confidence']} / {librow['source']}  "
              f"(carve-out: excluded from the match %)")

    grow = csv_find(GHIDRA_FUNCTIONS, def_rva, key="entry_rva")
    if grow:
        print(f"  ghidra    : {grow['name']}  size {grow['byte_size']} B")
    else:
        print("  ghidra    : (no function start at this RVA in the export)")

    if claim and REPORT.is_file():
        rep = json.loads(REPORT.read_text())
        pct = None
        for u in rep.get("units", []):
            if claim.get("unit") and u.get("name") != claim["unit"]:
                continue
            for fn in u.get("functions") or []:
                if fn.get("name") == claim["name"]:
                    pct = fn.get("fuzzy_match_percent")
                    break
            if pct is not None:
                break
        if pct is not None:
            print(f"  match     : {pct:.2f}% fuzzy"
                  + ("  (EXACT)" if pct >= 100.0 else ""))
    sys.exit(0)
