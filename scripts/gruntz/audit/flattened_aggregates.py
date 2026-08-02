#!/usr/bin/env python3
"""Find scalar declarations that are being used as aggregate storage.

This is a suspicion queue for a reconstruction defect that cast metrics cannot see:

    i32 m_left;
    i32 m_top;
    i32 m_right;
    i32 m_bottom;

    i32* p = &m_left;
    use(p[0], p[1], p[2], p[3]);

The pointer conversion is perfectly legal because ``&m_left`` really is ``i32*``.
What is wrong is the declaration: the source is viewing several adjacent scalars as
one lost RECT/POINT/array/record.  The same pattern occurs at file scope when several
separately declared globals are really one retail aggregate.

Two independent tells are reported:

``scalar-base``
    An integer/float pointer is initialized from the address of one scalar member or
    global.  Indexing beyond element zero or advancing the pointer strengthens the
    row, but is not required: the address escape itself has discarded the source
    model's claimed one-element extent.

``wide-io``
    Read/Write/memcpy/memset starts at one scalar member or global and spans more bytes
    than that declaration.  This is especially useful for flattened globals because
    no cast or intermediate pointer is required.

This is deliberately NOT a cleanliness ratchet.  A pointer to a scalar can be an
authentic out-parameter, and contiguous globals are not one object merely because
their retail RVAs are adjacent.  Prioritize scalar-base rows with indexing/arithmetic
or a matching wide-io row, then confirm the candidate from retail accesses,
serialization boundaries, callers, and data xrefs.

    python -m gruntz.audit.flattened_aggregates
    python -m gruntz.audit.flattened_aggregates --summary
    python -m gruntz.audit.flattened_aggregates --kind member
    python -m gruntz.audit.flattened_aggregates --kind global
"""
from __future__ import annotations

import argparse
import collections
import re
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")

SCALAR_WIDTHS = {
    "char": 1,
    "i8": 1,
    "u8": 1,
    "BYTE": 1,
    "i16": 2,
    "u16": 2,
    "WORD": 2,
    "short": 2,
    "i32": 4,
    "u32": 4,
    "float": 4,
    "int": 4,
    "long": 4,
    "LONG": 4,
    "DWORD": 4,
    "UINT": 4,
    "BOOL": 4,
    "i64": 8,
    "u64": 8,
    "double": 8,
    "__int64": 8,
}
SCALAR_TYPE = r"(?:" + "|".join(sorted(SCALAR_WIDTHS, key=len, reverse=True)) + r")"
IDENT = r"[A-Za-z_]\w*"
ORIGIN = rf"{IDENT}(?:(?:->|\.){IDENT})*"

# The delimiter intentionally excludes '[' and '*': arrays and pointers are already
# aggregates.  We want declarations that claim one scalar object.
SCALAR_DECL = re.compile(
    rf"\b(?P<type>{SCALAR_TYPE})\s+(?P<name>[mg]_\w+)\s*(?P<end>[;=,])"
)
SCALAR_COMMA_DECL = re.compile(
    rf"\b(?P<type>{SCALAR_TYPE})\s+{IDENT}\s*,\s*"
    rf"(?P<name>[mg]_\w+)\s*(?P<end>[;=,])"
)
NONSCALAR_DECL = re.compile(
    rf"\b(?P<type>{IDENT})\s+(?P<name>[mg]_\w+)\s*(?P<end>[;=,])"
)
PTR_FROM_ADDRESS = re.compile(
    rf"\b(?P<qual1>const\s+|volatile\s+)?(?P<type>{SCALAR_TYPE})\s*"
    rf"(?P<qual2>const\s+|volatile\s+)?\*\s*(?P<ptr>{IDENT})\s*=\s*\(*\s*&\s*"
    rf"(?P<origin>{ORIGIN})\s*\)*(?=\s*[,;)])"
)
DIRECT_INDEX = re.compile(
    rf"\(\s*&\s*(?P<origin>{ORIGIN})\s*\)\s*\[\s*(?P<index>0x[0-9a-fA-F]+|\d+)\s*\]"
)

IO_TWO_ARG = re.compile(
    rf"\b(?P<call>Read|Write|Send|Receive)\s*\(\s*&\s*(?P<origin>{ORIGIN})\s*,\s*"
    rf"(?P<size>0x[0-9a-fA-F]+|\d+)\b"
)
IO_THREE_ARG = re.compile(
    rf"\b(?P<call>memset|memcpy|memmove)\s*\(\s*&\s*(?P<origin>{ORIGIN})\s*,"
    rf"[^,;]{{0,200}},\s*(?P<size>0x[0-9a-fA-F]+|\d+)\b",
    re.S,
)


@dataclass(frozen=True)
class Candidate:
    path: str
    line: int
    kind: str
    origin: str
    tell: str
    detail: str


def mask_noncode(text: str) -> str:
    """Blank comments and literals while preserving offsets and newlines."""
    out = list(text)
    i = 0
    while i < len(text):
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = len(text) if j < 0 else j
            out[i:j] = " " * (j - i)
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = len(text) if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif text[i] in "\"'":
            quote = text[i]
            out[i] = " "
            i += 1
            while i < len(text):
                if text[i] == "\\" and i + 1 < len(text):
                    out[i] = out[i + 1] = " "
                    i += 2
                else:
                    ch = text[i]
                    if ch != "\n":
                        out[i] = " "
                    i += 1
                    if ch == quote:
                        break
        else:
            i += 1
    return "".join(out)


