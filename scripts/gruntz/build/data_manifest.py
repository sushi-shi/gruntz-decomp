#!/usr/bin/env python3
"""Generate the vostok `--data-manifest` from src DATA() definitions + their sizeof.

STATUS: proven but NOT yet wired into delink.py - see "Coverage" below.

The reviewed-data-topology delinker can emit each DATA()-annotated global as a real
named definition in its owning target object (right storage class + alignment, with
interior base relocations converted to COFF relocations, and function references to
it becoming EXTERNALS instead of duplicated 4-byte allocations) when it is handed a
data manifest. That is what makes objdiff able to score DATA at all.

Schema (read out of the delinker binary; it accepts a 9- or 10-column header):
    name  object  rva  size  storage  alignment  section_offset  scope  provenance
`section_offset = -` selects the legacy reviewed-allocation form (no candidate
section topology); a numeric offset instead enrolls the row in a candidate group.

Inputs, all already generated:
  * build/gen/symbol_names.csv - kind=data rows carry rva/name/unit and, since the
    DATA()-sizeof work, an exact type-derived `size` (labels.sizeof_qualtype);
  * the retail PE - classifies each RVA's storage (.rdata / initialized .data /
    loader-zero tail => rdata/data/bss) via analysis.data_audit.

Evidence rules (never fabricate an extent):
  * a row is enrolled ONLY with a proven size; the 393 rows whose declared type is
    not resolvable are withheld, not guessed;
  * a reviewed extent must FIT the span to its neighbour. An overlap PROVES one of
    the two models is wrong but not which, so BOTH are withheld and reported - the
    overlap list is a real reconstruction-defect worklist, not noise;
  * an allocation crossing a storage boundary is withheld.

Coverage (why this is not wired in yet)
--------------------------------------
A data manifest is the topology AUTHORITY for the objects it names: data that is not
enrolled stops being materialized into those target objects. Enrolling only the
DATA()-annotated globals therefore drops each unit's compiler-emitted data (string
literals `??_C@...`, the unsized globals, $T constant pools), which costs exact
functions that reference them - measured: 3 lost (soundfontpath BuildSoundFontPath,
gametext _$E1/_$E4, which reference the `??_C@` strings this manifest does not yet
carry). homm2 covers this with 5216 supplemental rows derived from the candidate
COFFs. Completing per-object coverage is the remaining step; until then delink.py
does not pass --data-manifest, so the code gate stays at its 2385 floor.

Measured with the 519 enrolled rows (strict, no --recover-data-relocs-from-pdb):
    matched_data     8/69184 (0.012%)  ->  38275/246684 (15.52%)
    exact            2385 -> 2382 (-3, the uncovered-string references above)

Usage:
    python -m gruntz.build.data_manifest              # -> build/gen/delink_data_manifest.tsv
    python -m gruntz.build.data_manifest --report     # print the withheld/defect lists
"""
from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "scripts"))

from gruntz.analysis.data_audit import read_pe, classify_pe_storage  # noqa: E402

SYMBOLS = REPO / "build/gen/symbol_names.csv"
EXE = REPO / "build/exe/GRUNTZ.EXE"
OUTPUT = REPO / "build/gen/delink_data_manifest.tsv"

HEADER = ("name", "object", "rva", "size", "storage", "alignment",
          "section_offset", "scope", "provenance")
# retail PE storage class -> the delinker's storage keyword
STORAGE = {"rdata": "rdata", "data-initialized": "data", "data-loader-zero-tail": "bss"}


def _alignment(rva, size):
    """Largest power of two (<=8) that divides BOTH the retail address and the size.

    Derived from observed facts rather than assumed: the retail RVA's own alignment
    is a hard property of the shipped image.
    """
    a = 1
    while a < 8 and rva % (a * 2) == 0 and size % (a * 2) == 0:
        a *= 2
    return a


def candidates(symbols=SYMBOLS, exe=EXE):
    """Enrollable rows + the withheld ones, with a reason for each."""
    pe = read_pe(exe)
    rows, withheld = [], []
    with Path(symbols).open(newline="") as f:
        for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            if (r.get("kind") or "") != "data":
                continue
            name, unit = r["name"], (r.get("unit") or "").strip()
            size_s = (r.get("size") or "").strip()
            rva = int(r["rva"], 16)
            if not size_s:
                withheld.append((rva, name, "no proven extent (type not resolvable)"))
                continue
            size = int(size_s, 16)
            start = classify_pe_storage(pe, rva)["class"]
            end = classify_pe_storage(pe, rva + size - 1)["class"]
            if start not in STORAGE:
                withheld.append((rva, name, "storage %s is not enrollable" % start))
                continue
            if start != end:
                withheld.append((rva, name, "extent crosses %s -> %s" % (start, end)))
                continue
            rows.append({"name": name, "object": "%s.c" % unit, "rva": rva, "size": size,
                         "storage": STORAGE[start], "alignment": _alignment(rva, size)})
    rows.sort(key=lambda x: x["rva"])
    # A reviewed extent must fit the span to its neighbour; an overlap proves one of
    # the pair is mis-modelled but not which, so neither is enrolled.
    bad, overlaps = set(), []
    for i in range(len(rows) - 1):
        a, b = rows[i], rows[i + 1]
        if a["rva"] + a["size"] > b["rva"]:
            bad.add(i)
            bad.add(i + 1)
            overlaps.append((a, b, a["rva"] + a["size"] - b["rva"]))
    enrolled = [r for i, r in enumerate(rows) if i not in bad]
    for i in sorted(bad):
        withheld.append((rows[i]["rva"], rows[i]["name"], "overlaps a neighbour"))
    return enrolled, withheld, overlaps


def manifest_bytes(rows):
    out = ["\t".join(HEADER)]
    for r in rows:
        out.append("\t".join([
            r["name"], r["object"], "0x%x" % r["rva"], "0x%x" % r["size"],
            r["storage"], "0x%x" % r["alignment"], "-", "external", "src-DATA-sizeof"]))
    return ("\n".join(out) + "\n").encode("utf-8")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-o", "--output", type=Path, default=OUTPUT)
    ap.add_argument("--report", action="store_true", help="print withheld + overlaps")
    args = ap.parse_args(argv)

    enrolled, withheld, overlaps = candidates()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(manifest_bytes(enrolled))
    from collections import Counter
    print("[data-manifest] enrolled %d row(s) -> %s" % (len(enrolled), args.output))
    print("[data-manifest] storage: " + ", ".join(
        "%s=%d" % kv for kv in sorted(Counter(r["storage"] for r in enrolled).items())))
    print("[data-manifest] withheld %d (never guessed): %s" % (
        len(withheld), ", ".join("%s=%d" % kv for kv in sorted(
            Counter(w[2].split("(")[0].strip() for w in withheld).items()))))
    if args.report:
        print("\n--- overlap contradictions (a real mis-modelling worklist) ---")
        for a, b, by in overlaps:
            print("  0x%06x %-42s +0x%-4x overlaps 0x%06x %s by 0x%x"
                  % (a["rva"], a["name"][:42], a["size"], b["rva"], b["name"][:38], by))
        print("\n--- withheld (no proven extent) ---")
        for rva, name, why in withheld[:20]:
            print("  0x%06x %-46s %s" % (rva, name[:46], why))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
