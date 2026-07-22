#!/usr/bin/env python3
"""label_style.py - label-macro representation ratchet.

Every label the pipeline reads is a MACRO with one canonical spelling
(CLAUDE.md conventions): addresses are zero-padded to 8 lowercase hex digits,
size/byte args are UNPADDED lowercase hex (0x0 = unknown), and an invocation
sits on ONE line (every label consumer is a single-line text scan - a
clang-format-wrapped invocation silently vanishes from symbol_names.csv).
Checked forms:

  RVA(0x00xxxxxx, 0xN)             DATA(0x00xxxxxx)
  VTBL(CClass, 0x00xxxxxx)         VTBL_ABSENT(CClass)
  RVA_COMPGEN(0x00xxxxxx, 0xN, <mangled>)
  DATA_SYMBOL(0x00xxxxxx, 0xN, <mangled>)
  SIZE(0xN)                        SIZE_UNKNOWN()
  SYMBOL(<mangled>)

Comments are blanked first (labels.py does the same), so prose may quote an
invocation freely. The retired comment-form carriers (`// @rva-symbol:` /
`// @data-symbol:`) are checked against RAW text - a resurrected comment row
would be silently ignored by the macro-only label pass, so it is FATAL here.

Comment @markers are a CLOSED vocabulary (docs/comment-markers.md): a comment
line may only LEAD with one of the blessed markers below - ad-hoc `// @name:`
fields rot into pseudo-conventions no tool reads (the 2026-07-22 sweep retired
@orphan/@flag/@fold-TODO/... into plain prose). Mid-line mentions are prose and
stay free.

  python -m gruntz.audit.label_style          # report violations
  python -m gruntz.audit.label_style --gate   # exit 1 on any violation (build tail)
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from gruntz.build.labels import blank_comments  # noqa: E402

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

ADDR = r"0x[0-9a-f]{8}"                 # zero-padded 8-digit lowercase RVA
HEXN = r"(?:0x0|0x[1-9a-f][0-9a-f]*)"   # unpadded lowercase hex (0x0 = unknown)
NAME = r"[A-Za-z_]\w*"                  # a plain class name
MANGLED = r"[^\s,()]+"                  # a verbatim mangled symbol

CANON = {
    "RVA": rf"RVA\({ADDR}, {HEXN}\)",
    "DATA": rf"DATA\({ADDR}\)",
    "VTBL": rf"VTBL\({NAME}, {ADDR}\)",
    "VTBL_ABSENT": rf"VTBL_ABSENT\({NAME}\)",
    "RVA_COMPGEN": rf"RVA_COMPGEN\({ADDR}, {HEXN}, {MANGLED}\)",
    "DATA_SYMBOL": rf"DATA_SYMBOL\({ADDR}, {HEXN}, {MANGLED}\)",
    "SIZE": rf"SIZE\({HEXN}\)",
    "SIZE_UNKNOWN": r"SIZE_UNKNOWN\(\)",
    "SYMBOL": r"SYMBOL\([^\s()]+\)",
}
CANON_RE = {k: re.compile(v) for k, v in CANON.items()}
# StatementMacros-formatted labels: clang-format ARG-WRAPS these past ColumnLimit
# (the WhitespaceSensitiveMacros carriers SYMBOL/RVA_COMPGEN/DATA_SYMBOL are
# wrap-immune - a giant mangled local-static name may legitimately run long).
WRAPPABLE = {"RVA", "DATA", "VTBL", "VTBL_ABSENT", "SIZE", "SIZE_UNKNOWN"}
# longest-first so RVA_COMPGEN/DATA_SYMBOL/SIZE_UNKNOWN/VTBL_ABSENT win their prefixes
FIND_RE = re.compile(
    r"\b(RVA_COMPGEN|DATA_SYMBOL|SIZE_UNKNOWN|VTBL_ABSENT|SIZE|RVA|DATA|VTBL|SYMBOL)\s*\(")
COMMENT_ROW_RE = re.compile(r"@(?:rva|data)-symbol:\s*\S+\s+0x[0-9a-fA-F]+")
# The blessed comment-marker vocabulary (docs/comment-markers.md). @stub blocks
# carry @confidence:/@source: tags (verify_stubs.py REQUIRES them); @early-stop /
# @identity-TODO are the CLAUDE.md function-state markers (stale_walls.py reads
# them); @interleaver records a linker-pooled out-of-line member's placement.
ALLOWED_MARKERS = {"stub", "early-stop", "identity-TODO", "confidence", "source",
                   "interleaver", "dead-code"}
MARKER_RE = re.compile(r"^\s*// ?@([A-Za-z][A-Za-z0-9_-]*)")


def scan(path: Path):
    raw = path.read_text(errors="replace")
    out = []
    for i, ln in enumerate(raw.splitlines(), 1):
        if COMMENT_ROW_RE.search(ln):
            out.append((i, "retired comment-form label row (use RVA_COMPGEN/DATA_SYMBOL)",
                        ln.strip()[:90]))
        m = MARKER_RE.match(ln)
        if m and m.group(1) not in ALLOWED_MARKERS:
            out.append((i, f"@{m.group(1)} is not a blessed comment marker "
                           "(docs/comment-markers.md) - write plain prose",
                        ln.strip()[:90]))
    raw_lines = raw.splitlines()
    for i, ln in enumerate(blank_comments(raw).splitlines(), 1):
        for m in FIND_RE.finditer(ln):
            name = m.group(1)
            hit = CANON_RE[name].match(ln, m.start())
            if hit is None:
                out.append((i, f"{name}(..) off-canon (want {CANON[name]}; "
                               "one line, 8-digit addr, unpadded hex size)",
                            ln.strip()[:90]))
            elif name in WRAPPABLE and len(raw_lines[i - 1]) > 100:
                # over ColumnLimit -> the next clang-format pass arg-wraps the
                # StatementMacros invocations, and every label consumer is a
                # single-line scan. Shorten/move the trailing comment.
                out.append((i, f"{name}(..) line exceeds 100 columns "
                               "(clang-format will wrap the invocation)",
                            ln.strip()[:90]))
    return out


def violations():
    out = []
    files = sorted(list((REPO / "src").rglob("*.cpp")) + list((REPO / "src").rglob("*.h"))
                   + list((REPO / "include").rglob("*.h")))
    for path in files:
        if path.name == "rva.h":
            continue  # the definitions themselves
        for line, why, text in scan(path):
            out.append(f"{path.relative_to(REPO)}:{line}: {why}\n    {text}")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any violation (build-tail ratchet)")
    args = ap.parse_args()
    viol = violations()
    for v in viol:
        print(v)
    if viol:
        print(f"label-style: {len(viol)} off-canon label invocation(s)")
        return 1 if args.gate else 0
    print("label-style: OK - every label macro is canonical "
          "(8-digit addr, unpadded hex size, single line)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
