#!/usr/bin/env python3
"""Verify source-backed stub metadata.

The source tree is the single source of truth for labeled-but-unmatched code:
each stub carries a compact `// @...` comment block ending in `// @stub`, e.g.

    // @confidence: high
    // @source: rtti-vptr
    // @address: 0x00b7b0
    // @size:    0x80
    // @stub
    void CAmbientSound::Stub_00b7b0() {}

This check keeps that metadata mechanically sane: every stub block carries the
required tags, addresses are valid and unique, and a `Stub_<rva>` placeholder
name agrees with its `@address`. (The old engine-label / @kind / @name /
@prototype cruft was folded away once the source itself became authoritative.)
"""
from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path


REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent.parent)
SRC = REPO / "src"
REQUIRED_TAGS = ("confidence", "source", "address", "size")
TAG_RE = re.compile(r"^\s*// @([A-Za-z_-]+):\s*(.*?)\s*$")
STUB_BODY_RE = re.compile(r"\bStub_([0-9a-fA-F]+)\b")


def norm_addr(value: str) -> str:
    return "0x%06x" % int(value, 16)


def scan_stub_blocks():
    """Yield (path, stub_line_no, tags, body) for each `// @stub` block.

    A block is the contiguous run of `// @tag:` comment lines immediately above
    a `// @stub` line, plus the first code line below it (the definition).
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
            body = ""
            for k in range(i + 1, min(i + 4, len(lines))):
                if lines[k].strip() and not lines[k].lstrip().startswith("//"):
                    body = lines[k].strip()
                    break
            yield path, i + 1, tags, body


def main() -> int:
    errors = []
    by_address = defaultdict(list)
    count = 0

    for path, line, tags, body in scan_stub_blocks():
        count += 1
        rel = path.relative_to(REPO)
        where = f"{rel}:{line}"

        for tag in REQUIRED_TAGS:
            if tag not in tags:
                errors.append(f"{where}: @stub block missing @{tag}")

        addr = tags.get("address", "")
        if addr:  # empty @address is allowed (unpinned deleting-dtor thunks)
            try:
                addr = norm_addr(addr)
                by_address[addr].append(where)
            except ValueError:
                errors.append(f"{where}: invalid @address {tags['address']!r}")
                addr = ""

        size = tags.get("size", "")
        if size:
            try:
                int(size, 16)
            except ValueError:
                errors.append(f"{where}: invalid @size {tags['size']!r}")

        m = STUB_BODY_RE.search(body)
        if m and addr and norm_addr(m.group(1)) != addr:
            errors.append(f"{where}: Stub_{m.group(1)} name disagrees with @address {addr}")

    for addr, sites in sorted(by_address.items()):
        if len(sites) > 1:
            errors.append(f"duplicate @address {addr}: " + ", ".join(sites))

    if errors:
        for err in errors:
            print(err, file=sys.stderr)
        return 1
    print(f"verified {count} stub blocks ({len(by_address)} pinned addresses)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
