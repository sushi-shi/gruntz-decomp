"""Audit an address migration between two complete Gruntz executables.

    gruntz sema version-delta --old "$GRUNTZ_EXE_V100" --new "$GRUNTZ_EXE_V101"
    gruntz sema version-delta --old ... --new ... --json

This is deliberately an audit, not a rewriter.  It tests the admitted 1.00
function and data spans against another image after masking PE HIGHLOW fixup
payloads.  A UNIQUE row means the complete admitted span occurs exactly once
in the destination image; MULTIPLE and CHANGED rows still require structural
adjudication.  In particular, this never guesses a new address from a nearby
delta band.

The result is conservative.  A source-identical function can fall in CHANGED
when a rel32 call crosses a version-shift boundary, because rel32 operands are
not PE relocations and are intentionally not masked here.

The active censuses must still describe ``--old``.  Once a migration has been
applied, run this from the pre-migration revision (or a worktree at it); using
the new census as though it described the old image would manufacture a false
address map, so the command refuses that state.
"""

from __future__ import annotations

import argparse
import bisect
import json
import os
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

from gruntz.core.pe import Pe
from gruntz.core.tsv import read as read_tsv
from gruntz.retail_labels.source import sweep_sites
from gruntz.sema import die
from gruntz.sema.image import Image


ROOT = Path(__file__).resolve().parents[3]
RETAIL = ROOT / "config" / "retail"


@dataclass(frozen=True)
class Span:
    rva: int
    size: int
    kind: str


@dataclass(frozen=True)
class Match:
    span: Span
    status: str
    new_rva: int | None


def _normal_section(pe: Pe, img: Image, name: str) -> tuple[int, bytes]:
    section = pe.section(name)
    start = section["va"]
    size = max(section["vsize"], section["rsize"])
    blob = bytearray(pe.read(start, size) or b"")
    for site in img.reloc:
        if start <= site and site + 4 <= start + len(blob):
            blob[site - start:site - start + 4] = b"\0\0\0\0"
    return start, bytes(blob)


def _section_name(pe: Pe, rva: int) -> str | None:
    for section in pe.sections:
        if (section["va"] <= rva
                < section["va"] + max(section["vsize"], section["rsize"])):
            return section["name"]
    return None


def _partition(path: Path, pe: Pe, allowed: set[str]) -> list[Span]:
    _banner, _header, rows = read_tsv(path)
    raw = [(int(row["rva"], 0), row.get("kind", "")) for row in rows]
    out: list[Span] = []
    for index, (rva, kind) in enumerate(raw):
        section_name = _section_name(pe, rva)
        if section_name not in allowed:
            continue
        section = pe.section(section_name)
        end = section["va"] + max(section["vsize"], section["rsize"])
        if index + 1 < len(raw) and _section_name(pe, raw[index + 1][0]) == section_name:
            end = raw[index + 1][0]
        if end > rva:
            out.append(Span(rva, end - rva, kind or "body"))
    return out


def _find_two(haystack: bytes, needle: bytes) -> list[int]:
    """Return zero, one, or two hits; two means 'at least two'."""
    hits: list[int] = []
    at = haystack.find(needle)
    while at >= 0 and len(hits) < 2:
        hits.append(at)
        at = haystack.find(needle, at + 1)
    return hits


def _match_partition(spans: list[Span], old_region: tuple[int, bytes],
                     new_region: tuple[int, bytes]) -> list[Match]:
    old_base, old_blob = old_region
    new_base, new_blob = new_region
    out: list[Match] = []
    cache: dict[bytes, list[int]] = {}
    for span in spans:
        offset = span.rva - old_base
        payload = old_blob[offset:offset + span.size]
        if len(payload) != span.size or not payload:
            out.append(Match(span, "unreadable", None))
            continue
        hits = cache.setdefault(payload, _find_two(new_blob, payload))
        if len(hits) == 1:
            out.append(Match(span, "unique", new_base + hits[0]))
        elif hits:
            out.append(Match(span, "multiple", None))
        else:
            out.append(Match(span, "changed", None))
    return out


