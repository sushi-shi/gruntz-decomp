#!/usr/bin/env python3
"""jump_tables.py - sieve the in-`.text` switch DISPATCH shape and the RVA extent.

cl 5.0 lowers a `switch` three ways and keeps whatever tables it needs in `.text`,
INSIDE the function's own COMDAT: a `cmp/je` ladder (no table), a dense
`jmp [T+eax*4]` (one DIR32 table), or - for a sparse-but-clustered case set - a
two-level `movzx ecx,[T1+eax]; jmp [T2+ecx*4]` (an un-relocated byte index LUT plus
a DIR32 table).  Those table bytes are scored by objdiff, so the `RVA()` size arg
has to end exactly where the function ends: one byte short and the table is
truncated, one gap too far and the retail linker's inter-function `0xCC` filler is
compared against nothing.  Both are invisible in `sema disasm --diff`, and both read
as a codegen disaster on a function whose code is fine.

Four defect classes, in the order they must be fixed (an extent bug MASKS a table
bug, so the table classes are only reported once the extent is clean):

  * **EXTENT-FILLER** - `[rva, rva+size)` ends inside the linker's `0xCC` gap.
    `CGrunt::LoadGruntTypeTable` 0x4dd50 carried 1168 filler bytes - 1152 `int3`
    instructions against a 3707-instruction target - and went 59.76% -> 87.11% on
    the size arg alone.  `CMapMgr::ComputeCellFlags` 46.04 -> 76.40,
    `CGrunt::WanderStep` 66.71 -> 83.44, `CInGameIcon::CInGameIcon` 79.11 -> 92.23.

  * **EXTENT-SHORT** - the jump table sits immediately PAST `rva+size`: the dwords
    there are base-relocated and point back into the function.  This is
    `rva-extent-must-include-switch-tables.md` made mechanical.
    `CDroppedObject::AdvanceFall` 96.23 -> 100.00 EXACT,
    `CTriggerMgr::LoadTileArrivalFx` 61.67 -> 75.97,
    `CGrunt::LoadPickupSprites` 54.73 -> 68.86, `CPlay::ExecCommand` 60.46 -> 67.35.

  * **TABLE-ONLY-BASE / TABLE-ONLY-TARGET** - one side chose an indexed jump and the
    other a comparison ladder.  That is a SOURCE fact (case density, the selector's
    signedness, whether a `default` exists, whether the arms are empty) - see
    `switch-density-byte-index-table-vs-tree.md`,
    `empty-switch-arms-fold-into-default-and-kill-the-jump-table.md`,
    `switch-key-unsigned-ja-vs-jg.md`.

  * **TABLE-ENTRIES** - both sides built a table but of different width, i.e. the
    case RANGE differs: an extra/missing arm, or an arm that folded into `default`.

Detection is reloc-shaped, never heuristic.  A jump table is a run of >= 3 DIR32
relocations at a 4-byte stride; no three instructions in a row can produce that (an
instruction carrying a DIR32 is >= 5 bytes).  The same rule reads the base obj, the
delinked target obj and - through `.reloc` - the retail image itself.

    python -m gruntz.audit.jump_tables                 # every defect
    python -m gruntz.audit.jump_tables --calibrate     # false-positive rate on 100% fns
    python -m gruntz.audit.jump_tables --kind extent   # one class
    python -m gruntz.audit.jump_tables --unit grunt
    python -m gruntz.audit.jump_tables --tsv out.tsv
"""
import argparse
import bisect
import csv
import json
import struct
import sys
from pathlib import Path

from gruntz.core.branches import is_local_label, obj_paths

REPO = Path(__file__).resolve().parents[3]
REPORT = REPO / "build" / "objdiff" / "report.json"
SYMBOLS = REPO / "build" / "gen" / "symbol_names.csv"
FUNCTIONS = REPO / "config" / "retail" / "functions.tsv"

# A `0xCC` run this long is never alignment padding: cl pads a `.text` COMDAT out to
# 16 with `0x90`, and it is the retail LINKER that fills the gap BETWEEN functions
# with `0xCC`.  Calibration (below) puts the false-positive rate at 0/3454 even at 4.
FILLER_MIN = 4
# cl aligns the next COMDAT to 16, so at most 15 `0x90` bytes can separate a
# function's last instruction from its own switch table.
PAD_MAX = 15


def _warn_wrong_tree():
    """Shout when this package was imported from a DIFFERENT worktree (insn_count's rule)."""
    import os
    d = os.environ.get("GRUNTZ_DIR")
    if d and Path(d).resolve() != REPO:
        print(f"WARNING: reading {REPO} but GRUNTZ_DIR={d} - this package was "
              f"imported from another worktree. Run "
              f"`export PYTHONPATH=$GRUNTZ_DIR/scripts` and retry.", file=sys.stderr)


