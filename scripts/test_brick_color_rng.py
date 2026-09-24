"""Three original/production color-roll protocols, including signed extremes.

Checks the bounded zero/nonzero draw and first signed threshold. It does not
prove caller equivalence, upstream configuration validity, or portable C++
overflow semantics. Decode stops before the embedded switch tables.
"""

import re
import struct
import unittest

from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import BRICK, Consumer, RAND, production


# Register fields and all branch offsets are checked below, not masked away.
PROTOCOL = re.compile(rb'\x85.\x75.\xe8....\x0f\xbe\xd0\x83\xe2.'
                      rb'\xeb.\xe8....\x99\xf7.\x42\x3b\x54\x24.', re.S)


def check_color(code):
    matches = list(PROTOCOL.finditer(code.data))
    require(len(matches) == 3, 'missing three color sample protocols')
    ins = decode(code.data[:matches[-1].end()])
    registers = []
    for m in matches:
        start = m.start()
        require(start in ins, 'color protocol not on instruction boundary')
        reg = re.fullmatch(r'(ebp|esi),\1', ins[start].operands)
        require(reg, 'unsupported full-dword total test')
        reg = reg[1]
        registers.append(reg)
        require(ins[start + 23].mnemonic == 'idiv'
                and ins[start + 23].operands == reg,
                'divisor no longer uses the tested signed32 total')
        require(ins[start + 12].operands == 'edx,0x1', 'zero parity changed')
        require(ins[start + 2].operands == hex(start + 17), 'nonzero draw guard changed')
        require(ins[start + 15].operands == hex(start + 26), 'zero arm no longer skips division')
        require(ins[start + 26].mnemonic == 'cmp', 'color threshold comparison changed')
        require(code.data[start + 30] == 0x7f, 'first threshold is not signed JG')
        for offset in (4, 17):
            require(code.call_name(ins[start + offset]) == RAND, 'color RNG referent changed')
    require(len(set(registers)) == 1, 'color consumers disagree on total carrier')
    return [m.start() for m in matches]


class BrickColorRngTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe

        pe = Pe()
        data = pe.read(0x810f0, 0xa80)
        require(data is not None, 'missing original brick caller')
        cls.samples = [production(BUILD / 'objdiff/base/brickzload.obj', BRICK),
                       Consumer('original brick caller', data, (), 0x810f0, pe)]

    def test_original_and_production_three_site_protocols(self):
        for code in self.samples:
            with self.subTest(owner=code.label):
                check_color(code)

    def test_divisor_parity_guard_and_signed_threshold_mutations_fail(self):
        for code in self.samples:
            for start in check_color(code):
                for offset in (3, 14, 16, 24, 30):
                    with self.subTest(owner=code.label, site=start, field=offset):
                        at = start + offset
                        changed = code.changed(at, bytes([code.data[at] ^ 1]))
                        with self.assertRaises(AssertionError):
                            check_color(changed)

    def test_both_draw_referents_at_all_sites_are_checked(self):
        for code in self.samples:
            for start in check_color(code):
                for off in (start + 4, start + 17):
                    if code.pe is not None:
                        changed = code.changed(off + 1, struct.pack('<i', 0x11fed0 - code.rva - off - 5))
                    else:
                        refs = tuple((at, kind, '_srand' if at == off + 1 else name)
                                     for at, kind, name in code.references)
                        require(refs != code.references, 'negative control changed no referent')
                        changed = Consumer(code.label, code.data, refs)
                    with self.subTest(owner=code.label, draw=off):
                        with self.assertRaisesRegex(AssertionError, 'referent'):
                            check_color(changed)

    def test_recognized_protocol_boundary_values(self):
        # The recognized CDQ/IDIV operates on nonnegative CRT outputs. For any
        # nonzero signed32 divisor its remainder equals rand % abs(divisor).
        # Test the arithmetic of that emitted protocol, never evaluate hi-1
        # with a signed C++ INT_MIN operand as a supposed source-level oracle.
        for code in self.samples:
            self.assertEqual(len(check_color(code)), 3)
        for total in (-0x80000000, -0x7fffffff, -1, 0, 1, 0x7fffffff):
            for value in range(0x8000):
                if total == 0:
                    narrowed = (value & 0xff) - (0x100 if value & 0x80 else 0)
                    self.assertEqual(narrowed & 1, value & 1)
                else:
                    quotient = value // abs(total) * (-1 if total < 0 else 1)
                    remainder = value - quotient * total
                    self.assertEqual(remainder + 1, value % abs(total) + 1)


if __name__ == '__main__':
    unittest.main()
