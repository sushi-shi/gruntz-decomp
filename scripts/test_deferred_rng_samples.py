"""Bounded object/retail controls for defender and toy integer RNG samples.

These check the sample, its home and signed comparison, not complete caller
equivalence or unrelated allocation/animation logic. Unsupported shapes fail.
"""

import re
import struct
import unittest

from test_rng_helper_consumers import Consumer, RAND, production
from test_entrance_player_guard import decode, require


DEFENDER = '?StepRowSpawn@CBattlezMapConfig@@QAEHH@Z'
TOY = '?UpdateArrival@CGrunt@@QAEHHH@Z'


def check_sample(code, toy):
    instructions = decode(code.data)
    if toy:
        # One-based result is compared to the full signed member at +190.
        pattern = rb'\xe8....\x99\xb9\x64\x00\x00\x00\xf7\xf9'
        pattern += rb'\x8b\x8e\x90\x01\x00\x00\x33\xc0\x42\x3b\xd1\x0f\x9d\xc0\x8b\xf8'
    else:
        # Keep the integer remainder, not an early boolean, in its stack home.
        pattern = rb'\xe8....\x99\xb9\x64\x00\x00\x00\xbf\x0f\x00\x00\x00\xf7\xf9'
        pattern += rb'\x8b\x4b\x08\x33\xed\x89\x54\x24\x38'
    matches = list(re.finditer(pattern, code.data, re.S))
    require(len(matches) == 1, 'missing or ambiguous integer sample protocol')
    start = matches[0].start()
    require(start in instructions, 'sample is not on an instruction boundary')
    call = instructions[start]
    require(code.call_name(call) == RAND, 'sample RNG referent changed')
    if not toy:
        tail = bytes.fromhex('8b 4c 24 38 8b 53 30 3b ca 7d 0c 3b e8 7d 08')
        require(code.data.count(tail) == 1, 'late signed defender comparison changed')
        late = code.data.index(tail)
        between = [i for i in instructions.values() if matches[0].end() <= i.offset < late]
        require(sum(i.mnemonic == 'call' for i in between) == 1,
                'sample no longer crosses exactly one budget conversion call')
        require(any(i.mnemonic == 'jne' and int(i.operands, 16) < i.offset
                    for i in between), 'sample no longer precedes the free-unit loop')
        compare = late + 10
    else:
        compare = start + 25
    return start, compare


class DeferredRngSampleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe

        pe = Pe()
        cls.samples = []
        for symbol, unit, rva, size, toy in (
                (DEFENDER, 'battlezmapconfig', 0x26470, 0x29d, False),
                (TOY, 'gruntentrancearrival', 0x62110, 0x5bc, True)):
            data = pe.read(rva, size)
            require(data is not None, 'original sample owner unavailable')
            cls.samples.extend(((production(BUILD / f'objdiff/base/{unit}.obj', symbol), toy),
                                (Consumer(symbol, data, (), rva, pe), toy)))

    def test_original_and_production_samples(self):
        for code, toy in self.samples:
            with self.subTest(owner=code.label):
                check_sample(code, toy)

    def test_wrong_modulus_or_signed_comparison_fails(self):
        for code, toy in self.samples:
            start, compare = check_sample(code, toy)
            offsets = (start + 7, compare, start + 21) if toy else (start + 7, compare)
            for offset in offsets:
                with self.subTest(owner=code.label, offset=offset):
                    changed = code.changed(offset, bytes([code.data[offset] ^ 1]))
                    with self.assertRaises(AssertionError):
                        check_sample(changed, toy)

    def test_wrong_rng_identity_fails(self):
        for code, toy in self.samples:
            start, _ = check_sample(code, toy)
            if code.pe is not None:
                changed = code.changed(start + 1, struct.pack('<i', 0x11fed0 - code.rva - start - 5))
            else:
                refs = tuple((off, kind, '_srand' if off == start + 1 else name)
                             for off, kind, name in code.references)
                require(refs != code.references, 'negative control changed no referent')
                changed = Consumer(code.label, code.data, refs)
            with self.subTest(owner=code.label):
                with self.assertRaisesRegex(AssertionError, 'referent'):
                    check_sample(changed, toy)


if __name__ == '__main__':
    unittest.main()
