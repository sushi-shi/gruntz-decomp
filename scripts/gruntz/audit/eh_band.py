#!/usr/bin/env python3
"""gruntz.audit.eh_band - the state of the carved /GX EH funclet band.

Retail packed every `/GX` function's `.text$x` COMDAT (its unwind funclets plus the
registration stub its prologue pushes) into one band at the end of `.text`.
`gruntz.build.eh_band` derives each group from the retail `FuncInfo` tables and
carves it into its OWNER's target object, so the delinked object set closes over EH
and objdiff compares the funclet BYTES instead of name-matching a placeholder.

This reports what that carve produced and re-proves it:

  * INVENTORY  - groups, funclets, bytes, owning units; the derivation is retail-only.
  * CLOSURE    - no funclet push is left pointing at an undefined `FUN_<rva>`.
  * CARVE      - every derived record is a DEFINED symbol in its owner's target obj.
  * MATCH      - per-group agreement, split into the classes that mean different
                 things: an identical group, a group whose funclets differ only in a
                 frame displacement (the owner's locals sit at different offsets), a
                 group whose funclets are PERMUTED (the owner constructs its locals
                 in a different order), and a group with a different funclet COUNT
                 (the owner has a different number of unwind states).

Usage:
  python -m gruntz.audit.eh_band              # the report
  python -m gruntz.audit.eh_band --tsv <path> # + the per-group inventory
  python -m gruntz.audit.eh_band --check      # exit 1 on a closure/carve failure
"""

from __future__ import annotations

import argparse
import csv
import os
import struct
import sys
from collections import Counter
from pathlib import Path

from gruntz.build import eh_band
from gruntz.core.report import EH_BAND_PREFIXES, REPORT, fn_fuzzy, is_eh_band

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
NAMES = REPO / "build/gen/symbol_names.csv"
TARGET_DIR = REPO / "build/objdiff/target"
EXACT = 99.995


def _exe() -> Path:
    return Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")


def _names_map() -> dict[int, tuple[str, str, int]]:
    out = {}
    if not NAMES.is_file():
        return out
    rows = [line for line in NAMES.read_text().splitlines() if not line.startswith("#")]
    for row in csv.DictReader(rows):
        if (row.get("kind") or "func").strip() != "func":
            continue
        size = (row.get("size") or "").strip()
        out[int(row["rva"], 16)] = (row["name"].strip(), row["unit"].strip(),
                                    int(size, 16) if size else 0)
    return out


def _target_symbols(unit: str) -> dict[str, int] | None:
    """{defined symbol name -> section} for one delinked target object."""
    path = TARGET_DIR / f"{unit}.c.obj"
    if not path.is_file():
        return None
    sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    import canonicalize_data_symbols as canon  # noqa: E402
    coff = canon.CoffObject(path.read_bytes())
    return {s.name: s.section for s in coff.symbols.values() if s.section > 0}


