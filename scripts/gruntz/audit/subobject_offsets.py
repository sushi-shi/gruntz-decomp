#!/usr/bin/env python3
"""subobject_offsets.py - the retail SUB-OBJECT PLACEMENT oracle for class layout.

`gruntz.audit.alloc_size` pins a class's TOTAL size from `push <n>; call ??2`.
A class can have exactly the right total size and still be wrong INSIDE - a
member declared 0x20 too early, a base sub-object placed where a member belongs.
Retail states every one of those offsets out loud, in the constructor:

    lea  ecx, [esi+0x38]        <- the sub-object's offset in `this`
    call ??0CMotionState@@QAE@XZ   <- and its TYPE

That pair is ground truth for "class C has a `CMotionState` at +0x38", and it is
the same evidence that resolved the CProjectile report (retail's `lea edi,[esi+0x38]`
is CMotionState, exactly where our header puts it - the `[esi+0x18]` we emit next
to it is `CUserBaseLink m_link`, from an INLINED base ctor retail calls out of line).
Three shapes are read, all inside a `??0C`/`??1C` body:

    base/member ctor   `lea ecx,[this+N]; call ??0Y`   -> Y sub-object at +N
    base/member dtor   `lea ecx,[this+N]; call ??1Y`   -> Y sub-object at +N
    secondary vtable   `mov [this+N], ??_7Y@@6B@`      -> Y base sub-object at +N
                                                          (N=0 is the primary vptr)

Each observation is checked against `build/gen/structs.json` - clang's real
layout of OUR class, with base fields flattened to absolute offsets. Two verdicts:

    NOT-A-FIELD   retail constructs a sub-object at an offset that is not any
                  field boundary in our layout. Our members above it are wrong.
    TYPE          the offset IS a field boundary but holds a different type.
                  Usually benign (the field is the sub-object's own first member,
                  flattened) - reported quietly, and only when the sub-object type
                  is one we model.

NOT A GATE. Known-innocent sources of a TYPE row: a base whose fields we flattened
(the sub-object type does not appear as a field NAME at all), a `char[N]` pad
standing in for an unmodelled member, and MSVC's empty-base placement.

    python -m gruntz.audit.subobject_offsets            # the mismatch report
    python -m gruntz.audit.subobject_offsets --all      # every observation
    python -m gruntz.audit.subobject_offsets --class C
"""
from __future__ import annotations

import argparse
import json
import re
import struct
from collections import defaultdict
from pathlib import Path

from gruntz.core import get_context
from gruntz.core.pe import ILT_HI, ILT_LO, IMAGEBASE

REPO = Path(__file__).resolve().parents[3]
_STRUCTS = REPO / "build/gen/structs.json"

_CTOR = re.compile(r"^\?\?0([A-Za-z_]\w*)@@")
_DTOR = re.compile(r"^\?\?1([A-Za-z_]\w*)@@")
# `??_7Derived@@6B@` is the primary vtable; `??_7Derived@@6BBase@@@` is the one
# for the `Base` SUB-OBJECT, and the sub-object is what a stamp at a nonzero
# offset places - so read the BASE name when it is present, not the derived one.
_VTBL = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B(?:([A-Za-z_]\w*)@@)?@?$")

# How far after a `lea` the `call` may sit. cl emits `lea ecx,[esi+N]; call` back
# to back, or spills the address through one register first; 16 bytes covers both
# and is tight enough that a stray 0x8D byte cannot manufacture a pair.
_LEA_TO_CALL = 16


def _sub_type(t):
    """`CMotionState` from a declared type string; None for non-class types."""
    t = t.strip()
    if t.endswith("*") or "[" in t or " " in t:
        # `CFoo *`, `char[4]`, `unsigned int` - not a by-value class sub-object
        if t.endswith("*") or "[" in t:
            return None
    return t if re.fullmatch(r"[A-Za-z_]\w*", t) else None


