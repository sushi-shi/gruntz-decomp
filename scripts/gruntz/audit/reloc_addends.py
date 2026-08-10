#!/usr/bin/env python3
"""Candidate reloc-ADDEND defects, tree-wide (gruntz.audit.reloc_addends).

Per function and per referenced symbol, compares the MULTISET of DIR32 addends in
the base obj against the delinked target obj. Order-insensitive on purpose: two
references to the same symbol at the same two offsets in a different order is an
operand-evaluation-order difference, not a wrong constant.

  ADDEND   - the reference resolves to a DIFFERENT retail address than retail's.
             The real find: the source indexes a different element of that datum.
  SELFREF  - same shape but the symbol is the function itself, i.e. a switch jump
             table inside .text. Derivative of an unmatched body, not independent.
  NAMING   - NOT a defect: every differing addend resolves to the same absolute
             retail address on both sides, under a different symbol NAME.

WHY `NAMING` EXISTS, and why the raw addend comparison cannot stand alone.  A
symbol reference is `name + addend`, and the two sides do not agree on how to
split an address into those two halves.  A ONE-PAST-THE-END loop sentinel is the
common case: cl emits `cmp edi, <DIR32 arr> + sizeof(arr)`, while the delinker
attributes the same absolute address to whatever symbol STARTS there - the next
datum, at addend 0.  Measured 2026-08-10 on the four-row census: TWO of the four
rows were this, i.e. the naive per-symbol comparison had a 50% false-positive
rate.  `?DoDataExchange@CBattlezDlg@@` was the pure case - base
`cmp edi, g_battlezLastMaxGruntz+0x10` and retail `81 ff 10 9d 62 00` are the
SAME FOUR BYTES after relocation; only the name on the reloc differs.

So every reference is resolved to an absolute RVA (`build/gen/symbol_names.csv`
for named symbols, the embedded address for the delinker's `DAT_<hex>`/`$gap_`
placeholders) and the two sides are compared on ADDRESSES.  Names that resolve on
neither side (string constants `??_C@`, `__imp__*` thunks, the normalizer's
`$Sdata_bss_*` renames) keep the by-name comparison, and an address that resolves
on only ONE side is left classified as a defect - the pass never invents a
pairing, so it can only ever REMOVE a false positive, never hide a real one.
"""
import csv
import json
import re
import struct
from collections import Counter
from pathlib import Path

