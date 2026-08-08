"""Sieve: our function reads a global a DIFFERENT NUMBER OF TIMES than retail does.

Caching a global (or a member) in a local is one of the highest-yield source bugs
in this campaign, and it is invisible to every diff we own. `sema disasm --diff`
masks address operands, so `mov eax,_g_gameReg` and `mov eax,[ebp+8]` read as one
`-`/`+` pair buried in a wall of scheduling noise; the block topology stays
identical; the function parks at 45-90% and gets labelled a regalloc wall.

The tell is a COUNT. cl 5.0 re-materialises a global at every use unless the
source hoisted it, so the number of DIR32 relocations naming a symbol inside one
function is a direct, uneditorialised readout of how many times the SOURCE
mentioned it.

  * `base < target` -- WE OVER-CACHED. A `CGruntzMgr* reg = g_gameReg;` at the top
    collapsed N reads to one. This is the expensive direction: the local needs a
    callee-saved home, so it takes a register out of circulation for the whole
    body and every later value spills. `CGrunt::LoadGruntCombatAnimations` read
    `_g_gameReg` 3 times against retail's 5, and splitting the reads back out per
    region took exactly-matching basic blocks from 41 to 120 (45.93 -> 50.43).
    Same shape through a member pointer: `CMulti::PumpB` 83.38 -> 92.36.
  * `base > target` -- WE INVENTED A READ, or retail hoisted one we did not. Much
    rarer, and usually means a common subexpression the source really did name.

See docs/patterns/invented-member-pointer-local.md and
docs/patterns/redundant-local-becomes-the-zero-register.md.

WINDOWING IS THE WHOLE TOOL
---------------------------
The base is COMDAT-per-function, so a base section IS one function. The delinked
target packs a whole unit's `.text` into ONE section, so reading "the target's
relocations" without a window picks up the NEIGHBOUR's and inflates every count.
A previous attempt at this sieve reported garbage for exactly that reason. Each
function is therefore read over `[target_offset, target_offset + base_extent)`,
additionally clamped at the next defined symbol in the target section.

It reads `build/objdiff/normalized/`, the copies objdiff actually scores, and it
must: `canonicalize_data_symbols` content-addresses compiler-private data, so a
pooled literal that Ghidra called `DAT_002126ec` and cl called `$SG30360` carries
the SAME name on both sides there and does not manufacture a row. (A lane burned
an afternoon on `DAT_002126ec` -- "GAME_TABHIGHLIGHT1", already fully
referenced -- reading the raw objs.) Normalization also rewrites embedded
jump-table DIR32 entries to `<function>+addend`, which is why self-references are
a recognised family rather than noise.

CALIBRATION
-----------
`--calibrate` restricts to the functions objdiff scores at 100.00%. Those agree by
construction, so any row there is a DETECTOR BUG, and the rate prints on every run.
It reached 0 of 4301 paired functions only after four filters, each of which was a
real false-positive family the first draft reported as a finding:

  1. the WINDOW above  (unbounded target reads picked up the neighbour);
  2. `$S<n>` / `$Sdata_data_<hash>` local-static suffixes canonicalized away -- the
     hash covers recorded relocations the delinker does not reproduce, so one
     `s_QUESTZ` reads as two different symbols;
  3. NONZERO ADDEND dropped, which is the delinker's unsized-datum fallback
     (`?s_table@?$CActRegPool@VCGrunt@@...` -> `?g_gruntDirNorthEast + 0x2b0`);
  4. a name only ONE side references INSIDE THE FUNCTION dropped.

Filter 4 is the one that costs coverage, and deliberately: it means this sieve
answers "how many times", never "which symbol". A reference we point at the wrong
global, or a pooled literal the two sides name differently
(`??_C@_01PFH@A` against `??_C@_0BE@MAOF@GAME_ACTIONAREA_RED`, same offset, same
byte -- what made `DAT_002126ec` eat a lane's afternoon), is a WRONG-REFERENT
defect and belongs to a reloc-sequence comparison. `--one-sided` shows them again.
`___except_list` (the `/GX` prologue slot the base spells with a DIR32 and the
delinker resolves without one) and self-references (a switch's jump table, which
`jcc_sieve` reads properly) are dropped by name and by `--self`.

    python -m gruntz.audit.global_refs                     # the ranked worklist
    python -m gruntz.audit.global_refs --calibrate         # the false-positive rate
    python -m gruntz.audit.global_refs --unit gruntcombat
    python -m gruntz.audit.global_refs --fn LoadGruntCombat
    python -m gruntz.audit.global_refs --rel32             # calls, not data
"""

from __future__ import annotations

