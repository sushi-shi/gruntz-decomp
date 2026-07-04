#!/usr/bin/env python3
"""Fatal guard: no src RVA() may also be a config/library_labels.csv row.

GRUNTZ.EXE statically links MFC (NAFXCW) + CRT. Library code is NEVER
hand-reconstructed in src/ - it gets a row in config/library_labels.csv (which
excludes it from the match denominator) and game code calls it through the real
headers (<Mfc.h>). A retail RVA must therefore be claimed by EXACTLY ONE of:

  * an RVA()/RVAU() annotation in src/  (a reconstructed GAME body), or
  * a library_labels.csv row            (a carved-out LIBRARY body).

Claiming the same RVA in BOTH is a double-claim on the same retail bytes - the
defect P0 reconciled. Two ways it recurs, both caught here:

  * a new src reconstruction lands on an RVA that FID had mislabeled as library
    (prune the false CSV row - the src is right), or
  * a hand-copy of Microsoft's code is reconstructed in src/ under a game name
    (carve it to the CSV and call the real MFC method via <Mfc.h> - the CSV is
    right).

The intersection of (src RVA()/RVAU() set) and (library_labels.csv rva column)
must be EMPTY. This is FATAL with no allowlist: fix the offending side, never
suppress. Run as part of `gruntz build` (scripts/gruntz/cli.py cmd_build).
"""
from __future__ import annotations

import csv
import re
import sys
from pathlib import Path


REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"
LIBRARY_LABELS = REPO / "config" / "library_labels.csv"

# RVA(0x.., 0x..) / RVAU(0x..) carrying a reconstructed body's retail address.
RVA_RE = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(?:0x[0-9a-fA-F]+|\d+)\s*\)")
RVAU_RE = re.compile(r"\bRVAU\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")


def norm_addr(value: str) -> str:
    return "0x%06x" % int(value, 16)


def src_rva_claims() -> dict:
    """rva -> "path:line" for every RVA()/RVAU() body annotation in src/."""
    claims = {}
    for path in sorted(SRC.rglob("*.cpp")):
        for i, line in enumerate(path.read_text().splitlines()):
            for m in list(RVA_RE.finditer(line)) + list(RVAU_RE.finditer(line)):
                claims.setdefault(norm_addr(m.group(1)),
                                  f"{path.relative_to(REPO)}:{i + 1}")
    return claims


def library_rows() -> dict:
    """rva -> "name (lib, confidence)" for every library_labels.csv row."""
    rows = {}
    if not LIBRARY_LABELS.exists():
        return rows
    for r in csv.DictReader(LIBRARY_LABELS.open()):
        rva = (r.get("rva") or "").strip()
        if not rva.startswith("0x"):
            continue
        try:
            key = norm_addr(rva)
        except ValueError:
            continue
        rows.setdefault(key, f"{r.get('name', '')} ({r.get('lib', '')}, {r.get('confidence', '')})")
    return rows


def main() -> int:
    claims = src_rva_claims()
    lib = library_rows()
    overlap = sorted(set(claims) & set(lib))
    if overlap:
        print(f"{len(overlap)} RVA(s) double-claimed by src AND config/library_labels.csv:",
              file=sys.stderr)
        for rva in overlap:
            print(f"  {rva}  src {claims[rva]}  <->  csv {lib[rva]}", file=sys.stderr)
        print("Each retail RVA must be a src RECONSTRUCTION xor a library CARVE-OUT.",
              file=sys.stderr)
        print("Fix: prune the false CSV row (src is a genuine game body), OR carve the",
              file=sys.stderr)
        print("hand-copied MFC/CRT body to the CSV and call the real routine via <Mfc.h>.",
              file=sys.stderr)
        return 1
    print(f"library-overlap: OK - {len(claims)} src RVA() claims, "
          f"{len(lib)} library rows, 0 overlap.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
