#!/usr/bin/env python3
"""reloc_multiset.py - per-function relocation-referent MULTISET diff, base vs target.

`gruntz sema disasm --diff` MASKS address operands, so a call to the WRONG callee,
a missing statement, or one inline expansion too many all print as "identical" or
as an unremarkable register shuffle. The referent multiset does not mask: if retail
calls `??0CString@@QAE@ABV0@@Z` twice and we call it once, that is a source defect
with a name on it, and it survives every scheduling and register difference.

This is the complement of `gruntz.audit.assert_relocs`, which audits only >=99.5%
functions and only looks for referents present on one side. Here the COUNT is the
signal and every function is in scope.

Measured (Bute module, one pass): `??0CString@@QAE@PBD@Z` base 2 / target 1 named a
`char*` return type that should have been `CString*`; `_isdigit` base 9 / target 10
named a missing loop test; `??0CButeValue@...` base 3 / target 4 named the /Ob1
expansion-count wall; `??1zErrHandling@@UAE@XZ` base 0 / target 1 was the delinker's
jump-table packing artifact (a documented FALSE positive - see below).

Filtered out, because they are structural to how the two sides are produced and
never a source defect:

  * `$L<n>` / `$<label>$<n>` local labels - MSVC splits switch arms into them, and
    they are also why the referent list must NOT be cut at the jump table: a naive
    per-symbol walk truncates every switch-carrying function (`ParseAttributeFile`
    read 7 referents instead of 114).
  * `__except_list`, `__ehreg$`, `__ehunwind$`, `__ehfuncinfo$` - the base obj takes
    relocations against the EH scaffolding the delinker carves into its own band.
  * self-references - our base takes a reloc against a `$L` arm, the delinked target
    against the function symbol itself, so every jump table shows up as N spurious
    self-referents.
  * `$S<serial>` FUNCTION-LOCAL STATICS, folded onto their target name by stem - cl
    spells one `_s_gruntDirEast$S22213`, the delinker `?s_gruntDirEast_244c18@@3U...`,
    and neither name can ever meet the other (see `static_alias_map`).

KNOWN FALSE POSITIVE (1): the delinker packs an unclaimed neighbour into the
preceding symbol, so a referent that appears ONLY on the target side, ONCE, in a
function whose RVA span abuts an unclaimed gap, is usually the neighbour's. Check
the span before believing it.

KNOWN FALSE POSITIVE (2) - the ONE-PAST-THE-END loop bound. `while (d < &buf[N])`
takes a relocation against the address just past `buf`, which IS the next global's
base address. Our base names it `buf + N`; the delinker has only the address, so it
names it `<next global>`. The pair therefore reads as "one reference moved to the
neighbour" - in EVERY function that walks that array. `fileimage` is the whole
worked example: six palette buffers laid out exactly 0x400 apart in declaration
order (0x283ef0 g_paletteRampBuf, 0x2842f0 s_palBmp, 0x2846f0 s_palPcx, 0x284af0
g_grayRamp, 0x284ef0 s_palPidData, 0x2852f0 s_palPcxData, 0x2856f0 g_warpU), and
all six Decode* functions report `<own buffer> N/N-1` plus `<next buffer> 0/1`.
There is no source defect and no legal spelling that avoids it: writing the bound
as the NEXT symbol would match the delinker's guess rather than what the compiler
really emitted, i.e. a fitted artifact.

Rows of that family are annotated `[folded-base artifact]`. Three shapes are
recognised, all one mechanism - cl folded a constant offset into an absolute
address, and only our side still knows which symbol it was an offset FROM:

  * ADJACENT, possibly through a CHAIN. Three back-to-back arrays each walked to
    their end make the middle one gain a reference and lose one, so it cancels
    out of the diff entirely and a one-hop test misses both ends (three of the
    five `CBattlezDlg::DoDataExchange` rows).
  * INTERIOR - a field of a global struct (`CNetSession::SendOne` writes
    g_netCmdSendMsg +1/+5/+9/+0xd/+0xe, read as five anonymous DAT_0064a0xx).
  * BELOW THE BASE - `buf[i - k]` relocates against `buf - k` (g_clut -2,
    g_mapNameBuf -4).

NOT recognised, and still to be read by hand: the same thing done to a STRING
LITERAL. `char sz[] = "Software"` copies the literal in three chunks, so the +4
and +8 loads come out as DAT_, but pooled literals carry no extent in
symbol_names.csv, so there is nothing to test containment against.
`RegistryHelper::Open` is the worked example and is byte-exact.
"""
import argparse
import collections
import re
import subprocess
import sys
from pathlib import Path

