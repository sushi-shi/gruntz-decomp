#!/usr/bin/env python3
"""DEPRECATED - superseded by `gruntz.py build`.

The matching build (configure -> ninja -> objdiff -> summary) now lives behind
the single `gruntz.py` entry point. This shim forwards to it so existing
references keep working. Prefer:

    nix develop .#build --command python3 gruntz.py build
    # or, inside the dev shell:
    gruntz build
"""
import subprocess
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent.parent)
print("[rebuild] DEPRECATED -> forwarding to `gruntz.py build`.", file=sys.stderr)
sys.exit(subprocess.run([sys.executable, str(REPO / "gruntz.py"), "build", *sys.argv[1:]],
                        cwd=str(REPO)).returncode)