def _rel32_equivalent(old: bytes, new: bytes) -> bool:
    """Whether equal-sized bodies differ only in matching rel32 operands.

    This recognizes the two i386 encodings whose displacement is fixed width:
    E8/E9 and 0F 8x.  It is a candidate class, not an exact-byte proof, because
    the byte walk is not a full x86 decoder.
    """
    if len(old) != len(new):
        return False
    operands = set()
    for index in range(len(old)):
        if (old[index] == new[index] and old[index] in (0xE8, 0xE9)
                and index + 5 <= len(old)):
            operands.update(range(index + 1, index + 5))
        if (index + 6 <= len(old) and old[index:index + 2] == new[index:index + 2]
                and old[index] == 0x0F and 0x80 <= old[index + 1] <= 0x8F):
            operands.update(range(index + 2, index + 6))
    return bool(operands) and all(
        left == right or index in operands
        for index, (left, right) in enumerate(zip(old, new))
    )


def _place_functions(matches: list[Match], old_region: tuple[int, bytes],
                     new_region: tuple[int, bytes]) -> list[Match]:
    """Place ambiguous rows at a neighbour-proven delta, then verify bytes.

    Exact placements are mechanical migration candidates.  rel32 placements
    are reported separately because they need a real disassembly check before
    an address rewriter may consume them.
    """
    old_base, old_blob = old_region
    new_base, new_blob = new_region
    anchors = [index for index, row in enumerate(matches) if row.new_rva is not None]
    if not anchors:
        return matches
    out = list(matches)
    for index, row in enumerate(matches):
        if row.new_rva is not None:
            continue
        prior = next((item for item in reversed(anchors) if item < index), None)
        following = next((item for item in anchors if item > index), None)
        deltas: list[int] = []
        for neighbour in (prior, following):
            if neighbour is None:
                continue
            anchor = matches[neighbour]
            delta = anchor.new_rva - anchor.span.rva
            if delta not in deltas:
                deltas.append(delta)
        old_at = row.span.rva - old_base
        payload = old_blob[old_at:old_at + row.span.size]
        exact: list[int] = []
        rel32: list[int] = []
        for delta in deltas:
            new_rva = row.span.rva + delta
            new_at = new_rva - new_base
            candidate = new_blob[new_at:new_at + row.span.size]
            if len(candidate) != row.span.size:
                continue
            if candidate == payload:
                exact.append(new_rva)
            elif _rel32_equivalent(payload, candidate):
                rel32.append(new_rva)
        if len(set(exact)) == 1:
            out[index] = Match(row.span, "placed-exact", exact[0])
        elif not exact and len(set(rel32)) == 1:
            out[index] = Match(row.span, "placed-rel32", rel32[0])
    return out


def _function_names() -> dict[int, str]:
    path = ROOT / "config" / "match_baseline.tsv"
    names: dict[int, str] = {}
    in_functions = False
    header: list[str] | None = None
    for line in path.read_text().splitlines():
        if line.startswith("# [functions]"):
            in_functions = True
            header = line.removeprefix("# [functions]\t").split("\t")
            continue
        if not in_functions or not line or line.startswith("#") or header is None:
            continue
        fields = line.split("\t")
        if len(fields) == len(header):
            row = dict(zip(header, fields))
            names[int(row["rva"], 0)] = row["function"]
    return names


def _summary(matches: list[Match]) -> dict[str, object]:
    statuses = Counter(row.status for row in matches)
    kinds: dict[str, dict[str, int]] = {}
    for row in matches:
        kinds.setdefault(row.span.kind, {})[row.status] = (
            kinds.setdefault(row.span.kind, {}).get(row.status, 0) + 1)
    deltas = Counter(
        row.new_rva - row.span.rva
        for row in matches if row.new_rva is not None
    )
    return {
        "total": len(matches),
        "status": dict(sorted(statuses.items())),
        "kind": dict(sorted(kinds.items())),
        "top_deltas": [
            {"delta": f"{delta:+#x}", "count": count}
            for delta, count in deltas.most_common(16)
        ],
    }