REPO = next((q for q in Path(__file__).resolve().parents if (q / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
BASE = REPO / "build/objdiff/normalized/base"
TGT = REPO / "build/objdiff/normalized/target"
COFF_TYPE = {0x06: "DIR32", 0x07: "DIR32NB", 0x14: "REL32"}
SIG = {"DIR32", "DIR32NB"}

# The delinker's placeholder names carry their own RVA; `$gap_` is the data-manifest
# filler.  Anchored and length-bounded so a real symbol can never match by accident.
HEX_NAME = re.compile(r"^(?:DAT|FUN|SUB|LAB|PTR|UNK)_0*([0-9a-fA-F]{4,8})$")
GAP_NAME = re.compile(r"^\$gap_([0-9a-fA-F]{4,8})$")


class Coff:
    def __init__(self, path):
        b = self.buf = Path(path).read_bytes()
        self.nsec = struct.unpack_from("<H", b, 2)[0]
        self.symptr = struct.unpack_from("<I", b, 8)[0]
        self.nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        self.strtab_off = self.symptr + self.nsym * 18
        self.sec = []
        for i in range(self.nsec):
            o = 20 + opt + i * 40
            name = b[o:o + 8].split(b"\0")[0].decode("latin1")
            if name.startswith("/"):
                off = int(name[1:])
                end = b.index(b"\0", self.strtab_off + off)
                name = b[self.strtab_off + off:end].decode("latin1")
            vsize, _va, rawsize, rawptr = struct.unpack_from("<IIII", b, o + 8)
            relptr, nrel = struct.unpack_from("<IxxxxH", b, o + 24)
            chars = struct.unpack_from("<I", b, o + 36)[0]
            self.sec.append(dict(name=name, rawptr=rawptr, rawsize=rawsize or vsize,
                                 relptr=relptr, nrel=nrel, chars=chars))

    def sym_name(self, idx):
        base = self.symptr + idx * 18
        if struct.unpack_from("<I", self.buf, base)[0] == 0:
            off = struct.unpack_from("<I", self.buf, base + 4)[0]
            end = self.buf.index(b"\0", self.strtab_off + off)
            return self.buf[self.strtab_off + off:end].decode("latin1")
        return self.buf[base:base + 8].split(b"\0")[0].decode("latin1")

    def iter_symbols(self):
        i = 0
        while i < self.nsym:
            base = self.symptr + i * 18
            value, secnum, _t, scl, naux = struct.unpack_from("<IhHBB", self.buf, base + 8)
            yield i, value, secnum, scl
            i += 1 + naux

    def relocs(self, si):
        s = self.sec[si]
        ptr, count, first = s["relptr"], s["nrel"], 0
        if not ptr:
            return []
        if s["chars"] & 0x01000000 and count == 0xFFFF:
            count = struct.unpack_from("<I", self.buf, ptr)[0]
            first = 1
        return sorted(struct.unpack_from("<IIH", self.buf, ptr + i * 10)
                      for i in range(first, count))

    def read_i32(self, si, off):
        s = self.sec[si]
        if not s["rawptr"]:
            return None
        p = s["rawptr"] + off
        if p + 4 > s["rawptr"] + s["rawsize"]:
            return None
        return struct.unpack_from("<i", self.buf, p)[0]

    def code_functions(self):
        by_sec = {}
        for idx, value, secnum, scl in self.iter_symbols():
            if secnum < 1 or secnum > self.nsec or scl not in (2, 3, 6):
                continue
            s = self.sec[secnum - 1]
            if not (s["chars"] & 0x20) or self.sym_name(idx) == s["name"]:
                continue
            by_sec.setdefault(secnum - 1, []).append((value, self.sym_name(idx)))
        out = {}
        for si, syms in by_sec.items():
            syms.sort()
            rl = self.relocs(si)
            for i, (v, n) in enumerate(syms):
                end = syms[i + 1][0] if i + 1 < len(syms) else self.sec[si]["rawsize"]
                grp = {}
                for site, idx, typ in rl:
                    if not (v <= site < end):
                        continue
                    ty = COFF_TYPE.get(typ, hex(typ))
                    if ty not in SIG:
                        continue
                    add = self.read_i32(si, site)
                    grp.setdefault(self.sym_name(idx), Counter())[add or 0] += 1
                out.setdefault(n, []).append(grp)
        return {k: v[0] for k, v in out.items() if len(v) == 1}


def load_symbol_rvas():
    """name -> retail RVA, from the build's own generated symbol map."""
    path = REPO / "build/gen/symbol_names.csv"
    out = {}
    if not path.exists():
        return out
    with path.open() as f:
        for row in csv.DictReader(f):
            out.setdefault(row["name"], int(row["rva"], 16))
    return out


NAME_RVA = load_symbol_rvas()


def sym_rva(name):
    """Absolute retail RVA a reloc's SYMBOL denotes, or None if unknowable."""
    if name in NAME_RVA:
        return NAME_RVA[name]
    m = HEX_NAME.match(name) or GAP_NAME.match(name)
    return int(m.group(1), 16) if m else None


def resolved_addresses(grp):
    """Multiset of absolute RVAs a function's DIR32 relocs point at (resolvable only)."""
    out = Counter()
    for name, addends in grp.items():
        base = sym_rva(name)
        if base is None:
            continue
        for addend, n in addends.items():
            out[base + addend] += n
    return out


rep = json.loads((REPO / "build/objdiff/report.json").read_text())
pct = {(u["name"], f["name"]): f.get("fuzzy_match_percent", 0.0)
       for u in rep["units"] for f in u.get("functions", [])}

rows = []
for bp in sorted(BASE.glob("*.obj")):
    unit = bp.stem
    tp = TGT / f"{unit}.c.obj"
    if not tp.exists():
        continue
    b, t = Coff(bp).code_functions(), Coff(tp).code_functions()
    for fn, bg in b.items():
        tg = t.get(fn)
        if tg is None:
            continue
        # Addresses one side reaches and the other does not - the only evidence
        # that a differing addend is a real divergence rather than a re-naming.
        b_addr, t_addr = resolved_addresses(bg), resolved_addresses(tg)
        b_only_addr, t_only_addr = b_addr - t_addr, t_addr - b_addr
        for sym in sorted(set(bg) | set(tg)):
            bc, tc = bg.get(sym, Counter()), tg.get(sym, Counter())
            if bc == tc or sum(bc.values()) != sum(tc.values()):
                continue
            base_add, tgt_add = sorted((bc - tc).elements()), sorted((tc - bc).elements())
            anchor = sym_rva(sym)
            if anchor is None:
                real_b, real_t = base_add, tgt_add          # unresolvable: stay by-name
            else:
                real_b = [a for a in base_add if b_only_addr[anchor + a]]
                real_t = [a for a in tgt_add if t_only_addr[anchor + a]]
            if not real_b and not real_t:
                cls = "NAMING"
            elif sym == fn:
                cls = "SELFREF"
            else:
                cls = "ADDEND"
            rows.append(dict(
                cls=cls, pct=pct.get((unit, fn), 0.0), unit=unit, fn=fn, sym=sym,
                base=base_add, target=tgt_add, real_base=real_b, real_target=real_t,
                refs=sum(bc.values())))

rows.sort(key=lambda r: ({"ADDEND": 0, "SELFREF": 1, "NAMING": 2}[r["cls"]], -r["pct"]))
out = REPO / "build/reloc-addend-worklist.tsv"
with out.open("w") as f:
    f.write("class\tfuzzy_pct\tunit\tfunction\tsymbol\trefs\tbase_addends\ttarget_addends\t"
            "unpaired_base\tunpaired_target\n")
    for r in rows:
        f.write(f"{r['cls']}\t{r['pct']:.2f}\t{r['unit']}\t{r['fn']}\t{r['sym']}\t"
                f"{r['refs']}\t{','.join(hex(x) for x in r['base'])}\t"
                f"{','.join(hex(x) for x in r['target'])}\t"
                f"{','.join(hex(x) for x in r['real_base'])}\t"
                f"{','.join(hex(x) for x in r['real_target'])}\n")
tally = Counter(r["cls"] for r in rows)
print(f"{len(rows)} row(s) -> {out}   " + "  ".join(f"{k}={v}" for k, v in sorted(tally.items())))
for r in rows:
    print(f"{r['cls']:8s} {r['pct']:7.2f}  {r['unit']:22s} {r['fn'][:56]:56s}")
    print(f"                  {r['sym'][:66]}")
    print(f"                  base {[hex(x) for x in r['base']]} -> retail "
          f"{[hex(x) for x in r['target']]}  ({r['refs']} ref(s))")
    if r["cls"] == "NAMING":
        print("                  same absolute address on both sides - renaming, not a defect")
    elif len(r["real_base"]) != len(r["base"]) or len(r["real_target"]) != len(r["target"]):
        print(f"                  unpaired (real): base {[hex(x) for x in r['real_base']]} -> "
              f"retail {[hex(x) for x in r['real_target']]}")
