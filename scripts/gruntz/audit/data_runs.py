#!/usr/bin/env python3
"""data_runs - consecutive-DATA runs, and which of them are PROVABLY one struct.

A run of adjacent DATA() definitions whose extents pack with no gap and share a
storage class *can* be one unmodelled struct rather than N globals. The NetCmdSlot
wire messages were exactly that (see below). But adjacency ALONE proves nothing:
MSVC lays out a TU's .data/.bss in declaration order, packed, so a gap-free run is
the DEFAULT for consecutively-declared globals. Two worked disproofs in-tree:
VideoConfig.cpp's 8 dialog HWNDs (each fetched by its own IDC via GetDlgItem) and
RezSync.cpp's 11 option flags (each set individually by the arg parser).

So this reports the runs, then applies the two signals that actually DISCRIMINATE:

  MISALIGNED   a multi-byte scalar member at an address its own element alignment
               forbids. MSVC 4-byte-aligns a standalone i32 global, so an i32 at an
               odd address is IMPOSSIBLE as a standalone - it can only be a member of
               a 1-byte-aligned (packed) record. Conclusive. This is what proved
               NetCmdSlot's gB_val @0x24a059 / gA_seq @0x24a8aa.
  WHOLE-RECORD a memcpy/memset whose destination is one member but whose length
               exceeds that member's own extent - i.e. the code treats the member's
               address as the base of a bigger object. Conclusive.

Corroborators (not proofs): the send/write length equalling the field extent
(SendOne's `m_payloadLen + 0xe` == the 1+4+4+4+1 header), a total extent that lands
exactly on the next global, and names that encode their own offsets (g_sfCfgA0/A2/B0).

Both conclusive sets are EMPTY as of the NetCmdSlot fold. A non-empty result here is
a real modelling defect; anything else needs retail-disassembly evidence (a common-base
indexed access), not a source grep.

    python -m gruntz.audit.data_runs           # runs + verdicts
    python -m gruntz.audit.data_runs --proven  # only the provable ones (gate-shaped)
"""
import argparse
import csv
import glob
import os
import re
import subprocess
import sys

from gruntz.audit.data_tu_order import parse_file, repo_root

# mangled scalar type letter -> element alignment MSVC gives a standalone global
_ALIGN = {"H": 4, "I": 4, "J": 4, "K": 4, "M": 4, "N": 8, "F": 2, "G": 2}
_COMDAT = ("??_", "$S", "??__")


def _symbols(root):
    """rva -> (size, unit, mangled) for every data claim with a proven extent."""
    out = {}
    p = os.path.join(root, "build/gen/symbol_names.csv")
    with open(p, newline="") as fh:
        for r in csv.DictReader(l for l in fh if not l.lstrip().startswith("#")):
            if (r.get("kind") or "") != "data":
                continue
            s = (r.get("size") or "").strip()
            out[int(r["rva"], 16)] = (int(s, 16) if s else None,
                                      (r.get("unit") or "").strip(), r["name"])
    return out


def _storage(root):
    out = {}
    p = os.path.join(root, "build/gen/delink_data_manifest.tsv")
    if not os.path.exists(p):
        return out
    with open(p, errors="replace") as fh:
        next(fh, None)
        for ln in fh:
            c = ln.rstrip("\n").split("\t")
            if len(c) >= 5:
                try:
                    out.setdefault(int(c[2], 16), c[4])
                except ValueError:
                    pass
    return out


def misaligned(syms):
    """Members no standalone global could occupy - conclusive struct evidence."""
    bad = []
    for rva, (size, unit, name) in sorted(syms.items()):
        if size is None or name.startswith(_COMDAT):
            continue
        m = re.match(r"^\?\w+@@3(.)", name)
        al = _ALIGN.get(m.group(1)) if m else None
        if al and rva % al:
            bad.append((rva, size, unit, name))
    return bad