import argparse
import collections
import glob
import json
import re
import struct
import sys
from pathlib import Path

from gruntz.core.branches import is_local_label

REPO = Path(__file__).resolve().parents[3]
NORM = REPO / "build" / "objdiff" / "normalized"
REPORT = REPO / "build" / "objdiff" / "report.json"

DIR32 = 0x06
REL32 = 0x14
MEM_EXECUTE = 0x20000000

# The `/GX` prologue's thread-information-block slot. The base spells it with a
# DIR32; the delinked target resolves the same absolute without a relocation, so
# every EH function would otherwise read "base-only ___except_list x1".
STRUCTURAL = {"___except_list", "__except_list"}

# cl's compiler-generated funclet/initializer COMDATs, after content-addressing.
# They have no RVA pin and so are never delinked -- they cannot pair.
COMPGEN = re.compile(r"^_?\$[ES][0-9]+$|^\$anon_(?:data|f32|f64)_[0-9a-f]+_[0-9]+$")

# A function-local static's compiler-private suffix. `canonicalize_data_symbols`
# content-addresses it, and the hash covers the datum's RECORDED RELOCATIONS as
# well as its bytes -- which the delinker does not always reproduce -- so the same
# logical `s_QUESTZ` reads `_s_QUESTZ$Sdata_data_87db2c..._0` in the base and
# `_s_QUESTZ$Sdata_data_5bc660..._0` in the target. Comparing those verbatim made
# `BuildMainMenuTree` (100.00% exact) the top row of the first draft of this sieve.
LOCAL_STATIC_SUFFIX = re.compile(r"\$S(?:data_data_[0-9a-f]+_[0-9]+|[0-9]+)$")
# cl's scalar-deleting-destructor slot is a COFF weak external whose default is the
# vector form; the normalizer already retargets it, this is belt and braces.
VDTOR = re.compile(r"^\?\?_E")


def canon(name: str) -> str:
    return VDTOR.sub("??_G", LOCAL_STATIC_SUFFIX.sub("", name))


# --------------------------------------------------------------------------- COFF

def _coff(path: Path):
    """[{name, size, chars, owners:[(off,name)], rel:[(off,name,type)]}] per section."""
    d = path.read_bytes()
    nsec = struct.unpack_from("<H", d, 2)[0]
    symptr, nsym = struct.unpack_from("<II", d, 8)
    optsz = struct.unpack_from("<H", d, 16)[0]
    strtab = symptr + nsym * 18

    def sname(off: int) -> str:
        return d[off:d.find(b"\0", off)].decode("latin1")

    idxname: dict[int, str] = {}
    owners: dict[int, list] = collections.defaultdict(list)
    i = 0
    while i < nsym:
        o = symptr + i * 18
        raw = d[o:o + 8]
        nm = (sname(strtab + struct.unpack_from("<I", d, o + 4)[0])
              if raw[:4] == b"\0\0\0\0" else raw.rstrip(b"\0").decode("latin1"))
        val = struct.unpack_from("<I", d, o + 8)[0]
        sec = struct.unpack_from("<h", d, o + 12)[0]
        idxname[i] = nm
        if sec > 0 and not nm.startswith("."):
            owners[sec].append((val, nm))
        i += 1 + d[o + 17]

    secs = []
    for s in range(nsec):
        o = 20 + optsz + s * 40
        name = d[o:o + 8].rstrip(b"\0").decode("latin1")
        if name.startswith("/"):
            name = sname(strtab + int(name[1:]))
        rawsize = struct.unpack_from("<I", d, o + 16)[0]
        rrp = struct.unpack_from("<I", d, o + 24)[0]
        nrel = struct.unpack_from("<H", d, o + 32)[0]
        chars = struct.unpack_from("<I", d, o + 36)[0]
        rawoff = struct.unpack_from("<I", d, o + 20)[0]
        rel = []
        for r in range(nrel):
            ro = rrp + r * 10
            off, si, ty = struct.unpack_from("<IIH", d, ro)
            addend = (struct.unpack_from("<I", d, rawoff + off)[0]
                      if rawoff and off + 4 <= rawsize else 0)
            rel.append((off, idxname.get(si, "?"), ty, addend))
        secs.append({"name": name, "size": rawsize, "chars": chars,
                     "owners": sorted(owners.get(s + 1, [])), "rel": sorted(rel)})
    return secs


