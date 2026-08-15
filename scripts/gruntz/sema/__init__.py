"""gruntz.sema - the semantic-navigation surface over the retail image.

    gruntz sema rva     <addr>            address dossier: the winning binding,
                                          its aliases, channel, extent, match%
    gruntz sema disasm  <rva|name>        annotated retail i386 assembly
    gruntz sema dump    <rva|name>        raw bytes + relocation targets + asm
    gruntz sema xref    <rva|name>        callers, callees and referent sites
    gruntz sema strings [<rva>|--find s]  string literals a function reaches
    gruntz sema vtable  <rva>             a vtable's slots / who holds a fn
    gruntz sema class   <Class|fn>        a class's vtables, slot by slot
    gruntz sema map     [sub ...]         retail address-space map
    gruntz sema match   <unit|rva|name>   objdiff scores for a unit / function

Every module is also a direct entry: `python3 -m gruntz.sema.xref 0x136180`.
`gruntz sema -` is batch mode: newline-delimited view commands on stdin,
answered against ONE loaded Model and image (a 40-query investigation pays one
parse instead of forty).

sema is a READ-ONLY consumer with four inputs and no policy of its own:
the Model (`gruntz.model.resolve`) for identity, the retail image
(`gruntz.core.pe`) for bytes, `build/objdiff/report.json` for scores and
`config/units.toml` for the unit list. It writes nothing.

Doctrine: assembly only. Nothing here decompiles - views annotate real
instruction bytes with Model labels, and a question the labels cannot answer
is reported as unanswered rather than guessed.

rc convention: 0 answered, 1 answered-NO (valid query, no hit / differs),
2 error (bad input, missing prerequisite).
"""

from __future__ import annotations

import sys

SUBCOMMANDS = {
    "rva": "gruntz.sema.rva",
    "disasm": "gruntz.sema.disasm",
    "dump": "gruntz.sema.dump_target",
    "dump_target": "gruntz.sema.dump_target",
    "xref": "gruntz.sema.xref",
    "strings": "gruntz.sema.strings",
    "vtable": "gruntz.sema.vtable",
    "class": "gruntz.sema.classof",
    "classof": "gruntz.sema.classof",
    "map": "gruntz.sema.map",
    "match": "gruntz.sema.match",
}


class SemaError(Exception):
    """A real error (bad input, missing prerequisite) - rc 2."""


def die(msg: str):
    """Raise the rc-2 error. Answered-NO paths return 1 instead."""
    raise SemaError(msg)


def parse_rva(text: str) -> int:
    """A hex address, with or without the 0x."""
    try:
        return int(text, 16)
    except ValueError:
        die(f"'{text}' is not a hex RVA (e.g. 0x00153810)")


def run(module: str, argv: list[str]) -> int:
    """Run one subcommand module's main(), mapping SemaError to rc 2."""
    import importlib
    try:
        return importlib.import_module(module).main(argv)
    except SemaError as e:
        print(f"[sema] ERROR: {e}", file=sys.stderr)
        return 2
    except BrokenPipeError:
        return 0


def batch() -> int:
    """Answer newline-delimited view commands from stdin in one process."""
    import shlex
    rc = 0
    for line in sys.stdin:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        print(f"== gruntz sema {line}")
        rc = main(shlex.split(line)) or rc
        sys.stdout.flush()
    return rc


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help", "help"):
        print(__doc__.strip())
        return 0 if argv else 2
    if argv[0] == "-":
        return batch()
    sub, rest = argv[0], argv[1:]
    if sub not in SUBCOMMANDS:
        print(f"gruntz sema: unknown view {sub!r} (have: "
              f"{', '.join(sorted(set(SUBCOMMANDS)))})", file=sys.stderr)
        return 2
    return run(SUBCOMMANDS[sub], rest)