def line_of(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def enclosing_block(text: str, offset: int) -> tuple[int, int]:
    """Return the smallest brace block containing offset, or the whole file."""
    stack: list[int] = []
    pairs: list[tuple[int, int]] = []
    for i, ch in enumerate(text):
        if ch == "{":
            stack.append(i)
        elif ch == "}" and stack:
            pairs.append((stack.pop(), i + 1))
    containing = [pair for pair in pairs if pair[0] < offset < pair[1]]
    return max(containing, key=lambda pair: pair[0]) if containing else (0, len(text))


def parse_int(token: str) -> int:
    return int(token, 0)


def origin_kind(origin: str) -> str:
    leaf = re.split(r"->|\.", origin)[-1]
    return "global" if leaf.startswith("g_") else "member"


def declarations(
    files: list[tuple[Path, str]],
) -> tuple[dict[str, set[int]], set[str]]:
    widths: dict[str, set[int]] = collections.defaultdict(set)
    aggregates: set[str] = set()
    for _, code in files:
        for pattern in (SCALAR_DECL, SCALAR_COMMA_DECL):
            for match in pattern.finditer(code):
                widths[match.group("name")].add(SCALAR_WIDTHS[match.group("type")])
        for match in NONSCALAR_DECL.finditer(code):
            if match.group("type") not in SCALAR_WIDTHS:
                aggregates.add(match.group("name"))
    return widths, aggregates


def scan() -> list[Candidate]:
    files = []
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix in (".cpp", ".h"):
                raw = path.read_text(errors="replace")
                files.append((path, mask_noncode(raw)))

    widths, aggregates = declarations(files)
    rows: set[Candidate] = set()
    for path, code in files:
        rel = str(path.relative_to(REPO))

        for match in PTR_FROM_ADDRESS.finditer(code):
            origin = match.group("origin")
            leaf = re.split(r"->|\.", origin)[-1]
            if leaf not in widths or leaf in aggregates:
                continue
            lo, hi = enclosing_block(code, match.start())
            scope = code[match.end():hi]
            ptr = match.group("ptr")
            indices = [parse_int(m.group(1)) for m in re.finditer(
                rf"\b{re.escape(ptr)}\s*\[\s*(0x[0-9a-fA-F]+|\d+)\s*\]", scope
            )]
            advanced = bool(re.search(
                rf"(?:\+\+\s*{re.escape(ptr)}\b|\b{re.escape(ptr)}\s*\+\+|"
                rf"\b{re.escape(ptr)}\s*\+=|\b{re.escape(ptr)}\s*\+\s*[1-9])",
                scope,
            ))
            qualifiers = ((match.group("qual1") or "") +
                          (match.group("qual2") or "")).strip()
            ptr_type = ((qualifiers + " ") if qualifiers else "") + match.group("type")
            detail = f"{ptr_type}* {ptr} <- &{origin}"
            if indices:
                detail += f"; literal index 0..{max(indices)}"
            if advanced:
                detail += "; pointer advanced"
            rows.add(Candidate(rel, line_of(code, match.start()), origin_kind(origin),
                               origin, "scalar-base", detail))

        for match in DIRECT_INDEX.finditer(code):
            origin = match.group("origin")
            leaf = re.split(r"->|\.", origin)[-1]
            index = parse_int(match.group("index"))
            if leaf in widths and leaf not in aggregates and index > 0:
                rows.add(Candidate(rel, line_of(code, match.start()), origin_kind(origin),
                                   origin, "scalar-base",
                                   f"direct &{origin}[{index}] view"))

        for pattern in (IO_TWO_ARG, IO_THREE_ARG):
            for match in pattern.finditer(code):
                origin = match.group("origin")
                leaf = re.split(r"->|\.", origin)[-1]
                if leaf not in widths or leaf in aggregates:
                    continue
                size = parse_int(match.group("size"))
                # A member spelling such as m_flags can occur in unrelated classes
                # with different widths.  Without a resolved receiver type, choosing
                # either width creates false evidence.  Scalar-base pointer rows do
                # not need the width, but wide-I/O rows do, so skip ambiguous names.
                if len(widths[leaf]) != 1:
                    continue
                declared = next(iter(widths[leaf]))
                if size <= declared:
                    continue
                detail = (f"{match.group('call')} starts at &{origin}: {size} bytes; "
                          f"scalar declaration is {declared} bytes")
                rows.add(Candidate(rel, line_of(code, match.start()), origin_kind(origin),
                                   origin, "wide-io", detail))

    return sorted(rows, key=lambda row: (row.path, row.line, row.tell, row.origin))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--summary", action="store_true", help="counts only")
    parser.add_argument("--kind", choices=("member", "global"), help="limit by owner kind")
    args = parser.parse_args()

    rows = scan()
    if args.kind:
        rows = [row for row in rows if row.kind == args.kind]
    counts = collections.Counter(row.tell for row in rows)
    kinds = collections.Counter(row.kind for row in rows)
    print("flattened aggregates: %d evidence sites | %d scalar-base | %d wide-io | "
          "%d member | %d global" %
          (len(rows), counts["scalar-base"], counts["wide-io"],
           kinds["member"], kinds["global"]))
    if not args.summary:
        for row in rows:
            print(f"{row.path}:{row.line}: {row.kind:6s} {row.tell:11s} {row.detail}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
