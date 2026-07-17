#!/usr/bin/env python3
"""vtable_slot_identity - what does a placeholder-named virtual slot really point at?

A vtable slot declared under a synthetic name (`SbiSlot3`, `Vslot0c`, `UserLogicVfuncA`,
`Slot00_13c530`, `Vfunc8`, `vN`, ...) binds to NO rva, so its `.rdata` DIR32 can never
match. But the retail slot DOES resolve somewhere, and since the ILT resolution landed
(docs/patterns/ilt-thunk-indirection.md) the delinked target names that destination
honestly instead of naming a jump thunk.

This joins the two sides per slot:

    base obj .rdata  ->  the placeholder name WE declared
    target .c.obj    ->  what the delinker resolved the slot to

and classifies the resolved side, because NOT every resolved name is an identity:

  SHADOW      the destination carries one of OUR OWN mangled claims - i.e. the body is
              already declared and bound elsewhere, usually as a NON-VIRTUAL sibling
              beside the placeholder. This is the harvestable set: placeholder and
              sibling are ONE function (fold them; make the sibling the virtual).
  UNBOUND     the destination is a Ghidra `FUN_`/`thunk_`/`LAB_` fallback = a body
              nobody has reconstructed. NOT an identity. Leave it.
  GHIDRA-LABEL the destination carries a bare Ghidra name (`ComputeFacing`). Ghidra
              guessed it; retail has no symbols. NOT evidence. Leave it.
  AGREES      base and target already say the same thing.

FAMILY SIZE is printed for every placeholder because a virtual's name is ONE name for
the whole hierarchy: `SbiSlot3` is declared by 8 classes, and its slot resolves to a
DIFFERENT sibling in each (DtorStatus / DtorRect / ClearFrame / ClearFrame2 / Free /
Reset). Promoting one leaf's sibling name onto the family would misname the other 7.
Only `family=1` rows are a safe 1:1 rename; `family>1` needs one name chosen for the
whole slot and every leaf's claim rebound with it.

Run: python -m gruntz.analysis.vtable_slot_identity [--all] [--unit U]
"""
from __future__ import annotations

import collections
import csv
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
BASE_DIR = REPO / "build" / "objdiff" / "base"
REPORT = REPO / "build" / "objdiff" / "report.json"
NAMES = REPO / "build" / "gen" / "symbol_names.csv"

# A synthetic/placeholder virtual name: carries a slot index or a raw address instead
# of a role. Kept in ONE place so the shapes can't drift apart (the cleanliness regex
# and the real tree already did - `SbiSlot3` matches none of its patterns).
PLACEHOLDER = re.compile(
    r"^\?(?:"
    r"\w*Slot\d{1,2}(?:_[0-9a-f]{4,})?"     # SbiSlot3, Slot00_13c530, Vslot0c
    r"|\w*Vfunc[0-9A-Fa-f]+"                 # UserLogicVfuncA, Vfunc8
    r"|\w*Vslot[0-9A-Fa-f]+"                 # Vslot0c
    r"|Method_[0-9a-f]{4,}"                  # Method_158b10
    r"|v\d+"                                 # v12
    r")@"
)


def _readobj(obj: Path) -> str:
    return subprocess.run(
        ["llvm-readobj", "--symbols", "--relocations", "--section-headers", str(obj)],
        capture_output=True, text=True).stdout


