"""Census of every `.bss` symbol pair: the base's inferred extent vs the target's.

`.bss` has no bytes, so objdiff's whole per-symbol comparison is the SIZE - and COFF
states a symbol size only for a COMMON symbol (whose `Value` is its size). For a
section-defined symbol objdiff SYNTHESISES one (`obj/read.rs: infer_symbol_sizes`):
the distance to the next symbol, i.e. the object PLUS whatever padding its allocator
left after it. The two sides use different allocators - cl for the base, the delinker's
aligned sequential append for the target - so that span differs for reasons that are
not the program.

That is why `nix/patches/objdiff-bss-inferred-extent.patch` compares a BSS symbol's
size only when at least one side actually STATES one. This tool is the evidence: it
re-derives the census that justified the patch directly from the normalized COFFs.

    $ python -m gruntz.audit.bss_extents            # summary + the histogram
    $ python -m gruntz.audit.bss_extents --rows     # every disagreement, every gap

Two rows exist, and only one of them is a defect:

  * `extent` - both sides carry the symbol, the inferred spans differ. NOT a defect.
    Measured 2026-08-09: 363 paired symbols, 50 disagreements, |delta| histogram
    {3: 1, 4: 49} - every one sub-alignment padding. If one ever exceeds the largest
    `.bss` alignment cl emits, THAT is worth reading: it is no longer padding.
  * `unpaired` - a target symbol with no base counterpart. A REAL defect, still scored
    as one, and today it is the whole residual `.bss` gap: the delinker enrolls the
    `GruntDirectionCell` header statics as `?s_gruntDirEast_22bd28@@3U...@A` while cl
    names them `_s_gruntDirEast$S17426`, and the normalizer content-addresses only the
    `$S` side, so the two can never pair. See
    docs/patterns/bss-symbol-size-inference-hole.md.
"""

from __future__ import annotations

import argparse
import collections
import json
import struct
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
NORMALIZED = REPO / "build" / "objdiff" / "normalized"
REPORT = REPO / "build" / "objdiff" / "report.json"

# The largest alignment MSVC 5.0 gives a `.bss` object here; a disagreement wider than
# this cannot be explained as the allocator's padding and wants reading.
MAX_BSS_ALIGN = 8

SECTION_SYMBOL_CLASS = 3


def _coff(path: Path):
    """(sections, symbols) - sections as (name, size), symbols as (name, value, section)."""
    blob = path.read_bytes()
    section_count = struct.unpack_from("<H", blob, 2)[0]
    symbol_table = struct.unpack_from("<I", blob, 8)[0]
    symbol_count = struct.unpack_from("<I", blob, 12)[0]
    optional = struct.unpack_from("<H", blob, 16)[0]
    sections, base = [], 20 + optional
    for i in range(section_count):
        row = blob[base + i * 40: base + (i + 1) * 40]
        sections.append((row[:8].rstrip(b"\0").decode("latin1"),
                         struct.unpack_from("<I", row, 16)[0]))
    strings = symbol_table + symbol_count * 18
    symbols, i = [], 0
    while i < symbol_count:
        row = blob[symbol_table + i * 18: symbol_table + (i + 1) * 18]
        if row[:4] == b"\0\0\0\0":
            at = strings + struct.unpack_from("<I", row, 4)[0]
            name = blob[at:blob.index(b"\0", at)].decode("latin1")
        else:
            name = row[:8].rstrip(b"\0").decode("latin1")
        symbols.append((name, struct.unpack_from("<I", row, 8)[0],
                        struct.unpack_from("<h", row, 12)[0], row[16]))
        i += 1 + row[17]
    return sections, symbols


