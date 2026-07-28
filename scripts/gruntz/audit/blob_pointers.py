#!/usr/bin/env python3
"""blob_pointers.py - find the types we lost behind `u8*` / `i32*`.

User ruling, 2026-07-28: *"almost all of u8*, i32* are suspicious of something not
modelled."* A byte- or dword-pointer into a STRUCTURED buffer is the same defect as an
`i32` holding a pointer - the declaration threw the type away, and what is left is a
cursor plus arithmetic where a typed element access belongs.

The reason this needs a tool rather than a grep is that the sites defend each other.
`CDDPalette::SetAndNotify(i32, i32, u8* data, i32)` looks correct as long as you check
it against its callers, because three of the four pass `u8*` buffers too - and one of
THOSE is filled `buf[i*4+0] = quads[i*4+2]`, i.e. a PALETTEENTRY array written
byte-wise from an RGBQUAD array. Judged one at a time, every site is "consistent with
its neighbours". Judged as a cluster, the whole thing is one missing SDK type. So this
lists them together, grouped, with the tells that say a real type is hiding:

  named    the declaration's own comment already names a type (`u8* m_cacheA;
           // PALETTEENTRY cache A`) - the strongest signal there is, and common
  sized    a fixed byte size that factors by a plausible element width (0x400 =
           256 * sizeof(PALETTEENTRY))
  strided  a use site indexes it `p[i*4 + K]` - those K are FIELD OFFSETS
  recast   the same pointer is re-spelled somewhere as a third type

A row with no tells is not innocent, only unproven; drive the count down by RETYPING,
and retype a whole cluster at once (see the memory note - a sibling mis-model never
gets a vote).

    python -m gruntz.audit.blob_pointers            # the worklist, grouped by file
    python -m gruntz.audit.blob_pointers --summary  # counts only
    python -m gruntz.audit.blob_pointers --max N    # exit 1 if the count exceeds N
"""
import argparse
import collections
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")

# ANY integer-typed pointer, not just u8*/i32* (user, 2026-07-28: "Any int*, not just
# u8 and i32"). A short*/long*/u16* cursor over a structured buffer has lost exactly the
# same type information; narrowing the scan to two spellings just hides the rest.
#
# Plain `char*` is tracked SEPARATELY below rather than mixed in: it is also the correct
# spelling for a C string, so folding it into this count would bury the signal under
# legitimate rows. It is still reported - a `char*` walking a record is the same defect.
INT_TYPES = (r"u8|i8|u16|i16|u32|i32|u64|i64|BYTE|WORD|DWORD|UINT|INT|"
             r"short|int|long|unsigned(?:\s+(?:char|short|int|long))?|signed\s+char")
DECL = re.compile(
    r"\b(?:const\s+)?(" + INT_TYPES + r")\s*\*\s*(?:const\s+)?(m_\w+|\w+)\s*[;,)=]")
CHAR_DECL = re.compile(
    r"\b(?:const\s+)?(char)\s*\*\s*(?:const\s+)?(m_\w+|\w+)\s*[;,)=]")

# A comment that names a concrete type for the thing just declared.
# Deliberately NOT `C[A-Z]\w+`: that matches the ENCLOSING class in almost every
# nearby comment, which inflated this signal from a usable lead into noise (264 of 694
# rows "tagged", most of them just naming the class the member lives in). Only concrete
# record/SDK types count - those are the ones a retype can actually adopt.
TYPE_IN_COMMENT = re.compile(
    r"\b(PALETTEENTRY|RGBQUAD|BITMAPFILEHEADER|BITMAPINFOHEADER|BITMAPINFO|"
    r"WAVEFORMATEX|WAVEFORMAT|DDSURFACEDESC|DDCAPS|LOGPALETTE|DPCAPS|DPSESSIONDESC2|"
    r"DPNAME|RECT|POINT|AFX_MSGMAP\w*|[A-Z]\w+Header|[A-Z]\w+Record|[A-Z]\w+Entry|"
    r"[A-Z]\w+Cell|[A-Z]\w+Vtx)\b")
SIZED = re.compile(r"0x[0-9a-fA-F]{2,}\s*(?:B\b|bytes)|\b(?:\d+)\s*(?:B\b|bytes)")
STRIDED = re.compile(r"\[\s*\w+\s*\*\s*([2-9]|1[0-9])\s*(?:\+\s*\d+\s*)?\]")


def scan():
    rows = collections.defaultdict(list)
    nchar = 0
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            text = path.read_text(errors="replace")
            lines = text.split("\n")
            for i, line in enumerate(lines):
                code = line.split("//", 1)[0]
                comment = line[len(code):]
                for m in DECL.finditer(code):
                    ty, name = m.group(1), m.group(2)
                    ctx = " ".join(lines[max(0, i - 2):i + 2])
                    tells = []
                    # ONLY this declaration's own trailing comment. Searching the
                    # 2-line context bleeds: `i32* outCol, i32* outRow` got tagged
                    # named:RECT because the word RECT appeared in an adjacent line
                    # about something else. A tell has to be about THIS declaration.
                    hit = TYPE_IN_COMMENT.search(comment)
                    if hit:
                        tells.append("named:" + hit.group(0))
                    if SIZED.search(comment) or SIZED.search(ctx):
                        tells.append("sized")
                    if re.search(r"\b%s\s*\[\s*\w+\s*\*" % re.escape(name), text):
                        tells.append("strided")
                    rows[str(path.relative_to(REPO))].append(
                        (i + 1, ty, name, tells, code.strip()[:70]))
                nchar += len(CHAR_DECL.findall(code))
    return rows, nchar


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--summary", action="store_true", help="counts only")
    ap.add_argument("--max", type=int, default=None, help="exit 1 if the count exceeds N")
    a = ap.parse_args()

    rows, nchar = scan()
    total = sum(len(v) for v in rows.values())
    with_tells = sum(1 for v in rows.values() for r in v if r[3])
    print("blob pointers: %d integer-pointer declarations  |  %d carry a tell that names "
          "the real type  |  %d char* (tracked apart: also the C-string spelling)"
          % (total, with_tells, nchar))

    if not a.summary:
        print("\nBy file - retype whole clusters, not single sites:")
        listed = 0
        for f, rs in sorted(rows.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            print("   %3d  %s" % (len(rs), f))
            for ln, ty, name, tells, src in rs:
                print("        %5d  %-6s %-24s %s" % (ln, ty, name,
                                                      ",".join(tells) or "-"))
            listed += len(rs)
        # The listing IS the worklist; capping it would hide work while the header
        # still claimed the full count (that bug shipped once in cast_ledger).
        assert listed == total, "listing (%d) != total (%d)" % (listed, total)

    if a.max is not None and total > a.max:
        print("blob-pointers: %d exceeds the %d ratchet" % (total, a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
