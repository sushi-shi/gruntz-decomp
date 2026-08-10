"""gruntz.core.data_universe - retail's data bytes as the denominator.

WHY THIS EXISTS
---------------
`matched_data` answers "of the bytes we enrolled, how many are byte-equal?" It
cannot answer "how much of retail's data did we enroll?", because **we generate
both sides of the comparison**: a datum with no `DATA()` pin is never carved into
a target object and never declared in a base object, so the byte enters neither
side of the objdiff pair (docs: `data-matching-problems.md` #1). A 100% there is
therefore compatible with any amount of unenrolled data, and it read as
`100.00% size-weighted across 723,491 B` while only about **40%** of retail's initialized
data was enrolled.

Three numbers, never one:

    gross coverage  of ALL RETAIL data bytes, how many does a claim cover?
                    denominator = the PE section table, which we did not write
    reconstructable coverage  enrolled / bytes not proven private and
                    unreachable by the checked static-reachability partition
    fidelity  of the enrolled bytes, how many match?
              this is the old headline, and it is the *narrow* question

and `.bss` is reported APART from initialized data. Zero-fill matching zero-fill
is real but nearly free, and blending it into one headline is how "data is done"
got believed: `.bss` alone is 76.8% of `matched_data`'s denominator.

Ownership cannot exclude a reachable byte.  Library-owned data directly named by
game/compiler code, or reached through pointers in enrolled/game-visible data,
stays eligible.  Traversal stops at proven library functions, and unclassified
bytes remain eligible so uncertainty cannot improve the score.

THE THREE REGIONS, ALL FROM THE PE SECTION TABLE
-------------------------------------------------
`.rdata`  raw_size bytes stored in the retail image, including the 88-byte
          FileAlignment tail. Coverage is deliberately a file-byte metric, so
          that tail remains in the denominator and the partition reports it as
          padding rather than silently removing it.
`.data`   the first raw_size bytes: initialized data stored in the file.
`.bss`    `.data`'s virtual tail past raw_size - zero fill, no stored bytes.
          This image has no separate `.bss` section header; the boundary is
          `.data`'s raw_size (0x229400 here).

COVERAGE COUNTS DISTINCT RETAIL BYTES, NOT PER-UNIT SUMS. objdiff's
`total_data` adds up each unit's section sizes, so a COMDAT string literal
defined in N units counts N times: 723,491 per-unit bytes over a data universe
that is only 898,900 bytes wide, of which 660,782 distinct bytes are actually
covered. Enrolment is a property of an ADDRESS, so it is counted once.
"""
from __future__ import annotations

import csv
from pathlib import Path

from gruntz.core.pe import PE, REPO

CLAIMS = REPO / "build/gen/delink_data_manifest.tsv"
SECTIONS = REPO / "build/gen/delink_data_section_manifest.tsv"
PARTITION = REPO / "config/retail/data-coverage-partition.tsv"

#: region key -> the README/report label
LABELS = {
    "rdata": "`.rdata`",
    "data": "`.data` (initialized)",
    "bss": "`.bss` (zero fill)",
}


def regions(pe: PE | None = None) -> dict[str, tuple[int, int]]:
    """`{region: (lo_rva, hi_rva)}` for the three data regions of the image."""
    pe = pe or PE()
    sec = {s["name"]: s for s in pe.sections}
    rd, da = sec[".rdata"], sec[".data"]
    return {
        "rdata": (rd["rva"], rd["rva"] + rd["raw_size"]),
        "data": (da["rva"], da["rva"] + da["raw_size"]),
        "bss": (da["rva"] + da["raw_size"], da["rva"] + da["virtual_size"]),
    }


def _rows(path: Path):
    if not path.is_file():
        return []
    with path.open() as f:
        return list(csv.DictReader(f, delimiter="\t"))