from gruntz.core.branches import is_local_label


# Resolve REPO from the CWD first, not __file__: in a worktree the shell's PYTHONPATH
# can point at MAIN's scripts/, so `python -m ...` would mis-resolve to main.
def _find_repo() -> Path:
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists() and (p / "build" / "objdiff").exists():
                return p
    return Path(__file__).resolve().parents[3]


REPO = _find_repo()

RELOC_RE = re.compile(r"^[0-9a-f]{8}:\s+IMAGE_REL_I386_(\w+)\s+(.*)$")
NOISE = ("__except_list", "__eh", "__unwindtable$", "__tryblocktable$", "$L", "$")


def referents(obj: Path) -> dict:
    """{function -> [referent, ...]} for one COFF, switch arms folded into their fn."""
    out = subprocess.run(["llvm-objdump", "-dr", "--x86-asm-syntax=intel", str(obj)],
                         capture_output=True, text=True).stdout
    res, cur = {}, None
    for ln in out.splitlines():
        s = ln.strip()
        if s.endswith(">:") and "<" in s:
            name = s.split("<", 1)[1][:-2]
            if cur is not None and is_local_label(name):
                continue          # a switch arm belongs to the function above it
            cur = name
            res.setdefault(cur, [])
            continue
        m = RELOC_RE.match(s)
        if m and cur is not None:
            res[cur].append(m.group(2).strip())
    return res


def code_extents(obj: Path) -> dict:
    """{function -> code bytes attributed to it} for one COFF.

    THE DELINKED OBJ IS ONE FLAT `.text`, our base obj is one COMDAT PER FUNCTION.
    So a base symbol's extent is its own section, while a target symbol's extent runs
    to the NEXT symbol - and any body the delinker did not name is silently swallowed
    by the function above it, along with all of its relocations. That is how
    `CMultiBootyState::LoadGameAssetNamespaces` reads `g_multiBootyGeom base 5 target 15`
    while the retail disassembly of its own 0xd7d bytes takes exactly ONE relocation
    against that array: the target symbol spans 0x3a4c-0x1f88 bytes, the base one 0xcf0.
    Comparing the two extents is what tells a real count difference from that artifact.
    """
    import struct
    b = obj.read_bytes()
    nsec, = struct.unpack_from("<H", b, 2)
    symptr, nsym = struct.unpack_from("<II", b, 8)
    strtab = symptr + nsym * 18
    secs = []
    for i in range(nsec):
        o = 20 + i * 40
        name = b[o:o + 8].rstrip(b"\0").decode("latin-1")
        rawsize, = struct.unpack_from("<I", b, o + 16)
        secs.append((name, rawsize))
    syms, i = [], 0
    while i < nsym:
        o = symptr + i * 18
        raw = b[o:o + 8]
        if raw[:4] == b"\0\0\0\0":
            off, = struct.unpack_from("<I", raw, 4)
            end = b.index(b"\0", strtab + off)
            name = b[strtab + off:end].decode("latin-1")
        else:
            name = raw.rstrip(b"\0").decode("latin-1")
        value, secnum, _typ, cls, naux = struct.unpack_from("<IhHBB", b, o + 8)
        if cls == 2 and 1 <= secnum <= nsec and secs[secnum - 1][0].startswith(".text"):
            syms.append((secnum, value, name))
        i += 1 + naux
    syms.sort()
    out = {}
    for j, (secnum, value, name) in enumerate(syms):
        end = secs[secnum - 1][1]
        if j + 1 < len(syms) and syms[j + 1][0] == secnum:
            end = syms[j + 1][1]
        out.setdefault(name, max(0, end - value))
    return out


STATIC_S_RE = re.compile(r"^_?(?P<stem>[A-Za-z_][A-Za-z0-9_]*)\$S\d+$")
MANGLED_DATA_RE = re.compile(r"^\?(?P<stem>[A-Za-z_][A-Za-z0-9_]*?)"
                             r"(?:_[0-9a-f]{4,8})?@@3")


