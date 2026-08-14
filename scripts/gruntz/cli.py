"""gruntz - the umbrella CLI.

    gruntz tool <name> [args...]     drive one external tool (gruntz/tool/)

Subcommands grow with the rebuild; `tool` forwards to the named module's own
main(), so `gruntz tool cl ...` and `python3 -m gruntz.tool.cl ...` (the form
ninja rule lines use) are the same entry.
"""

from __future__ import annotations

import sys

TOOLS = ("wine", "cl", "link", "rc")


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        print(f"\ntools: {', '.join(TOOLS)}")
        return 0 if argv else 2
    cmd, rest = argv[0], argv[1:]
    if cmd == "tool":
        if not rest or rest[0] not in TOOLS:
            print(f"gruntz tool: pick one of {', '.join(TOOLS)}", file=sys.stderr)
            return 2
        import importlib
        mod = importlib.import_module(f"gruntz.tool.{rest[0]}")
        sys.argv = [f"gruntz tool {rest[0]}", *rest[1:]]
        return mod.main()
    print(f"gruntz: unknown command {cmd!r} (the rebuild grows these "
          "step by step; see scripts/gruntz/__init__.py)", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
