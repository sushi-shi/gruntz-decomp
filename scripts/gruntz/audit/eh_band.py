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
  * STATES     - our `maxState` against retail's, straight out of both `FuncInfo`
                 records. A difference is the band's strongest structural claim:
                 our function builds a different number of destructible objects, or
                 nests their scopes differently, than retail's. It sees cases the
                 funclet list cannot, because two states unwinding one object share
                 a funclet address and collapse into one entry.
  * MATCH      - per-group agreement, split into the classes that mean different
                 things: an identical group, a group whose funclets differ only in a
                 frame displacement (the owner's locals sit at different offsets), a
                 group whose funclets are PERMUTED (the owner constructs its locals
                 in a different order), and a group with a different funclet COUNT
                 (the owner has a different number of unwind states).
  * CENSUS     - `--census`: WHY each group's funclets differ, decided on the BYTES
                 rather than on objdiff's score. Both sides are read out of the
                 normalized objects, every relocation operand is masked and its
                 target NAME taken instead, and every `[ebp+disp]` displacement is
                 split out of the opcode skeleton. That separates the two questions
                 a raw diff conflates:
                   frame-offset       same shapes, same destructors, our locals sit
                                      somewhere else - a layout difference;
                   permuted           the same funclets in a different ORDER - we
                                      construct the objects in a different sequence;
                   different-targets  the funclet destroys a DIFFERENT THING. This
                                      is the dominant class, and it is not a codegen
                                      difference at all - it is a wrong type, a
                                      wrong base, or a mis-attributed compiler
                                      helper, repeated across every TU that uses it.
                 `--census --top N` histograms `ours -> retail` over the differing
                 funclets, which is what turns 147 scattered rows into three
                 families of one wrong symbol each.

Usage:
  python -m gruntz.audit.eh_band              # the report
  python -m gruntz.audit.eh_band --tsv <path> # + the per-group inventory
  python -m gruntz.audit.eh_band --census     # + the shape census + its worklist
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
BASE_DIR = REPO / "build/objdiff/normalized/base"
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


def _our_states(band) -> dict[str, int]:
    """{owner -> the unwind-state count OUR object declares} from the base COFFs.

    THE STRONGEST STATEMENT THE BAND MAKES, and the one no other view exposes.
    cl writes the function's `maxState` - the number of EH states it has to unwind
    through - into the `_s_FuncInfo` record at offset 4, so our count and retail's
    are both plain integers and comparing them is exact. A difference means our
    reconstruction builds a different number of destructible objects, or nests
    their scopes differently, than retail's did; the funclet-address list cannot
    say that on its own, because two states that unwind the same object share one
    funclet address and collapse into one entry.

    Read from the NORMALIZED base, where `canonicalize_data_symbols` has already
    renamed cl's `$T<n>` label to `__ehfuncinfo$<owner>` - so this needs no
    structural walk of its own, and a group whose record our object does not carry
    is simply absent rather than guessed at.
    """
    sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    import canonicalize_data_symbols as canon  # noqa: E402
    wanted = {eh_band.funcinfo_symbol(group.owner): group.owner for group in band}
    out: dict[str, int] = {}
    for path in sorted(BASE_DIR.glob("*.obj")):
        payload = path.read_bytes()
        try:
            coff = canon.CoffObject(payload)
        except ValueError:
            continue
        for symbol in coff.symbols.values():
            owner = wanted.get(symbol.name)
            if owner is None or symbol.section <= 0:
                continue
            section = coff.sections[symbol.section - 1]
            offset = section.raw_offset + symbol.value
            if offset + 8 > len(payload):
                continue
            magic, states = struct.unpack_from("<Ii", payload, offset)
            if magic == eh_band.FUNCINFO_MAGIC:
                out[owner] = states
    return out


#: modrm bytes for `[ebp+disp8]` / `[ebp+disp32]`, any destination register.
_EBP_DISP8 = frozenset((0x45, 0x4D, 0x55, 0x5D, 0x65, 0x6D, 0x75, 0x7D))
_EBP_DISP32 = frozenset((0x85, 0x8D, 0x95, 0x9D, 0xA5, 0xAD, 0xB5, 0xBD))
#: the opcodes a funclet uses to address the parent frame (`mov`, `lea`, `mov` store).
_FRAME_OPCODES = frozenset((0x8B, 0x8D, 0x89))


def _funclet_bodies(path: Path):
    """{band symbol: (payload with relocation operands zeroed, target names)}."""
    if not path.is_file():
        return {}
    sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    import canonicalize_data_symbols as canon  # noqa: E402
    payload = path.read_bytes()
    try:
        coff = canon.CoffObject(payload)
    except ValueError:
        return {}
    per_section: dict[int, list] = {}
    for symbol in coff.symbols.values():
        if symbol.section <= 0:
            continue
        if coff.sections[symbol.section - 1].characteristics & canon.MEM_EXECUTE:
            per_section.setdefault(symbol.section, []).append(
                (symbol.value, symbol.index, symbol.name))
    for entries in per_section.values():
        entries.sort()
    relocations: dict[int, list] = {}
    for relocation in coff.relocations:
        relocations.setdefault(relocation.section, []).append(
            (relocation.site, coff.symbols[relocation.symbol_index].name))
    out = {}
    for index, entries in per_section.items():
        section = coff.sections[index - 1]
        for position, (value, _symbol, name) in enumerate(entries):
            if not name.startswith(eh_band.EHUNWIND_PREFIX):
                continue
            end = (entries[position + 1][0] if position + 1 < len(entries)
                   else section.raw_size)
            body = bytearray(payload[section.raw_offset + value:
                                     section.raw_offset + end])
            targets = []
            for site, target in sorted(relocations.get(index, [])):
                if value <= site < end:
                    body[site - value:site - value + 4] = b"\0\0\0\0"
                    targets.append(target)
            # cl pads the last funclet of a COMDAT to alignment; retail's band is
            # packed. Neither padding is part of the funclet.
            out[name] = (bytes(body).rstrip(b"\x90\xcc"), tuple(targets))
    return out


def _funclet_shape(entry):
    """`(skeleton, frame displacements, targets)` - the displacements split out.

    A frame displacement is what moves when our LOCALS are laid out differently;
    keeping it out of the skeleton is what separates that question from "the
    funclet destroys something else", which is the one that matters.
    """
    body, targets = entry
    skeleton = bytearray(body)
    displacements = []
    offset = 0
    while offset < len(body) - 2:
        if body[offset] in _FRAME_OPCODES and body[offset + 1] in _EBP_DISP8:
            displacements.append(struct.unpack_from("<b", body, offset + 2)[0])
            skeleton[offset + 2] = 0
            offset += 3
            continue
        if (body[offset] in _FRAME_OPCODES and body[offset + 1] in _EBP_DISP32
                and offset + 6 <= len(body)):
            displacements.append(struct.unpack_from("<i", body, offset + 2)[0])
            skeleton[offset + 2:offset + 6] = b"\0\0\0\0"
            offset += 6
            continue
        offset += 1
    return (bytes(skeleton), tuple(displacements), targets)


def census(band):
    """`(Counter of verdicts, [(verdict, group, ours, theirs)])` over the band."""
    per_unit: dict[str, list] = {}
    for group in band:
        per_unit.setdefault(group.unit, []).append(group)
    verdicts, rows = Counter(), []
    for unit, groups in sorted(per_unit.items()):
        ours_all = _funclet_bodies(REPO / "build/objdiff/normalized/base"
                                   / f"{unit}.obj")
        theirs_all = _funclet_bodies(REPO / "build/objdiff/normalized/target"
                                     / f"{unit}.c.obj")
        for group in groups:
            keys = [eh_band.unwind_symbol(group.owner, index)
                    for index in range(len(group.funclets))]
            if any(k not in ours_all or k not in theirs_all for k in keys):
                verdicts["unbuilt"] += 1
                continue
            ours = [_funclet_shape(ours_all[k]) for k in keys]
            theirs = [_funclet_shape(theirs_all[k]) for k in keys]
            names = ([(s, t) for s, _d, t in ours], [(s, t) for s, _d, t in theirs])
            if ours == theirs:
                verdict = "identical"
            elif names[0] == names[1]:
                verdict = "frame-offset"
            elif sorted(ours) == sorted(theirs):
                verdict = "permuted"
            elif sorted(names[0]) == sorted(names[1]):
                verdict = "permuted+offset"
            elif [t for _s, _d, t in ours] != [t for _s, _d, t in theirs]:
                verdict = "different-targets"
            else:
                verdict = "different-code"
            verdicts[verdict] += 1
            rows.append((verdict, group, ours, theirs))
    return verdicts, rows


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
    parser.add_argument("--census", action="store_true",
                        help="classify WHY each group's funclets differ, on bytes.")
    parser.add_argument("--top", type=int, default=20,
                        help="how many `ours -> retail` families to list (--census).")
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

    # STATES: our `maxState` vs retail's - the band's strongest structural claim.
    ours = _our_states(band)
    divergent = sorted(
        (group.unit, group.owner, ours[group.owner], group.states)
        for group in band
        if group.owner in ours and ours[group.owner] != group.states)
    print(f"[eh-band] STATES     {len(ours) - len(divergent)}/{len(ours)} group(s) "
          f"declare retail's unwind-state count")
    for unit, owner, mine, retail in divergent:
        print(f"[eh-band]   STATES   {unit}: {owner}  ours {mine} vs retail {retail}"
              f"  ({'extra' if mine > retail else 'missing'} "
              f"{abs(mine - retail)} destructible scope(s))")

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
                  "below 100%: the stub's only operands are its `FuncInfo` and "
                  "`__CxxFrameHandler`, so this is a naming/extent defect in the "
                  "`.xdata$x` enrollment, not a code one.")
    else:
        rows = [(group, "no-report", []) for group in band]

    if args.tsv:
        out = Path(args.tsv)
        out.parent.mkdir(parents=True, exist_ok=True)
        with out.open("w", newline="") as stream:
            writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
            writer.writerow(("start", "stub", "end", "funclets", "our_states",
                             "retail_states", "unit", "owner", "verdict"))
            for group, verdict, _marks in rows:
                writer.writerow((f"0x{group.start:08x}", f"0x{group.stub:08x}",
                                 f"0x{group.end:08x}", len(group.funclets),
                                 ours.get(group.owner, "-"), group.states,
                                 group.unit, group.owner, verdict))
        print(f"[eh-band] wrote {out}")

    if args.census:
        verdicts, shapes = census(band)
        total = sum(verdicts.values()) or 1
        print("[eh-band] CENSUS     " + ", ".join(
            f"{verdict} {count} ({100.0 * count / total:.1f}%)"
            for verdict, count in verdicts.most_common()))
        # WHICH SYMBOL is wrong, over every differing funclet. One row here is one
        # modelling decision, however many TUs repeat it.
        families, examples = Counter(), {}
        for verdict, group, ours, theirs in shapes:
            if verdict != "different-targets":
                continue
            for mine, retail in zip(ours, theirs):
                if mine[2] == retail[2]:
                    continue
                key = (" + ".join(mine[2]) or "-", " + ".join(retail[2]) or "-")
                families[key] += 1
                examples.setdefault(key, f"{group.unit}:{group.owner}")
        if families:
            print("[eh-band]            differing funclet targets, ours -> retail:")
            for (mine, retail), count in families.most_common(args.top):
                print(f"[eh-band]   {count:4d}  {mine}")
                print(f"[eh-band]         -> {retail}")
                print(f"[eh-band]            e.g. {examples[(mine, retail)]}")

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