def _site_summary(functions: list[Match], data: list[Match]) -> dict[str, object]:
    exact = {
        row.span.rva: row.new_rva
        for row in functions + data
        if row.status in ("unique", "placed-exact")
    }
    rel32_candidates = {
        row.span.rva: row.new_rva
        for row in functions if row.status == "placed-rel32"
    }
    sites = sweep_sites()
    by_macro: dict[str, dict[str, int]] = {}
    unique_rvas: set[int] = set()
    mapped_rvas: set[int] = set()
    occurrences = mapped_occurrences = 0
    for macro, rows in sites.items():
        total = sum(len(where) for where in rows.values())
        known = sum(len(where) for rva, where in rows.items() if rva in exact)
        candidate = sum(
            len(where) for rva, where in rows.items() if rva in rel32_candidates)
        by_macro[macro] = {
            "unique_rvas": len(rows),
            "unique_mechanically_mapped": sum(rva in exact for rva in rows),
            "unique_rel32_candidates": sum(
                rva in rel32_candidates for rva in rows),
            "occurrences": total,
            "occurrences_mechanically_mapped": known,
            "occurrences_rel32_candidates": candidate,
        }
        unique_rvas.update(rows)
        mapped_rvas.update(rva for rva in rows if rva in exact)
        occurrences += total
        mapped_occurrences += known
    return {
        "unique_rvas": len(unique_rvas),
        "unique_mechanically_mapped": len(mapped_rvas),
        "occurrences": occurrences,
        "occurrences_mechanically_mapped": mapped_occurrences,
        "unique_rel32_candidates": len(
            unique_rvas & set(rel32_candidates)),
        "occurrences_rel32_candidates": sum(
            len(where)
            for rows in sites.values()
            for rva, where in rows.items()
            if rva in rel32_candidates),
        "by_macro": dict(sorted(by_macro.items())),
    }


_ADDRESS_FIELDS = {
    "rva", "function_rva", "target_rva", "site_rva",
    "lo", "hi", "start", "end",
}


def _tracked_rows(path: Path) -> tuple[list[str], list[list[str]]]:
    """Read a tracked table, including the two intentionally special forms."""
    lines = path.read_text().splitlines()
    if path.name == "link_order.tsv":
        marker = next(line for line in lines if line.startswith("# seq\t"))
        header = marker.removeprefix("# ").split("\t")
        return header, [
            line.split("\t") for line in lines
            if line and not line.startswith("#")
        ]
    if path.name == "match_baseline.tsv":
        marker = next(line for line in lines if line.startswith("# [functions]\t"))
        header = marker.removeprefix("# [functions]\t").split("\t")
        start = lines.index(marker) + 1
        return header, [
            line.split("\t") for line in lines[start:]
            if line and not line.startswith("#")
        ]
    _banner, header, rows = read_tsv(path)
    return header, [[row.get(field, "") for field in header] for row in rows]


def _address_inventory(functions: list[Match], data: list[Match]) -> dict[str, object]:
    exact_rows = [
        row for row in functions + data
        if row.status in ("unique", "placed-exact") and row.new_rva is not None
    ]
    candidate_rows = [
        row for row in functions
        if row.status == "placed-rel32" and row.new_rva is not None
    ]
    exact_rows.sort(key=lambda row: row.span.rva)
    candidate_rows.sort(key=lambda row: row.span.rva)
    exact_starts = [row.span.rva for row in exact_rows]
    candidate_starts = [row.span.rva for row in candidate_rows]

    def contains(rva: int, rows: list[Match], starts: list[int]) -> bool:
        index = bisect.bisect_right(starts, rva) - 1
        return index >= 0 and rva < rows[index].span.rva + rows[index].span.size

    exact_boundaries = {
        row.span.rva + row.span.size
        for row in exact_rows
    }
    candidate_boundaries = {
        row.span.rva + row.span.size
        for row in candidate_rows
    }
    paths = sorted(RETAIL.glob("*.tsv")) + [ROOT / "config" / "match_baseline.tsv"]
    tables: dict[str, dict[str, int]] = {}
    totals = Counter()
    for path in paths:
        header, rows = _tracked_rows(path)
        indexes = [index for index, field in enumerate(header) if field in _ADDRESS_FIELDS]
        counts = Counter()
        for fields in rows:
            for index in indexes:
                if index >= len(fields) or not fields[index].lower().startswith("0x"):
                    continue
                counts["address_cells"] += 1
                rva = int(fields[index], 0)
                if (contains(rva, exact_rows, exact_starts)
                        or rva in exact_boundaries):
                    counts["mechanically_mapped"] += 1
                elif (contains(rva, candidate_rows, candidate_starts)
                      or rva in candidate_boundaries):
                    counts["rel32_candidates"] += 1
                else:
                    counts["requiring_adjudication"] += 1
        if counts["address_cells"]:
            key = str(path.relative_to(ROOT))
            tables[key] = dict(counts)
            totals.update(counts)
    return {"totals": dict(totals), "tables": tables}