def rdata_vtables(obj: Path):
    """{defining ??_7 symbol: [(slot_offset, reloc_symbol)]} per .rdata COMDAT."""
    lines = _readobj(obj).splitlines()
    sections: dict[int, str] = {}
    sec_syms = collections.defaultdict(list)
    relocs = collections.defaultdict(list)
    i = 0
    while i < len(lines):
        s = lines[i].strip()
        if s == "Section {":
            num = name = None
            j, d = i + 1, 1
            while j < len(lines) and d > 0:
                t = lines[j].strip()
                if t.endswith("{"):
                    d += 1
                elif t == "}":
                    d -= 1
                m = re.match(r"^Number: (\d+)$", t)
                if m and num is None:
                    num = int(m.group(1))
                m = re.match(r"^Name: (\S+)", t)
                if m and name is None:
                    name = m.group(1)
                j += 1
            if num is not None:
                sections[num] = name
            i = j
            continue
        if s == "Symbol {":
            name = value = sec = None
            j, d = i + 1, 1
            while j < len(lines) and d > 0:
                t = lines[j].strip()
                if t.endswith("{"):
                    d += 1
                elif t == "}":
                    d -= 1
                    if d == 0:
                        break
                m = re.match(r"^Name: (.+)$", t)
                if m and name is None:
                    name = m.group(1)
                m = re.match(r"^Value: (\d+)$", t)
                if m and value is None:
                    value = int(m.group(1))
                m = re.match(r"^Section: .*\((\d+)\)$", t)
                if m and sec is None:
                    sec = int(m.group(1))
                j += 1
            if name and sec and sec > 0 and value is not None:
                sec_syms[sec].append((value, name))
            i = j
            continue
        i += 1
    cur = None
    for line in lines:
        s = line.strip()
        m = re.match(r"^Section \((\d+)\) (\S+) \{$", s)
        if m:
            cur = int(m.group(1))
            continue
        m = re.match(r"^0x([0-9A-Fa-f]+) (IMAGE_REL_\S+) (.*)$", s)
        if m and cur is not None:
            relocs[cur].append((int(m.group(1), 16),
                                re.sub(r" \(\d+\)$", "", m.group(3).strip())))
    out = {}
    for idx, nm in sections.items():
        if nm != ".rdata":
            continue
        for val, sname in sorted(sec_syms.get(idx, [])):
            if val == 0 and sname.startswith("??_7"):
                out[sname] = relocs.get(idx, [])
                break
    return out


class Image:
    """The retail PE, RVA-addressable, plus the ILT band.

    The slot is resolved HERE rather than read off the delinked obj's reloc, because
    a reloc name alone is a trap: when no symbol sits exactly at the destination the
    delinker emits `nearest-preceding-symbol + addend`, and the addend lives in the
    section BYTES, not in the reloc record - so `llvm-readobj` prints a confident
    name for what is really "some unrelated function, 0x40 bytes earlier". That
    fallback is what makes a ctor (`??0CResolveNode@@QAE@XZ`) or an unrelated class's
    dtor appear to be a vtable slot's identity. Only addend == 0 is an identity.
    """

    def __init__(self, exe: Path):
        import struct
        self.struct = struct
        self.data = exe.read_bytes()
        d = self.data
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        self.image_base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.secs = []
        for i in range(nsec):
            o = pe + 24 + opt + i * 40
            name = d[o:o + 8].rstrip(b"\0").decode()
            vsize, vaddr, _rsize, raddr = struct.unpack_from("<IIII", d, o + 8)
            self.secs.append((name, vaddr, vsize, raddr))
        self.ilt_lo, self.ilt_hi = self._detect_ilt()

    def off(self, rva):
        for _n, vaddr, vsize, raddr in self.secs:
            if vaddr <= rva < vaddr + vsize:
                o = raddr + (rva - vaddr)
                return o if o < len(self.data) else None
        return None

    def u32(self, rva):
        o = self.off(rva)
        return None if o is None else self.struct.unpack_from("<I", self.data, o)[0]

    def _detect_ilt(self):
        text = next((s for s in self.secs if s[0] == ".text"), None)
        if not text:
            return (0, 0)
        start, end = text[1], text[1] + text[2]
        while start < end and self.data[self.off(start)] == 0xCC:
            start += 1
        cur = start
        while cur + 5 <= end and self.data[self.off(cur)] == 0xE9:
            cur += 5
        return (start, cur)

    def resolve(self, rva):
        """Follow an ILT thunk to its body; identity otherwise."""
        if self.ilt_lo <= rva < self.ilt_hi and (rva - self.ilt_lo) % 5 == 0:
            o = self.off(rva)
            if o is not None and self.data[o] == 0xE9:
                disp = self.struct.unpack_from("<i", self.data, o + 1)[0]
                return rva + 5 + disp
        return rva


