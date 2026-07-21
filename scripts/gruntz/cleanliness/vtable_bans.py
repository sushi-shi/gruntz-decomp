#!/usr/bin/env python3
"""Hard-fail guard for the four manual-vtable idioms.

These were the WAP hand-rolled-vtable metrics that the vtable campaign drove to 0:

  * ``*Vtbl`` structs            - ``struct/class \\w*Vtbl\\w*`` (a manual vtable type)
  * ``->vtbl`` accesses          - ``-> ... vtbl``            (a manual vtable deref)
  * ``g_*Vtbl`` globals          - ``g_...Vtbl``              (a vtable-datum extern/stamp)
  * ``m_vtbl`` / ``m_vptr`` members - a hand-rolled vptr field at +0x00

Every explicit vtable must be a real C++ ``virtual`` (catalogued via ``VTBL(...)``),
so NONE of these idioms may reappear. This guard scans ``src/`` + ``include/`` (comments
and string/char literals blanked, so annotations/data do not false-positive), prints
every offending ``file:line: token``, and exits nonzero if any are present. Wired into
``gruntz build`` as a FATAL gate. Runnable as ``python -m gruntz.cleanliness.vtable_bans``.
"""
from __future__ import annotations

import sys

from gruntz.core.class_meta import _blank_comments, rel, source_files
from gruntz.cleanliness.board import METRICS

# Pull the four vtable regexes straight from the cleanliness scoreboard so the guard
# and the score can never drift apart.
_BANNED_LABELS = (
    "*Vtbl structs",
    "->vtbl accesses",
    "g_*Vtbl globals",
    "m_vtbl/m_vptr members",
)
BANNED = [(label, rx) for label, rx, _ in METRICS if label in _BANNED_LABELS]


def scan():
    """Yield (label, path, lineno, token) for every banned-idiom occurrence."""
    for path in source_files():
        text = _blank_comments(path.read_text(errors="ignore"))
        for label, rx in BANNED:
            for m in rx.finditer(text):
                yield label, path, text.count("\n", 0, m.start()) + 1, m.group(0).strip()


def main() -> int:
    hits = list(scan())
    if not hits:
        print("vtable-bans: OK - none of the 4 banned manual-vtable idioms present")
        return 0
    by_label = {}
    for label, path, lineno, tok in hits:
        by_label.setdefault(label, []).append((path, lineno, tok))
    print(
        f"vtable-bans: FAIL - {len(hits)} banned manual-vtable idiom(s) present "
        f"(these must stay 0 - convert to real virtuals):",
        file=sys.stderr,
    )
    for label, _rx in BANNED:
        xs = by_label.get(label, [])
        if not xs:
            continue
        print(f"  [{label}] {len(xs)}:", file=sys.stderr)
        for path, lineno, tok in xs[:100]:
            print(f"    {rel(path)}:{lineno}: {tok}", file=sys.stderr)
        if len(xs) > 100:
            print(f"    ... and {len(xs) - 100} more", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