def audit(old_path: Path, new_path: Path) -> dict[str, object]:
    old_pe, new_pe = Pe(old_path), Pe(new_path)
    _banner, _header, census_rows = read_tsv(RETAIL / "data.tsv")
    census_rvas = {int(row["rva"], 0) for row in census_rows}
    old_data_start = old_pe.section(".data")["va"]
    if old_data_start not in census_rvas:
        die(
            "the active retail census no longer describes --old "
            f"(.data starts at 0x{old_data_start:x}, but no census row does); "
            "run version-delta from the pre-migration revision"
        )
    old_img, new_img = Image(old_pe), Image(new_pe)
    old_text = _normal_section(old_pe, old_img, ".text")
    new_text = _normal_section(new_pe, new_img, ".text")
    function_spans = _partition(RETAIL / "functions.tsv", old_pe, {".text"})
    functions = _place_functions(
        _match_partition(function_spans, old_text, new_text), old_text, new_text)

    data_spans = _partition(
        RETAIL / "data.tsv", old_pe, {".rdata", ".data", ".idata"})
    data_matches: list[Match] = []
    for name in (".rdata", ".data", ".idata"):
        section_spans = [
            row for row in data_spans if _section_name(old_pe, row.rva) == name
        ]
        if section_spans:
            data_matches.extend(_match_partition(
                section_spans,
                _normal_section(old_pe, old_img, name),
                _normal_section(new_pe, new_img, name),
            ))

    names = _function_names()
    changed_named = [
        {"old_rva": f"0x{row.span.rva:08x}",
         "size": f"0x{row.span.size:x}",
         "kind": row.span.kind,
         "name": names.get(row.span.rva, "")}
        for row in functions
        if row.status not in ("unique", "placed-exact") and names.get(row.span.rva)
    ]
    sections = []
    for old_section in old_pe.sections:
        try:
            new_section = new_pe.section(old_section["name"])
        except KeyError:
            continue
        sections.append({
            "name": old_section["name"],
            "old_rva": f"0x{old_section['va']:x}",
            "new_rva": f"0x{new_section['va']:x}",
            "old_virtual_size": f"0x{old_section['vsize']:x}",
            "new_virtual_size": f"0x{new_section['vsize']:x}",
        })
    return {
        "old": str(old_path),
        "new": str(new_path),
        "sections": sections,
        "functions": _summary(functions),
        "data": _summary(data_matches),
        "source_labels": _site_summary(functions, data_matches),
        "tracked_address_cells": _address_inventory(functions, data_matches),
        "named_functions_requiring_adjudication": changed_named,
    }


def _print(report: dict[str, object]) -> None:
    print(f"old: {report['old']}")
    print(f"new: {report['new']}")
    for heading in ("functions", "data"):
        row = report[heading]
        status = ", ".join(f"{key}={value}" for key, value in row["status"].items())
        print(f"{heading}: total={row['total']}  {status}")
        print("  delta bands: " + ", ".join(
            f"{item['delta']}:{item['count']}" for item in row["top_deltas"]))
    sites = report["source_labels"]
    print("source labels: "
          f"{sites['unique_mechanically_mapped']}/{sites['unique_rvas']} unique RVAs; "
          f"{sites['occurrences_mechanically_mapped']}/{sites['occurrences']} occurrences; "
          f"rel32 candidates={sites['unique_rel32_candidates']}")
    for macro, row in sites["by_macro"].items():
        print(f"  {macro}: {row['unique_mechanically_mapped']}/"
              f"{row['unique_rvas']} unique, "
              f"{row['occurrences_mechanically_mapped']}/"
              f"{row['occurrences']} occurrences, "
              f"rel32 candidates={row['unique_rel32_candidates']}")
    print("named functions requiring adjudication: "
          f"{len(report['named_functions_requiring_adjudication'])}")
    cells = report["tracked_address_cells"]["totals"]
    print("tracked address cells: "
          f"{cells.get('mechanically_mapped', 0)}/"
          f"{cells.get('address_cells', 0)} mechanically mapped; "
          f"rel32 candidates={cells.get('rel32_candidates', 0)}; "
          f"adjudication={cells.get('requiring_adjudication', 0)}")
    for path, row in report["tracked_address_cells"]["tables"].items():
        print(f"  {path}: {row.get('mechanically_mapped', 0)}/"
              f"{row['address_cells']} exact, "
              f"rel32={row.get('rel32_candidates', 0)}, "
              f"adjudicate={row.get('requiring_adjudication', 0)}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--old", default=os.environ.get("GRUNTZ_EXE_V100"))
    parser.add_argument("--new", default=os.environ.get("GRUNTZ_EXE_V101"))
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    if not args.old or not args.new:
        die("pass --old/--new or enter the flake shell that exports "
            "GRUNTZ_EXE_V100 and GRUNTZ_EXE_V101")
    report = audit(Path(args.old), Path(args.new))
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        _print(report)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
