"""gruntz.rsrc - the resource section, proven from source.

    gruntz rsrc check               compile src/Gruntz/Gruntz.rc with the era
                                    RC.EXE and byte-compare every payload
                                    (type, name, lang, bytes, payload order)
                                    against the retail PE's .rsrc - total
                                    coverage both directions. Exit 0 identical,
                                    1 a real deviation, 2 could not run (no era
                                    rc.exe / unwritable --out / unreadable PE)

src/Gruntz/Gruntz.rc plus src/Gruntz/res/*.{ico,cur} is the ONE carrier of
retail's 75 resources. The era RC.EXE (toolchain r3+, via gruntz.tool.rc)
compiles it; the check gate proves it against the retail image itself
(gruntz.core.pe). No extracted resource bytes are stored anywhere - the
retail image is the only oracle.
"""

from __future__ import annotations

_SUBS = ("check",)


def main(argv=None) -> int:
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0
    if not argv or argv[0] not in _SUBS:
        print(__doc__.strip(), file=sys.stderr)
        what = f"unknown subcommand {argv[0]!r}" if argv else "no subcommand"
        print(f"\ngruntz rsrc: {what} - pick one of: {', '.join(_SUBS)}",
              file=sys.stderr)
        return 2
    sub, rest = argv[0], argv[1:]
    from gruntz.rsrc.check import main as check_main
    sys.argv = [f"gruntz rsrc {sub}", *rest]
    return check_main(rest)
