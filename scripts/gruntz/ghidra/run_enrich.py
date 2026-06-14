#!/usr/bin/env python3
"""run_enrich.py - PyGhidra driver for the Gruntz enrichment + export.

Replaces the old `analyzeHeadless ... -postScript apply.py -postScript export.py`
invocation. On the flake's Ghidra 12.0.4, `analyzeHeadless` routes `.py` scripts
to PyGhidra but never *starts* PyGhidra, so the scripts die with "Ghidra was not
started with PyGhidra". This driver instead boots PyGhidra in-process (CPython3 +
JPype), imports/analyzes the program once, and runs apply.py + export.py as
GhidraScripts under the PyGhidraScriptProvider (so they get `currentProgram`,
`monitor`, `state` and the flat-API globals just like before).

Run it with the dev shell's `python3` (the one carrying the `pyghidra` package):

    python3 run_enrich.py <exe> <proj_location> <proj_name> <apply.py> <export.py> [--no-analyze]

- <proj_location>/<proj_name>.{gpr,rep} is the project (non-nested layout, to
  match the existing build/ghidra-named/gruntz.{gpr,rep}).
- First run imports + auto-analyzes the EXE (SEVERAL MINUTES). If the program is
  already in the project it is reused; pass --no-analyze to skip re-analysis when
  iterating (re-running apply/export on an already-analyzed DB).
"""
import sys
from pathlib import Path

import pyghidra


def main() -> int:
    args = sys.argv[1:]
    analyze = True
    if "--no-analyze" in args:
        analyze = False
        args = [a for a in args if a != "--no-analyze"]
    if len(args) < 5:
        print(__doc__)
        return 2
    exe, proj_loc, proj_name, apply_s, export_s = args[:5]

    pyghidra.start()

    # open_program (the non-deprecated ghidra_script + a managed Project) needs a
    # framework `Project` handle that open_program's FlatProgramAPI does not
    # expose. _setup_project is exactly what open_program calls internally; it
    # imports the EXE (or reuses an existing program of the same name) and hands
    # back both the GhidraProject and the Program, and GhidraProject.getProject()
    # is the framework Project that ghidra_script wants.
    from pyghidra.core import _setup_project, _analyze_program
    from ghidra.app.script import GhidraScriptUtil
    from ghidra.program.flatapi import FlatProgramAPI

    gproject, program = _setup_project(
        binary_path=exe,
        project_location=proj_loc,
        project_name=proj_name,
        nested_project_location=False,
    )
    project = gproject.getProject()  # framework Project for ghidra_script

    GhidraScriptUtil.acquireBundleHostReference()
    try:
        if analyze:
            flat = FlatProgramAPI(program)
            _analyze_program(flat, program)  # only analyzes if not yet analyzed

        # apply.py mutates the DB (names/prototypes/structs/enums); export.py then
        # dumps functions.csv/symbols.csv from the enriched DB. Each runs as a
        # GhidraScript with currentProgram=program.
        for script in (apply_s, export_s):
            print(f"[run_enrich] running {Path(script).name} ...", flush=True)
            pyghidra.ghidra_script(script, project, program=program)
    finally:
        GhidraScriptUtil.releaseBundleHostReference()
        gproject.save(program)
        gproject.close()

    print("[run_enrich] done.", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
