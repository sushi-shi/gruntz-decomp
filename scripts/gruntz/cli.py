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
    gruntz play                      build + link, install the candidate into
                                     the game env, run it under gamescope
                                     (integer-scaled; --retail = the control)
    gruntz configure                 re-emit build/build.ninja
    gruntz sema <sub>                read-only investigation views (disasm,
                                     xref, rva, vtable, classof, strings, ...)
    gruntz walls <sub>               the wall campaign: inventory, diagnose,
                                     inline-model
    gruntz lineage <sub>             discover, inventory and verify surviving
                                     LithTech source-lineage decisions
    gruntz permute <verb>            classified state/variant search or island campaign
    gruntz ghidra <sub>              one-way viewer export: the retail image
                                     as a labelled Ghidra project (build,
                                     update, verify, status, export)
    gruntz verify <sub>              status / check (the MAX gate) / bank
                                     (baseline + README, manual) /
                                     fingerprints
    gruntz rsrc check                compile Gruntz.rc with era rc.exe,
                                     byte-compare 75/75 vs the retail .rsrc
    gruntz lsp <verb>                clangd-backed refs / hover / rename (the
                                     type-aware bulk member renamer)
    gruntz init                      local setup (the build wine prefix; the
                                     dev-shell hook runs this at entry)

Subcommands grow with the rebuild; `tool` forwards to the named module's own
main(), so `gruntz tool cl ...` and `python3 -m gruntz.tool.cl ...` (the form
ninja rule lines use) are the same entry.
"""

from __future__ import annotations

import sys

TOOLS = ("wine", "cl", "link", "rc", "delinker", "pdbutil", "objdiff",
         "objdump", "ghidra", "clangd", "rez")


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
    if cmd in ("sema", "walls", "ghidra", "verify", "rsrc", "lsp", "lineage"):
        import importlib
        return importlib.import_module(f"gruntz.{cmd}").main(rest)
    if cmd == "permute":
        if not rest or rest[0] in ("-h", "--help"):
            print("gruntz permute candidates [options]\n"
                  "gruntz permute campaign [--rva <rva>] [options]\n"
                  "gruntz permute state --source <tu.cpp> --rva <rva> [options]\n"
                  "gruntz permute variants <tu.cpp> <rva> [options]\n"
                  "  candidates: classify every live source-owned residual\n"
                  "  campaign: run N islands and retain M distinct best solutions\n"
                  "  state: classified, disposable compiler-state search\n"
                  "  variants: reviewed exact axes x AST shapes x TU state")
            return 0 if rest else 2
        if rest[0] in ("candidates", "campaign"):
            from gruntz.permute.campaign import main as campaign_main
            return campaign_main(rest)
        if rest[0] not in ("state", "variants"):
            print("gruntz permute: unknown verb " + repr(rest[0])
                  + " (have: candidates, campaign, state, variants)", file=sys.stderr)
            return 2
        verb, permute_args = rest[0], rest[1:]
        if any(value in ("-h", "--help") for value in permute_args):
            if verb == "state":
                from gruntz.permute.tu_state_noise import main as permute_main
            else:
                from gruntz.permute.match_variants import main as permute_main
            return permute_main(permute_args)
        rva_arg = (
            next((
                permute_args[index + 1]
                for index, value in enumerate(permute_args[:-1])
                if value == "--rva"
            ), None)
            if verb == "state"
            else (permute_args[1] if len(permute_args) >= 2 else None)
        )
        if rva_arg is None:
            print(f"gruntz permute {verb}: an RVA is required", file=sys.stderr)
            return 2
        from contextlib import redirect_stdout
        from io import StringIO
        from gruntz.walls.diagnose import diagnose
        diagnosis = StringIO()
        with redirect_stdout(diagnosis):
            result = diagnose(rva_arg)
        report = diagnosis.getvalue()
        print(report, end="")
        if result or "class: REGALLOC/SCHEDULING" not in report:
            print(f"gruntz permute {verb}: refused - permutation requires a "
                  "REGALLOC/SCHEDULING diagnosis", file=sys.stderr)
            return 2
        from gruntz.model import resolve
        from gruntz.verify.baseline import load as load_baseline
        rva = int(rva_arg, 0)
        if rva >= 0x400000:
            rva -= 0x400000
        binding = next((row for row in resolve().functions if row.rva == rva), None)
        bank = load_baseline().get((binding.unit, binding.name)) if binding else None
        if bank and bank["hist"] >= 100.0:
            print(f"gruntz permute {verb}: refused - historical MAX is already "
                  "100%", file=sys.stderr)
            return 2
        if verb == "state":
            from gruntz.permute.tu_state_noise import main as permute_main
        else:
            from gruntz.permute.match_variants import main as permute_main
        return permute_main(permute_args)
    if cmd in ("build", "link", "match", "play"):
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
