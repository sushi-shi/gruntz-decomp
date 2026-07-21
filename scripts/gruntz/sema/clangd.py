"""gruntz.sema.clangd - the clangd-LSP sema subcommands: symbol / def / refs /
hover / rename (all USR-exact, no grep collisions).

Engine: gruntz.analysis.clangd_query (shared with fingerprints/rename_member;
also runnable directly as `python -m gruntz.analysis.clangd_query`).
"""
import sys

from gruntz.sema._common import run_tool


def _point_argv(args) -> list:
    """`<file> <line> [<col>]` -> the clangd_query positional list."""
    return [args.file, args.line] + ([args.col] if args.col is not None else [])


def run_symbol(args) -> None:
    sys.exit(run_tool("gruntz.analysis.clangd_query", ["symbol", args.query]))


def run_point(args) -> None:                      # def / refs / hover share this
    sys.exit(run_tool("gruntz.analysis.clangd_query", [args.sema, *_point_argv(args)]))


def run_rename(args) -> None:
    argv = ["rename", *_point_argv(args), args.new_name]
    if args.dry_run:
        argv.append("--dry-run")
    sys.exit(run_tool("gruntz.analysis.clangd_query", argv))
