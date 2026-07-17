#!/usr/bin/env python3
"""gruntz.match.vtable_virtuality - every vtable's SLOTS must be real virtuals.

vtable_coverage checks each analysed vtable is bound by a ``VTBL(Name, rva)``. This
goes one step further, per the mandate: the class ``Name`` must be a REAL polymorphic
class whose ``virtual`` methods actually MODEL the vtable's slots - not a fabricated
name, and not a de-virtualized shell. For a vtable of N slots the class + its bases
must declare at least N virtual methods (each inherited slot comes from a base, each
new/override slot is a ``virtual`` in the class itself).

A ``VTBL(Name, rva)`` is a VIOLATION when:
  * ``Name`` is not defined as a class/struct anywhere in src/ + include/   (a
    fabricated / placeholder name - e.g. ``CEngVt_1ef670`` - that models nothing), OR
  * the class's resolved virtual count (own ``virtual`` decls + those reachable
    through its source/library bases) is LESS than the vtable's slot count (a
    de-virtualized or under-modelled class - some slots have no virtual behind them).

MFC/CRT vtables (config/library_vtables.csv) are exempt - they are library, catalogued
not reconstructed. Prints every violation and exits nonzero; wired into ``gruntz build``
as a FATAL gate. Runnable as ``python -m gruntz.match.vtable_virtuality`` (``--list``).
"""
from __future__ import annotations

import csv
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from gruntz.match.class_meta import _blank_comments, source_files

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
LIB_CSV = REPO / "config" / "library_vtables.csv"

_CLASS_HEAD = re.compile(r"\b(?:struct|class)\s+(\w+)\b([^;{]*)\{")
_VTBL = re.compile(r"\bVTBL\s*\(\s*(\w+)\s*,\s*(0x[0-9a-fA-F]+)\s*\)")

# Library base classes whose vtable slot-count we know, so a derived class that only
# INHERITS (adds no own virtual) is still credited its base's slots.
LIB_BASE_SLOTS = {
    "CObject": 5, "CCmdTarget": 8, "CWnd": 100, "CDialog": 104,  # generous MFC upper bounds
    "CFile": 12, "CException": 4, "CGdiObject": 4,
}


def _slot_counts_from_scan():
    """{rva: slot_count} for every analysed vtable (vtable_scan)."""
    with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as tf:
        out = tf.name
    try:
        subprocess.run([sys.executable, "-m", "gruntz.analysis.vtable_scan", "--csv", out],
                       cwd=str(REPO), capture_output=True, text=True, check=True)
        sizes = {}
        for r in csv.DictReader(open(out)):
            try:
                sizes[int(r["start_rva"], 16)] = int(r.get("size", 0))
            except (ValueError, TypeError):
                pass
        return sizes
    finally:
        try:
            Path(out).unlink()
        except OSError:
            pass


def _index_classes():
    """{name: (own_virtual_count, [base_names])} over every class def in src/+include/."""
    out = {}
    for path in source_files():
        text = _blank_comments(path.read_text(errors="ignore"))
        for m in _CLASS_HEAD.finditer(text):
            name, bases_txt = m.group(1), m.group(2)
            # brace-match the body
            b = text.index("{", m.start())
            depth, j = 0, b
            while j < len(text):
                if text[j] == "{":
                    depth += 1
                elif text[j] == "}":
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            body = text[b:j]
            own = len(re.findall(r"\bvirtual\b", body))
            bases = re.findall(r"(?:public|protected|private|virtual)?\s*([\w:]+)",
                               bases_txt.split(":", 1)[1]) if ":" in bases_txt else []
            bases = [x.split("::")[-1] for x in bases if x and x not in
                     ("public", "protected", "private", "virtual")]
            # first def wins; a later fuller def can only help -> keep the max own count
            prev = out.get(name)
            if prev is None or own > prev[0]:
                out[name] = (own, bases)
    return out


def resolved_virtuals(name, classes, seen=None):
    """Own virtuals + those reachable through source/library bases (transitive)."""
    if seen is None:
        seen = set()
    if name in seen:
        return 0
    seen.add(name)
    if name in LIB_BASE_SLOTS:
        return LIB_BASE_SLOTS[name]
    if name not in classes:
        return 0
    own, bases = classes[name]
    return own + sum(resolved_virtuals(b, classes, seen) for b in bases)


def source_vtbls():
    """[(name, rva, path, lineno)] for every VTBL(Name, 0x..) in src/+include/."""
    out = []
    for path in source_files():
        text = _blank_comments(path.read_text(errors="ignore"))
        for m in _VTBL.finditer(text):
            out.append((m.group(1), int(m.group(2), 16),
                        path, text.count("\n", 0, m.start()) + 1))
    return out


def main() -> int:
    sizes = _slot_counts_from_scan()
    classes = _index_classes()
    lib_rvas = set()
    if LIB_CSV.exists():
        for r in csv.DictReader(LIB_CSV.open()):
            try:
                lib_rvas.add(int(r["rva"], 16))
            except (ValueError, TypeError):
                pass

    violations = []
    unverifiable = []
    checked = 0
    for name, rva, path, ln in source_vtbls():
        if rva in lib_rvas:
            continue
        # No scan size = we do not know how many slots to require. The old default of 1
        # was a GUESS that silently made the check vacuous for that vtable (any class with
        # a single virtual passes a 1-slot requirement). Nothing to compare -> say so and
        # skip, rather than assert against a number nobody measured. 0 cases today; it
        # stays 0 only because someone looks.
        if rva not in sizes:
            unverifiable.append((name, rva, path, ln))
            continue
        n_slots = sizes[rva]
        checked += 1
        if name not in classes:
            violations.append((name, rva, n_slots, 0, "no class definition (fabricated name)", path, ln))
            continue
        nv = resolved_virtuals(name, classes)
        if nv < n_slots:
            violations.append((name, rva, n_slots, nv, "under-virtualized (slots not backed by virtuals)", path, ln))

    if "--list" in sys.argv:
        for name, rva, path, ln in sorted(source_vtbls()):
            if rva in lib_rvas:
                continue
            nv = resolved_virtuals(name, classes) if name in classes else 0
            print(f"  0x{rva:06x} slots={sizes.get(rva,'?'):<3} virtuals={nv:<3} {name}")

    if violations:
        print(f"vtable-virtuality: {len(violations)} of {checked} source-bound vtable(s) NOT modelled by "
              f"real virtuals (fabricated name or de-virtualized class):", file=sys.stderr)
        for name, rva, slots, nv, why, path, ln in sorted(violations, key=lambda v: v[1]):
            print(f"  0x{rva:06x} {name:24} slots={slots:<3} virtuals={nv:<3} {why}", file=sys.stderr)
        return 1
    print(f"vtable-virtuality: all {checked} source-bound vtables modelled by real virtuals")
    if unverifiable:
        print(f"    ({len(unverifiable)} VTBL'd rva(s) UNVERIFIABLE - vtable_scan found no "
              f"vtable there, so the slot count is unknown and nothing was asserted; "
              f"vtable_coverage owns that binding)")
        for name, rva, path, ln in sorted(unverifiable, key=lambda v: v[1]):
            print(f"      0x{rva:06x} {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