def main() -> int:
    show_all = "--all" in sys.argv
    only = None
    if "--unit" in sys.argv:
        only = sys.argv[sys.argv.index("--unit") + 1]

    if not REPORT.is_file():
        print("vtable_slot_identity: build/objdiff/report.json missing - run `gruntz build --fast`",
              file=sys.stderr)
        return 1

    import os
    exe = os.environ.get("GRUNTZ_EXE")
    if not exe or not Path(exe).is_file():
        print("vtable_slot_identity: $GRUNTZ_EXE unset/missing (enter `nix develop`)",
              file=sys.stderr)
        return 1
    img = Image(Path(exe))

    claim_at = {}          # rva -> our mangled name
    vt_rva = {}            # ??_7 name -> rva
    for r in csv.DictReader(NAMES.open()):
        rva = int(r["rva"], 16)
        if r.get("kind") == "func":
            claim_at[rva] = r["name"]
        if r["name"].startswith("??_7"):
            vt_rva[r["name"]] = rva

    report = json.loads(REPORT.read_text())
    rows = []
    family = collections.defaultdict(set)   # placeholder stem -> {classes}

    for u in report["units"]:
        if only and u["name"] != only:
            continue
        if not [s for s in u["sections"] if s["name"] == ".rdata"]:
            continue
        b = BASE_DIR / (u["name"] + ".obj")
        if not b.exists():
            continue
        for vt, brel in rdata_vtables(b).items():
            if vt not in vt_rva:
                continue
            for (bo, bn) in brel:
                if not PLACEHOLDER.match(bn):
                    continue
                family[bn.split("@")[0]].add(bn.split("@")[1] if "@" in bn else "?")
                slot = img.u32(vt_rva[vt] + bo)
                if slot is None:
                    continue
                dest = img.resolve(slot - img.image_base)
                # EXACT destination only: a claim that merely PRECEDES `dest` is the
                # delinker's addend fallback, not this slot's identity.
                tn = claim_at.get(dest)
                if tn is None:
                    kind, tn = "NO-EXACT-SYMBOL", "(dest %#08x unclaimed)" % dest
                elif tn == bn:
                    kind = "AGREES"
                else:
                    kind = "SHADOW"
                rows.append((u["name"], vt, bo, bn, tn, kind))

    counts = collections.Counter(r[5] for r in rows)
    print("placeholder-named vtable slots examined:", len(rows))
    for k, v in counts.most_common():
        print("  %5d  %s" % (v, k))
    print()

    harvest = [r for r in rows if r[5] == "SHADOW"]
    # de-dup: the same vtable is emitted into every referencing obj (COMDAT)
    uniq = {}
    for unit, vt, off, bn, tn, kind in harvest:
        uniq.setdefault((vt, off, bn, tn), unit)

    print("=== HARVESTABLE (SHADOW: destination already carries one of our claims) ===")
    print("%-34s %-8s %-40s %-46s %s" % ("vtable", "slot", "placeholder", "resolves to (our claim)", "family"))
    for (vt, off, bn, tn), unit in sorted(uniq.items()):
        stem = bn.split("@")[0]
        fam = len(family[stem])
        if not show_all and fam > 1:
            continue
        print("%-34s +%-7s %-40s %-46s %d" % (vt[:34], hex(off), bn[:40], tn[:46], fam))

    print()
    print("=== FAMILY SIZES (a virtual's name is ONE name for the whole hierarchy) ===")
    for stem, classes in sorted(family.items(), key=lambda kv: -len(kv[1])):
        flag = "  <-- shared: a per-leaf rename would misname the others" if len(classes) > 1 else ""
        print("  %-24s %d class(es)%s" % (stem.lstrip("?"), len(classes), flag))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
