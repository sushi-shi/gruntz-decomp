#!/usr/bin/env python3
"""gruntz.audit.channels - the base-superset-of-providers invariant + bands.

The channel model has ONE base per address space and providers layered on it:

    functions.tsv (rva+kind)  <=  RVA()/RVA_COMPGEN claims (symbol_names),
                                  RVA_DYNINIT pins, functions_zlib.tsv,
                                  ACTIVE functions_static_libs.tsv rows
    data.tsv      (rva+kind)  <=  data_vtables.tsv, data_static_libs.tsv,
                                  data_zlib.tsv, data_compgen.tsv
                                  (DATA() claims + strings are proven against
                                  the census by data_denominator --check via
                                  the delink manifest, a superset of both)

Every provider rva must be an admitted base row - a claim that is not a base
start would carve an extent the partition does not know. Providers may not
disagree with a base row's structural kind.

BANDS (config/retail/bands.tsv) classify regions for reporting; only the hard
invariants are gated: bands tile their sections, every ilt-thunks row is
kind=thunk, every eh-band .text row is kind=eh.

    python -m gruntz.audit.channels             # report
    python -m gruntz.audit.channels --gate      # exit 1 on any violation
    python -m gruntz.audit.channels --unmatched # unclaimed base rows by band
"""
from __future__ import annotations

import argparse
import bisect
import csv
import sys
from collections import Counter
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

BANDS = REPO / "config/retail/bands.tsv"
SYMBOLS = REPO / "build/gen/symbol_names.csv"


def bands() -> list[tuple[int, int, str]]:
    out = []
    for line in BANDS.read_text().splitlines():
        if not line.strip() or line.startswith("#") or line.startswith("lo\t"):
            continue
        lo, hi, band, *_ = line.split("\t")
        out.append((int(lo, 16), int(hi, 16), band))
    out.sort()
    return out


def band_of(rows: list[tuple[int, int, str]], rva: int) -> str:
    i = bisect.bisect_right(rows, (rva, 1 << 62, "")) - 1
    if i >= 0 and rows[i][0] <= rva < rows[i][1]:
        return rows[i][2]
    return "?"


def _tsv(path: Path) -> list[dict]:
    if not path.is_file():
        return []
    with path.open(newline="") as f:
        return list(csv.DictReader(
            (line for line in f if not line.lstrip().startswith("#")),
            delimiter="\t"))