def enrolled_runs() -> list[tuple[int, int]]:
    """Maximal `[lo, hi)` runs of retail bytes that objdiff can compare.

    A byte is enrolled when an enrolled datum claims it OR a placed candidate
    section rebuilds it - the same union `gruntz.audit.data_coverage` uses, and
    for the same reason: a `/GR` vtable COMDAT is 4 bytes longer than the `??_7`
    extent (the complete-object-locator pointer in front of it has no datum of
    its own) and the delinker still rebuilds those 4 bytes.

    Returns `[]` when the delink manifests are absent, so callers degrade to
    "coverage unknown" rather than to a fabricated 0%.
    """
    ext = set()
    for r in _rows(CLAIMS):
        if str(r.get("provenance", "")).startswith("provisional-band-gap"):
            # A band-gap row carves retail bytes so the MISS costs in objdiff;
            # it models nothing, so it may not count as coverage.
            continue
        ext.add((int(r["rva"], 16), int(r["size"], 16)))
    for r in _rows(SECTIONS):
        if r.get("rva", "-") == "-":       # non-affine: claims no retail range
            continue
        ext.add((int(r["rva"], 16), int(r["size"], 16)))
    merged: list[list[int]] = []
    for a, size in sorted(ext):
        b = a + size
        if merged and a <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b)
        else:
            merged.append([a, b])
    return [(a, b) for a, b in merged]


def _clip(runs, lo, hi) -> int:
    return sum(min(b, hi) - max(a, lo) for a, b in runs if max(a, lo) < min(b, hi))


def _merge_intervals(intervals):
    """Canonical union of ``[lo, hi)`` intervals."""
    out = []
    for lo, hi in sorted(intervals):
        if out and lo <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], hi))
        else:
            out.append((lo, hi))
    return out


def _partition(regs, enrolled):
    """Read the checked unenrolled-byte partition used by eligible coverage.

    A stale or malformed artifact returns ``None``.  The build gate reports the
    actionable diff; the scoreboard must never print a flattering figure from a
    partition that no longer accounts for the current enrollment gaps exactly.
    """
    if not PARTITION.is_file():
        return None
    by_region = {"rdata": {"eligible_unenrolled": 0, "excluded": 0},
                 "data": {"eligible_unenrolled": 0, "excluded": 0},
                 "bss": {"eligible_unenrolled": 0, "excluded": 0}}
    categories: dict[str, int] = {}
    intervals = {"rdata": [], "data": [], "bss": []}
    try:
        with PARTITION.open(newline="") as f:
            for row in csv.DictReader(f, delimiter="\t"):
                key = row["section"].lstrip(".")
                if key not in by_region:
                    return None
                lo, hi, size = int(row["rva"], 16), int(row["end"], 16), int(row["size"])
                if hi - lo != size or size <= 0:
                    return None
                rlo, rhi = regs[key]
                if not (rlo <= lo < hi <= rhi):
                    return None
                intervals[key].append((lo, hi))
                field = ("eligible_unenrolled" if row["coverage"] == "eligible"
                         else "excluded" if row["coverage"] == "excluded" else None)
                if field is None:
                    return None
                by_region[key][field] += size
                verdict = row["verdict"]
                categories[verdict] = categories.get(verdict, 0) + size
    except (OSError, KeyError, ValueError):
        return None
    for key, (lo, hi) in regs.items():
        covered = _merge_intervals(
            (max(a, lo), min(b, hi)) for a, b in enrolled
            if max(a, lo) < min(b, hi))
        expected = []
        cursor = lo
        for a, b in covered:
            if cursor < a:
                expected.append((cursor, a))
            cursor = max(cursor, b)
        if cursor < hi:
            expected.append((cursor, hi))
        # Partition rows may split one gap into many proof classes, but their
        # union must be the exact current complement, not merely the same byte
        # count.  This prevents a shifted stale artifact flattering the score.
        if _merge_intervals(intervals[key]) != expected:
            return None
    return {"regions": by_region, "categories": categories}


