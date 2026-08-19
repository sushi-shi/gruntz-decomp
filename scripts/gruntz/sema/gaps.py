"""gruntz.sema.gaps - derive unclaimed code between adjacent same-file claims.

This is the source-claim arithmetic oracle from
docs/patterns/same-file-interior-gap-scan-finds-missing-bodies.md. It deliberately
does not depend on Ghidra's function carving: tiny accessors, thunks, and compiler-
generated bodies are exactly what an analyzer tends to omit.

    gruntz sema gaps [--class substantive|thunk|trivial|band|switch-table]
                      [--unit UNIT] [--limit N]

Rows disappear only when a source claim covers them. Classification is navigation,
never permission to leave executable bytes unclaimed.
"""

from __future__ import annotations

import argparse
import struct

from gruntz.core.pe import image
from gruntz.model import resolve
from gruntz.retail_labels.source import sweep_sites


CHANNEL_MACRO = {
    "src": "RVA",
    "src_compgen": "RVA_COMPGEN",
    "src_dyninit": "RVA_DYNINIT",
}


def _file(site: str) -> str:
    return site.rsplit(":", 1)[0]


def _site_files() -> dict[tuple[str, int], set[str]]:
    sites = sweep_sites()
    return {
        (macro, rva): {_file(site) for site in where}
        for macro, rows in sites.items()
        for rva, where in rows.items()
    }


def _trim(rva: int, payload: bytes) -> tuple[int, bytes]:
    lo, hi = 0, len(payload)
    while lo < hi and payload[lo] in (0x90, 0xCC):
        lo += 1
    while hi > lo and payload[hi - 1] in (0x90, 0xCC):
        hi -= 1
    return rva + lo, payload[lo:hi]


def _split(rva: int, payload: bytes) -> list[tuple[int, bytes]]:
    """Split padding-separated, 16-byte-aligned bodies inside one claim gap.

    cl/link padding is a run of 90/cc bytes ending at the next aligned body.
    Requiring both a run of at least four bytes and an aligned next byte avoids
    treating an incidental nop in real code as a function boundary.
    """
    rva, payload = _trim(rva, payload)
    if not payload:
        return []
    out = []
    body_start = 0
    i = 0
    while i < len(payload):
        if payload[i] not in (0x90, 0xCC):
            i += 1
            continue
        pad_start = i
        while i < len(payload) and payload[i] in (0x90, 0xCC):
            i += 1
        if i < len(payload) and i - pad_start >= 4 and (rva + i) % 0x10 == 0:
            body = payload[body_start:pad_start]
            if body:
                out.append((rva + body_start, body))
            body_start = i
    body = payload[body_start:]
    if body:
        out.append((rva + body_start, body))
    return out


def _switch_table(payload: bytes, prev) -> bool:
    if len(payload) < 8 or len(payload) % 4:
        return False
    base = image().image_base
    lo, hi = base + prev.rva, base + prev.rva + prev.size
    values = struct.unpack(f"<{len(payload) // 4}I", payload)
    return all(lo <= value < hi for value in values)


def _kind(payload: bytes, prev) -> str:
    if _switch_table(payload, prev):
        return "switch-table"
    if len(payload) == 5 and payload[0] == 0xE9:
        return "thunk"
    if len(payload) <= 8:
        return "trivial"
    # The current census's repeated compiler/runtime omissions are all large.
    # Keep this payload-only: unit names and known addresses are not evidence.
    if len(payload) >= 0x100:
        return "band"
    return "substantive"


def census() -> list[dict]:
    files = _site_files()
    claims = []
    for binding in resolve().functions:
        macro = CHANNEL_MACRO.get(binding.channel)
        where = files.get((macro, binding.rva), set()) if macro else set()
        if where and binding.size:
            claims.append((binding, where))
    claims.sort(key=lambda row: row[0].rva)

    pe = image()
    rows = []
    for (prev, prev_files), (nxt, next_files) in zip(claims, claims[1:]):
        shared = sorted(prev_files & next_files)
        if not shared:
            continue
        start, end = prev.rva + prev.size, nxt.rva
        if start >= end:
            continue
        payload = pe.read(start, end - start)
        if payload is None:
            continue
        for rva, body in _split(start, payload):
            rows.append({
                "rva": rva,
                "size": len(body),
                "kind": _kind(body, prev),
                "file": shared[0],
                "unit": prev.unit if prev.unit == nxt.unit else f"{prev.unit}|{nxt.unit}",
                "prev": prev.name,
                "next": nxt.name,
                "bytes": body[:16].hex(),
            })
    return rows


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz sema gaps", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--class", dest="kind",
                    choices=("substantive", "thunk", "trivial", "band", "switch-table"))
    ap.add_argument("--unit")
    ap.add_argument("--limit", type=int, default=100)
    args = ap.parse_args(argv)

    rows = census()
    counts = {kind: sum(row["kind"] == kind for row in rows)
              for kind in ("band", "thunk", "trivial", "substantive", "switch-table")}
    sizes = {kind: sum(row["size"] for row in rows if row["kind"] == kind)
             for kind in counts}
    print(f"same-file unclaimed gaps: {len(rows)} row(s), "
          f"{sum(row['size'] for row in rows)} B after edge padding")
    for kind in counts:
        print(f"  {kind:<12} {counts[kind]:3} row(s) {sizes[kind]:5} B")

    selected = [row for row in rows
                if (not args.kind or row["kind"] == args.kind)
                and (not args.unit or args.unit in row["unit"])]
    for row in selected[:args.limit]:
        print(f"0x{row['rva']:06x}+0x{row['size']:<3x} {row['kind']:<12} "
              f"[{row['unit']}] {row['file']}")
        print(f"    {row['prev'][:64]} -> {row['next'][:64]}")
        print(f"    {row['bytes']}")
    if len(selected) > args.limit:
        print(f"... {len(selected) - args.limit} more (--limit)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