def violations() -> list[str]:
    from gruntz.core.library_labels import active_rows
    from gruntz.core.retail_data import REGIONS, by_rva as data_by_rva
    from gruntz.core.retail_functions import TEXT_END, TEXT_LO, all_rows

    errors: list[str] = []

    # --- bands: tile .text and the data sections, no overlap, known edges.
    rows = bands()
    for prev, cur in zip(rows, rows[1:]):
        if prev[1] > cur[0]:
            errors.append(f"bands overlap at 0x{cur[0]:08x}")
    spans = {(TEXT_LO, TEXT_END)} | {tuple(REGIONS[k]) for k in ("rdata", "bss")}
    spans.add((REGIONS["data"][0], REGIONS["bss"][1]))  # data+bss are contiguous
    for lo, hi in ((TEXT_LO, TEXT_END), (REGIONS["rdata"][0], REGIONS["rdata"][1]),
                   (REGIONS["data"][0], REGIONS["bss"][1])):
        cursor = lo
        for b_lo, b_hi, _name in rows:
            if b_lo == cursor and b_hi <= hi:
                cursor = b_hi
        if cursor != hi:
            errors.append(f"bands do not tile [0x{lo:08x}, 0x{hi:08x}) - "
                          f"first hole at 0x{cursor:08x}")

    # --- function base + kind/band invariants.
    fn_rows = all_rows()
    fn_by_rva = {r["rva"]: r for r in fn_rows}
    for r in fn_rows:
        b = band_of(rows, r["rva"])
        if b == "ilt-thunks" and r["kind"] != "thunk":
            errors.append(f"0x{r['rva']:08x} kind {r['kind']!r} inside ilt-thunks")
        if b == "eh-band" and r["kind"] != "eh":
            errors.append(f"0x{r['rva']:08x} kind {r['kind']!r} inside eh-band")
        if r["kind"] == "eh" and b != "eh-band":
            errors.append(f"kind=eh row 0x{r['rva']:08x} outside eh-band ({b})")

    def need_fn(rva: int, who: str) -> None:
        if rva not in fn_by_rva:
            errors.append(f"{who} claims 0x{rva:08x}, not a functions.tsv row")

    if Path(SYMBOLS).is_file():
        with Path(SYMBOLS).open(newline="") as f:
            for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
                if (r.get("kind") or "") == "func":
                    need_fn(int(r["rva"], 16), f"src claim {r['name']}")
    for r in _tsv(REPO / "config/retail/functions_zlib.tsv"):
        need_fn(int(r["rva"], 16), f"functions_zlib {r['name']}")
    for r in active_rows():
        raw = (r.get("rva") or "").strip()
        if raw.startswith("0x"):
            need_fn(int(raw, 16), f"static-lib label {r['name']}")
    from gruntz.core.dyninit import rows as dyninit_rows
    for r in dyninit_rows(REPO):
        need_fn(r["rva"], f"RVA_DYNINIT pin {r['owner']}")

    # --- data base: every provider rva is an admitted census row of the
    #     matching kind.
    data = data_by_rva()

    def need_data(rva: int, who: str, kind: str | None) -> None:
        row = data.get(rva)
        if row is None:
            errors.append(f"{who} claims 0x{rva:08x}, not a data.tsv row")
        elif kind is not None and row["kind"] != kind:
            errors.append(f"{who} at 0x{rva:08x}: census kind {row['kind']!r}, "
                          f"expected {kind!r}")

    for r in _tsv(REPO / "config/retail/data_vtables.tsv"):
        need_data(int(r["rva"], 16), f"data_vtables {r['name']}", "vtable")
    for r in _tsv(REPO / "config/retail/data_static_libs.tsv"):
        name = r["name"]
        kind = ("vtable" if name.startswith("??_7") else
                "rtti" if name.startswith("??_R") else None)
        need_data(int(r["rva"], 16), f"data_static_libs {name}", kind)
    for r in _tsv(REPO / "config/retail/data_zlib.tsv"):
        need_data(int(r["rva"], 16), f"data_zlib {r['name']}", None)
    for r in _tsv(REPO / "config/retail/data_compgen.tsv"):
        need_data(int(r["rva"], 16), f"data_compgen {r['name']}",
                  r["class"])

    return errors


def unmatched_by_band(limit: int = 15) -> None:
    from gruntz.core.function_universe import classify
    rows = bands()
    universe, meta = classify()
    per_band: Counter = Counter()
    per_band_bytes: Counter = Counter()
    for r in meta["unmatched"]:
        b = band_of(rows, r["rva"])
        per_band[b] += 1
        per_band_bytes[b] += r["size"]
    print("unclaimed base rows by band:")
    for b, n in per_band.most_common():
        print(f"  {b:12} {n:5} rows  {per_band_bytes[b]:>9,} B")
    for b, _n in per_band.most_common():
        sel = [r for r in meta["unmatched"] if band_of(rows, r["rva"]) == b]
        sel.sort(key=lambda r: -r["size"])
        print(f"\n{b} - largest {min(limit, len(sel))}:")
        for r in sel[:limit]:
            print(f"  0x{r['rva']:08x} {r['size']:>7,} B")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gate", action="store_true")
    ap.add_argument("--unmatched", action="store_true",
                    help="list unclaimed base rows grouped by band")
    args = ap.parse_args()
    if args.unmatched:
        unmatched_by_band()
        return 0
    errors = violations()
    if errors:
        print(f"channels: {len(errors)} violation(s):", file=sys.stderr)
        for e in errors[:80]:
            print(f"  {e}", file=sys.stderr)
        if len(errors) > 80:
            print(f"  ... {len(errors) - 80} more", file=sys.stderr)
        return 1 if args.gate else 0
    print("channels: OK - every provider rva is an admitted base row and the "
          "band invariants hold")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
