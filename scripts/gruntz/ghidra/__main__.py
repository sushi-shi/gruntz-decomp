"""`python3 -m gruntz.ghidra <verb> ...` - the same dispatch `gruntz ghidra` uses."""

from __future__ import annotations

import sys

from gruntz.ghidra import main

if __name__ == "__main__":
    sys.exit(main())
