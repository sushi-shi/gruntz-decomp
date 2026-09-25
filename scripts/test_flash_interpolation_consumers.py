"""Original-backed dark-ramp and complete non-EH referent controls.

The dark loop matches completely; the bright loop's two surplus FXCH and the
separately scored EH owner are not certified by this bounded test.
"""

from dataclasses import replace
import struct
import unittest

from test_ambient_scaler_consumers import facts
from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer, production


NAME = '?FlashTable@CShadeTableCache@@QAEPAUCShadeTable@@PAUtagPALETTEENTRY@@HHHH@Z'
DARK_START = 0x1c8
DARK_END = 0x3b5


def non_eh(code):
    if code.pe is not None:
        return code
    special = [ref for ref in code.references
               if ref[0] == 3 or ref[2] == '__except_list']
    require(len(special) == 4 and special[0][0] == 3,
            'EH-table / FS:0 reference census changed')
    require(all(kind == 6 and struct.unpack_from('<I', code.data, site)[0] == 0
                for site, kind, _ in special), 'EH/FS reference kind or addend changed')
    return replace(code, references=tuple(ref for ref in code.references
                                          if ref not in special))


def compare(compiled, original, resolver):
    actual, actual_refs = facts(non_eh(compiled), resolver)
    expected, original_refs = facts(original, resolver)
    # The original first push is the independently scored EH owner. FS:0
    # operands aren't original PE base relocations.
    original_refs = tuple(ref for ref in original_refs if ref[0] != 3)
    require(len(actual_refs) == len(original_refs) == 35,
            'complete non-EH fixup count changed')
    require([(kind, target) for _, kind, target in actual_refs]
            == [(kind, target) for _, kind, target in original_refs],
            'ordered raw references or addends changed')
    for code in (compiled, original):
        instructions = decode(code.data)
        require(DARK_START in instructions and DARK_END in instructions,
                'dark phase boundaries changed')
        phase = code.data[DARK_START:DARK_END]
        require(phase.count(bytes.fromhex('b8 1f 85 eb 51')) == 6,
                'six signed percent divisions changed')
    require(actual[DARK_START:DARK_END] == expected[DARK_START:DARK_END],
            'complete dark phase instructions/branches changed')
    require([ref for ref in actual_refs if DARK_START <= ref[0] < DARK_END]
            == [ref for ref in original_refs if DARK_START <= ref[0] < DARK_END],
            'dark phase fixup offsets changed')
    return actual_refs


class FlashInterpolationConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe
        from gruntz.verify.assert_relocs import Resolver

        cls.pe = Pe()
        cls.resolver = Resolver()
        cls.compiled = production(BUILD / 'objdiff/base/shadetablecache.obj', NAME)
        cls.original = Consumer('original FlashTable', cls.pe.read(0x14df40, 0x5f4),
                                (), 0x14df40, cls.pe)

    def test_complete_dark_phase_and_all_non_eh_ordered_references(self):
        refs = compare(self.compiled, self.original, self.resolver)
        self.assertEqual(sum(target == 0x1efb40 for _, _, target in refs), 2)
        self.assertEqual(sum(target == 0x1efb44 for _, _, target in refs), 12)
        self.assertEqual(sum(target == 0x1efb48 for _, _, target in refs), 6)
        for address, value in ((0x1efb40, 1.0), (0x1efb44, 255.0),
                               (0x1efb48, 0.01)):
            self.assertEqual(self.pe.read(address, 4), struct.pack('<f', value))

    def test_changed_signed_division_and_clamp_branch_are_rejected(self):
        for side, code in enumerate((self.compiled, self.original)):
            reciprocal = code.data.index(bytes.fromhex('b8 1f 85 eb 51'), DARK_START)
            clamp = code.data.index(bytes.fromhex('f6 c4 01 74 32'), DARK_START)
            for offset, byte in ((reciprocal + 1, b'\x20'), (clamp + 3, b'\x75')):
                damaged = code.changed(offset, byte)
                pair = (damaged, self.original) if side == 0 else (self.compiled, damaged)
                with self.subTest(side=side, offset=offset), self.assertRaises(AssertionError):
                    compare(*pair, self.resolver)

    def test_wrong_missing_repeated_reference_and_addend_are_rejected(self):
        code = self.compiled
        reference = next(ref for ref in code.references if ref[2] == '?g_percentScale@@3MB')
        site, kind, _ = reference
        wrong = replace(code, references=tuple(
            (off, k, '?g_one@@3MB') if off == site else (off, k, name)
            for off, k, name in code.references))
        missing = replace(code, references=tuple(ref for ref in code.references
                                                if ref != reference))
        repeated = replace(code, references=code.references + (reference,))
        addend = code.changed(site, struct.pack('<I', 4))
        for damaged in (wrong, missing, repeated, addend):
            with self.subTest(refs=damaged.references), self.assertRaises(AssertionError):
                compare(damaged, self.original, self.resolver)


if __name__ == '__main__':
    unittest.main()
