#!/usr/bin/env python3
"""Class-size completeness check: every class/struct definition carries a size
annotation - EITHER ``SIZE(Class, bytes)`` (exact, verified against retail) OR
``SIZE_UNKNOWN(Class)`` (tracked, byte size not yet pinned).

The goal is a single source of truth for class layout: no class silently lacks a
size record. A class NAME satisfied once (in any file) counts as satisfied for all
its per-TU shim definitions (see gruntz.match.class_meta for the scoping rules).

Prints the violator worklist (class NAMES with a definition but no SIZE/
SIZE_UNKNOWN) and exits nonzero if any. Runnable as
``python -m gruntz.match.class_sizes``.
"""
from __future__ import annotations

import sys

from gruntz.match.class_meta import rel, size_annotated_names, unique_class_defs


def main() -> int:
    defs = unique_class_defs()
    annotated = size_annotated_names()
    violators = sorted((n, defs[n]) for n in defs if n not in annotated)
    total = len(defs)
    ok = total - len(violators)
    if violators:
        print(f"class-size completeness: {ok}/{total} class names annotated; "
              f"{len(violators)} MISSING SIZE/SIZE_UNKNOWN:", file=sys.stderr)
        for name, (path, lineno) in violators:
            print(f"  {rel(path)}:{lineno}: {name}", file=sys.stderr)
        return 1
    print(f"class-size completeness: all {total} class names carry SIZE/SIZE_UNKNOWN")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
