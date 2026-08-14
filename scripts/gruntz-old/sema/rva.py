"""gruntz.sema.rva - `gruntz sema rva`: one-shot address dossier.

Joins the source/retail symbol inventory, functions_static_libs.tsv and the
objdiff report - pure lookups over gruntz.core, no analysis. A vtable-slot RVA
(an ILT jmp-thunk) is chased to the body and THAT is reported, so
`sema rva <slot>` and nvim's vg resolve the method.
"""
import sys

from gruntz.core import get_context
from gruntz.core.pe import REPO
from gruntz.core.retail_functions import by_rva as retail_functions_by_rva
from gruntz.sema._common import csv_find, die, src_loc_of


def run(args) -> None:
    try:
        rva = int(args.addr, 16)
    except ValueError:
        die(f"'{args.addr}' is not a hex RVA (e.g. 0x00080850)")
    ctx = get_context()
    db = ctx.symbols
    print(f"RVA 0x{rva:08x}")

    def src_claim(r):
        nm_unit = db.names.get(r)
        return None if nm_unit is None or nm_unit[1] == "retail" else nm_unit

    claim, def_rva, via = src_claim(rva), rva, None
    if not claim:
        from gruntz.core import vtable_scan as vs
        body = vs.chase_thunk(rva)
        if body is not None and src_claim(body):
            claim, def_rva, via = src_claim(body), body, body
    if claim:
        nm, unit = claim
        via_s = f"  (via ILT thunk -> 0x{via:08x})" if via is not None else ""
        print(f"  src claim : {nm}  [{unit}] ({db.kind.get(def_rva, '?')}){via_s}")
        loc = src_loc_of(def_rva)
        if loc:
            print(f"  src loc   : {loc[0]}:{loc[1]}")
    else:
        print("  src claim : (none - not reconstructed under src/)")

    from gruntz.core.library_labels import rows as library_rows
    librow = next((r for r in library_rows()
                   if int(r["rva"], 16) == def_rva), None)
    if librow:
        print(f"  library   : {librow['name']}  {librow['lib']} / "
              f"{librow['confidence']} / {librow['source']}  "
              f"(carve-out: excluded from the match %)")

    retail = retail_functions_by_rva().get(def_rva)
    if retail:
        print(f"  retail    : admitted function boundary, size {retail['size']} B")
    else:
        print("  retail    : (no function start at this RVA)")

    if claim:
        pct = ctx.report.fn_pct(claim[0], unit=claim[1] or None)
        if pct is not None:
            print(f"  match     : {pct:.2f}% fuzzy"
                  + ("  (EXACT)" if pct >= 100.0 else ""))
    sys.exit(0)


if __name__ == "__main__":
    # These modules are `gruntz sema <cmd>` implementations: the CLI owns argparse
    # and calls run(args), so `python -m` would import-and-exit silently (rc 0, no
    # output) and read as "identical"/"no findings". Fail loudly instead.
    import sys
    sys.exit("%s is a `gruntz sema` implementation, not a standalone entry point - "
             "run `gruntz sema %s ...` (or `gruntz sema -` for a batch on stdin)."
             % (__name__, __name__.rsplit(".", 1)[-1]))
