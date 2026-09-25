"""Bounded production/original controls for combat helper composition.

Check flash arithmetic through its window stores, allowing only the measured
six stack-home displacement changes; four named reference identities and four
literal payloads (not final literal placement); and the occupancy RMW/reload/store
protocol. This does not certify either complete caller or occupancy indexing.
"""

from dataclasses import replace
import re
import struct
import unittest

from test_ambient_scaler_consumers import facts
from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer, production

LOAD = '?LoadGruntCombatAnimations@CGrunt@@QAEHW4PickupType@@HHHHHH0@Z'
STEP = '?StepBehavior@CGrunt@@UAEXPAD@Z'


class LiteralResolver:
    """Use actual COFF literal payloads; never invent their linked addresses."""

    def __init__(self, resolver, obj):
        self.base, self.img, self.obj = resolver, resolver.img, obj

    def chase(self, address):
        return self.base.chase(address)

    def resolve_base(self, name, kind, addend):
        if not name.startswith(('??_C@', '$T')):
            return self.base.resolve_base(name, kind, addend)
        require(kind == 6 and addend == 0, 'literal relocation kind/addend changed')
        found = [(section, offset) for index, offset, section in self.obj.iter_symbols()
                 if section > 0 and self.obj.sym_name(index) == name]
        require(len(found) == 1, 'missing/ambiguous emitted literal')
        section, offset = found[0]
        if name.startswith('??_C@'):
            return {('string', self.obj.cstring(section, offset))}
        return {('fp64', self.obj.section_payload(section)[offset:offset + 8])}


def clipped(code, start, end):
    return Consumer(code.label, code.data[start:end],
                    tuple((site - start, kind, name) for site, kind, name in code.references
                          if start <= site < end), code.rva + start, code.pe)


def flash_phase(code, resolver):
    anchor = bytes.fromhex('68 88 13 00 00')
    require(code.data.count(anchor) == 1, 'missing/ambiguous flash denominator default')
    start = code.data.index(anchor) - 0x27
    phase = clipped(code, start, start + 0x89)
    masked, refs = facts(phase, resolver)
    if phase.pe is not None:
        # Keep the retail addresses independently constrained, then compare
        # payloads against actual emitted literals, not fabricated aliases.
        pools = {0x2d: (0x20df98, 'string'), 0x32: (0x20a9ec, 'string'),
                 0x5c: (0x1e9a30, 'fp64'), 0x66: (0x1e9a40, 'fp64')}
        converted = []
        for site, kind, target in refs:
            if site in pools:
                address, tag = pools[site]
                require(kind == 6 and target == address, 'original literal target changed')
                payload = phase.pe.read(target, 64 if tag == 'string' else 8)
                require(payload is not None, 'unreadable original literal')
                if tag == 'string':
                    require(b'\0' in payload, 'unterminated original literal')
                    payload = payload.split(b'\0', 1)[0]
                target = (tag, payload)
            converted.append((site, kind, target))
        refs = tuple(converted)
    result = bytearray(masked)
    homes = []
    for instruction in decode(masked).values():
        match = re.search(r'\[esp\+0x([0-9a-f]+)\]', instruction.operands)
        if match:
            require(instruction.size == 4 and instruction.mnemonic in ('mov', 'fild'),
                    'unsupported flash temporary access')
            homes.append((instruction.offset, int(match[1], 16)))
    require(len(homes) == 6, 'flash unsigned conversion home census changed')
    origin = homes[0][1]
    require(origin in (0x14, 0x24), 'unreviewed flash stack home')
    require([offset - origin for _, offset in homes] == [0, 4, 0, 0, 4, 0],
            'flash zero extension or shared QWORD homes changed')
    for site, offset in homes:
        result[site + 3] = offset - origin + 0x14
    require(len(refs) == 8, 'flash reference count changed')
    return bytes(result), refs


