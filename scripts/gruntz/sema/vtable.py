"""gruntz.sema.vtable - `gruntz sema vtable`: binary vtable finder (any vtable,
RTTI or not; ILT thunks chased): dump a vtable's slots, or find which
vtable/slot holds a fn - the coverage the src-side VTBL/hierarchy graph lacks
(non-RTTI tables, thunk-indirect slots).

Engine: gruntz.core.vtable_scan (shared with the build/gates; also runnable
directly as `python -m gruntz.core.vtable_scan`).
"""
import sys

from gruntz.sema._common import call_main, die


def run(args) -> None:
    tgt = args.target
    try:
        rva = int(tgt, 16)
    except ValueError:
        die(f"'{tgt}' is not a hex RVA (vtable takes a vtable start or a fn RVA)")
    if args.dump:
        mode = "--dump"
    elif args.holds:
        mode = "--holds"
    else:  # auto: a discovered vtable start -> dump; otherwise treat as a fn -> holds
        from gruntz.core import vtable_scan as vs
        mode = "--dump" if vs.vtable_at(rva) is not None else "--holds"
    sys.exit(call_main("gruntz.core.vtable_scan", [mode, tgt]))
