"""`python3 -m gruntz.sema <view> ...` - the same dispatch `gruntz sema` uses."""

from __future__ import annotations

import sys

from gruntz.sema import main

if __name__ == "__main__":
    sys.exit(main())
