#!/usr/bin/env python3
"""this_offsets.py - the retail FIELD-ACCESS oracle for class layout.

The third and widest layout yardstick, after `alloc_size` (total size, from
`push <n>; call ??2`) and `subobject_offsets` (sub-object placement, from
`lea ecx,[this+N]; call ??0Y`). Those two see only allocation and construction
sites. This one reads what every METHOD does: each `[this + N]` in the body of
`?Member@Class@@...AE...` is retail asserting that class `Class` has something
live at offset N.

Four verdicts, all structural:

    PAST-SIZEOF  N is at or beyond our sizeof(Class). Either the class is short
                 or - much more often - the METHOD is misattributed and really
                 belongs to a bigger class. `?IsAtSavedScreenPos@CUserLogic@@`
                 read +0x17c/+0x180 out of a 0x34-byte CUserLogic: those are
                 `CGrunt::m_lastTilePx`, and the body carried a
                 `static_cast<CGrunt*>(this)` to hide it.
    HOLE         N is inside the object but inside no declared field. Our members
                 above it are wrong, or a member is missing.
    STRADDLE     the ACCESS WIDTH runs off the end of the field at N - the field
                 boundary is in the wrong place even though N is a boundary.
    STRIDE       retail subscripts a member array with a scale the declared
                 element size is not divisible by, i.e. the element TYPE is wrong.

`this` is tracked from ecx (every `AE` mangling is __thiscall) through
register-to-register copies; eax/ecx/edx die at every call, and any other write
to a tracked register drops it, so an inlined callee walking a member pointer is
never read as `this`.

Field EXTENTS come from build/gen/structs.json plus a primitive-size table; a
field whose type we cannot size is stretched to the next field's offset, which
can only SUPPRESS a hole, never invent one.

    python -m gruntz.audit.this_offsets              # the report
    python -m gruntz.audit.this_offsets --class CFoo # one class, every offset
    python -m gruntz.audit.this_offsets --holes      # HOLE rows too (noisier)

Whole-image 2026-08-08: 3802 bodies, 11332 owned `[this+N]` accesses, all four
verdicts EMPTY.

NOT A GATE. Innocent HOLE sources: a `char[N]` pad we sized short, a union we
modelled as one arm, and the 4 bytes of a vptr in a class whose polymorphism we
have not realized yet.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import tempfile
from collections import defaultdict
from pathlib import Path

from gruntz.core import get_context
from gruntz.core.pe import ILT_HI, ILT_LO

REPO = Path(__file__).resolve().parents[3]
_STRUCTS = REPO / "build/gen/structs.json"

# `?Member@Class@@<access><cv>AE...` - the trailing `AE` of the access code is
# MSVC's __thiscall marker, which is what guarantees ecx holds `this` on entry.
# `?Member@Class@@...AE...`, plus the special names that carry the class
# DIRECTLY after the operator code and so have no leading `@`: `??0Class@@` (ctor),
# `??1Class@@` (dtor), `??_GClass@@` / `??_EClass@@` (deleting destructors),
# `??4Class@@` (operator=). Leaving those out dropped ~700 bodies - and a ctor is
# where a class's field offsets are most densely written.
_MEMBER = re.compile(r"^\?(?:\?_?[0-9A-Z])?([A-Za-z_]\w*)@@[A-Z]*AE"
                     r"|^\?\??[A-Za-z_0-9~]*@([A-Za-z_]\w*)@@[A-Z]*AE")
_REGS = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
_MEM = re.compile(r"(?:(BYTE|WORD|DWORD|QWORD|TBYTE|XMMWORD) PTR )?"
                  r"\[(e[a-z][a-z])((?:[+-])0x[0-9a-f]+)?\]")
_WIDTH = {"BYTE": 1, "WORD": 2, "DWORD": 4, "QWORD": 8, "TBYTE": 10,
          "XMMWORD": 16}
# `[this + idx*scale + disp]` - a subscripted MEMBER ARRAY. The scale IS retail's
# `sizeof(element)`, so it checks the array's element type independently of the
# array's offset (see docs: a member array is a container object, and one wrong
# element size shifts every later element without moving the array itself).
_SIB = re.compile(r"\[(e[a-z][a-z])\+e[a-z][a-z]\*(\d)((?:[+-])0x[0-9a-f]+)?\]")
_LINE = re.compile(r"^\s*([0-9a-f]+):\t([0-9a-f ]+)\t(\S+)\s*(.*)$")

_PRIM = {"i8": 1, "u8": 1, "char": 1, "BYTE": 1, "bool": 1, "_Bool": 1,
         "i16": 2, "u16": 2, "short": 2, "WORD": 2,
         "i32": 4, "u32": 4, "int": 4, "long": 4, "DWORD": 4, "UINT": 4,
         "LONG": 4, "ULONG": 4, "HRESULT": 4, "BOOL": 4, "COLORREF": 4,
         "i64": 8, "u64": 8, "float": 4, "double": 8}


def disassemble():
    """{rva: (mnemonic, operands)} for the whole .text, one objdump pass."""
    ctx = get_context()
    _n, tva, _vsz, rp, rsz = ctx.pe.text
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(ctx.pe.data[rp:rp + rsz])
        path = f.name
    try:
        out = subprocess.run(
            ["objdump", "-D", "-b", "binary", "-m", "i386", "-Mintel",
             f"--adjust-vma=0x{tva:x}", path],
            capture_output=True, text=True).stdout
    finally:
        Path(path).unlink(missing_ok=True)
    insn = {}
    for line in out.splitlines():
        m = _LINE.match(line)
        if m:
            insn[int(m.group(1), 16)] = (m.group(3), m.group(4))
    return insn


_VTOFF = None


def _vtable_offsets():
    """{class: {base_off, ...}} - every vtable slot retail emits for a class,
    primary (0) and MI-secondary, UNIONED UP THE BASE CHAIN.

    A derived class inherits its base's secondary vptr slot at the same offset
    but does not always own a catalog row for it, so the union is what makes the
    slot visible (CBSecStream/CButeTree/CButeNode all carry zPTree's zPtrColl
    vptr at +8).
    """
    global _VTOFF
    if _VTOFF is None:
        try:
            from gruntz.audit.subobject_offsets import _base_map
            from gruntz.core.vtable_hierarchy import build_registry
            own = {n: set(ci.vtables) for n, ci in build_registry()[0].items()}
            bases = _base_map()

            def up(n, depth=0):
                s = set(own.get(n, ()))
                if depth < 6:
                    for b in bases.get(n, ()):
                        s |= up(b, depth + 1)
                return s
            _VTOFF = {n: up(n) for n in set(own) | set(bases)}
        except Exception:
            _VTOFF = {}
    return _VTOFF


class Layout:
    """Declared byte coverage per class, from structs.json."""

    def __init__(self):
        self.size, self.cover, self.fields = {}, {}, {}
        if not _STRUCTS.is_file():
            return
        raw = {}
        for e in json.load(_STRUCTS.open()):
            raw.setdefault(e["name"], e)
        self._raw = raw
        for name, e in raw.items():
            self.size[name] = e.get("size") or 0
            self.fields[name] = {f["offset"]: (f["name"], f["type"])
                                 for f in e.get("fields", [])}
            self.cover[name] = self._coverage(e)

    def _tsize(self, t, depth=0):
        t = re.sub(r"\b(const|struct|class|unsigned|signed)\b", "", t or "").strip()
        n = 1
        m = re.search(r"\[(\d*)\]", t)
        if m:
            n = int(m.group(1) or 0) or 0
            t = t[:m.start()].strip()
        if t.endswith("*") or "(" in t:
            base = 4
        elif t in _PRIM:
            base = _PRIM[t]
        elif depth < 4 and t in getattr(self, "_raw", {}):
            base = self._raw[t].get("size") or 0
        else:
            return None
        return base * n if n else None

    def elem_size(self, t):
        """sizeof(element) for an array type string, else None."""
        m = re.search(r"\[(\d*)\]", t or "")
        if not m:
            return None
        return self._tsize(t[:m.start()].strip())

    def extents(self, name):
        """[(start, end, field, type)] sorted; unions keep the WIDEST arm."""
        e = getattr(self, "_raw", {}).get(name)
        if not e:
            return []
        flds = sorted(e.get("fields", []), key=lambda f: f["offset"])
        size = e.get("size") or 0
        out = {}
        for i, f in enumerate(flds):
            off = f["offset"]
            n = self._tsize(f["type"])
            if n is None:
                nxt = flds[i + 1]["offset"] if i + 1 < len(flds) else size
                n = max(nxt - off, 4)
            cur = out.get(off)
            if cur is None or n > cur[1] - off:
                out[off] = (off, off + n, f["name"], f["type"])
        if flds and flds[0]["offset"] >= 4:
            out.setdefault(0, (0, 4, "<vptr>", "void *"))
        return sorted(out.values())

    def _coverage(self, e):
        """Byte set the declared members occupy, plus the VPTR SLOTS.

        A vptr is never a field in clang's dump, so every polymorphic class - and
        every class with a polymorphic base sub-object - has 4 bytes that no field
        covers. Those are not holes. The primary one is at 0 (visible as "the first
        field starts at 4", or as a class with no fields at all, like CObject);
        the secondary ones sit at each MI base offset, which retail's own vtable
        catalog states (`??_7C@@6BBase@@@` carries its base_off).
        """
        flds = sorted(e.get("fields", []), key=lambda f: f["offset"])
        size = e.get("size") or 0
        cov = set()
        if size >= 4 and (not flds or flds[0]["offset"] >= 4):
            cov |= set(range(0, 4))
        for off in _vtable_offsets().get(e["name"], ()):
            cov |= set(range(off, off + 4))
        for i, f in enumerate(flds):
            off = f["offset"]
            n = self._tsize(f["type"])
            if n is None:                    # unsizable: stretch to the next field
                nxt = flds[i + 1]["offset"] if i + 1 < len(flds) else size
                n = max(nxt - off, 4)
            cov |= set(range(off, off + n))
        return cov


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--class", dest="klass")
    ap.add_argument("--holes", action="store_true", help="report HOLE rows too")
    ap.add_argument("--min-hole", type=int, default=1,
                    help="only classes with at least this many hole offsets")
    a = ap.parse_args()

    ctx = get_context()
    sym = ctx.symbols
    lay = Layout()
    if not lay.size:
        print("[this_offsets] build/gen/structs.json missing - run `gruntz structs`")
        return 2
    insn = disassemble()

    past = defaultdict(lambda: defaultdict(set))   # cls -> off -> {fn rva}
    hole = defaultdict(lambda: defaultdict(set))
    strad = defaultdict(lambda: defaultdict(set))  # cls -> (off,w) -> {fn rva}
    stride = defaultdict(lambda: defaultdict(set))  # cls -> (off,scale)
    ext = {}
    nfn = 0
    for rva, (nm, _u) in sorted(sym.names.items()):
        if ILT_LO <= rva < ILT_HI:
            continue
        m = _MEMBER.match(nm)
        if not m:
            continue
        cls = m.group(1) or m.group(2)
        if a.klass and cls != a.klass:
            continue
        if cls not in lay.size or not lay.size[cls]:
            continue
        size = sym.fsize.get(rva)
        if not size or rva not in insn:
            continue
        nfn += 1
        live = {"ecx"}
        p, end = rva, rva + size
        while p < end and p in insn:
            mn, ops = insn[p]
            for wtok, reg, disp in _MEM.findall(ops):
                if reg in live:
                    off = int(disp, 16) if disp else 0
                    if off < 0:
                        continue
                    if off >= lay.size[cls]:
                        past[cls][off].add(rva)
                        continue
                    if off not in lay.cover[cls]:
                        hole[cls][off].add(rva)
                        continue
                    w = _WIDTH.get(wtok)
                    if not w or mn == "lea":
                        continue
                    if cls not in ext:
                        ext[cls] = lay.extents(cls)
                    fld = None
                    for st, en, fn_, ft in ext[cls]:
                        if st <= off < en:
                            fld = (st, en, fn_, ft)
                    if fld and off + w > fld[1]:
                        strad[cls][(off, w)].add(rva)
            for reg, sc, disp in _SIB.findall(ops):
                if reg not in live:
                    continue
                off, sc = (int(disp, 16) if disp else 0), int(sc)
                if off < 0 or off >= lay.size[cls] or sc == 1:
                    continue
                if cls not in ext:
                    ext[cls] = lay.extents(cls)
                fld = None
                for st, en, fn_, ft in ext[cls]:
                    if st <= off < en:
                        fld = (st, en, fn_, ft)
                if fld is None:
                    continue
                es = lay.elem_size(fld[3])
                # The SIB scale is a FACTOR of the element size, not the element
                # size: cl strength-reduces `i * 104` to `(i * 13) * 8`, so every
                # array in the tree shows a scale of 1/2/4/8 (104=13*8, 568=71*8,
                # 100=25*4, 28=7*4, 24=3*8). Only a scale the element size is NOT
                # divisible by can be a real element-type disagreement.
                if es and es % sc:
                    stride[cls][(off, sc)].add((rva, fld[2], fld[3], es))
            # register liveness
            if mn == "call":
                live -= {"eax", "ecx", "edx"}
            elif mn == "mov" and "," in ops:
                dst, src = [x.strip() for x in ops.split(",", 1)]
                if dst in _REGS:
                    if src in live:
                        live.add(dst)
                    else:
                        live.discard(dst)
            elif mn in ("pop", "lea", "add", "sub", "xor", "or", "and", "inc",
                        "dec", "imul", "shl", "shr", "sar", "neg", "not",
                        "movsx", "movzx", "sbb", "adc"):
                dst = ops.split(",", 1)[0].strip()
                live.discard(dst)
            # next instruction: objdump's own successive addresses
            nxt = p + 1
            while nxt < end and nxt not in insn:
                nxt += 1
            p = nxt

    if past:
        print(f"=== PAST SIZEOF  ({len(past)} classes) - a method reads `this` "
              f"beyond the class's proven size")
        for cls in sorted(past, key=lambda c: -max(past[c])):
            offs = sorted(past[cls])
            fns = sorted({f for s in past[cls].values() for f in s})
            print(f"  {cls:<28} sizeof 0x{lay.size[cls]:x}  offsets "
                  f"{', '.join(f'0x{o:x}' for o in offs[:10])}"
                  f"{' ...' if len(offs) > 10 else ''}")
            for f in fns[:6]:
                print(f"        {sym.name_of(f)[0]}")
    if a.holes and hole:
        rows = [(c, sorted(o)) for c, o in hole.items()
                if len(o) >= a.min_hole]
        print(f"\n=== HOLE  ({len(rows)} classes) - an offset inside the object "
              f"that no declared member covers")
        for cls, offs in sorted(rows, key=lambda r: -len(r[1])):
            print(f"  {cls:<28} sizeof 0x{lay.size[cls]:x}  "
                  f"{len(offs)} offsets: "
                  f"{', '.join(f'0x{o:x}' for o in offs[:14])}"
                  f"{' ...' if len(offs) > 14 else ''}")

    if strad:
        rows = sorted(strad.items(), key=lambda r: -len(r[1]))
        print(f"\n=== STRADDLE  ({len(rows)} classes) - an access whose WIDTH runs "
              f"off the end of the field we declare at that offset")
        for cls, hits in rows:
            for (off, w), fns in sorted(hits.items()):
                fld = next((f for f in lay.extents(cls) if f[0] <= off < f[1]), None)
                d = (f"{fld[3]} {fld[2]} @0x{fld[0]:x}..0x{fld[1]:x}"
                     if fld else "?")
                print(f"  {cls:<26} read {w}B @+0x{off:<5x} but ours is {d}")
                for f in sorted(fns)[:3]:
                    print(f"        {sym.name_of(f)[0]}")

    if stride:
        print(f"\n=== ARRAY STRIDE  ({len(stride)} classes) - retail subscripts a "
              f"member array with an element size we do not declare")
        for cls, hits in sorted(stride.items()):
            for (off, sc), info in sorted(hits.items()):
                rva, fname, ftype, es = sorted(info)[0]
                print(f"  {cls:<26} +0x{off:<5x} retail stride {sc}  ours "
                      f"{ftype} {fname} (element {es})")
                for r, _f, _t, _e in sorted(info)[:3]:
                    print(f"        {sym.name_of(r)[0]}")

    nholes = sum(len(v) for v in hole.values())
    print(f"\n{nfn} thiscall member bodies read; "
          f"{len(past)} classes with PAST-SIZEOF access, "
          f"{len(hole)} classes with {nholes} hole offsets"
          f"{'' if a.holes else ' (--holes to list)'}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