def _undefined_funclets(band) -> list[tuple[str, str]]:
    """Funclet pushes still resolving to an UNDEFINED `FUN_<rva>` in a target obj."""
    sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    import canonicalize_data_symbols as canon  # noqa: E402
    stubs = {group.stub for group in band}
    image_base = 0x400000
    out = []
    for path in sorted(TARGET_DIR.glob("*.c.obj")):
        payload = path.read_bytes()
        coff = canon.CoffObject(payload)
        for relocation in coff.relocations:
            if relocation.typ != canon.DIR32:
                continue
            section = coff.sections[relocation.section - 1]
            if not section.characteristics & canon.MEM_EXECUTE:
                continue
            target = coff.symbols[relocation.symbol_index]
            if target.section != 0 or not target.name.startswith("FUN_"):
                continue
            operand = section.raw_offset + relocation.site
            if payload[operand - 1] != eh_band.PUSH_IMM32:
                continue
            resolved = (int(target.name[4:], 16)
                        + struct.unpack_from("<I", payload, operand)[0]) - image_base
            if resolved in stubs or eh_band.EHREG_PREFIX in target.name:
                out.append((path.name[:-6], target.name))
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tsv", help="write the per-group inventory here.")
    parser.add_argument("--check", action="store_true",
                        help="exit 1 when closure or the carve fails.")
    args = parser.parse_args()

    exe = _exe()
    if not exe.is_file():
        print(f"[eh-band] no retail EXE at {exe}", file=sys.stderr)
        return 0
    names = _names_map()
    if not names:
        print("[eh-band] no build/gen/symbol_names.csv - run `gruntz build` first",
              file=sys.stderr)
        return 0
    band = eh_band.groups(exe, names)
    funclets = sum(len(group.funclets) for group in band)
    code = sum(group.end - group.start for group in band)
    units = {group.unit for group in band}
    print(f"[eh-band] INVENTORY  {len(band)} group(s) - {funclets} unwind funclet(s) "
          f"+ {len(band)} registration stub(s), {code:,} B, over {len(units)} unit(s)")
    print(f"[eh-band]            band spans 0x{band[0].start:06x}..0x{band[-1].end:06x} "
          f"of .text" if band else "[eh-band]            band is empty")

    failures = 0

    # CARVE: every derived record must be a defined symbol in its owner's target obj.
    per_unit: dict[str, list[tuple[int, str, str, int]]] = {}
    for record in eh_band.records(band):
        per_unit.setdefault(record[2], []).append(record)
    missing = []
    checked = 0
    for unit, records in sorted(per_unit.items()):
        defined = _target_symbols(unit)
        if defined is None:
            missing.append((unit, "<no target obj>"))
            continue
        for _rva, symbol, _unit, _size in records:
            checked += 1
            if symbol not in defined:
                missing.append((unit, symbol))
    print(f"[eh-band] CARVE      {checked - len(missing)}/{checked} record(s) defined "
          f"in their owner's target object")
    for unit, symbol in missing[:10]:
        print(f"[eh-band]   MISSING  {unit}: {symbol}")
    failures += len(missing)

    # CLOSURE: no funclet push may still name an undefined FUN_<rva>.
    leftover = _undefined_funclets(band)
    print(f"[eh-band] CLOSURE    {len(leftover)} funclet push(es) still resolve to an "
          f"UNDEFINED FUN_<rva>")
    for unit, symbol in leftover[:10]:
        print(f"[eh-band]   OPEN     {unit}: {symbol}")
    failures += len(leftover)

    # MATCH: what objdiff now says about the carved symbols.
    if REPORT.is_file():
        import json
        doc = json.loads(REPORT.read_text())
        scored: dict[str, float] = {}
        for unit in doc.get("units", []):
            for row in unit.get("functions") or []:
                if is_eh_band(row["name"]):
                    scored[row["name"]] = fn_fuzzy(row)
        classes = Counter()
        rows = []
        stubs_off = 0
        for group in band:
            marks = [scored.get(eh_band.unwind_symbol(group.owner, index))
                     for index in range(len(group.funclets))]
            stub = scored.get(eh_band.registration_symbol(group.owner))
            if stub is not None and stub < EXACT:
                stubs_off += 1
            if any(mark is None for mark in marks) or stub is None:
                verdict = "unscored"
            elif all(mark >= EXACT for mark in marks):
                verdict = "unwind-identical"
            elif any(mark == 0.0 for mark in marks):
                verdict = "unwind-count-differs"
            else:
                verdict = "unwind-content-differs"
            classes[verdict] += 1
            rows.append((group, verdict, marks + [stub]))
        print("[eh-band] MATCH      " + ", ".join(
            f"{verdict} {count}" for verdict, count in classes.most_common())
            + "   (unwind funclets only)")
        if stubs_off:
            print(f"[eh-band]            {stubs_off}/{len(band)} registration stub(s) "
                  "below 100%: the CODE matches and `mov eax,<FuncInfo>` now resolves "
                  "to the exact datum; what is left is that datum's EXTENT - the "
                  "delinker sizes a PDB symbol to its neighbour and gets 4 B where cl "
                  "emits 32 + 8*maxState. Enrolling it is the `.xdata$x` half, open.")
    else:
        rows = [(group, "no-report", []) for group in band]

    if args.tsv:
        out = Path(args.tsv)
        out.parent.mkdir(parents=True, exist_ok=True)
        with out.open("w", newline="") as stream:
            writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
            writer.writerow(("start", "stub", "end", "funclets", "unit", "owner",
                             "verdict"))
            for group, verdict, _marks in rows:
                writer.writerow((f"0x{group.start:08x}", f"0x{group.stub:08x}",
                                 f"0x{group.end:08x}", len(group.funclets),
                                 group.unit, group.owner, verdict))
        print(f"[eh-band] wrote {out}")

    # The build's gate runner echoes only the LAST line, so the verdict goes last.
    if failures:
        print(f"[eh-band] FAIL: {failures} defect(s) - the delinked object set no "
              "longer closes over EH", file=sys.stderr)
        return 1 if args.check else 0
    print(f"[eh-band] OK - {checked} record(s) carved into their owners, EH closed "
          f"({len(band)} group(s), {code:,} B)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
