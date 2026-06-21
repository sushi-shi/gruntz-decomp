#!/usr/bin/env python3
"""gruntz.match.gen_unmatched - emit src/Stub/Unmatched.cpp, the survey TU of the
smallest REAL (non-compiler-generated) unmatched function bodies.

Why this exists: the engine_label_stubs backlog is anchored on the names Ghidra
could attach, which - in the low .text range - are the *incremental-linking jump
thunks* (5-byte `E9 rel32`), not the real bodies. The actual function bodies sit
at higher RVAs and Ghidra left them unnamed (`FUN_<va>`). So "unmatched" is really
~2.4k unnamed bodies, invisible to the named backlog. This tool enumerates them.

The universe (mirrors gruntz.match.status.engine_universe):
    functions.csv  (every in-.text function: entry_rva, byte_size, name)
  - config/library_labels.csv         (FID-identified CRT/MFC library code)
  - build/gen/symbol_names.csv        (already pulled into a unit = "started")
  - compiler/linker-generated machinery, detected by NAME and by reading the
    retail bytes:
        thunk_*                                  (Ghidra jump thunks)
        size-5  E9 rel32                         (incremental-linking thunks)
        size-6  FF 25 [__imp]                    (import thunks)
        Catch@ / Unwind@ / Cleanup@ / Filter@    (EH funclets)
        ??_G / ??_E (deleting dtors), ??_7/8/9 (vftables), ??_C (strings),
        ??_R (RTTI), label* (data labels)        (compiler data/thunks)
        size <= 2                                 (alignment / `ret` padding)
  - the 3 stray library/runtime leftovers FID missed (_abort, __NLG_Notify1, CRect)

What remains is emitted, smallest-first, as a uniform compilable stub:

    // FUN_0042ed90  size 0x5   xor eax, eax; ret 4
    RVA(0x02ed90, 0x5)
    SYMBOL(_sub_0042ed90)
    void sub_0042ed90(void) {}

`sub_<va>` mirrors Ghidra's `FUN_<va>` (grep across both); RVA() carries the RVA
(codebase convention) and the size, so the delinker carves the retail bytes and
objdiff lists each as a ~0% worklist item. The shape comment (first instructions
of the retail body) makes the file scannable: pick a tiny one, read its shape,
reconstruct it, then move the stub into its real class TU.

Regenerate after a build (functions.csv / symbol_names.csv refresh):
    python -m gruntz.match.gen_unmatched          # writes src/Stub/Unmatched.cpp
    python -m gruntz.match.gen_unmatched --check   # print counts, write nothing
"""
from __future__ import annotations

import argparse
import csv
import os
import re
import struct
import subprocess
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
FUNCS_CSV = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
FID_CSV = REPO / "config" / "library_labels.csv"
SYMS_CSV = REPO / "build" / "gen" / "symbol_names.csv"
OUT = REPO / "src" / "Stub" / "Unmatched.cpp"
IMAGE_BASE = 0x400000

FUNCLET = re.compile(r"^(Catch|Unwind|Cleanup|Filter|tryblocktable|ehfuncinfo)@", re.I)
DATA_GEN = ("??_7", "??_8", "??_9", "??_C", "??_R", "??_G", "??_E", "??_I", "??_F")
# library/runtime leftovers FID misses but that are not game code
STRAY_LIB = {"_abort", "__NLG_Notify1", "__NLG_Notify", "CRect"}


def rint(s: str) -> int:
    s = str(s).strip()
    return int(s, 16) if s.lower().startswith("0x") else int(s)


class PE:
    """Minimal PE section map + byte reader for an MZ/PE image."""

    def __init__(self, path: Path):
        self.data = path.read_bytes()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        nsec = struct.unpack_from("<H", self.data, pe + 6)[0]
        opt = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.secs = []
        for i in range(nsec):
            o = pe + 24 + opt + i * 40
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", self.data, o + 8)
            self.secs.append((vaddr, vsize, rawptr, rawsize))

    def off(self, rva: int):
        for va, vs, rp, rs in self.secs:
            if va <= rva < va + max(vs, rs):
                return rp + (rva - va)
        return None

    def bytes(self, rva: int, n: int) -> bytes:
        o = self.off(rva)
        return self.data[o:o + n] if o is not None else b""


def load_universe(pe: PE):
    funcs = {}
    with open(FUNCS_CSV) as f:
        for r in csv.DictReader(f):
            try:
                funcs[rint(r["entry_rva"])] = (int(r["byte_size"]), r.get("name", ""))
            except (ValueError, KeyError):
                continue
    lib = set()
    with open(FID_CSV) as f:
        for r in csv.DictReader(f):
            try:
                lib.add(rint(r["rva"]))
            except (ValueError, KeyError):
                pass
    # "handled" = an RVA already carried by symbol_names.csv under a name that is
    # NOT one of OUR OWN `Stub_<rva>` placeholders. This keeps the regen idempotent:
    # rerun after a build (where all 2406 placeholders ARE in symbol_names.csv) and
    # they reappear; once an agent matches one and gives it a real name (or moves it
    # into a class TU), it becomes "handled" and drops off the survey. Without this
    # carve-out the second run would exclude every placeholder and emit nothing.
    handled = set()
    if SYMS_CSV.is_file():
        for line in SYMS_CSV.read_text().splitlines():
            p = line.strip().split(",")
            try:
                rva, name = int(p[0], 16), p[1]
            except (ValueError, IndexError):
                continue
            if name != f"?Stub_{rva:06x}@@YAXXZ":   # not our placeholder -> handled
                handled.add(rva)
    return funcs, lib, handled