def _functions(secs) -> dict[str, tuple[int, int, list]]:
    """{symbol: (start, end, section_relocs)} for every function body in one obj.

    `end` is the next defined symbol in the same section, or the section size.
    Local block labels (`$L27`, `$loop$4`) belong to the enclosing COMDAT and are
    not boundaries; reading them as boundaries truncates every function that
    contains a `goto`.
    """
    out: dict[str, tuple[int, int, list]] = {}
    for sec in secs:
        if not sec["chars"] & MEM_EXECUTE:
            continue
        bounds = [(v, n) for v, n in sec["owners"] if not is_local_label(n)]
        for i, (val, nm) in enumerate(bounds):
            end = bounds[i + 1][0] if i + 1 < len(bounds) else sec["size"]
            out[nm] = (val, end, sec["rel"])
    return out


def _refs(start: int, end: int, relocs, want: int, dropped) -> collections.Counter:
    """The multiset of ADDEND-ZERO relocation targets in [start, end).

    A nonzero addend is dropped, and that single rule is what makes this sieve
    usable. `?s_table@?$CActRegPool@VCGrunt@@@@2V?$zDArray...` -- a `.bss` static
    the delinker never enrolled -- resolves on the target side to
    `?g_gruntDirNorthEast + 0x2b0`, so `CGrunt::FireActivation` (258 B, 100.00%
    exact) read as twenty phantom `g_gruntDir*` deltas. Every fallback carries the
    distance from the symbol it landed on; a source-level `g_foo` read carries 0.

    The cost is symmetric and small: a constant-index array element
    (`_g_hitTable + 0x40`) is spelled with an addend on BOTH sides, so it drops
    from both and cannot invent a row -- it just is not covered.
    """
    c: collections.Counter = collections.Counter()
    for off, nm, ty, addend in relocs:
        if ty != want or not start <= off < end:
            continue
        if addend:
            dropped["nonzero addend (delinker fallback / array element)"] += 1
            continue
        c[canon(nm)] += 1
    return c


def _universe(secs) -> set[str]:
    """Every symbol name this object knows: defined, or named by a relocation.

    A name only ONE side has ever heard of cannot be compared, and the asymmetry
    is structural rather than a source fact. The big producer is the delinker's
    unsized-datum fallback: a `.bss` static it never enrolled resolves to
    `<previous named symbol> + addend`, so `?s_table@?$CActRegPool@VCDroppedObject
    @@@@2V?$...` x22 in the base reads as `_g_watchBlinkB` x20 + `?g_customLevelText`
    x2 in the target -- 22 phantom "global" deltas in a function that is 100.00%
    exact. The same filter absorbs the pooled-literal naming split (`??_C@_01PFH@A`
    against `??_C@_0BE@MAOF@GAME_ACTIONAREA_RED`, same offset, same byte).

    Consequence, stated rather than hidden: a reference we invented to a global
    retail's whole unit never touches is invisible here. That is a different
    defect (a WRONG referent, not a wrong COUNT) and `reloc_sequence` reads it.
    """
    names: set[str] = set()
    for sec in secs:
        for _off, nm in sec["owners"]:
            names.add(canon(nm))
        for _off, nm, _ty, _add in sec["rel"]:
            names.add(canon(nm))
    return names


# --------------------------------------------------------------------------- scan

def _scores() -> dict[tuple[str, str], tuple[float, int]]:
    """{(unit, symbol): (fuzzy_percent, size)} from the objdiff report."""
    if not REPORT.is_file():
        return {}
    doc = json.loads(REPORT.read_text())
    out = {}
    for unit in doc.get("units", []):
        for fn in unit.get("functions", []) or []:
            out[(unit["name"], fn["name"])] = (
                float(fn.get("fuzzy_match_percent", 0.0)), int(fn.get("size", 0)))
    return out


class Row:
    __slots__ = ("unit", "sym", "size", "pct", "over", "under", "self_delta")

    def __init__(self, unit, sym, size, pct, over, under, self_delta):
        self.unit, self.sym, self.size, self.pct = unit, sym, size, pct
        self.over, self.under, self.self_delta = over, under, self_delta

    @property
    def magnitude(self) -> int:
        return sum(self.over.values()) + sum(self.under.values())

    @property
    def rank(self) -> int:
        return self.magnitude * self.size


