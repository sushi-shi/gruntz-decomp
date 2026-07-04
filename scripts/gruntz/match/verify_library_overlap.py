#!/usr/bin/env python3
"""Fatal guard: no src claim may also be a config/library_labels.csv row.

GRUNTZ.EXE statically links MFC (NAFXCW) + CRT. Library code is NEVER
hand-reconstructed in src/ - it gets a row in config/library_labels.csv (which
excludes it from the match denominator) and game code calls it through the real
headers (<Mfc.h>). A retail RVA must therefore be claimed by EXACTLY ONE of:

  * a src reconstruction  (a GAME body/global), or
  * a library_labels.csv row  (a carved-out LIBRARY body).

Claiming the same RVA in BOTH is a double-claim on the same retail bytes - the
defect P0 reconciled. Two ways it recurs, both caught here:

  * a new src reconstruction lands on an RVA that FID had mislabeled as library
    (prune the false CSV row - the src is right), or
  * a hand-copy of Microsoft's code is reconstructed in src/ under a game name
    (carve it to the CSV and call the real MFC method via <Mfc.h> - the CSV is
    right).

WHAT COUNTS AS A "src claim" (the widened set). A retail RVA is claimed by src
through ANY of these, all folded into build/gen/symbol_names.csv by the labels
step - so the FULL generated symbol set is the authoritative claim set, not just
the RVA()/RVAU() macros the first cut of this guard parsed:

  * rva-macro   RVA(0x.., 0x..) / RVAU(0x..)     - a reconstructed body
  * rva-symbol  // @rva-symbol: <mangled> <rva>  - a self-contained fn label
                                                    (e.g. a ??_G deleting-dtor
                                                    thunk the compiler synthesizes)
  * data        DATA(0x..) / // @data-symbol:     - a named game global / vtable

The FID matcher is fuzzy, so it emits false positives (a real function has ONE
address, yet ??_G__non_rtti_object recurred at ~189 RVAs, __fpclear at ~45): an
@rva-symbol thunk or a DATA() global landing on such a row is the recurring
defect this widened guard exists to catch - which the RVA()-macro-only cut could
not see (macro-only overlap was already 0 while the full set overlapped at 45).

THE ONE DELIBERATE COEXISTENCE (vendored library). zlib 1.0.4 is compiled from
real vendored source (vendor/zlib-1.0.4/*.c) AND FID-identifies as library. Its
functions therefore sit in BOTH build/gen/symbol_names.csv (named for the
delinker via config/zlib_labels.csv, the vendored static-config table) AND
config/library_labels.csv (FID-tagged). status.py resolves this with "claimed
wins": a carve-out that is also reconstructed counts as a real target. That is a
THIRD category - vendored library, source held - not a double-claim, and the
names AGREE (_deflate_stored == _deflate_stored). This guard excludes it by
SOURCE (the config/zlib_labels.csv vendored table), NOT by an RVA allowlist:
every retail RVA is still a src reconstruction xor a library carve-out xor a
vendored-library body.

The intersection of (src claim set MINUS vendored) and (library_labels.csv rva
column) must be EMPTY. This is FATAL with no allowlist: fix the offending side,
never suppress. `gruntz build` regenerates build/gen/symbol_names.csv (ninja
labels edge) BEFORE running this gate, so the generated set is fresh here; run
standalone against a stale/absent generated file, the guard falls back to a src
parse (which carries every claim kind above and naturally excludes the vendored
table) so it never passes vacuously. Run as part of `gruntz build`
(scripts/gruntz/cli.py cmd_build).
"""
from __future__ import annotations

import csv
import re
import sys
from pathlib import Path


REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"
INCLUDE = REPO / "include"
LIBRARY_LABELS = REPO / "config" / "library_labels.csv"
GEN_NAMES = REPO / "build" / "gen" / "symbol_names.csv"
# The vendored static-config table (zlib 1.0.4): library code we hold source for,
# co-listed in symbol_names.csv AND library_labels.csv by design ("claimed wins").
VENDORED_CONFIG = REPO / "config" / "zlib_labels.csv"

