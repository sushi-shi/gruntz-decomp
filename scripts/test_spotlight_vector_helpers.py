"""Bounded Spotlight vector-write controls against production COFF and retail PE.

These are phase/width/referent checks, not whole-function equivalence or a claim
that the remaining FP allocation and Tick exit topology are resolved.
"""

from dataclasses import replace
import re
import struct
import unittest

from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer, production


OWNERS = {
    '?Tick@CSpotLight@@QAEHXZ': (0xb1af0, 0x318),
    '?Update@CSpotLight@@QAEHXZ': (0xb1ee0, 0x11d),
}
FOCUS = bytes.fromhex('db 40 5c dd 5e 70 db 40 60 dd 5e 78')
PHASE = re.compile(rb'\xdd\x5e\x60\xdd\x5e\x68(?:\xd9\xc9)?\x74\x0c'
                   + re.escape(FOCUS))


class SpotlightResolver:
    """Resolve the one pooled A string by actual COFF/PE payload and use site."""

    def __init__(self, path, pe):
        from gruntz.delink.coffx import Obj
        from gruntz.verify.assert_relocs import Resolver

        self.inner = Resolver()
        self.img = self.inner.img
        self.literal = '??_C@_01PFH@A?$AA@'
        obj = Obj(path)
        payloads = [obj.section_payload(sec)[off:off + 2]
                    for index, off, sec in obj.iter_symbols()
                    if sec > 0 and obj.sym_name(index) == self.literal]
        require(payloads == [b'A\0'], 'compiled animation string payload changed')
        require(pe.read(0x20a454, 2) == b'A\0', 'original animation string changed')

    def chase(self, target):
        return self.inner.chase(target)

    def resolve_base(self, name, kind, addend):
        if name == self.literal:
            require(kind == 6 and addend == 0, 'animation string addend changed')
            return {0x20a454}
        return self.inner.resolve_base(name, kind, addend)


def check_phases(code):
    instructions = decode(code.data)
    matches = list(PHASE.finditer(code.data))
    require(len(matches) == 1,
            'rotation stores / focus guard / alternating conversions changed')
    start = matches[0].start()
    require(start in instructions, 'rotation store is not an instruction boundary')
    join = matches[0].end()
    require(join in instructions, 'focus skip misses translation entry')
    # Both paths join the translation, after the intermediate position write.
    tail = list(instructions.values())[list(instructions).index(join):]
    angle = next((i for i in tail if i.mnemonic == 'fstp'
                  and i.operands == 'QWORD PTR [esi+0x90]'), None)
    require(angle is not None, 'missing final angle store')
    stores = [i.operands for i in tail if i.offset < angle.offset
              and i.mnemonic == 'fstp' and '[esi+' in i.operands]
    require(stores == ['QWORD PTR [esi+0x60]', 'QWORD PTR [esi+0x68]'],
            'translated position pair/order/width changed')
    return start, join


def update_references(compiled, original, resolver):
    actual = []
    for off, kind, name in compiled.references:
        require(kind in (6, 0x14), 'unsupported Update fixup')
        addend = struct.unpack_from('<I', compiled.data, off)[0]
        targets = resolver.resolve_base(name, kind, addend)
        require(len(targets) == 1, 'unresolved Update referent')
        actual.append((off, kind, next(iter(targets))))
    expected = [(site - original.rva, 6, resolver.chase(target))
                for site, target in resolver.img.relocs_in(
                    original.rva, original.rva + len(original.data))]
    for i in decode(original.data).values():
        if i.mnemonic == 'call':
            require(original.data[i.offset] == 0xe8, 'unsupported Update call')
            target = (original.rva + i.after
                      + struct.unpack_from('<i', original.data, i.offset + 1)[0])
            expected.append((i.offset + 1, 0x14, resolver.chase(target)))
    require(len(actual) == len(expected) == 5, 'Update ordered fixup count changed')
    require([(kind, target) for _, kind, target in sorted(actual)]
            == [(kind, target) for _, kind, target in sorted(expected)],
            'Update ordered fixup identities/addends changed')


def check_unsigned_delta(code):
    # Both actual Update owners use ECX and a zero-extended QWORD scratch.
    matches = list(re.finditer(rb'\xc7\x44\x24(.)\x00{4}\x89\x4c\x24(.)',
                              code.data, re.S))
    require(len(matches) == 1, 'unsigned frame-delta construction changed')
    m = matches[0]
    require(m[1][0] == m[2][0] + 4, 'frame-delta high/low homes changed')
    load = b'\xdf\x6c\x24' + m[2]
    require(code.data.count(load) == 1, 'missing unsigned frame-delta QWORD load')
    return m.start()


class SpotlightVectorTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe
        pe = Pe()
        cls.resolver = SpotlightResolver(BUILD / 'objdiff/base/spotlight.obj', pe)
        cls.pairs = {}
        for name, (rva, size) in OWNERS.items():
            data = pe.read(rva, size)
            require(data is not None, 'missing original Spotlight owner')
            cls.pairs[name] = (
                production(BUILD / 'objdiff/base/spotlight.obj', name),
                Consumer('original ' + name, data, (), rva, pe),
            )

    def test_actual_rotation_focus_translation_phases(self):
        for pair in self.pairs.values():
            for code in pair:
                with self.subTest(owner=code.label):
                    check_phases(code)

    def test_missing_intermediate_store_or_wrong_focus_width_is_rejected(self):
        for pair in self.pairs.values():
            for code in pair:
                start, join = check_phases(code)
                for offset, byte in ((start, b'\x90\x90\x90'), (start + 2, b'\x70'),
                                     (join - 12, b'\xdd'), (join - 7, b'\x78')):
                    with self.subTest(owner=code.label, offset=offset):
                        with self.assertRaises(AssertionError):
                            check_phases(code.changed(offset, byte))

    def test_focus_branch_must_rejoin_translation(self):
        for pair in self.pairs.values():
            for code in pair:
                _, join = check_phases(code)
                with self.subTest(owner=code.label), self.assertRaises(AssertionError):
                    check_phases(code.changed(join - 13, b'\x0f'))

    def test_update_keeps_unsigned_frame_delta(self):
        for code in self.pairs['?Update@CSpotLight@@QAEHXZ']:
            start = check_unsigned_delta(code)
            with self.subTest(owner=code.label), self.assertRaises(AssertionError):
                check_unsigned_delta(code.changed(start + 4, b'\xff'))

    def test_update_all_five_ordered_references(self):
        update_references(*self.pairs['?Update@CSpotLight@@QAEHXZ'], self.resolver)

    def test_wrong_reference_or_added_repeat_is_rejected(self):
        compiled, original = self.pairs['?Update@CSpotLight@@QAEHXZ']
        first = compiled.references[0]
        wrong = replace(compiled, references=(
            (first[0], first[1], '?g_gameReg@@3PAVCGruntzMgr@@A'),
        ) + compiled.references[1:])
        repeated = replace(compiled, references=compiled.references + (first,))
        missing = replace(compiled, references=compiled.references[1:])
        literal = next(off for off, _, name in compiled.references
                       if name == self.resolver.literal)
        wrong_addend = compiled.changed(literal, struct.pack('<I', 1))
        for code in (wrong, repeated, missing, wrong_addend):
            with self.assertRaises(AssertionError):
                update_references(code, original, self.resolver)


if __name__ == '__main__':
    unittest.main()
