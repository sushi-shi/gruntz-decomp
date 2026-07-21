"""gruntz.sema.classof - `gruntz sema class`: vtable slots + hierarchy tags for
a class; give a fn RVA/mangled name instead to find its owning slot(s)
(new/override/inherited + origin class), falling back to the raw binary scan
for non-RTTI vtables and thunk-indirect slots.

Engine: gruntz.core.vtable_hierarchy (+ vtable_scan for the binary
fallback); also runnable as `python -m gruntz.core.vtable_hierarchy`.
"""

import sys

from gruntz.sema._common import call_main, die


def _class_of_fn(query: str) -> int:
    """FUNCTION -> vtable topology: which class(es) hold this fn in which slot,
    tagged new/override/inherited with the origin class; then the owner's table."""
    import csv as _csv
    import tempfile
    rva = None
    try:
        rva = int(query, 16)
    except ValueError:
        pass
    with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as tf:
        tmp = tf.name
    rc = call_main("gruntz.core.vtable_hierarchy", ["--csv", tmp])
    if rc:
        die("vtable_hierarchy --csv failed (run `gruntz init` first?)")
    hits = []
    for r in _csv.DictReader(open(tmp)):
        try:
            row_rva = int(r["fn_rva"], 16)
        except ValueError:
            continue
        if (rva is not None and row_rva == rva) or \
           (rva is None and query in (r.get("fn_name") or "")):
            hits.append(r)
    if not hits:
        # Fall back to the BINARY scan: covers non-RTTI vtables and thunk-indirect
        # slots the reconstructed VTBL/hierarchy graph can't see (`sema vtable --holds`).
        if rva is not None:
            try:
                from gruntz.core import vtable_scan as vs
                bhits = vs.find_holding(rva)
            except Exception:
                bhits = []
            if bhits:
                print(f"vtable slots holding {query} (binary scan):")
                for v, k, via in bhits:
                    cls = v["rtti"] or f"non-rtti {vs.fn_label(v['first'])}"
                    print(f"  vtable 0x{v['start']:06x} ({v['start']+vs.IMAGEBASE:#010x})  "
                          f"{vs.confidence(v):<9} slot[{k}] (+0x{k*4:x})"
                          f"{'  via thunk' if via else ''}   {cls}")
                return 0
        print(f"no vtable slot holds '{query}' (not a virtual / command-table dispatched, "
              f"or its slot points elsewhere)")
        return 1
    print(f"vtable slots holding {query}:")
    owners = []
    for r in hits:
        print(f"  {r['class']}  slot[{r['slot_index']}]  {r['disposition']:<9} "
              f"origin={r['origin_class']}  {r['fn_name']}")
        if r["disposition"] in ("new", "override") and r["class"] not in owners:
            owners.append(r["class"])
    for owner in owners[:2]:  # the defining class(es), not every inheritor
        print()
        call_main("gruntz.core.vtable_hierarchy", ["--class", owner])
    return 0


def run(args) -> None:
    argv = ["--class", args.name]
    if args.tree:
        argv.append("--tree")
    # hex RVA or mangled/Fn-name -> function->slot topology lookup instead
    looks_fn = args.name.lower().startswith("0x") or "@" in args.name
    if looks_fn:
        sys.exit(_class_of_fn(args.name))
    sys.exit(call_main("gruntz.core.vtable_hierarchy", argv))