def static_alias_map(base_names, target_names) -> dict:
    """{base `$S` name -> the target name for the SAME datum}, matched by stem.

    A FUNCTION-LOCAL STATIC IS SPELLED DIFFERENTLY ON THE TWO SIDES AND CAN NEVER
    PAIR BY NAME. cl decorates it with a per-TU serial - `_s_gruntDirEast$S22213` -
    while the delinker names it from `symbol_names.csv`, i.e. from its C++ mangling
    plus an RVA disambiguator: `?s_gruntDirEast_244c18@@3UGruntDirectionCell@@A`.
    The serial is assigned by declaration order in the TU, so it is not even stable
    across our own edits. Left alone, every such datum produces TWO rows - the whole
    base count against 0 and the whole target count against 0 - which is a naming
    artifact, not a referent defect. objdiff does not see it either: normalize
    content-addresses `$S` statics and pairs them at 100%.

    Folding by stem keeps a REAL count difference visible (it just reports under the
    target's name) while the split disappears. `triggermgrhittest` was nine such
    pairs; `bootystateactivate` shows the other case - both sides already agree on
    the `$S` spelling, so nothing is folded and its rows stand.
    """
    by_stem = {}
    for n in target_names:
        m = MANGLED_DATA_RE.match(n) or STATIC_S_RE.match(n)
        if m:
            by_stem.setdefault(m.group("stem"), n)
    out = {}
    for n in base_names:
        m = STATIC_S_RE.match(n)
        if m and n not in target_names:
            tgt = by_stem.get(m.group("stem"))
            if tgt is not None:
                out[n] = tgt
    return out


