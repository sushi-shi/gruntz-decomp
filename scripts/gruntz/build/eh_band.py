#!/usr/bin/env python3
"""gruntz.build.eh_band - carve retail's packed /GX EH funclet band into its owners.

WHAT THE BAND IS.  cl 5.0 compiles every `/GX` function that owns a destructible
object into two pieces: the body, and a small EXECUTE COMDAT (`.text$x`) holding
that function's EH funclets -

    <unwind funclet 0>   mov ecx,[ebp-X] ; jmp <dtor>      \\ one per unwind state,
    <unwind funclet 1>   lea ecx,[ebp-Y] ; jmp <dtor>      /  in state order
    <registration stub>  mov eax,<FuncInfo> ; jmp __CxxFrameHandler

and the function's prologue does `push OFFSET <registration stub>` to build its
EXCEPTION_REGISTRATION.  The retail linker packed every one of those COMDATs into
one contiguous band at the end of `.text` (RVA 0x1d7d00..0x1e3c25 in GRUNTZ.EXE).

WHY IT NEEDED CARVING.  No delinked object covered the band, so each of those
prologue pushes decomposed as an UNDEFINED `FUN_005exxxx` external plus a nonzero
addend: the delinked object set did not close over EH (a relink would fail), and
objdiff could only name-match the reference, never compare the funclet BYTES.

THE DERIVATION IS RETAIL-ONLY.  For every function a `src/` unit claims:
  1. scan its retail body for `push imm32` where imm32 lands on a `b8 .. e9 ..`
     stub inside `.text` - that is its registration stub;
  2. read the `FuncInfo` the stub loads (magic 0x19930520) and walk its unwind map
     for the funclet addresses (and the try-block map's catch handlers, if any);
  3. the group is [min(funclet addr), stub + 10) - the stub is always last.
Nothing here reads the base objects, so the carve states a fact about GRUNTZ.EXE,
not about our reconstruction.  It is cross-checked against the base `.text$x`
section extents by `python -m gruntz.audit.eh_band`.

THE SYMBOLS MIRROR cl's OWN LABELS.  cl puts a class-6 `$L` label on every
funclet in the COMDAT, so that is exactly what is carved - anything coarser is
truncated at the base's next label and compares against the wrong extent:

    __ehunwind$<owner>$<n>   the n-th unwind funclet, n counted in ADDRESS order
    __ehreg$<owner>          [stub, stub + 10), the registration stub

Address order is state order on both sides (cl emits the funclets in the order the
unwind map indexes them), so `$n` names the same EH state on the base and on the
target without either side reading the other's unwind map.  A reconstruction that
builds its locals in the wrong order therefore shows up as several funclets
differing, not as one silently-reordered blob.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

FUNCINFO_MAGIC = 0x19930520
PUSH_IMM32 = 0x68
MOV_EAX_IMM32 = 0xB8
JMP_REL32 = 0xE9
STUB_SIZE = 10          # `b8 imm32` + `e9 rel32`

EHREG_PREFIX = "__ehreg$"
EHUNWIND_PREFIX = "__ehunwind$"


def registration_symbol(owner: str) -> str:
    return EHREG_PREFIX + owner


def unwind_symbol(owner: str, index: int) -> str:
    return f"{EHUNWIND_PREFIX}{owner}${index}"


def is_band_symbol(name: str) -> bool:
    """True for the two names this module carves (used by the metric filters)."""
    return name.startswith(EHREG_PREFIX) or name.startswith(EHUNWIND_PREFIX)


@dataclass(frozen=True)
class Group:
    owner_rva: int
    owner: str
    unit: str
    funclets: tuple[int, ...]   # unwind funclet starts, ascending (== state order)
    stub: int                   # the pushed registration stub

    @property
    def start(self) -> int:
        return self.funclets[0] if self.funclets else self.stub

    @property
    def end(self) -> int:
        return self.stub + STUB_SIZE


class _Image:
    def __init__(self, path: Path):
        self.data = Path(path).read_bytes()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.image_base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.sections = []
        for index in range(count):
            base = pe + 24 + optional + index * 40
            name = self.data[base:base + 8].rstrip(b"\0").decode("latin-1")
            vsize, vaddr, raw_size, raw_ptr = struct.unpack_from("<IIII", self.data, base + 8)
            self.sections.append((name, vaddr, max(vsize, raw_size), raw_ptr, raw_size))
        text = next(s for s in self.sections if s[0] == ".text")
        self.text_lo, self.text_hi = text[1], text[1] + text[2]

    def read(self, rva: int, size: int) -> bytes | None:
        for _name, vaddr, vsize, raw_ptr, raw_size in self.sections:
            if vaddr <= rva and rva + size <= vaddr + vsize:
                offset = raw_ptr + (rva - vaddr)
                if offset + size > len(self.data):
                    return None
                return self.data[offset:offset + size]
        return None


def _funclets(image: _Image, funcinfo_rva: int) -> tuple[set[int], int] | None:
    header = image.read(funcinfo_rva, 28)
    if header is None:
        return None
    magic, states, unwind_map, tries, try_map, _n_ip, _p_ip = struct.unpack_from(
        "<IiIiIiI", header, 0)
    if magic != FUNCINFO_MAGIC or states < 0 or tries < 0:
        return None
    out: set[int] = set()
    if states:
        entries = image.read(unwind_map - image.image_base, 8 * states)
        if entries is None:
            return None
        for index in range(states):
            _to_state, action = struct.unpack_from("<iI", entries, index * 8)
            if action:
                out.add(action - image.image_base)
    for index in range(tries):
        block = image.read(try_map - image.image_base + index * 20, 20)
        if block is None:
            return None
        _low, _high, _catch_high, catches, handler_array = struct.unpack_from("<iiiiI", block, 0)
        table = image.read(handler_array - image.image_base, 16 * catches)
        if table is None:
            return None
        for slot in range(catches):
            _adjectives, _type, _disp, handler = struct.unpack_from("<IIiI", table, slot * 16)
            out.add(handler - image.image_base)
    return out, states


def groups(exe: Path, names_map: dict[int, tuple]) -> list[Group]:
    """Every EH funclet group owned by a function `names_map` attributes to a unit.

    ``names_map`` is synth_pdb's ``{rva: (name, unit, size, kind)}`` overlay (the
    3/4/5-tuple shapes it tolerates are all accepted; only the first three fields
    are read).  Returned groups are sorted by start RVA and never overlap.
    """
    image = _Image(exe)
    found: dict[int, Group] = {}
    for rva in sorted(names_map):
        entry = names_map[rva]
        name, unit, size = entry[0], entry[1], entry[2]
        if len(entry) > 3 and entry[3] != "func":
            continue
        if size <= 0 or not unit:
            continue
        body = image.read(rva, size)
        if body is None:
            continue
        for offset in range(len(body) - 4):
            if body[offset] != PUSH_IMM32:
                continue
            stub = struct.unpack_from("<I", body, offset + 1)[0] - image.image_base
            if not image.text_lo <= stub < image.text_hi:
                continue
            code = image.read(stub, STUB_SIZE)
            if not (code and code[0] == MOV_EAX_IMM32 and code[5] == JMP_REL32):
                continue
            parsed = _funclets(image, struct.unpack_from("<I", code, 1)[0] - image.image_base)
            if parsed is None:
                continue
            addresses, _states = parsed
            if addresses and max(addresses) >= stub:
                # A funclet at or past the registration stub would make the group
                # non-contiguous; refuse to guess an extent for it.
                continue
            found[stub] = Group(owner_rva=rva, owner=name, unit=unit,
                                funclets=tuple(sorted(addresses)), stub=stub)
    ordered = sorted(found.values(), key=lambda g: g.start)
    for previous, current in zip(ordered, ordered[1:]):
        if current.start < previous.end:
            raise ValueError(
                f"EH band groups overlap: 0x{previous.start:08x}..0x{previous.end:08x} "
                f"({previous.owner}) and 0x{current.start:08x} ({current.owner})")
    return ordered


def records(band: list[Group]) -> list[tuple[int, str, str, int]]:
    """``(rva, symbol, unit, size)`` rows for synth_pdb's names_map overlay."""
    out = []
    for group in band:
        bounds = list(group.funclets) + [group.stub]
        for index, start in enumerate(group.funclets):
            out.append((start, unwind_symbol(group.owner, index), group.unit,
                        bounds[index + 1] - start))
        out.append((group.stub, registration_symbol(group.owner), group.unit, STUB_SIZE))
    return out
