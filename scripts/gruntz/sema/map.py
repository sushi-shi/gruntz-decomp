"""gruntz.sema.map - `gruntz sema map`: retail .text space map.

Layout overview / RVA-range listing / gaps / per-file breakdown (owner: TU /
MFC / CRT / EH / thunk / unknown). Forwards straight to the engine, which owns
the overview / range / at / gaps / units / find subcommands + --json.

Engine: gruntz.analysis.exe_map (shared with the docs/exe-map suite; also
runnable directly as `python -m gruntz.analysis.exe_map`).
"""
import sys

from gruntz.sema._common import run_tool


def run(args) -> None:
    sys.exit(run_tool("gruntz.analysis.exe_map", args.rest))