def scan(unit_filter=None, want=DIR32, keep_self=False, both_sides=False):
    """[Row] for every function whose reference multiset differs, plus the totals."""
    scores = _scores()
    units = sorted(Path(p).stem for p in glob.glob(str(NORM / "base" / "*.obj")))
    if unit_filter:
        units = [u for u in units if u == unit_filter]
    rows, seen, dropped = [], 0, collections.Counter()
    for u in units:
        base, target = NORM / "base" / f"{u}.obj", NORM / "target" / f"{u}.c.obj"
        if not base.is_file() or not target.is_file():
            continue
        try:
            bsecs, tsecs = _coff(base), _coff(target)
        except (OSError, struct.error, ValueError):
            continue
        bf, tf = _functions(bsecs), _functions(tsecs)
        buniv, tuniv = _universe(bsecs), _universe(tsecs)
        for sym in sorted(set(bf) & set(tf)):
            if COMPGEN.match(sym):
                dropped["compiler-generated COMDAT"] += 1
                continue
            bs, be, brel = bf[sym]
            ts, te, trel = tf[sym]
            extent = be - bs
            # Window the target to the BASE function's extent, and never past the
            # next definition packed into the same delinked section.
            te = min(te, ts + extent)
            b = _refs(bs, be, brel, want, dropped)
            t = _refs(ts, te, trel, want, dropped)
            for name in [n for n in b if n not in tuniv]:
                dropped["symbol the target never names"] += b.pop(name)
            for name in [n for n in t if n not in buniv]:
                dropped["symbol the base never names"] += t.pop(name)
            for name in STRUCTURAL:
                if b.pop(name, 0) or t.pop(name, 0):
                    dropped["___except_list prologue"] += 1
            self_delta = b.pop(sym, 0) - t.pop(sym, 0)
            if not keep_self and self_delta:
                dropped["self-reference (jump table)"] += 1
            elif keep_self and self_delta:
                (b if self_delta > 0 else t)[sym] = abs(self_delta)
            seen += 1
            over, under = t - b, b - t          # base<target ; base>target
            if not both_sides:
                shared = set(b) & set(t)
                for c in (over, under):
                    for name in [n for n in c if n not in shared]:
                        dropped["one side never names it in THIS function"] += c.pop(name)
            if not over and not under:
                continue
            pct, size = scores.get((u, sym), (0.0, extent))
            rows.append(Row(u, sym, size or extent, pct, over, under, self_delta))
    rows.sort(key=lambda r: (-r.rank, r.unit, r.sym))
    return rows, seen, dropped


# --------------------------------------------------------------------------- cli

def _fmt(counter) -> str:
    return " ".join(f"{n}{'' if c == 1 else f' x{c}'}"
                    for n, c in sorted(counter.items(), key=lambda kv: -kv[1])) or "-"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit", help="restrict to one objdiff unit")
    ap.add_argument("--fn", help="substring filter on the symbol")
    ap.add_argument("--rel32", action="store_true",
                    help="count REL32 (call/jmp) targets instead of DIR32 data refs")
    ap.add_argument("--self", action="store_true",
                    help="keep self-references (jump-table entries) in the counts")
    ap.add_argument("--one-sided", action="store_true",
                    help="also report names only ONE side references in the function "
                         "(a different defect - a wrong referent, not a wrong count - "
                         "and the residual false-positive family, see the docstring)")
    ap.add_argument("--calibrate", action="store_true",
                    help="only the 100.00%%-exact functions -- the false-positive set")
    ap.add_argument("--min-pct", type=float, default=0.0)
    ap.add_argument("--max-pct", type=float, default=100.0)
    ap.add_argument("--top", type=int, default=40)
    args = ap.parse_args(argv)

    want = REL32 if args.rel32 else DIR32
    rows, seen, dropped = scan(args.unit, want, args.self, args.one_sided)
    exact = [r for r in rows if r.pct >= 100.0]

    shown = exact if args.calibrate else [
        r for r in rows if args.min_pct <= r.pct <= args.max_pct and r.pct < 100.0]
    if args.fn:
        shown = [r for r in rows if args.fn in r.sym]

    print(f"objs: {NORM.relative_to(REPO)}   reloc: "
          f"{'REL32 (calls)' if args.rel32 else 'DIR32 (data)'}")
    if dropped:
        print("filtered: " + ", ".join(f"{k}={v}" for k, v in sorted(dropped.items())))
    print(f"paired functions: {seen}   differing: {len(rows)}   "
          f"of those 100%%-exact (FALSE POSITIVES): {len(exact)}"
          f"  [{100.0 * len(exact) / max(len(rows), 1):.1f}%]\n")

    for r in shown[:args.top]:
        print(f"{r.pct:6.2f}%  {r.size:>6}B  rank {r.rank:>7}  {r.unit}  {r.sym}")
        if r.over:
            print(f"        base UNDER-reads (we over-cached): {_fmt(r.over)}")
        if r.under:
            print(f"        base OVER-reads  (we invented):    {_fmt(r.under)}")
    if len(shown) > args.top:
        print(f"\n... {len(shown) - args.top} more")
    return 0


if __name__ == "__main__":
    sys.exit(main())