def bss_extents(path: Path) -> dict[str, tuple[int, int]]:
    """{name: (section offset, INFERRED extent)} for the object's `.bss`.

    Reproduces objdiff's inference exactly: the extent is the distance to the next
    symbol in the same section, or to the section's end for the last one.
    """
    sections, symbols = _coff(path)
    out: dict[str, tuple[int, int]] = {}
    for index in [i + 1 for i, (name, _) in enumerate(sections) if name == ".bss"]:
        section_size = sections[index - 1][1]
        placed = sorted((value, name) for (name, value, section, cls) in symbols
                        if section == index
                        and not (cls == SECTION_SYMBOL_CLASS and name == ".bss"))
        for i, (offset, name) in enumerate(placed):
            end = placed[i + 1][0] if i + 1 < len(placed) else section_size
            out[name] = (offset, end - offset)
    return out


def census():
    """([(unit, extent rows, unpaired rows)], paired symbol count).

    Pairing is by EXACT name, which is what objdiff does for these symbols: its own
    `get_normalized_symbol_name` only collapses an all-numeric suffix, and cl's
    `name$S<n>` does not qualify (our normalizer content-addresses it instead).
    """
    target_dir, base_dir = NORMALIZED / "target", NORMALIZED / "base"
    rows, paired = [], 0
    for target_obj in sorted(target_dir.glob("*.obj")):
        unit = target_obj.name[:-len(".c.obj")] if target_obj.name.endswith(".c.obj") \
            else target_obj.stem
        base_obj = base_dir / f"{unit}.obj"
        if not base_obj.is_file():
            continue
        target, base = bss_extents(target_obj), bss_extents(base_obj)
        if not target:
            continue
        paired += sum(1 for n in target if n in base)
        extent = [(n, target[n][1], base[n][1])
                  for n in sorted(target) if n in base and target[n][1] != base[n][1]]
        unpaired = [(n, target[n][1]) for n in sorted(target) if n not in base]
        if extent or unpaired:
            rows.append((unit, extent, unpaired))
    return rows, paired


def section_percents() -> dict[str, tuple[int, float]]:
    if not REPORT.is_file():
        return {}
    doc = json.loads(REPORT.read_text())
    return {u["name"]: (int(s.get("size", 0)), s.get("fuzzy_match_percent", 0.0))
            for u in doc.get("units", []) for s in (u.get("sections") or [])
            if s["name"] == ".bss"}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--rows", action="store_true", help="print every row, not a summary")
    args = ap.parse_args()

    if not (NORMALIZED / "target").is_dir():
        print("no build/objdiff/normalized - run `gruntz build` first")
        return 1

    rows, paired = census()
    percents = section_percents()
    deltas = collections.Counter()
    extent_total = unpaired_total = 0
    wide = []
    for unit, extent, unpaired in rows:
        extent_total += len(extent)
        unpaired_total += len(unpaired)
        for name, t, b in extent:
            deltas[abs(t - b)] += 1
            if abs(t - b) > MAX_BSS_ALIGN:
                wide.append((unit, name, t, b))
        if args.rows:
            size, pct = percents.get(unit, (0, 0.0))
            print(f"== {unit}  .bss {size:,} B  {pct:.5f}%")
            for name, t, b in extent:
                print(f"   extent    {name:<56} target {t:#x}  base {b:#x}"
                      f"  delta {t - b:+d}")
            for name, t in unpaired:
                print(f"   unpaired  {name:<56} target {t:#x}   <- REAL defect")

    print(f"\npaired `.bss` symbols : {paired}")
    print(f"extent disagreements : {extent_total}   |delta| histogram "
          f"{dict(sorted(deltas.items()))}")
    print(f"unpaired target syms : {unpaired_total}   (the real `.bss` worklist)")
    if wide:
        print(f"\nWIDER THAN {MAX_BSS_ALIGN}-BYTE ALIGNMENT - not explicable as padding, read these:")
        for unit, name, t, b in wide:
            print(f"  {unit:<24} {name:<52} target {t:#x} base {b:#x}")
    else:
        print(f"every disagreement is <= {MAX_BSS_ALIGN} bytes, i.e. allocator padding, "
              "not a size")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
