"""Enumerate the engine's (unmatched) functions and attribute each to an owning TU.

`(unmatched)` = every real reconstruction-target function (in `.text`, not a
carve-out) that no src/ TU has started yet. status.py counts them; this tool
NAMES them and says WHERE each should be homed, so the count can be driven to 0.

Attribution is RVA-first (the campaign's spatial-home policy): a bodiless target
belongs to the TU that owns its retail `.text` neighborhood. We find the nearest
already-claimed code symbol below and above each unmatched RVA (from
build/gen/symbol_names.csv, which carries rva->unit); if both sides agree the
owner is unambiguous, otherwise we surface both candidates and pick the closer.

Usage (from a `nix develop .#build` shell):
  python -m gruntz.audit.unmatched_attribute                 # grouped worklist
  python -m gruntz.audit.unmatched_attribute --bands 4       # + N disjoint RVA bands
  python -m gruntz.audit.unmatched_attribute --tsv out.tsv   # write machine-readable TSV

The tool self-checks its enumeration against status.engine_universe()'s count and
warns on drift (the two must agree, or the carve-out filter has changed).
"""

from __future__ import annotations

import argparse
import bisect
import csv
import os
import struct
import sys
from pathlib import Path

from gruntz.core.library_labels import active_rvas

REPO = Path(__file__).resolve().parents[3]
FUNCS_CSV = REPO / "build/ghidra-enrich/exports/functions.csv"
FID_CSV = REPO / "config/library_labels.csv"
SYM_CSV = REPO / "build/gen/symbol_names.csv"


def _rint(s: str) -> int:
    s = str(s).strip()
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def _load_rows():
    """(rva, size, name, is_thunk) for every in-.text retail function."""
    rows = []
    with open(FUNCS_CSV) as f:
        for r in csv.DictReader(f):
            try:
                rva, sz = _rint(r["entry_rva"]), int(r["byte_size"])
            except Exception:
                continue
            if r.get("in_text", "1") != "1":
                continue
            name = r.get("name", "")
            is_thunk = name.startswith("thunk_") or r.get("is_thunk", "0") == "1"
            rows.append((rva, sz, name, is_thunk))
    rows.sort()
    return rows


def _load_lib() -> set[int]:
    return active_rvas(FID_CSV)


def _load_claimed_and_units():
    """Return (claimed_rvas:set, code_syms:[(rva,unit)] sorted).

    `code_syms` is only the CODE symbols (kind != data) that carry a unit, used
    to bracket an unmatched RVA to its owning TU.
    """
    claimed: set[int] = set()
    code_syms: list[tuple[int, str]] = []
    if not SYM_CSV.is_file():
        return claimed, code_syms
    with open(SYM_CSV) as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            head = (r.get("rva") or "").strip()
            if not head.lower().startswith("0x"):
                continue
            try:
                rva = int(head, 16)
            except ValueError:
                continue
            claimed.add(rva)
            if (r.get("kind") or "").strip() != "data":
                unit = (r.get("unit") or "").strip()
                if unit:
                    code_syms.append((rva, unit))
    code_syms.sort()
    return claimed, code_syms


def _iat_thunks(rows) -> set[int]:
    """6-byte FF25/FF15 IAT import thunks, caught by opcode from the retail bytes."""
    iat: set[int] = set()
    exe = os.environ.get("GRUNTZ_EXE") or str(REPO / "build" / "exe" / "GRUNTZ.EXE")
    try:
        d = Path(exe).read_bytes()
        e = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, e + 6)[0]
        optsz = struct.unpack_from("<H", d, e + 20)[0]
        secs = []
        for i in range(nsec):
            b = e + 24 + optsz + i * 40
            va = struct.unpack_from("<I", d, b + 12)[0]
            rsz = struct.unpack_from("<I", d, b + 16)[0]
            rp = struct.unpack_from("<I", d, b + 20)[0]
            secs.append((va, rsz, rp))

        def _off(rva):
            for va, rsz, rp in secs:
                if va <= rva < va + rsz:
                    return rp + (rva - va)
            return None

        claimed_dummy = None  # not needed; caller filters claimed separately
        for rva, sz, _n, _t in rows:
            if sz <= 7:
                o = _off(rva)
                if o is not None and d[o] == 0xFF and d[o + 1] in (0x25, 0x15):
                    iat.add(rva)
    except (OSError, struct.error, IndexError):
        pass
    return iat


def unmatched_targets():
    """The list of (rva, size, name) that are real targets with no src body yet.

    Mirrors status.engine_universe()'s final `else` branch exactly.
    """
    rows = _load_rows()
    lib = _load_lib()
    claimed, code_syms = _load_claimed_and_units()
    iat = _iat_thunks(rows)
    ilt_end = min((rva for rva, sz, _n, _t in rows if sz > 5), default=0)

    out = []
    for rva, sz, name, is_thunk in rows:
        if rva in claimed:
            continue
        if rva < ilt_end or is_thunk or rva in iat:
            continue
        if sz <= 7 and (name.startswith("FUN_") or name.startswith("Unmatched_")):
            continue
        if rva in lib:
            continue
        if name.startswith("Unwind@"):
            continue
        out.append((rva, sz, name))
    return out, code_syms


