"""Bounded production/original controls for ambient phase and duration caps.

No game execution or whole-function equivalence claim. Check the full-width
phase toggle, store-before-logical-half, signed cap edge and actual fade calls.
Unexpected compiler shapes fail closed for review.
"""

import re
import struct
import unittest

from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer, production


OWNER = '?Update@CRandomAmbientSound@@UAEXHHH@Z'
FADE = '?FadePlayback@CAmbientSound@@QAEXHHH@Z'
TOGGLE = bytes.fromhex('8b 46 54 83 f0 01 89 46 54')
CAP = re.compile(
    rb'\x89\x56\x50\xd1\xea\x81\xfa\xe8\x03\x00\x00'
    rb'\x7e\x05\xba\xe8\x03\x00\x00\x52\x6a\x64\x6a([\x00\x01])'
    rb'\x8b\xce\xe8....', re.S)


def check(code):
    instructions = decode(code.data)
    require(code.data.count(TOGGLE) == 1, 'full-width phase toggle changed')
    toggle = code.data.index(TOGGLE)
    require(toggle in instructions, 'toggle is not an instruction boundary')
    caps = list(CAP.finditer(code.data))
    require(len(caps) == 2, 'duration cap/store/shift protocol changed')
    require([m[1] for m in caps] == [b'\x01', b'\x00'], 'fade phase order changed')
    calls = []
    for cap in caps:
        require(toggle < cap.start() and cap.start() in instructions,
                'cap no longer follows the phase toggle')
        call = instructions[cap.end() - 5]
        require(call.mnemonic == 'call', 'missing fade call')
        if code.pe is None:
            refs = [(kind, name) for site, kind, name in code.references
                    if site == call.offset + 1]
            require(refs == [(0x14, FADE)], 'production fade referent changed')
        else:
            target = code.rva + call.after + struct.unpack_from('<i', code.data, call.offset + 1)[0]
            # Retail calls the existing linker thunk, not an invented alias.
            require(target == 0x21bc, 'original fade thunk referent changed')
            thunk = code.pe.read(target, 5)
            require(thunk is not None and thunk[0] == 0xe9, 'missing retail fade thunk')
            require(target + 5 + struct.unpack_from('<i', thunk, 1)[0] == 0xc2a0,
                    'original fade implementation referent changed')
        calls.append(call)
    return toggle, caps, calls


class AmbientRangeHelperTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe

        pe = Pe()
        data = pe.read(0xcb30, 0x168)
        require(data is not None, 'original ambient owner unavailable')
        cls.samples = (
            production(BUILD / 'objdiff/base/worldsoundset.obj', OWNER),
            Consumer('original ambient Update', data, (), 0xcb30, pe),
        )

    def test_actual_production_and_original_protocol(self):
        for code in self.samples:
            with self.subTest(owner=code.label):
                check(code)

    def test_complete_ordered_references_resolve_to_original_addresses(self):
        from gruntz.verify.assert_relocs import Resolver

        resolver = Resolver()
        compiled, original = self.samples
        actual = []
        for site, kind, name in compiled.references:
            self.assertIn(kind, (6, 0x14))
            addend = struct.unpack_from('<I', compiled.data, site)[0]
            targets = resolver.resolve_base(name, kind, addend)
            self.assertEqual(len(targets), 1, (site, name, targets))
            actual.append((site, kind, next(iter(targets))))
        expected = [(site - original.rva, 6, resolver.chase(target))
                    for site, target in resolver.img.relocs_in(original.rva,
                                                              original.rva + len(original.data))]
        for instruction in decode(original.data).values():
            if instruction.mnemonic != 'call':
                continue
            self.assertEqual(original.data[instruction.offset], 0xe8)
            target = (original.rva + instruction.after
                      + struct.unpack_from('<i', original.data, instruction.offset + 1)[0])
            expected.append((instruction.offset + 1, 0x14, resolver.chase(target)))
        self.assertEqual(len(actual), 10)
        self.assertEqual(len(expected), 10)
        self.assertEqual([(kind, target) for _, kind, target in sorted(actual)],
                         [(kind, target) for _, kind, target in sorted(expected)])

    def test_wrong_phase_operation_is_rejected(self):
        for code in self.samples:
            toggle, _, _ = check(code)
            with self.subTest(owner=code.label), self.assertRaises(AssertionError):
                check(code.changed(toggle + 5, b'\x02'))

    def test_wrong_width_store_shift_cap_or_tie_edge_is_rejected(self):
        for code in self.samples:
            _, caps, _ = check(code)
            for cap in caps:
                for offset, byte in ((2, b'\x54'), (4, b'\xfa'),
                                     (7, b'\xe7'), (11, b'\x7c')):
                    with self.subTest(owner=code.label, cap=cap.start(), offset=offset):
                        with self.assertRaises(AssertionError):
                            check(code.changed(cap.start() + offset, byte))

    def test_wrong_fade_referent_is_rejected(self):
        for code in self.samples:
            _, _, calls = check(code)
            for call in calls:
                if code.pe is None:
                    refs = tuple((site, kind, '_rand' if site == call.offset + 1 else name)
                                 for site, kind, name in code.references)
                    changed = Consumer('wrong fade COFF referent', code.data, refs)
                else:
                    changed = code.changed(call.offset + 1,
                                           struct.pack('<i', 0x11fee0 - code.rva - call.after))
                with self.subTest(owner=code.label, call=call.offset):
                    with self.assertRaisesRegex(AssertionError, 'referent'):
                        check(changed)


if __name__ == '__main__':
    unittest.main()