class Obj:
    """One COFF object: section bytes, per-section DIR32 relocs, symbol extents."""

    def __init__(self, path: Path):
        b = path.read_bytes()
        self.buf = b
        nsec = struct.unpack_from("<H", b, 2)[0]
        symptr = struct.unpack_from("<I", b, 8)[0]
        nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        strtab = symptr + nsym * 18
        self.sec = []
        for i in range(nsec):
            o = 20 + opt + i * 40
            name = b[o:o + 8].split(b"\0")[0].decode("latin1")
            if name.startswith("/"):
                off = int(name[1:])
                name = b[strtab + off:b.index(b"\0", strtab + off)].decode("latin1")
            vsz, _va, rsz, rp, relptr, _ln, nrel, _nl = struct.unpack_from("<IIIIIIHH", b, o + 8)
            chars = struct.unpack_from("<I", b, o + 36)[0]
            rel = sorted(va for k in range(nrel)
                         for va, _si, ty in [struct.unpack_from("<IIH", b, relptr + k * 10)]
                         if ty == 6)                       # IMAGE_REL_I386_DIR32
            self.sec.append({"name": name, "size": rsz or vsz, "raw": rp,
                             "code": bool(chars & 0x20), "dir32": rel})

        def sym_name(idx):
            base = symptr + idx * 18
            if struct.unpack_from("<I", b, base)[0] == 0:
                off = struct.unpack_from("<I", b, base + 4)[0]
                return b[strtab + off:b.index(b"\0", strtab + off)].decode("latin1")
            return b[base:base + 8].split(b"\0")[0].decode("latin1")

        self.syms, i = {}, 0
        while i < nsym:
            base = symptr + i * 18
            value, secnum, _typ, scl, naux = struct.unpack_from("<IhHBB", b, base + 8)
            name = sym_name(i)
            if secnum > 0 and scl in (2, 3) and not name.startswith("."):
                self.syms.setdefault(name, (secnum, value))
            i += 1 + naux

    def extent(self, name):
        """(secnum, start, end) of one FUNCTION - its own `$L` arm labels do not end it."""
        if name not in self.syms:
            return None
        sn, off = self.syms[name]
        if sn > len(self.sec) or not self.sec[sn - 1]["code"]:
            return None
        after = sorted(v for n, (s, v) in self.syms.items()
                       if s == sn and v > off and not is_local_label(n))
        return sn, off, (after[0] if after else self.sec[sn - 1]["size"])

    def table(self, sn, start, end):
        """(offset, entries) of the widest DIR32 jump table in [start,end), or None."""
        rel = [r for r in self.sec[sn - 1]["dir32"] if start <= r < end]
        best, i = None, 0
        while i < len(rel):
            j = i
            while j + 1 < len(rel) and rel[j + 1] - rel[j] == 4:
                j += 1
            n = j - i + 1
            if n >= 3 and (best is None or n > best[1]):
                best = (rel[i], n)
            i = j + 1
        return best


class Image:
    """The retail image, as the extent oracle: `.text` bytes, base relocs, fn starts."""

    def __init__(self):
        from gruntz.core import pe
        self.pe = pe.PE()
        self.reloc = self.pe.reloc_sites
        self.starts = []
        if FUNCTIONS.is_file():
            for ln in FUNCTIONS.read_text().splitlines():
                if ln.startswith("#"):
                    continue
                p = ln.split("\t")
                try:
                    self.starts.append(int(p[0], 16))
                except (ValueError, IndexError):
                    pass
            self.starts.sort()

    def byte(self, rva):
        return self.pe.data[self.pe.off(rva)]

    def dword(self, rva):
        return struct.unpack_from("<I", self.pe.data, self.pe.off(rva))[0]

    def next_start(self, rva):
        k = bisect.bisect_right(self.starts, rva)
        return self.starts[k] if k < len(self.starts) else rva + 0x20000

    def trailing_cc(self, rva, size):
        """How many `0xCC` bytes the declared extent ends in."""
        n = 0
        while n < size and self.byte(rva + size - 1 - n) == 0xCC:
            n += 1
        return n

    def own_table_after(self, rva, size):
        """Entry count of a jump table that starts (after <=15 `0x90` pad) past the extent.

        A table entry is base-relocated AND points back inside the function, which no
        following function's code can fake: a stray DIR32 in the next function points at
        a global or at itself, not into our body."""
        end = rva + size
        p = end
        while p < end + PAD_MAX and self.byte(p) == 0x90:
            p += 1
        n = 0
        while p + 4 * n in self.reloc:
            v = self.dword(p + 4 * n) - 0x400000
            if not (rva <= v < end):
                break
            n += 1
        return (p, n) if n >= 3 else None

    def true_end(self, rva):
        """Where the function really ends: the first `0xCC` run of >=8 before the next fn."""
        limit = self.next_start(rva)
        i = rva
        while i < limit:
            if self.byte(i) == 0xCC:
                j = i
                while j < limit and self.byte(j) == 0xCC:
                    j += 1
                if j - i >= 8:
                    return i
                i = j
            else:
                i += 1
        return limit


def claims(path=SYMBOLS):
    """{name: (rva, size)} for every FUNCTION the build pinned."""
    out = {}
    if not path.is_file():
        return out
    with path.open() as fh:
        for r in csv.DictReader(fh):
            if r.get("kind") != "func":
                continue
            try:
                out[r["name"]] = (int(r["rva"], 16), int(r["size"], 16))
            except (ValueError, KeyError):
                pass
    return out


