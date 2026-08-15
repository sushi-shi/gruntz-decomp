"""gruntz - the umbrella CLI.

    gruntz tool <name> [args...]     drive one external tool (gruntz/tool/)
    gruntz labels [--all|--unit U]   source labels -> claim fragments (+ the
                                     tree-wide completeness sweep)
    gruntz model                     resolve claims x censuses -> bindings
    gruntz delink                    model -> synth pdb -> retail target objs
    gruntz compare                   base vs target -> objdiff report + summary
    gruntz build                     configure-if-needed + ninja (the loop:
                                     cl -> labels -> model -> delink -> compare)
    gruntz link                      the opt-in candidate link (EXE + .map)
    gruntz match                     build, then the compare summary for the
                                     units whose objs changed
    gruntz configure                 re-emit build/build.ninja
    gruntz sema <sub>                read-only investigation views (disasm,
                                     xref, rva, vtable, classof, strings, ...)
    gruntz walls <sub>               the wall campaign: inventory, diagnose,
                                     inline-model
    gruntz init                      local setup (the build wine prefix; the
                                     dev-shell hook runs this at entry)

Subcommands grow with the rebuild; `tool` forwards to the named module's own
main(), so `gruntz tool cl ...` and `python3 -m gruntz.tool.cl ...` (the form
ninja rule lines use) are the same entry.
"""

from __future__ import annotations

import sys

TOOLS = ("wine", "cl", "link", "rc", "delinker", "pdbutil", "objdiff",
         "objdump")


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        print(f"\ntools: {', '.join(TOOLS)}")
        return 0 if argv else 2
    cmd, rest = argv[0], argv[1:]
    if cmd in ("labels", "model", "delink", "compare"):
        import importlib
        mod = importlib.import_module(
            {"labels": "gruntz.retail_labels.source", "model": "gruntz.model",
             "delink": "gruntz.delink.run", "compare": "gruntz.compare.run"}[cmd])
        sys.argv = [f"gruntz {cmd}", *rest]
        return mod.main()
    if cmd in ("sema", "walls"):
        import importlib
        return importlib.import_module(f"gruntz.{cmd}").main(rest)
    if cmd in ("build", "link", "match"):
        from gruntz.graph.verbs import VERBS
        return VERBS[cmd](rest)
    if cmd == "configure":
        from gruntz.graph.emit import main as configure
        sys.argv = ["gruntz configure", *rest]
        return configure()
    if cmd == "init":
        from gruntz.tool import ToolError
        from gruntz.tool.wine import init_prefix, verify_prefix
        try:
            init_prefix()
            verify_prefix()
        except ToolError as e:
            print(f"[init] {e}", file=sys.stderr)
            return 1
        print("[init] build wine prefix OK (the graph/init steps grow with "
              "the rebuild)")
        return 0
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
