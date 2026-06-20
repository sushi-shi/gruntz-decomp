#!/usr/bin/env python3
"""Verify source-backed stub metadata.

The source tree is the single source of truth for labeled-but-unmatched code:
each stub carries a compact `// @...` comment block ending in `// @stub`, with
its retail address on an RVA()/RVAU() macro (src/rva.h) above the definition:

    // @confidence: high
    // @source: rtti-vptr
    // @stub
    RVA(0x00b7b0, 0x80)
    void CAmbientSound::Stub_00b7b0() {}

This check keeps that metadata mechanically sane: every stub block carries the
required tags (@confidence, @source), addresses/sizes parse and are unique, and a
`Stub_<rva>` placeholder name agrees with its RVA. The address/size live in the
RVA macro (labels.py reads it from LLVM IR); an unpinned deleting-dtor thunk may
carry no RVA at all. (The old `// @address:`/`// @size:` comments + the engine-
label / @kind / @name / @prototype cruft were folded away once the source itself
became authoritative.)
"""
from __future__ import annotations

import csv
import re
import sys
from collections import defaultdict
from pathlib import Path


REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"
REQUIRED_TAGS = ("confidence", "source")
TAG_RE = re.compile(r"^\s*// @([A-Za-z_-]+):\s*(.*?)\s*$")
STUB_BODY_RE = re.compile(r"\bStub_([0-9a-fA-F]+)\b")
# RVA(0x.., 0x..) or RVAU(0x..) carrying the stub's retail address (+ size).
RVA_RE = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\)")
RVAU_RE = re.compile(r"\bRVAU\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")


def norm_addr(value: str) -> str:
    return "0x%06x" % int(value, 16)


def scan_stub_blocks():
    """Yield (path, stub_line_no, tags, addr, size, body) for each `// @stub`.

    A block is the contiguous run of `// @tag:` comment lines immediately above a
    `// @stub` line; the retail address/size come from the RVA()/RVAU() macro
    between the `// @stub` line and the definition (or None for an unpinned
    thunk), and `body` is the definition line (for the Stub_<rva> name check).
    """
    for path in sorted(SRC.rglob("*.cpp")):
        lines = path.read_text().splitlines()
        for i, line in enumerate(lines):
            if line.strip() != "// @stub":
                continue
            tags = {}
            j = i - 1
            while j >= 0:
                m = TAG_RE.match(lines[j])
                if not m:
                    break
                tags.setdefault(m.group(1), m.group(2))
                j -= 1
            addr = size = None
            body = ""
            for k in range(i + 1, min(i + 5, len(lines))):
                s = lines[k].strip()
                if not s or s.startswith("//"):
                    continue
                mr = RVA_RE.search(s)
                mu = RVAU_RE.search(s)
                if mr:
                    addr, size = mr.group(1), mr.group(2)
                    continue
                if mu:
                    addr = mu.group(1)
                    continue
                body = s          # the definition line
                break
            yield path, i + 1, tags, addr, size, body


def main() -> int:
    errors = []
    by_address = defaultdict(list)
    count = 0

    for path, line, tags, addr, size, body in scan_stub_blocks():
        count += 1
        rel = path.relative_to(REPO)
        where = f"{rel}:{line}"

        for tag in REQUIRED_TAGS:
            if tag not in tags:
                errors.append(f"{where}: @stub block missing @{tag}")

        if addr:  # no RVA macro is allowed (unpinned deleting-dtor thunks)
            try:
                addr = norm_addr(addr)
                by_address[addr].append(where)
            except ValueError:
                errors.append(f"{where}: invalid RVA address {addr!r}")
                addr = ""

        if size:
            try:
                int(size, 16)
            except ValueError:
                errors.append(f"{where}: invalid RVA size {size!r}")

        m = STUB_BODY_RE.search(body)
        if m and addr and norm_addr(m.group(1)) != addr:
            errors.append(f"{where}: Stub_{m.group(1)} name disagrees with RVA {addr}")

    for addr, sites in sorted(by_address.items()):
        if len(sites) > 1:
            errors.append(f"duplicate @address {addr}: " + ", ".join(sites))

    # Cross-check: a stub address must NOT also be a MATCHED function - else it's
    # a stale stub duplicating reconstructed code (and labels.py skips the stub
    # unit, so its own dup-guard can't see it). "Matched" = an RVA()/RVAU() macro
    # NOT in a `// @stub` block, or a row in config/zlib_labels.csv.
    matched = {}
    for path in sorted(SRC.rglob("*.cpp")):
        lines = path.read_text().splitlines()
        for k, ln in enumerate(lines):
            for mm in list(RVA_RE.finditer(ln)) + list(RVAU_RE.finditer(ln)):
                if any(lines[x].strip() == "// @stub" for x in range(max(0, k - 4), k)):
                    continue  # this RVA belongs to a stub, not a matched function
                try:
                    matched.setdefault(norm_addr(mm.group(1)), f"{path.relative_to(REPO)}:{k + 1}")
                except ValueError:
                    pass
    zlib_cfg = REPO / "config" / "zlib_labels.csv"
    if zlib_cfg.exists():
        for r in csv.reader(zlib_cfg.open()):
            if r and r[0].startswith("0x"):
                try:
                    matched.setdefault(norm_addr(r[0]), f"config/zlib_labels.csv ({r[1]})")
                except ValueError:
                    pass
    for addr, sites in sorted(by_address.items()):
        if addr in matched:
            errors.append(f"stub @address {addr} ({sites[0]}) duplicates a MATCHED "
                          f"function at {matched[addr]} - prune the stale stub")

    if errors:
        for err in errors:
            print(err, file=sys.stderr)
        return 1
    print(f"verified {count} stub blocks ({len(by_address)} pinned addresses)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