def attribute(rva, code_syms):
    """Return (owner_unit, confidence, evidence) by RVA-neighborhood bracket."""
    if not code_syms:
        return ("?", "none", "no symbol map")
    rvas = [r for r, _u in code_syms]
    i = bisect.bisect_left(rvas, rva)
    below = code_syms[i - 1] if i > 0 else None
    above = code_syms[i] if i < len(code_syms) else None
    if below and above:
        if below[1] == above[1]:
            return (below[1], "high", f"bracketed by {below[1]} "
                    f"({below[0]:#08x}<{rva:#08x}<{above[0]:#08x})")
        db, da = rva - below[0], above[0] - rva
        near = below if db <= da else above
        return (near[1], "low", f"between {below[1]}@{below[0]:#08x} and "
                f"{above[1]}@{above[0]:#08x}; nearer={near[1]} (+{min(db, da):#x})")
    only = below or above
    return (only[1], "low", f"edge; nearest {only[1]}@{only[0]:#08x}")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--bands", type=int, default=0,
                    help="also print N disjoint RVA bands (roughly equal counts) for fan-out")
    ap.add_argument("--tsv", type=str, default="",
                    help="write a machine-readable TSV (rva,size,name,owner,confidence,evidence)")
    args = ap.parse_args(argv)

    if not FUNCS_CSV.is_file():
        print("error: Ghidra export missing; run a build first.", file=sys.stderr)
        return 1

    targets, code_syms = unmatched_targets()
    rowset = []
    for rva, sz, name in targets:
        owner, conf, ev = attribute(rva, code_syms)
        rowset.append((rva, sz, name, owner, conf, ev))

    # Self-check vs status.py's (unmatched) row = real_fn - objdiff_total_functions.
    # A small gap (a few) is normal: a real fn can carry a symbol_names entry (so we
    # treat it as started/home-able) yet not appear among objdiff's counted functions.
    # A LARGE gap means the carve-out filter drifted from status.engine_universe().
    try:
        import json

        from gruntz.match import status
        eng = status.engine_universe()
        rep = json.loads((REPO / "build/objdiff/report.json").read_text())
        objdiff_total = sum(len(u.get("functions", [])) for u in rep["units"])
        status_un = max(0, eng["real_fn"] - objdiff_total) if eng else None
        if status_un is not None:
            gap = abs(status_un - len(rowset))
            note = "OK" if gap <= 5 else "DRIFT — carve-out filter diverged from status.py"
            print(f"# self-check: status (unmatched)={status_un}, "
                  f"enumerated={len(rowset)} (gap {gap}: {note})\n")
    except Exception as e:
        print(f"# (self-check skipped: {e})\n")

    by_owner: dict[str, list] = {}
    for row in rowset:
        by_owner.setdefault(row[3], []).append(row)

    print(f"# (unmatched) targets: {len(rowset)}  across {len(by_owner)} owner TU(s)\n")
    for owner in sorted(by_owner, key=lambda o: -len(by_owner[o])):
        items = sorted(by_owner[owner])
        confs = {c for *_r, c, _e in ((*i,) for i in items)}  # noqa
        hi = sum(1 for i in items if i[4] == "high")
        print(f"## {owner:28} {len(items):3} fn(s)  ({hi} high-confidence)")
        for rva, sz, name, _o, conf, ev in items:
            tag = "" if conf == "high" else f"  [{conf}: {ev}]"
            print(f"    {rva:#08x}  {sz:5}B  {name}{tag}")
        print()

    if args.bands > 0:
        rowset.sort()
        n = len(rowset)
        per = (n + args.bands - 1) // args.bands
        print("=" * 60)
        print(f"# {args.bands} disjoint RVA bands (~{per} fns each) for fan-out:\n")
        for b in range(args.bands):
            chunk = rowset[b * per:(b + 1) * per]
            if not chunk:
                continue
            lo, hi = chunk[0][0], chunk[-1][0]
            owners = sorted({r[3] for r in chunk})
            print(f"BAND {b + 1}: {lo:#08x}..{hi:#08x}  {len(chunk)} fns  "
                  f"owners: {', '.join(owners)}")

    if args.tsv:
        with open(args.tsv, "w") as f:
            f.write("rva\tsize\tname\towner\tconfidence\tevidence\n")
            for rva, sz, name, owner, conf, ev in sorted(rowset):
                f.write(f"{rva:#08x}\t{sz}\t{name}\t{owner}\t{conf}\t{ev}\n")
        print(f"\nwrote {args.tsv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