_BASES = re.compile(
    r"\b(?:class|struct)\s+([A-Za-z_]\w*)\s*(?:final\s*)?:\s*([^{;]+)\{")


def _base_map():
    """{class: {direct base names}} scanned lexically from src/ + include/.

    structs.json records no inheritance at all (clang's field dump FLATTENS bases
    into absolute offsets), so this is the only way to tell "retail constructs a
    `StreamFeeder` at +0x6c, we declare a `StreamVoiceFeeder` there" from a real
    disagreement - `StreamVoiceFeeder : StreamFeeder`, so they are the same bytes.
    Lexical, deliberately: it only ever SUPPRESSES a row.
    """
    out = defaultdict(set)
    for root in ("src", "include"):
        for p in (REPO / root).rglob("*"):
            if p.suffix not in (".h", ".cpp"):
                continue
            try:
                txt = p.read_text(errors="ignore")
            except OSError:
                continue
            for m in _BASES.finditer(txt):
                for tok in re.split(r",", m.group(2)):
                    tok = re.sub(r"\b(public|private|protected|virtual)\b", "", tok)
                    tok = tok.strip()
                    if re.fullmatch(r"[A-Za-z_]\w*", tok):
                        out[m.group(1)].add(tok)
    return out


class Layout:
    """Our side: {class: {offset: (name, type)}} plus sizes, from structs.json."""

    def __init__(self):
        self.fields, self.size = {}, {}
        self.bases = _base_map()
        if not _STRUCTS.is_file():
            return
        for e in json.load(_STRUCTS.open()):
            n = e["name"]
            if n in self.fields:
                continue
            self.size[n] = e.get("size")
            self.fields[n] = {f["offset"]: (f["name"], f["type"])
                              for f in e.get("fields", [])}

    def at(self, cls, off):
        f = self.fields.get(cls)
        return None if f is None else f.get(off)

    def is_base_of(self, base, derived, depth=0):
        if base == derived:
            return True
        if depth > 6:
            return False
        return any(self.is_base_of(base, b, depth + 1)
                   for b in self.bases.get(derived, ()))

    def shifted_match(self, cls, off, sub):
        """Every field `sub` declares at relative r reappears at off+r in `cls`.

        The positive form of the placement check: a sub-object at +N shows up in
        a flattened layout as its own fields shifted by N, whether or not N itself
        is a field boundary (a polymorphic sub-object's +0 is only its vptr).
        """
        s, c = self.fields.get(sub), self.fields.get(cls)
        if not s or not c:
            return False
        return all(off + r in c for r in s)

    def leads_with(self, outer, inner, depth=0):
        """`inner` is `outer`, its base, or `outer`'s member at offset 0
        (recursively) - i.e. a sub-object of type `inner` really does start where
        an `outer` starts. `CUserBaseLink` leads with `zBitVec m_str`."""
        if depth > 4 or not outer:
            return False
        if self.is_base_of(inner, outer):
            return True
        f = self.fields.get(outer)
        if not f:
            return False
        first = f.get(0)
        return bool(first) and self.leads_with(_sub_type(first[1]) or "",
                                               inner, depth + 1)


