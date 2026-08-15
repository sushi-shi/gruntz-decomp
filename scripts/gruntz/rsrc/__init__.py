"""gruntz.rsrc - the resource section, proven from source.

    python3 -m gruntz.rsrc check    compile src/Gruntz/Gruntz.rc with the era
                                    RC.EXE and byte-compare every payload
                                    (type, name, lang, bytes, payload order)
                                    against the retail PE's .rsrc - total
                                    coverage both directions; exit nonzero on
                                    any deviation

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
    if not argv or argv[0] in ("-h", "--help") or argv[0] not in _SUBS:
        print(__doc__.strip())
        return 0 if argv and argv[0] in ("-h", "--help") else 2
    sub, rest = argv[0], argv[1:]
    from gruntz.rsrc.check import main as check_main
    sys.argv = [f"gruntz rsrc {sub}", *rest]
    return check_main()
