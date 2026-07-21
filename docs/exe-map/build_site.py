#!/usr/bin/env python3
"""Regenerate the exe-map website: scatter -> flags -> charts -> dashboard.

    nix develop --command python docs/exe-map/build_site.py

Runs each generator in order (they read the retail data via gruntz.core.exe_map,
so they need scripts/ on PYTHONPATH - the nix dev shell provides it). Every step
writes its output (JSON + HTML) next to these scripts, i.e. into docs/exe-map/."""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STEPS = ["scatter.py",         # scatter.json + scatter_methods.json (+ printed summary)
         "flag_outliers.py",   # flags.json (+ printed misplacement worklist)
         "make_chart.py",      # scatter.html + scatter_methods.html
         "make_dashboard.py",  # misplacement.html
         "make_reloc.py",      # reloc.html (self-regenerates reloc_fidelity.json live)
         "homm2_baseline.py",  # homm2_va.csv snapshot (reads homm2-decomp, read-only)
         "make_homm2.py",      # homm2.html (ground-truth baseline vs Gruntz)
         "split_plan.py",      # SPLIT_PLAN.md (actionable split/move worklist)
         # demo_oracle.py is NOT in the default pipeline (slow, needs GruntDem.exe);
         # run it manually to refresh demo_oracle.json after big re-homing waves.
         "deep_layout.py",     # deep_layout.json + TU_MIGRATION.md (uses flags.json)
         "make_deep.py"]       # deep.html (the deep map + original-TU partition)

for step in STEPS:
    print(f"\n=== {step} ===")
    subprocess.run([sys.executable, os.path.join(HERE, step)], check=True, cwd=HERE)

print("\nbuilt: docs/exe-map/{scatter,scatter_methods,misplacement}.html")
