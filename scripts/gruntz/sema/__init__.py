"""gruntz.sema - the `gruntz sema` semantic-navigation surface, one module per
subcommand (browse this directory to see every tool):

    xref.py         retail caller/callee graph        (engine: analysis/xref.py)
    disasm.py       target/base/rich/diff disassembly (engine: sema/dump_target.py + llvm-objdump)
    dump_target.py  retail fn dump: bytes+disasm+relocs (disasm's target-side engine)
    rva.py          address dossier (claim/lib/ghidra/%)
    match.py        per-function/unit match summary   (engine: match/status.py)
    classof.py      class vtable slots / fn->slot     (engine: analysis/vtable_hierarchy.py)
    vtable.py       binary vtable dump / holder find  (engine: analysis/vtable_scan.py)
    strings.py      per-fn string set / reverse find
    map.py          retail .text space map            (engine: analysis/exe_map.py)
    clangd.py       symbol/def/refs/hover/rename      (engine: analysis/clangd_query.py)

Shared engines stay in gruntz/analysis (they serve build gates and campaign
tools too); a sema-only engine lives here. cli.py owns argparse and delegates
each subcommand to <module>.run(args) via run_logged().
"""
import sys


def run_logged(args) -> None:
    """Dispatch a sema subcommand with usage logging (rc captured through
    sys.exit; covers the delegating AND the inline-dossier subcommands)."""
    from gruntz.sema import _common
    rc = 0
    try:
        args.func(args)
    except SystemExit as e:
        rc = e.code if isinstance(e.code, int) else (0 if e.code is None else 1)
        _common.log_invocation(rc)
        raise
    _common.log_invocation(rc)