def whole_record(root, syms):
    """memcpy/memset into ONE member with a length exceeding that member."""
    own = {}
    for _, (size, _, name) in syms.items():
        m = re.match(r"^\?(\w+)@@3", name) or re.match(r"^_(\w+)$", name)
        if m and size is not None:
            own[m.group(1)] = size
    out = subprocess.run(
        ["grep", "-rnE", r"(memcpy|memset|memmove|ZeroMemory|CopyMemory)\s*\(\s*&",
         os.path.join(root, "src"), os.path.join(root, "include")],
        capture_output=True, text=True).stdout
    hits = []
    for ln in out.splitlines():
        m = re.search(r"(memcpy|memset|memmove|ZeroMemory|CopyMemory)\s*\(\s*&\s*"
                      r"([A-Za-z_]\w*)\s*,\s*[^,]+,\s*([^)]+)\)", ln)
        if not m:
            continue
        var, size = m.group(2), m.group(3).strip()
        if not re.fullmatch(r"0x[0-9a-fA-F]+|\d+", size):
            continue
        v, o = int(size, 0), own.get(var)
        if o is not None and v > o:
            hits.append((var, o, v, ln.strip()))
    return hits


def runs(root, syms, stor):
    """Gap-free, same-storage runs of >=2 ordinary data definitions, per file."""
    found = []
    files = sorted(glob.glob(os.path.join(root, "src", "**", "*.cpp"), recursive=True))
    for path in files:
        if "/Stub/" in path:
            continue
        defs = []
        for b in parse_file(path):
            if not b.is_def or b.name is None:
                continue
            size, _, mangled = syms.get(b.rva, (None, "", ""))
            if mangled.startswith(_COMDAT):
                continue  # COMDAT data is linker-pooled, not linearly laid out
            defs.append((b.rva, b.name, size, stor.get(b.rva), b.line))
        defs.sort()
        cur = []
        for d in defs:
            if cur:
                prva, _, psize, pstor, _ = cur[-1]
                gapless = psize is not None and prva + psize == d[0]
                if not (gapless and pstor is not None and pstor == d[3]):
                    if len(cur) >= 2:
                        found.append((path, list(cur)))
                    cur = []
            cur.append(d)
        if len(cur) >= 2:
            found.append((path, list(cur)))
    found.sort(key=lambda r: (-len(r[1]), r[0]))
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--proven", action="store_true",
                    help="only the conclusive signals (exit 1 if any)")
    args = ap.parse_args()
    root = repo_root()
    syms, stor = _symbols(root), _storage(root)

    mis = misaligned(syms)
    whole = whole_record(root, syms)

    print(f"[data-runs] MISALIGNED members: {len(mis)}   "
          f"WHOLE-RECORD ops: {len(whole)}  (both 0 = no provable struct-fold left)")
    for rva, size, unit, name in mis:
        print(f"   MISALIGNED  {rva:#010x} {size:#x} {unit:<16} {name}")
    for var, o, v, ln in whole:
        print(f"   WHOLE-RECORD {var} (member {o:#x}, op {v:#x})\n      {ln[:140]}")
    if args.proven:
        return 1 if (mis or whole) else 0

    rs = runs(root, syms, stor)
    total = sum(len(r[1]) for r in rs)
    print(f"\n[data-runs] {len(rs)} gap-free same-storage run(s), {total} definition(s). "
          f"Adjacency alone is NOT evidence - see the module docstring.")
    for path, run in rs:
        span = run[-1][0] + (run[-1][2] or 0) - run[0][0]
        print(f"\n{os.path.relpath(path, root)}  [{len(run)} defs, "
              f"{run[0][0]:#08x}..{run[-1][0]:#08x}, {span:#x} B, .{run[0][3]}]")
        for rva, name, size, _s, line in run:
            sz = "?" if size is None else hex(size)
            print(f"    {rva:#010x}  +{rva - run[0][0]:<5x} {sz:<6} L{line:<6} {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