def occupancy_protocol(code):
    result = []
    for opcode, mask, store in ((0x64, 0xdf, rb'\xc7\x44.\x04\xff\xff\xff\xff'),
                                (0x4c, 0x20, rb'\x89\x4c.\x04')):
        matches = list(re.finditer(bytes((0x80, opcode)) + rb'.\x03' + bytes((mask,)),
                                   code.data, re.S))
        require(len(matches) == 1, 'missing/ambiguous occupancy memory RMW')
        start = matches[0].start()
        tail = re.search(store, code.data[start + 5:start + 45], re.S)
        require(tail is not None, 'missing ordered occupant store')
        end = start + 5 + tail.end()
        instructions = list(decode(code.data[start:end]).values())
        require(all(i.mnemonic in ('and', 'or', 'add', 'mov', 'shl') for i in instructions),
                'unexpected occupancy call or guard')
        loads = [i for i in instructions[1:-1] if i.mnemonic == 'mov'
                 and re.fullmatch(r'e\w\w,DWORD PTR \[e\w\w\+0x8\]', i.operands)]
        require(len(loads) == 1, 'row table not reloaded between the two writes')
        require(any(i.offset > loads[0].offset and i.mnemonic == 'mov'
                    and re.fullmatch(r'e\w\w,DWORD PTR \[e\w\w\+e\w\w\*1\]', i.operands)
                    for i in instructions[1:-1]), 'row not reloaded after table')
        result.append((start, end, start + loads[0].offset))
    require(result[0][1] < result[1][0], 'acquisition precedes release')
    return result


class CombatHelperConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe
        from gruntz.delink.coffx import Obj
        from gruntz.verify.assert_relocs import Resolver
        cls.pe, cls.resolver = Pe(), Resolver()
        path = BUILD / 'objdiff/base/gruntcombat.obj'
        cls.resolver = LiteralResolver(cls.resolver, Obj(path))
        cls.pairs = [(production(path, name),
                      Consumer('original ' + name, cls.pe.read(rva, size), (), rva, cls.pe))
                     for name, rva, size in ((LOAD, 0x597a0, 0x13c0), (STEP, 0x5d210, 0x1554))]

    def test_complete_flash_phase_named_references_and_literal_payloads(self):
        actual = flash_phase(self.pairs[1][0], self.resolver)
        expected = flash_phase(self.pairs[1][1], self.resolver)
        self.assertEqual(actual, expected)
        self.assertEqual([target for _, _, target in actual[1]][-3:],
                         [('fp64', struct.pack('<d', 1.0)),
                          ('fp64', struct.pack('<d', 750.0)), 0x11f570])
        self.assertEqual(self.pe.read(0x1e9a30, 8), struct.pack('<d', 1.0))
        self.assertEqual(self.pe.read(0x1e9a40, 8), struct.pack('<d', 750.0))

    def test_occupancy_write_reload_store_protocol(self):
        for code in self.pairs[0]:
            self.assertEqual(len(occupancy_protocol(code)), 2)

    def test_missing_row_reload_or_changed_mask_is_rejected(self):
        for code in self.pairs[0]:
            for start, _, reload in occupancy_protocol(code):
                for offset, payload in ((start + 4, b'\x10'), (reload, b'\x90' * 3)):
                    with self.assertRaises(AssertionError):
                        occupancy_protocol(code.changed(offset, payload))

    def test_flash_guard_square_and_high_word_mutations_are_rejected(self):
        for code in self.pairs[1]:
            reference = flash_phase(code, self.resolver)
            for pattern, relative, byte in ((b'\x7c\x04', 0, b'\x72'),
                    (b'\xd9\xc0\xd8\xc9', 3, b'\xc8'),
                    (b'\x89\x5c\x24', 1, b'\x44')):
                start = code.data.index(bytes.fromhex('6888130000')) - 0x27
                site = code.data.index(pattern, start, start + 0x89)
                with self.assertRaises(AssertionError):
                    self.assertEqual(flash_phase(code.changed(site + relative, byte), self.resolver),
                                     reference)

    def test_wrong_missing_repeated_flash_references_and_addends_are_rejected(self):
        compiled, original = self.pairs[1]
        target = flash_phase(original, self.resolver)
        start = compiled.data.index(bytes.fromhex('6888130000')) - 0x27
        reference = next(ref for ref in compiled.references if ref[0] == start + 0x66)
        site, kind, _ = reference
        wrong = replace(compiled, references=tuple(
            (off, k, '?g_wingzScale@@3NB') if off == site else (off, k, name)
            for off, k, name in compiled.references))
        missing = replace(compiled, references=tuple(r for r in compiled.references if r != reference))
        repeated = replace(compiled, references=compiled.references + (reference,))
        for code in (wrong, missing, repeated, compiled.changed(site, struct.pack('<I', 8))):
            with self.assertRaises(AssertionError):
                self.assertEqual(flash_phase(code, self.resolver), target)


if __name__ == '__main__':
    unittest.main()