def audit(unit: str) -> list:
    base = REPO / "build/objdiff/base" / (unit + ".obj")
    target = REPO / "build/objdiff/target" / (unit + ".c.obj")
    if not base.is_file() or not target.is_file():
        return []
    b, t = referents(base), referents(target)
    eb, et = code_extents(base), code_extents(target)
    alias = static_alias_map({x for v in b.values() for x in v},
                             {x for v in t.values() for x in v})
    findings = []
    for fn in sorted(b):
        if fn not in t:
            continue
        def keep(n):
            return not (n.startswith(NOISE) or n == fn)
        cb = collections.Counter(alias.get(x, x) for x in b[fn] if keep(x))
        ct = collections.Counter(x for x in t[fn] if keep(x))
        rows = [(k, cb.get(k, 0), ct.get(k, 0)) for k in sorted(set(cb) | set(ct))
                if cb.get(k, 0) != ct.get(k, 0)]
        if rows:
            lb, lt = eb.get(fn, 0), et.get(fn, 0)
            packed = (lb and lt > lb + max(32, lb // 8))
            findings.append((unit, fn, rows, (lb, lt) if packed else None))
    return findings


def symbol_extents() -> dict:
    """{name: (rva, size)} from build/gen/symbol_names.csv, for adjacency checks."""
    out = {}
    csv_path = REPO / "build/gen/symbol_names.csv"
    if not csv_path.is_file():
        return out
    with csv_path.open() as f:
        for line in f:
            if not line.startswith("0x"):
                continue
            parts = line.rstrip("\n").split(",")
            if len(parts) < 4:
                continue
            try:
                out[parts[1]] = (int(parts[0], 16), int(parts[3], 16))
            except ValueError:
                continue
    return out


MAX_HOPS = 3
IMAGE_BASE = 0x400000
FOLD_BACK = 16


def one_past_end(rows, extents) -> set:
    """Names whose extent ABUTS a base-side symbol's extent - the folded-base artifact.

    cl folds a constant index into the address: `&buf[N]` becomes the absolute
    address just past `buf` (which IS the next global's base), and `buf[i - 1]`
    becomes `[buf-1 + i]`, whose base lands inside the PRECEDING global. Our base
    obj relocs against `buf` with the addend; the delinker has only the resolved
    address, so it names the neighbour. Either adjacency direction is the artifact.

    THREE OR MORE consecutive arrays each walked to their end CHAIN the artifact,
    and the middle link then cancels out: with `a`, `b`, `c` laid out back to back,
    the `&a[N]` bound is named `b` on the target side while the `&b[N]` bound is
    named `c`, so `b` gains one reference AND loses one and never appears in the
    diff at all. A one-hop adjacency test therefore sees a surplus on `c` with no
    neighbouring deficit and reports a defect that is not there (three of the five
    `CBattlezDlg::DoDataExchange` rows were exactly this). So walk the adjacency
    chain, hopping only through symbols whose own counts are BALANCED.
    """
    def span(name):
        if name not in extents:
            return None
        rva, size = extents[name]
        return (rva, rva + size) if size else None

    delta = {n: b - t for n, b, t in rows}
    by_start, by_end = {}, {}
    for n, (rva, size) in extents.items():
        if not size:
            continue
        by_start.setdefault(rva, []).append(n)
        by_end.setdefault(rva + size, []).append(n)

    def chase(sp, step, key):
        """Walk `step` from `sp` through balanced symbols to a base-side surplus."""
        seen, chain = set(), []
        for _ in range(MAX_HOPS):
            nbrs = step.get(key(sp), [])
            hit = [n for n in nbrs if delta.get(n, 0) > 0]
            if hit:
                return chain + hit
            nxt = [n for n in nbrs
                   if delta.get(n, 0) == 0 and span(n) and n not in seen]
            if not nxt:
                return []
            seen.add(nxt[0])
            chain.append(nxt[0])
            sp = span(nxt[0])
        return []

    flagged = set()
    for name, b, t in rows:
        if t <= b:
            continue
        sp = span(name)
        if not sp:
            continue
        for step, key in ((by_end, lambda s: s[0]), (by_start, lambda s: s[1])):
            reached = chase(sp, step, key)
            if reached:
                flagged.add(name)
                flagged.update(reached)
                break
    flagged |= folded_interior(rows, extents)
    return flagged


ANON_RE = re.compile(r"^(?:DAT|FUN|LAB)_([0-9a-fA-F]{6,8})$")


def folded_interior(rows, extents) -> set:
    """Target-only `DAT_<addr>` naming a spot INSIDE a base-side symbol.

    The same folded-base mechanism as `one_past_end`, one step earlier: cl reaches a
    FIELD of a global (`mov [_g_msg+1],al`) or copies a literal into a local buffer
    (`char sz[] = "Software"` becomes three loads at +0/+4/+8) with the offset folded
    into the absolute address. Our base takes ONE relocation against the symbol plus
    an addend; the delinker only has the resolved address, and there is no symbol
    there, so it emits `DAT_<addr>`. The pair reads as "N references became 1 plus
    N-1 anonymous ones" - `CNetSession::SendOne` (g_netCmdSendMsg 7 -> 2 plus five
    DAT_0064a0xx) and `RegistryHelper::Open` (the "Software" literal 3 -> 1 plus
    DAT_0061a068/06c) are the two worked examples, and both are byte-exact.
    """
    def contains(name, addr):
        """Is `addr` a folded offset off `name`'s base?

        The window is the symbol's own extent, plus its one-past-the-end address
        (`&buf[N]`), plus a short run BELOW its base - `buf[i - k]` compiles to
        `[buf-k + i]`, whose relocated base sits k bytes before the symbol
        (g_clut -2, g_mapNameBuf -4). Anonymous DAT names carry no extent of
        their own, which is why `one_past_end` cannot see any of these.
        """
        if name not in extents:
            return False
        rva, size = extents[name]
        return size > 1 and rva - FOLD_BACK <= addr <= rva + size and addr != rva

    surplus = [n for n, b, t in rows if b > t]
    flagged = set()
    for name, b, t in rows:
        if t <= b:
            continue
        m = ANON_RE.match(name)
        if not m:
            continue
        raw = int(m.group(1), 16)
        for addr in (raw, raw - IMAGE_BASE):
            hit = [s for s in surplus if contains(s, addr)]
            if hit:
                flagged.add(name)
                flagged.update(hit)
                break
    return flagged


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("units", nargs="*", help="unit names (default: every base obj)")
    ap.add_argument("--summary", action="store_true", help="counts only")
    args = ap.parse_args(argv if argv is not None else sys.argv[1:])

    units = args.units or sorted(p.stem for p in (REPO / "build/objdiff/base").glob("*.obj"))
    extents = symbol_extents()
    total = packed_n = 0
    for unit in units:
        for u, fn, rows, packed in audit(unit):
            total += 1
            if packed:
                packed_n += 1
            if args.summary:
                continue
            print(f"-- {u}  {fn}")
            if packed:
                print("     !! delinker packing: target symbol spans %d B against the "
                      "base's %d B, so some rows below belong to an UNNAMED neighbour"
                      % (packed[1], packed[0]))
            flagged = one_past_end(rows, extents)
            for k, x, y in rows:
                note = "  [folded-base artifact]" if k in flagged else ""
                print("     %-58s base %2d target %2d%s" % (k[:58], x, y, note))
    print(f"reloc-multiset: {total} function(s) whose referent counts differ "
          f"across {len(units)} unit(s); {packed_n} of them have a target symbol that "
          f"swallowed an unnamed neighbour")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
