#!/usr/bin/env python3
"""ghidra_metadata_apply.py - PyGhidra driver for the Gruntz enrichment + export.

Replaces the old `analyzeHeadless ... -postScript apply.py -postScript export.py`
invocation. On the flake's Ghidra 12.0.4, `analyzeHeadless` routes `.py` scripts
to PyGhidra but never *starts* PyGhidra, so the scripts die with "Ghidra was not
started with PyGhidra". This driver instead boots PyGhidra in-process (CPython3 +
JPype), imports/analyzes the program once, and runs apply.py + export.py as
GhidraScripts under the PyGhidraScriptProvider (so they get `currentProgram`,
`monitor`, `state` and the flat-API globals just like before).

Run it with the dev shell's `python3` (the one carrying the `pyghidra` package):

    python3 ghidra_metadata_apply.py <exe> <proj_location> <proj_name> <apply.py> <export.py> [--no-analyze]

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
            # Enable the Aggressive Instruction Finder before analyzeAll. It is OFF
            # by default; default auto-analysis (on both Ghidra 11.4.2 and 12.0.4)
            # carves only ~9,176 .text functions and leaves real code in
            # unreferenced gaps undisassembled. AIF disassembles those gaps,
            # recovering ~+130 genuine .text functions (verified: every one lands
            # inside .text, none in .data; ~83 are >=16-byte bodies, ~35 are 5-byte
            # incremental-linker jmp thunks) that apply.py's CSV seeding does NOT
            # cover. These become new matchable-function candidates. The other
            # analyzers (RTTI/Switch/Shared-Return/Function-ID/Demangler) are
            # already enabled by default and recover nothing extra; the vtable-only
            # methods (RunMessageLoop, CState/CPlay stubs, ...) are unreachable by
            # ANY analyzer and are seeded explicitly by apply.py from
            # symbol_names/engine_labels/library_labels instead.
            #
            # COST: AIF roughly 4x's the Ghidra analysis phase - measured on
            # GRUNTZ.EXE at 77s (default) -> 281s (+AIF), i.e. +~204s / ~3.4 min.
            # That cost is paid ONLY here, on the `analyze` path (`gruntz init` /
            # `gruntz init --reimport`), which is one-time/idempotent. The per-edit
            # `gruntz build` loop never re-runs Ghidra analysis, so it is unaffected.
            from ghidra.program.model.listing import Program
            opts = program.getOptions(Program.ANALYSIS_PROPERTIES)
            tx = program.startTransaction("enable-aggressive-instruction-finder")
            try:
                opts.setBoolean("Aggressive Instruction Finder", True)
            finally:
                program.endTransaction(tx, True)

            flat = FlatProgramAPI(program)
            _analyze_program(flat, program)  # only analyzes if not yet analyzed

        # apply.py mutates the DB (names/prototypes/structs/enums); export.py then
        # dumps functions.csv/symbols.csv from the enriched DB. Each runs as a
        # GhidraScript with currentProgram=program.
        for script in (apply_s, export_s):
            print(f"[ghidra_metadata_apply] running {Path(script).name} ...", flush=True)
            pyghidra.ghidra_script(script, project, program=program)
    finally:
        GhidraScriptUtil.releaseBundleHostReference()
        gproject.save(program)
        gproject.close()

    print("[ghidra_metadata_apply] done.", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