def scan(names_by_unit, pins, image):
    hits = []
    for unit in sorted(names_by_unit):
        b_path, t_path = obj_paths(unit)
        if not b_path.is_file() or not t_path.is_file():
            continue
        try:
            base, tgt = Obj(b_path), Obj(t_path)
        except Exception as exc:                                   # pragma: no cover
            print(f"  ! {unit}: {exc}", file=sys.stderr)
            continue
        for name, fuzzy in names_by_unit[unit]:
            pin = pins.get(name)
            extent_bad = False
            if pin and image:
                rva, size = pin
                cc = image.trailing_cc(rva, size)
                if cc >= FILLER_MIN:
                    good = image.true_end(rva) - rva
                    extent_bad = True
                    hits.append(("EXTENT-FILLER", unit, name, fuzzy, cc,
                                 f"0x{rva:06x} size 0x{size:x} ends in {cc} B of linker "
                                 f"0xCC filler - real extent 0x{good:x}"))
                else:
                    t = image.own_table_after(rva, size)
                    if t:
                        good = image.true_end(rva) - rva
                        extent_bad = True
                        hits.append(("EXTENT-SHORT", unit, name, fuzzy, t[1],
                                     f"0x{rva:06x} size 0x{size:x} excludes a "
                                     f"{t[1]}-entry jump table at 0x{t[0]:06x} - "
                                     f"real extent 0x{good:x}"))
            if extent_bad:
                continue                     # an extent bug masks every table reading
            be, te = base.extent(name), tgt.extent(name)
            if not be or not te:
                continue
            bt = base.table(*be)
            tt = tgt.table(*te)
            if bt and not tt:
                hits.append(("TABLE-ONLY-BASE", unit, name, fuzzy, bt[1],
                             f"base builds a {bt[1]}-entry jump table, "
                             f"target dispatches without one"))
            elif tt and not bt:
                hits.append(("TABLE-ONLY-TARGET", unit, name, fuzzy, tt[1],
                             f"target builds a {tt[1]}-entry jump table, "
                             f"base dispatches without one"))
            elif bt and tt and bt[1] != tt[1]:
                hits.append(("TABLE-ENTRIES", unit, name, fuzzy, abs(bt[1] - tt[1]),
                             f"base {bt[1]} entries vs target {tt[1]}"))
    return hits


def main(argv=None) -> int:
    _warn_wrong_tree()
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--report", type=Path, default=REPORT)
    ap.add_argument("--unit")
    ap.add_argument("--kind", choices=("extent", "table", "all"), default="all")
    ap.add_argument("--calibrate", action="store_true",
                    help="scan ONLY the 100%%-exact functions; every hit is a detector bug")
    ap.add_argument("--min-fuzzy", type=float, default=0.0)
    ap.add_argument("--max-fuzzy", type=float, default=99.9999)
    ap.add_argument("--tsv", type=Path)
    args = ap.parse_args(argv)

    if not args.report.is_file():
        print(f"no objdiff report at {args.report} - run `gruntz build --fast` first",
              file=sys.stderr)
        return 2
    rep = json.loads(args.report.read_text())

    names_by_unit, total = {}, 0
    for u in rep["units"]:
        unit = u["name"]
        if args.unit and unit != args.unit:
            continue
        for fn in u.get("functions", []):
            f = float(fn.get("fuzzy_match_percent", 0.0))
            if args.calibrate:
                if f < 100.0:
                    continue
            elif not (args.min_fuzzy <= f <= args.max_fuzzy):
                continue
            names_by_unit.setdefault(unit, []).append((fn["name"], f))
            total += 1

    hits = scan(names_by_unit, claims(), Image())
    if args.kind == "extent":
        hits = [h for h in hits if h[0].startswith("EXTENT")]
    elif args.kind == "table":
        hits = [h for h in hits if h[0].startswith("TABLE")]
    hits.sort(key=lambda h: (h[0], -h[4]))

    scope = "100%-exact" if args.calibrate else f"[{args.min_fuzzy}, {args.max_fuzzy}]%"
    print(f"{len(hits)} hit(s) over {total} {scope} function(s) in "
          f"{len(names_by_unit)} unit(s)")
    if args.calibrate:
        rate = 100.0 * len(hits) / total if total else 0.0
        print(f"false-positive rate on functions that MUST agree: "
              f"{len(hits)}/{total} = {rate:.3f}%")
    for kind, unit, name, fuzzy, _mag, detail in hits:
        print(f"  {kind:<18} {unit:<22} {fuzzy:6.2f}%  {name}")
        print(f"    {detail}")

    if args.tsv:
        with args.tsv.open("w") as fh:
            fh.write("kind\tunit\tfuzzy\tmagnitude\tname\tdetail\n")
            for kind, unit, name, fuzzy, mag, detail in hits:
                fh.write(f"{kind}\t{unit}\t{fuzzy:.4f}\t{mag}\t{name}\t{detail}\n")
        print(f"-> {args.tsv}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
