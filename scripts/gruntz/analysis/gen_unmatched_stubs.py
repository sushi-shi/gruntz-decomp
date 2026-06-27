#!/usr/bin/env python3
"""gruntz.analysis.gen_unmatched_stubs - stub every real reconstruction target that
has no stub yet, so it enters the objdiff worklist instead of sitting in the
`(unmatched)` row.

Sibling of gen_boundary_stubs / gen_attributed_stubs / gen_class_stubs - those each
carve a SPECIFIC slice (boundary FUN_ bodies, proximity-attributed members, trace
ties). This is the catch-all: whatever is left of the engine that is a genuine
reconstruction target - i.e. NOT already claimed in src/, and NOT one of the carve-
out categories the match-% excludes (see gruntz.match.status.engine_universe):
  - the leading ILT jmp-thunk table (<=5 B `jmp rel32`, RVA below the first real fn),
  - FID-identified CRT/MFC library code,
  - compiler /GX EH unwind funclets (Unwind@*),
  - byte-pattern linker/compiler glue Ghidra names `FUN_` (IAT import thunks
    `jmp ds:[__imp_*]`, EH catch-return trampolines) - see docs/unmatched-survey.md.
Trivially-small leaves (<= --min-size, default 5 B: 1-byte vtable padding, tiny
dispatch) are skipped too - nothing to reconstruct.

Emits src/Stub/Unmatched.cpp: `RVA(rva,size) void Unmatched_<rva>() {}` free-function
stubs (the `engine_unmatched` unit), each trailed by `// <ghidra name>` so a matcher
sees the recovered name/class hint (`~CTileTrigger`, `ClassUnknown_93_00d170`, ...).
No header (free functions need no class decl); a matcher reconstructs each and moves
it to its real class TU, shrinking this file toward empty.

Run: python -m gruntz.analysis.gen_unmatched_stubs [--min-size 5]
"""
import argparse
import csv
import os
import re
import struct
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
FUNCS = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
FID = REPO / "config" / "library_labels.csv"
SRC = REPO / "src" / "Stub" / "Unmatched.cpp"
LIB_SRC = REPO / "src" / "Stub" / "UnmatchedLibrary.cpp"
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build" / "exe" / "GRUNTZ.EXE")


def curated_lib_rvas():
    """RVAs hand-curated as CRT/MFC/zlib library implementations (NOT reconstruction
    targets) - read back from the existing UnmatchedLibrary.cpp so a regen PRESERVES
    the split instead of re-deriving it heuristically. See docs/HANDOFF-unmatched.md."""
    lib = set()
    if LIB_SRC.exists():
        for m in re.finditer(r"\bRVA\(\s*(0x[0-9a-fA-F]+)", LIB_SRC.read_text(errors="ignore")):
            lib.add(int(m.group(1), 16))
    return lib

MACRO = re.compile(r"\b(?:RVA|RVAU|DATA|SYMBOL)\s*\(\s*(0x[0-9a-fA-F]+)"
                   r"|@rva-symbol:\s*\S+\s+(0x[0-9a-fA-F]+)")


def glue_rvas(rvas):
    """RVAs whose body is pure linker/compiler glue, not a reconstruction target:
    the linker's IAT import thunk (`jmp ds:[__imp_*]`, ff 25) and the compiler's
    EH catch-return trampoline (pop eax;pop ecx;xchg [esp],eax;jmp eax). Ghidra
    names both `FUN_`, so they survive the thunk_/Unwind@ filters and sit at high
    scattered RVAs (not the leading ILT run) - this is the byte-pattern backstop.
    See docs/unmatched-survey.md. Best-effort: no EXE -> empty set."""
    if not EXE.exists():
        return set()
    d = EXE.read_bytes()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    optsz = struct.unpack_from("<H", d, pe + 20)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    secs = [struct.unpack_from("<IIII", d, pe + 24 + optsz + i * 40 + 8) for i in range(nsec)]

    def at(rva, n):
        for vsz, va, rsz, rp in secs:
            if va <= rva < va + max(vsz, rsz):
                o = rva - va + rp
                return d[o:o + n]
        return b""

    glue = set()
    for rva in rvas:
        b = at(rva, 7)
        if b[:2] == b"\xff\x25" or b == b"\x58\x59\x87\x04\x24\xff\xe0":
            glue.add(rva)
    return glue


