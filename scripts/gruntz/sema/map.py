"""gruntz.sema.map - `gruntz sema map`: retail .text space map.

Layout overview / RVA-range listing / gaps / per-file breakdown (owner: TU /
MFC / CRT / EH / thunk / unknown). Forwards straight to the engine, which owns
the overview / range / at / gaps / units / find subcommands + --json.

Engine: gruntz.core.exe_map (shared with the docs/exe-map suite; also
runnable directly as `python -m gruntz.core.exe_map`).
"""
import sys


def run(args) -> None:
    from gruntz.core import exe_map
    sys.exit(exe_map.main(list(args.rest)) or 0)


if __name__ == "__main__":
    # These modules are `gruntz sema <cmd>` implementations: the CLI owns argparse
    # and calls run(args), so `python -m` would import-and-exit silently (rc 0, no
    # output) and read as "identical"/"no findings". Fail loudly instead.
    import sys
    sys.exit("%s is a `gruntz sema` implementation, not a standalone entry point - "
             "run `gruntz sema %s ...` (or `gruntz sema -` for a batch on stdin)."
             % (__name__, __name__.rsplit(".", 1)[-1]))