# rva-macro: RVA(0x.., 0x..) / RVAU(0x..)  - a reconstructed body's retail address.
RVA_RE = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(?:0x[0-9a-fA-F]+|\d+)\s*\)")
RVAU_RE = re.compile(r"\bRVAU\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
# rva-symbol: `// @rva-symbol: <mangled> <rva> [<size>]` - a self-contained fn label.
RSYM_RE = re.compile(r"@rva-symbol:\s*\S+\s+(0x[0-9a-fA-F]+)")
# data: DATA(0x..) and `// @data-symbol: <mangled> <rva> [<size>]` - a named global.
DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
DSYM_RE = re.compile(r"@data-symbol:\s*\S+\s+(0x[0-9a-fA-F]+)")


def norm_addr(value: str) -> str:
    return "0x%06x" % int(value, 16)


def vendored_rvas() -> set:
    """RVAs of the vendored static-config TUs (config/zlib_labels.csv). These are
    library bodies we hold source for; they are DELIBERATELY in both the generated
    symbol set and library_labels.csv ("claimed wins", status.py) and are excluded
    from the overlap by source category, not by an RVA allowlist."""
    rvas = set()
    if not VENDORED_CONFIG.exists():
        return rvas
    for r in csv.reader(VENDORED_CONFIG.open()):
        if r and r[0].strip().lower().startswith("0x"):
            try:
                rvas.add(norm_addr(r[0].strip()))
            except ValueError:
                pass
    return rvas


def src_claims() -> dict:
    """rva -> (claim-kind, "path:line") for every src-authored claim. Parses src/
    and include/ directly, so it is always available (the never-vacuous fallback)
    and carries the claim kind + source location for the diagnostic. Naturally
    excludes the vendored table (those names live in config/zlib_labels.csv, not
    in src comments)."""
    claims = {}
    files = list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")) + list(INCLUDE.rglob("*.h"))
    for path in sorted(files):
        try:
            text = path.read_text(errors="replace")
        except OSError:
            continue
        for i, line in enumerate(text.splitlines()):
            for m in RVA_RE.finditer(line):
                claims.setdefault(norm_addr(m.group(1)), ("rva-macro", f"{path.relative_to(REPO)}:{i + 1}"))
            for m in RVAU_RE.finditer(line):
                claims.setdefault(norm_addr(m.group(1)), ("rva-macro", f"{path.relative_to(REPO)}:{i + 1}"))
            for m in RSYM_RE.finditer(line):
                claims.setdefault(norm_addr(m.group(1)), ("rva-symbol", f"{path.relative_to(REPO)}:{i + 1}"))
            for m in DATA_RE.finditer(line):
                claims.setdefault(norm_addr(m.group(1)), ("data", f"{path.relative_to(REPO)}:{i + 1}"))
            for m in DSYM_RE.finditer(line):
                claims.setdefault(norm_addr(m.group(1)), ("data", f"{path.relative_to(REPO)}:{i + 1}"))
    return claims


def generated_claims() -> dict:
    """rva -> (claim-kind, location, name) for the FULL src-authored claim set,
    MINUS the vendored table. Primary source: the generated build/gen/symbol_names.csv
    (authoritative - every RVA()/RVAU() body, @rva-symbol pin, DATA()/@data-symbol
    and ??_7 vtable folded in by the labels step). Augmented with a direct src parse
    so a stale/absent generated file cannot make the gate pass vacuously and so
    every offending row carries its claim kind + src location."""
    src = src_claims()
    vend = vendored_rvas()
    claims = {}

    if GEN_NAMES.exists():
        for r in csv.DictReader(GEN_NAMES.open()):
            rva = (r.get("rva") or "").strip()
            if not rva.lower().startswith("0x"):
                continue
            try:
                key = norm_addr(rva)
            except ValueError:
                continue
            if key in vend:                        # vendored library body - co-listed by design
                continue
            if key in src:
                kind, loc = src[key]
            else:                                  # gen-only (e.g. ??_7 vtable data) - no src line
                kind = "data" if (r.get("kind") or "") == "data" else "rva-symbol"
                loc = "(build/gen/symbol_names.csv)"
            claims[key] = (kind, loc, r.get("name", ""))

    # Fallback / augment: the src parse is the never-vacuous floor. Present even when
    # GEN_NAMES is stale/absent; carries claim kind + line for anything the generated
    # file dropped (e.g. an unbound DATA() the labels authority-check discarded).
    for rva, (kind, loc) in src.items():
        claims.setdefault(rva, (kind, loc, ""))

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
    claims = generated_claims()
    lib = library_rows()

    # Never pass vacuously: src always carries thousands of RVA()/DATA() claims, so
    # an empty claim set means the parse itself failed - fail loud rather than green.
    if not claims:
        print("library-overlap: FATAL - parsed 0 src claims (src empty or unreadable?); "
              "refusing to pass vacuously.", file=sys.stderr)
        return 1

    overlap = sorted(set(claims) & set(lib))
    if overlap:
        print(f"{len(overlap)} RVA(s) double-claimed by src AND config/library_labels.csv:",
              file=sys.stderr)
        print(f"  {'rva':>10}  {'claim-kind':<10}  src <-> csv", file=sys.stderr)
        for rva in overlap:
            kind, loc, name = claims[rva]
            src_desc = f"{name} @ {loc}" if name else loc
            print(f"  {rva:>10}  {kind:<10}  src {src_desc}  <->  csv {lib[rva]}", file=sys.stderr)
        print("Each retail RVA must be a src RECONSTRUCTION xor a library CARVE-OUT.",
              file=sys.stderr)
        print("Fix: prune the false CSV row (src is a genuine game body), OR carve the",
              file=sys.stderr)
        print("hand-copied MFC/CRT body to the CSV and call the real routine via <Mfc.h>.",
              file=sys.stderr)
        return 1

    by_kind = {}
    for kind, _loc, _name in claims.values():
        by_kind[kind] = by_kind.get(kind, 0) + 1
    kinds = ", ".join(f"{n} {k}" for k, n in sorted(by_kind.items()))
    src_note = "" if GEN_NAMES.exists() else " (symbol_names.csv absent - src-parse fallback)"
    print(f"library-overlap: OK - {len(claims)} src claims ({kinds}){src_note}, "
          f"{len(lib)} library rows, 0 overlap.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
