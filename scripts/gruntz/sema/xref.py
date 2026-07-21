"""gruntz.sema.xref - `gruntz sema xref`: retail caller/callee graph (attribution).

Who calls this fn (default), its own call targets (--callees), every call site
(--raw), or the caller ancestry tree chasing ILT thunks (--tree [--depth N]).
Targets: RVAs, exact mangled names, `CClass::Member`, or bare `Member` names
(ambiguous names print the candidate list).

Engine: gruntz.analysis.xref (shared with the attribution tools; also runnable
directly as `python -m gruntz.analysis.xref`).
"""
import sys

from gruntz.sema._common import run_tool


def run(args) -> None:
    flags = (["--callees"] if args.callees else []) + (["--raw"] if args.raw else [])
    if args.tree:
        flags += ["--tree", "--depth", str(args.depth)]
    sys.exit(run_tool("gruntz.analysis.xref", flags + args.target))
