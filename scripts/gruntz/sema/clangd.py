"""gruntz.sema.clangd - the clangd-LSP sema subcommands: refs / hover / rename
(all USR-exact, no grep collisions). `symbol`/`def` were retired (0 uses in
9,771 logged calls; the harness LSP covers them) - rename stays because the
harness LSP has no rename.

Engine: gruntz.analysis.clangd_query (shared with fingerprints/rename_member;
also runnable directly as `python -m gruntz.analysis.clangd_query`).
"""
import sys

from gruntz.sema._common import call_main


def _point_argv(args) -> list:
    """`<file> <line> [<col>]` -> the clangd_query positional list."""
    return [args.file, args.line] + ([args.col] if args.col is not None else [])


def run_point(args) -> None:                      # refs / hover share this
    sys.exit(call_main("gruntz.analysis.clangd_query", [args.sema, *_point_argv(args)]))


def run_rename(args) -> None:
    argv = ["rename", *_point_argv(args), args.new_name]
    if args.dry_run:
        argv.append("--dry-run")
    sys.exit(call_main("gruntz.analysis.clangd_query", argv))