def classify(pe: PE, rva: int, sz: int, name: str) -> str:
    if name.startswith("thunk_"):
        return "thunk"
    b = pe.bytes(rva, 6)
    if sz == 5 and b[:1] == b"\xe9":
        return "ilt-thunk"
    if sz == 6 and b[:2] == b"\xff\x25":
        return "import-thunk"
    if FUNCLET.match(name):
        return "eh-funclet"
    if name.startswith(DATA_GEN) or name.startswith("label"):
        return "data"
    if sz <= 2:
        return "padding"
    if name in STRAY_LIB or not name.startswith("FUN_"):
        # only unnamed FUN_ bodies are real, un-attributed game code; a non-FUN_
        # name here is a library/runtime leftover (FID gap) - not our target.
        return "named-leftover"
    return "REAL"


def disasm_text(exe: str):
    """{va: 'mnemonic operands'} for the whole .text, intel syntax, one objdump call."""
    out = subprocess.run(
        ["llvm-objdump", "-d", "--x86-asm-syntax=intel", "--no-show-raw-insn",
         "--section=.text", exe],
        capture_output=True, text=True, check=True).stdout
    insns = {}
    line_re = re.compile(r"^\s*([0-9a-fA-F]+):\s*\t(.*)$")
    for line in out.splitlines():
        m = line_re.match(line)
        if not m:
            continue
        va = int(m.group(1), 16)
        txt = re.sub(r"\s+", " ", m.group(2).split("#")[0].strip())
        if txt:
            insns[va] = txt
    return insns


def shape(insns: dict, addrs: list, va: int, sz: int, budget: int = 64) -> str:
    """First instructions of [va, va+sz), joined; truncated to ~budget chars."""
    import bisect
    i = bisect.bisect_left(addrs, va)
    parts = []
    while i < len(addrs) and addrs[i] < va + sz:
        parts.append(insns[addrs[i]])
        i += 1
    s = "; ".join(parts)
    if len(s) > budget:
        s = s[:budget - 1].rstrip() + "…"
    return s or "(no disasm)"


def build_rows(pe: PE, insns: dict):
    funcs, lib, started = load_universe(pe)
    addrs = sorted(insns)
    rows, counts = [], {}
    for rva, (sz, name) in funcs.items():
        if rva in lib or rva in started:
            continue
        k = classify(pe, rva, sz, name)
        counts[k] = counts.get(k, 0) + 1
        if k != "REAL":
            continue
        va = IMAGE_BASE + rva
        rows.append((sz, rva, va, shape(insns, addrs, va, sz)))
    rows.sort(key=lambda r: (r[0], r[1]))   # smallest first, then by rva
    return rows, counts


HEADER = """\
#include <rva.h>
// Unmatched.cpp - the smallest REAL (non-compiler-generated) unmatched bodies.
//
// GENERATED by `python -m gruntz.match.gen_unmatched`. DO NOT hand-edit: when you
// match one, MOVE the stub out into its real class TU (then regenerate so it drops
// off this list). See the generator's docstring for the exact filter.
//
// Each entry is a real function body Ghidra left unnamed (`FUN_<va>`): the named
// backlog tracks the incremental-linking THUNKS, not these bodies, so these stay
// invisible to it. The `Stub_<rva>` placeholder follows the src/Stub/ @stub
// convention (RVA() carries the RVA + retail byte size; verify_stubs guards the
// addresses; labels.py -> delink -> objdiff lists each as a ~0% worklist item).
// The leading `// FUN_<va> ...` comment is Ghidra's name + the first instructions
// of the retail body, so the file is scannable: pick a tiny one, read its shape,
// reconstruct it, then move the stub into its real class TU.
//
// Sorted smallest-first - the trivial leaves (getters/no-ops/forwarders) are the
// quickest wins. Count below is regenerated each run.
"""


def emit(rows) -> str:
    lines = [HEADER, f"// {len(rows):,} unmatched bodies follow, smallest first.", ""]
    for sz, rva, va, shp in rows:
        lines.append(f"// FUN_{va:08x}  size 0x{sz:x}   {shp}")
        lines.append("// @confidence: low")
        lines.append("// @source: ghidra-fn")
        lines.append("// @stub")
        lines.append(f"RVA(0x{rva:06x}, 0x{sz:x})")
        lines.append(f"void Stub_{rva:06x}() {{}}")
        lines.append("")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=os.environ.get("GRUNTZ_EXE", ""))
    ap.add_argument("--check", action="store_true", help="print counts, write nothing")
    args = ap.parse_args()
    if not args.exe or not Path(args.exe).is_file():
        sys.exit("need the retail EXE: set $GRUNTZ_EXE (run inside `nix develop`)")
    if not FUNCS_CSV.is_file():
        sys.exit(f"missing {FUNCS_CSV} - run `gruntz build` first")
    pe = PE(Path(args.exe))
    insns = disasm_text(args.exe)
    rows, counts = build_rows(pe, insns)
    for k in sorted(counts, key=lambda k: -counts[k]):
        print(f"  {counts[k]:6d}  {k}", file=sys.stderr)
    print(f"  -> {len(rows)} REAL unmatched bodies", file=sys.stderr)
    if args.check:
        return
    OUT.write_text(emit(rows))
    print(f"wrote {OUT.relative_to(REPO)}", file=sys.stderr)


if __name__ == "__main__":
    main()
