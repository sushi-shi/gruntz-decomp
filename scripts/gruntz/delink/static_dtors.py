"""Provision pinned function-local static destructors in their owning TU.

The pin gives retail extent/ownership, not a volatile cl `$E<n>` name. Recover
that build's name from the argument to the same named function's sole atexit
call. The callback is emitted as real target code: ordinary content/relocation
canonicalization still compares its body, receiver and destructor target.
Nothing here asserts byte equivalence or renames an undefined callback.
"""

from __future__ import annotations

import re
import struct
from collections import defaultdict
from pathlib import Path

from gruntz.delink.coffx import Obj
from gruntz.tool.objdump import disassemble


_HELPER = re.compile(r"^_?\$E[0-9]+$")
_INSTRUCTION = re.compile(
    r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s+)+\s*\t([^\t]+)$")
_EXECUTE = 0x20000000


def argument_site(code: bytes, call: int) -> int | None:
    """Prove a push-immediate is the sole, still-live atexit argument.

    Decode from the function's real start. Only register/data operations may
    intervene; a call, control transfer or possible stack mutation rejects it.
    This admits VC5 scheduling the guard/default stores after the push, without
    treating arbitrary nearby 0x68 bytes as instruction boundaries.
    """
    instructions = []
    for line in disassemble(code).splitlines():
        match = _INSTRUCTION.match(line)
        if match:
            instructions.append((int(match[1], 16), match[2].strip()))
    preceding = [(site, text) for site, text in instructions if site <= call]
    if not preceding or preceding[-1][0] != call or code[call] != 0xE8:
        return None
    for site, text in reversed(preceding[:-1]):
        op, _, operands = text.partition(" ")
        if op == "push":
            return site + 1 if code[site] == 0x68 else None
        if op not in {"mov", "or", "xor", "lea", "nop"}:
            return None
        if "esp" in operands or "ebp" in operands:
            # Reading an EBP value is harmless, but needs no broader decoder:
            # exclude only EBP/ESP destinations and any stack-relative access.
            destination = operands.split(",", 1)[0].strip()
            if destination in {"esp", "ebp"} or "[esp" in operands or "[ebp" in operands:
                return None
    return None


def callback_size(code: bytes) -> int | None:
    """Two complete VC5 local-static teardown forms, with only tail padding."""
    body = code.rstrip(b"\x90\xcc")
    if body == b"\xc3":
        return 1
    if len(body) == 10 and body[0] == 0xB9 and body[5] == 0xE9:
        return 10
    return None


def _base_callbacks(obj: Obj, owners: set[str]) -> dict[str, tuple[str, int]]:
    definitions = defaultdict(list)
    for index, value, section in obj.iter_symbols():
        if section > 0:
            definitions[obj.sym_name(index)].append((value, section))
    result = {}
    for owner in owners:
        entries = definitions.get(owner, ())
        if len(entries) != 1:
            continue
        start, section = entries[0]
        if not obj.section_table[section - 1]["characteristics"] & _EXECUTE:
            continue
        payload = obj.section_payload(section)
        following = [offset for offset, _name in obj.defined_symbols(section) if offset > start]
        end = min(following, default=len(payload))
        relocs = obj.typed_relocations(section)
        calls = [site for site, target in relocs.items()
                 if start < site < end and target == ("_atexit", 0x14)]
        if len(calls) != 1:
            continue
        operand = argument_site(payload[start:end], calls[0] - start - 1)
        if operand is None:
            continue
        operand += start
        name, typ = relocs.get(operand, ("", 0))
        if typ != 6 or not _HELPER.fullmatch(name):
            continue
        if struct.unpack_from("<I", payload, operand)[0] != 0:
            continue
        helpers = definitions.get(name, ())
        if len(helpers) != 1:
            continue
        offset, helper_section = helpers[0]
        if not obj.section_table[helper_section - 1]["characteristics"] & _EXECUTE:
            continue
        extent = callback_size(obj.section_payload(helper_section)[offset:])
        if extent is not None:
            result[owner] = (name, extent)
    return result


def provision(model, names_map: dict, base_dir: Path, image) -> dict[int, tuple[str, str, int]]:
    """Derive transient COFF names; unproved, ambiguous or unpinned sites stay out."""
    pins = {b.rva: b for b in model.functions if b.channel == "src_dyninit"}
    atexit = {b.rva for b in model.functions if b.name == "_atexit"}
    if not pins or not atexit:
        return {}
    owners_by_unit = defaultdict(dict)
    for rva, (name, unit, size) in names_map.items():
        if unit in {pin.unit for pin in pins.values()} and size > 0:
            owners_by_unit[unit][name] = (rva, size)

    def is_atexit(target: int) -> bool:
        seen = set()
        while target not in seen:
            if target in atexit:
                return True
            seen.add(target)
            code = image.pe.read(target, 5)
            if not code or code[0] != 0xE9:
                return False
            target += 5 + struct.unpack_from("<i", code, 1)[0]
        return False

    result = {}
    for unit, owners in owners_by_unit.items():
        path = base_dir / f"{unit}.obj"
        if not path.is_file():
            continue
        callbacks = _base_callbacks(Obj(path), set(owners))
        for owner, (name, _base_size) in callbacks.items():
            rva, size = owners[owner]
            code = image.pe.read(rva, size)
            if not code:
                continue
            calls = [i for i in range(len(code) - 4)
                     if code[i] == 0xE8 and is_atexit(
                         rva + i + 5 + struct.unpack_from("<i", code, i + 1)[0])]
            if len(calls) != 1:
                continue
            operand = argument_site(code, calls[0])
            if operand is None or rva + operand not in image.reloc_sites:
                continue
            target = struct.unpack_from("<I", code, operand)[0] - image.image_base
            pin = pins.get(target)
            if pin is None or pin.unit != unit or target in names_map:
                continue
            body = image.pe.read(target, pin.size)
            extent = callback_size(body) if body else None
            if extent is None:
                continue
            entry = (name, unit, extent)
            if target in result and result[target] != entry:
                raise ValueError(f"ambiguous pinned static destructor at 0x{target:x}")
            result[target] = entry
    return result
