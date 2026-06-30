#!/usr/bin/env python3
"""bootstrap_windows.py - the Windows port of flake.nix (run me on Windows).

flake.nix sets up the Gruntz decompilation dev environment on Linux (MSVC 5.0
under wine, plus objdiff / vostok-delinker / Ghidra / LLVM / the retail EXE).
This script does the equivalent on NATIVE Windows, where MSVC 5.0 needs no wine:

    py bootstrap_windows.py            # provision + write activate scripts
    py bootstrap_windows.py --init     # also run `gruntz init` (builds the Ghidra DB)
    py bootstrap_windows.py --shell    # provision + drop into an activated shell
    py bootstrap_windows.py --print-hashes

It downloads a SHA256-pinned toolchain into build\\win-toolchain\\ (git-ignored,
like everything under build/) and emits build\\win-toolchain\\activate.{ps1,bat}
that export the same environment the flake's shellHook does - minus wine. After
activating, the normal `gruntz <cmd>` workflow (init / build / status / ...) runs
unchanged, compiling with cl.exe natively.

Prerequisites: Python 3.11+ and git. The Rust nightly toolchain (rustup) is only
needed to build vostok-delinker; everything else is a pinned binary download.
Only stdlib is used here, so a fresh interpreter is enough to get started.

See docs/windows-setup.md for the full story.
"""

import os
import sys
from pathlib import Path

# scripts/ is THE package root; put it on sys.path so `gruntz.win` imports BEFORE
# any environment is set up (the chicken-and-egg the flake's PYTHONPATH solves).
_HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(_HERE / "scripts"))

if sys.version_info < (3, 11):
    raise SystemExit(f"[bootstrap] Python 3.11+ required (have {sys.version.split()[0]}); "
                     "the toolchain reads config/units.toml via tomllib.")

from gruntz.win.bootstrap import main  # noqa: E402

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
