"""gruntz.sema.clangd - the clangd-LSP sema subcommands: refs / hover / rename
(all USR-exact, no grep collisions). `symbol`/`def` were retired (0 uses in
9,771 logged calls; the harness LSP covers them) - rename stays because the
harness LSP has no rename.

Engine: gruntz.core.clangd_query (shared with fingerprints/rename_member;
also runnable directly as `python -m gruntz.core.clangd_query`).
"""
import sys

from gruntz.sema._common import call_main


def _point_argv(args) -> list:
    """`<file> <line> [<col>]` -> the clangd_query positional list."""
    return [args.file, args.line] + ([args.col] if args.col is not None else [])


def run_point(args) -> None:                      # refs / hover share this
    sys.exit(call_main("gruntz.core.clangd_query", [args.sema, *_point_argv(args)]))


def run_rename(args) -> None:
    argv = ["rename", *_point_argv(args), args.new_name]
    if args.dry_run:
        argv.append("--dry-run")
    sys.exit(call_main("gruntz.core.clangd_query", argv))


if __name__ == "__main__":
    # These modules are `gruntz sema <cmd>` implementations: the CLI owns argparse
    # and calls run(args), so `python -m` would import-and-exit silently (rc 0, no
    # output) and read as "identical"/"no findings". Fail loudly instead.
    import sys
    sys.exit("%s is a `gruntz sema` implementation, not a standalone entry point - "
             "run `gruntz sema %s ...` (or `gruntz sema -` for a batch on stdin)."
             % (__name__, __name__.rsplit(".", 1)[-1]))