def measures(report_doc=None, pe: PE | None = None) -> dict:
    """Coverage + fidelity per region, plus the objdiff figure for continuity.

    ```
    {region: {"retail": B, "enrolled": B, "coverage": %, "fidelity": %,
              "objdiff_bytes": B, "objdiff_weighted": B},
     "initialized": {...},          # .rdata + .data
     "objdiff": {"matched_data": B, "total_data": B}}
    ```

    `coverage` is measured against retail; `fidelity` against objdiff's own
    per-unit section rows (the only per-byte match signal that exists), so it is
    a property of the enrolled set and NOT comparable to `coverage`'s
    denominator. When the delink manifests are absent every `enrolled`/`coverage`
    is `None` - unknown, never 0.
    """
    import json

    pe = pe or PE()
    regs = regions(pe)
    runs = enrolled_runs()

    if report_doc is None:
        p = REPO / "build/objdiff/report.json"
        report_doc = json.loads(p.read_text()) if p.is_file() else {}

    # objdiff's per-unit section rows, keyed by section name (per-unit SUMS: a
    # folded COMDAT counts once per unit that defines it).
    od: dict[str, dict] = {}
    for u in report_doc.get("units", []):
        for s in u.get("sections", []):
            if s["name"] == ".text":
                continue
            row = od.setdefault(s["name"], {"bytes": 0, "weighted": 0.0})
            size = int(s["size"])
            row["bytes"] += size
            row["weighted"] += size * float(s.get("fuzzy_match_percent") or 0.0) / 100.0

    def sect(key):
        return od.get({"rdata": ".rdata", "data": ".data", "bss": ".bss"}[key],
                      {"bytes": 0, "weighted": 0.0})

    out: dict[str, dict] = {}
    for key, (lo, hi) in regs.items():
        s = sect(key)
        enrolled = _clip(runs, lo, hi) if runs else None
        out[key] = {
            "retail": hi - lo,
            "enrolled": enrolled,
            "coverage": (100.0 * enrolled / (hi - lo)) if enrolled is not None and hi > lo else None,
            "objdiff_bytes": s["bytes"],
            "objdiff_weighted": s["weighted"],
            "fidelity": (100.0 * s["weighted"] / s["bytes"]) if s["bytes"] else None,
        }
    init_retail = out["rdata"]["retail"] + out["data"]["retail"]
    init_enrolled = (None if runs == [] else
                     out["rdata"]["enrolled"] + out["data"]["enrolled"])
    ib = out["rdata"]["objdiff_bytes"] + out["data"]["objdiff_bytes"]
    iw = out["rdata"]["objdiff_weighted"] + out["data"]["objdiff_weighted"]
    out["initialized"] = {
        "retail": init_retail,
        "enrolled": init_enrolled,
        "coverage": (100.0 * init_enrolled / init_retail) if init_enrolled is not None else None,
        "objdiff_bytes": ib,
        "objdiff_weighted": iw,
        "fidelity": (100.0 * iw / ib) if ib else None,
    }
    part = (_partition(regs, runs)
            if init_enrolled is not None else None)
    for key in ("rdata", "data", "bss"):
        row = part["regions"][key] if part else None
        excluded = row["excluded"] if row else None
        eligible = out[key]["retail"] - excluded if row else None
        out[key].update({
            "excluded": excluded,
            "eligible_retail": eligible,
            "eligible_unenrolled": row["eligible_unenrolled"] if row else None,
            "reconstructable_coverage": (
                100.0 * out[key]["enrolled"] / eligible
                if eligible and out[key]["enrolled"] is not None else None),
        })
    init_excluded = (sum(out[k]["excluded"] for k in ("rdata", "data"))
                     if part else None)
    init_eligible = init_retail - init_excluded if part else None
    out["initialized"].update({
        "excluded": init_excluded,
        "eligible_retail": init_eligible,
        "eligible_unenrolled": (
            sum(out[k]["eligible_unenrolled"] for k in ("rdata", "data"))
            if part else None),
        "reconstructable_coverage": (
            100.0 * init_enrolled / init_eligible
            if init_eligible and init_enrolled is not None else None),
    })
    out["partition"] = part
    m = report_doc.get("measures", {})
    out["objdiff"] = {
        "matched_data": int(m.get("matched_data") or 0),
        "total_data": int(m.get("total_data") or 0),
        "matched_data_percent": float(m.get("matched_data_percent") or 0.0),
    }
    return out