class Sweep:
    def __init__(self, ctx=None):
        self.ctx = ctx or get_context()
        self.pe = self.ctx.pe
        self.sym = self.ctx.symbols
        _n, self.tva, self.tvsz, trp, trsz = self.pe.text
        self.tb = self.pe.data[trp:trp + trsz]

    def _callee(self, p):
        rel = struct.unpack_from("<i", self.tb, p - self.tva + 1)[0]
        tgt = p + 5 + rel
        it = self.pe.ilt_target(tgt)
        return it if it is not None else tgt

    def _name(self, rva):
        return self.sym.name_of(rva)[0]

    def bodies(self):
        """[(rva, size, class, kind)] for every retail ctor/dtor with an extent."""
        out = []
        for rva, (nm, _u) in self.sym.names.items():
            if ILT_LO <= rva < ILT_HI:
                continue
            m = _CTOR.match(nm) or _DTOR.match(nm)
            if not m:
                continue
            sz = self.sym.fsize.get(rva)
            if not sz:
                continue
            out.append((rva, sz, m.group(1), "ctor" if nm[2] == "0" else "dtor"))
        return sorted(out)

    def observe(self, rva, size):
        """[(kind, offset, type_name, site)] sub-objects placed by this body.

        `this` starts in ecx and is tracked through `mov reg, ecx` copies; a call
        clobbers eax/ecx/edx. Only `[thisreg + N]` forms are read, so a `lea` off
        some other pointer can never be mistaken for a sub-object of `this`.
        """
        out = []
        live = {1}                       # ecx holds `this` on entry (__thiscall)
        pend = {}                        # reg -> (offset, site) from a recent lea
        p, end = rva, rva + size
        while p < end:
            o = p - self.tva
            if o + 7 >= len(self.tb):
                break
            by = self.tb[o]
            if by == 0x8B and (self.tb[o + 1] & 0xC0) == 0xC0:
                dst, src = (self.tb[o + 1] >> 3) & 7, self.tb[o + 1] & 7
                if src in live:
                    live.add(dst)
                    pend.pop(dst, None)
                elif src in pend:
                    pend[dst] = pend[src]
                else:
                    live.discard(dst)
                    pend.pop(dst, None)
                p += 2
                continue
            if by == 0x8D:               # lea r32, [r/m]
                m = self.tb[o + 1]
                mod, reg, rm = m >> 6, (m >> 3) & 7, m & 7
                if rm != 4 and mod != 3:
                    if mod == 0 and rm != 5:
                        disp, ln = 0, 2
                    elif mod == 1:
                        disp, ln = self.tb[o + 2], 3
                    elif mod == 2:
                        disp, ln = struct.unpack_from("<i", self.tb, o + 2)[0], 6
                    else:
                        disp, ln = None, 2
                    if disp is not None and rm in live:
                        pend[reg] = (disp, p)
                        live.discard(reg)
                        p += ln
                        continue
                    if disp is not None:
                        pend.pop(reg, None)
                        live.discard(reg)
                        p += ln
                        continue
                p += 2
                continue
            if by == 0xE8:
                tgt = self._callee(p)
                nm = self._name(tgt)
                m = _CTOR.match(nm) or _DTOR.match(nm)
                if m:
                    if 1 in pend and p - pend[1][1] <= _LEA_TO_CALL:
                        out.append(("sub", pend[1][0], m.group(1), p))
                    elif 1 in live:
                        out.append(("sub", 0, m.group(1), p))
                live -= {0, 1, 2}
                pend = {k: v for k, v in pend.items() if k not in (0, 1, 2)}
                p += 5
                continue
            if by == 0xC7:               # mov [r/m], imm32
                m = self.tb[o + 1]
                mod, reg, rm = m >> 6, (m >> 3) & 7, m & 7
                if reg == 0 and rm != 4 and mod != 3:
                    if mod == 0 and rm != 5:
                        disp, ln = 0, 6
                    elif mod == 1:
                        disp, ln = self.tb[o + 2], 7
                    elif mod == 2:
                        disp, ln = struct.unpack_from("<i", self.tb, o + 2)[0], 10
                    else:
                        disp, ln = None, 6
                    if disp is not None:
                        imm = struct.unpack_from("<I", self.tb, o + ln - 4)[0]
                        g = self.sym.gsyms.get(imm - IMAGEBASE) or ""
                        mm = _VTBL.match(g)
                        if mm and rm in live:
                            out.append(("vptr", disp,
                                        mm.group(2) or mm.group(1), p))
                        p += ln
                        continue
                p += 2
                continue
            p += 1
        return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--class", dest="klass")
    a = ap.parse_args()

    sw, lay = Sweep(), Layout()
    if not lay.fields:
        print("[subobject_offsets] build/gen/structs.json missing - run "
              "`gruntz structs` first")
        return 2

    seen = defaultdict(set)        # class -> {(kind, off, type)}
    for rva, size, cls, kind in sw.bodies():
        if a.klass and cls != a.klass:
            continue
        for k, off, ty, site in sw.observe(rva, size):
            seen[cls].add((k, off, ty, rva))

    notfield, typerow, unmodelled, blind, ok = [], [], [], [], []
    for cls in sorted(seen):
        flds = lay.fields.get(cls)
        if flds is None:
            unmodelled.append(cls)
            continue
        for k, off, ty, rva in sorted(seen[cls], key=lambda r: r[1]):
            if off == 0:
                ok.append((cls, k, off, ty, None))
                continue
            hit = flds.get(off)
            if hit is None:
                # structs.json BLIND SPOTS - three shapes it structurally cannot
                # show, so absence there is not evidence of a layout bug:
                #   * a secondary base's VPTR slot (a base sub-object is flattened
                #     into absolute field offsets; its vptr is not a field);
                #   * an EMPTY class member (no fields => the generator emits no
                #     record for the type at all, e.g. CButeMgr's CButeTail);
                #   * a POLYMORPHIC sub-object, whose first four bytes are again
                #     its vptr - so its fields land at off+4, off+8, ... and off
                #     itself is bare. Verified positively: every one of Y's own
                #     field offsets must reappear, shifted by off, in C.
                if k == "vptr" or ty not in lay.fields:
                    blind.append((cls, k, off, ty, rva))
                elif lay.shifted_match(cls, off, ty):
                    ok.append((cls, k, off, ty, ("<vptr slot>", ty)))
                else:
                    notfield.append((cls, k, off, ty, rva, lay.size.get(cls)))
            elif lay.leads_with(_sub_type(hit[1]) or "", ty):
                ok.append((cls, k, off, ty, hit))
            elif ty in lay.fields and _sub_type(hit[1]) not in (ty, None):
                typerow.append((cls, k, off, ty, hit))
            else:
                ok.append((cls, k, off, ty, hit))

    if notfield:
        print(f"=== NOT A FIELD BOUNDARY  ({len(notfield)}) - retail places a "
              f"sub-object where our layout has no field")
        for cls, k, off, ty, rva, sz in notfield:
            tail = "  (PAST our sizeof)" if sz is not None and off >= sz else ""
            print(f"  {cls:<30} +0x{off:<5x} {ty:<26} [{k}] "
                  f"in {sw._name(rva)}{tail}")
    if typerow:
        print(f"\n=== TYPE AT A KNOWN OFFSET  ({len(typerow)}) - the offset is a "
              f"field, but of another modelled class")
        for cls, k, off, ty, hit in typerow:
            print(f"  {cls:<30} +0x{off:<5x} retail {ty:<24} ours "
                  f"{hit[1]} {hit[0]}  [{k}]")
    if blind:
        print(f"\n=== structs.json BLIND SPOT  ({len(blind)}) - a secondary-base "
              f"vptr slot or an empty-class member; not checkable, not a bug")
        for cls, k, off, ty, rva in blind:
            print(f"  {cls:<30} +0x{off:<5x} {ty:<26} [{k}]")
    if unmodelled:
        print(f"\n=== CLASS NOT IN structs.json  ({len(unmodelled)})")
        print("  " + ", ".join(unmodelled))
    if a.all:
        print(f"\n=== AGREES  ({len(ok)})")
        for cls, k, off, ty, hit in ok:
            print(f"  {cls:<30} +0x{off:<5x} {ty:<26} [{k}]")

    print(f"\n{len(seen)} classes with ctor/dtor sub-object evidence; "
          f"{len(notfield)} NOT-A-FIELD, {len(typerow)} type rows, {len(blind)} blind, "
          f"{len(ok)} agree, {len(unmodelled)} classes unmodelled.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