def rint(s):
    s = str(s).strip()
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def load_labeled():
    """Every RVA already claimed anywhere in src/ (except our own output) + the FID
    library list, so labels.py's duplicate-RVA guard never fires and we never re-stub
    a function another unit already owns."""
    lab = set()
    for f in (REPO / "src").rglob("*.cpp"):
        if f.resolve() == SRC.resolve():
            continue
        try:
            for a, b in MACRO.findall(f.read_text(errors="ignore")):
                lab.add(int(a or b, 16))
        except OSError:
            pass
    for c in ("library_labels.csv", "zlib_labels.csv"):
        p = REPO / "config" / c
        if p.exists():
            for r in csv.reader(open(p)):
                try:
                    lab.add(int(r[0], 16))
                except (ValueError, IndexError):
                    pass
    return lab


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--min-size", type=lambda s: int(s, 0), default=5,
                    help="skip functions of <= this many bytes (default 5)")
    args = ap.parse_args()

    rows = []  # (rva, size, name)
    with open(FUNCS) as f:
        for r in csv.DictReader(f):
            try:
                rows.append((rint(r["entry_rva"]), int(r["byte_size"]), r.get("name", "")))
            except (ValueError, KeyError):
                continue
    fid = set()
    with open(FID) as f:
        for r in csv.DictReader(f):
            try:
                fid.add(rint(r["rva"]))
            except (ValueError, KeyError):
                pass
    # The leading contiguous <=5 B run is the linker ILT jmp-table; real code starts
    # at the first >5 B function (same bound as engine_universe).
    ilt_end = min((rva for rva, sz, _n in rows if sz > 5), default=0)
    labeled = load_labeled()
    glue = glue_rvas([rva for rva, sz, _n in rows if sz > args.min_size and rva >= ilt_end])

    keep = [(rva, sz, name) for rva, sz, name in rows
            if sz > args.min_size
            and rva >= ilt_end
            and not name.startswith("thunk_")
            and not name.startswith("Unwind@")
            and rva not in fid
            and rva not in labeled
            and rva not in glue]
    keep.sort()

    # Split the worklist (Unmatched.cpp) from the curated CRT/MFC/zlib library set
    # (UnmatchedLibrary.cpp) so agents don't waste effort on non-targets.
    lib_set = curated_lib_rvas()
    handle = [e for e in keep if e[0] not in lib_set]
    lib = [e for e in keep if e[0] in lib_set]

    def emit(path, header, entries):
        sl = header + ["#include <rva.h>", ""]
        for rva, sz, name in entries:
            sl.append("RVA(0x%08x, 0x%x) void Unmatched_%06x() {}  // %s"
                      % (rva, sz, rva, name or "?"))
        sl.append("")
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("\n".join(sl))

    emit(SRC, [
        "// AUTO-GENERATED by gruntz.analysis.gen_unmatched_stubs - do not hand-edit.",
        "// engine_unmatched WORKLIST: real game/engine reconstruction targets that",
        "// still need matching (not claimed in any other src/ unit, not a carve-out:",
        "// ILT jmp-thunk / FID library / EH unwind funclet / byte-pattern glue). The",
        "// CRT/MFC/zlib library implementations are split into UnmatchedLibrary.cpp.",
        "// A matcher reconstructs each entry into its real class TU - see",
        "// docs/HANDOFF-unmatched.md - shrinking this file toward empty.",
    ], handle)
    emit(LIB_SRC, [
        "// AUTO-GENERATED by gruntz.analysis.gen_unmatched_stubs - do not hand-edit.",
        "// engine_unmatched_lib: CRT/MFC/zlib/compiler IMPLEMENTATIONS, split out of",
        "// the worklist (Unmatched.cpp). NOT reconstruction targets - matched by USING",
        "// the statically-linked library / FID-tagging, never hand-written. The RVA set",
        "// is curated (read back from this file across regens); see docs/HANDOFF-unmatched.md.",
    ], lib)
    print("wrote %s (%d worklist) + %s (%d library); min-size>%#x; ilt_end=%#x; "
          "skipped %d already-labeled, %d byte-pattern glue"
          % (SRC, len(handle), LIB_SRC, len(lib), args.min_size, ilt_end, len(labeled), len(glue)))


if __name__ == "__main__":
    main()
