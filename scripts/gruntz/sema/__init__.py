"""gruntz.sema - the `gruntz sema` semantic-navigation surface, one module per
subcommand (browse this directory to see every tool):

    xref.py         retail caller/callee graph        (in-process over core)
    disasm.py       target/base/rich/diff disassembly (dump_target + llvm-objdump)
    dump_target.py  retail fn dump: bytes+disasm+relocs (in-process over core)
    rva.py          address dossier (claim/lib/ghidra/%) (in-process over core)
    strings.py      per-fn string set / reverse find  (in-process over core)
    match.py        per-function/unit match summary   (in-process: match/status)
    classof.py      class vtable slots / fn->slot     (in-process: vtable_hierarchy)
    vtable.py       binary vtable dump / holder find  (in-process: vtable_scan)
    map.py          retail .text space map            (in-process: core/exe_map)
    clangd.py       refs/hover/rename                 (in-process: clangd_query)

Everything runs IN ONE PROCESS over gruntz/core (Context: EXE + symbol db
loaded once); child processes only for true externals (llvm-objdump, the
clangd server, wine cl). `gruntz sema -` is batch mode: newline-delimited
sema commands on stdin answered against one Context. rc convention: 0
answered, 1 answered-NO, 2 error. cli.py owns argparse and delegates each
subcommand to <module>.run(args) via run_logged().
"""


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
