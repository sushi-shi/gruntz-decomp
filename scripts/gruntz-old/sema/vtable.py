"""gruntz.sema.vtable - `gruntz sema vtable`: binary vtable finder (any vtable,
RTTI or not; ILT thunks chased): dump a vtable's slots, or find which
vtable/slot holds a fn - the coverage the src-side catalog/hierarchy graph lacks
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


if __name__ == "__main__":
    # These modules are `gruntz sema <cmd>` implementations: the CLI owns argparse
    # and calls run(args), so `python -m` would import-and-exit silently (rc 0, no
    # output) and read as "identical"/"no findings". Fail loudly instead.
    import sys
    sys.exit("%s is a `gruntz sema` implementation, not a standalone entry point - "
             "run `gruntz sema %s ...` (or `gruntz sema -` for a batch on stdin)."
             % (__name__, __name__.rsplit(".", 1)[-1]))
