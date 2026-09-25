"""Bounded original-backed controls for the motion square composition.

Arrival bodies differ only in the explicitly checked three-instruction x87
schedule below. Step checks all ordered constant uses and call/sqrt census,
not its unresolved complete arithmetic, stores or CFG equivalence.
"""

from dataclasses import replace
import struct
import unittest

from test_ambient_scaler_consumers import facts
from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer, production


OWNERS = (
    ('?Step@CMotionState@@QAEXN@Z', 0x16ecd0, 0x6e6),
    ('?ArrivalVelX@CMotionState@@QAENN@Z', 0x16f3c0, 0x61),
    ('?ArrivalVelY@CMotionState@@QAENN@Z', 0x16f430, 0x61),
)
ZERO = 0x1f0500
NEG_TWO = 0x1f0508
POOLS = (('?g_motionNegHalf@@3NB', 0x1f04f8, -0.5),
         ('?g_motionZero@@3NB', ZERO, 0.0),
         ('?g_motionNegTwo@@3NB', NEG_TWO, -2.0))


def constant_roles(code, resolver):
    masked, refs = facts(code, resolver)
    instructions = decode(code.data)
    require(not any(i.mnemonic == 'call' for i in instructions.values()),
            'unexpected call in motion arithmetic')
    roles = []
    for site, kind, target in refs:
        require(kind == 6 and site - 2 in instructions,
                'constant is not an absolute x87 operand')
        instruction = instructions[site - 2]
        require(instruction.size == 6 and code.data[site - 2:site] in
                (b'\xdc\x0d', b'\xdc\x1d', b'\xdd\x05'),
                'constant width or arithmetic operation changed')
        roles.append((instruction.mnemonic, target))
    return masked, refs, roles


def check_step(compiled, original, resolver):
    actual = constant_roles(compiled, resolver)
    expected = constant_roles(original, resolver)
    require(len(actual[1]) == len(expected[1]) == 48,
            'complete Step constant count changed')
    require(actual[2] == expected[2], 'ordered Step constant roles changed')
    for code in (compiled, original):
        require(sum(i.mnemonic == 'fsqrt' for i in decode(code.data).values()) == 9,
                'three arrival expansions per axis changed')


def check_arrival(compiled, original, resolver):
    actual, refs, roles = constant_roles(compiled, resolver)
    expected, original_refs, original_roles = constant_roles(original, resolver)
    require(len(actual) == len(expected) == 0x61, 'arrival extent changed')
    require(len(refs) == len(original_refs) == 5, 'arrival constant count changed')
    require(actual[:0x29] == expected[:0x29]
            and actual[0x37:] == expected[0x37:],
            'arrival member arithmetic, guards, root or return changed')
    # The remaining region is either retail's zero-load/exchange/multiply or
    # the current multiply/zero-load/exchange schedule. Do not call this exact.
    schedules = {
        bytes.fromhex('dd05 00000000 d9c9 dc0d 00000000'):
            [(0x2b, 6, ZERO), (0x33, 6, NEG_TWO)],
        bytes.fromhex('dc0d 00000000 dd05 00000000 d9c9'):
            [(0x2b, 6, NEG_TWO), (0x31, 6, ZERO)],
    }
    for body, references in ((actual, refs), (expected, original_refs)):
        middle = body[0x29:0x37]
        require(middle in schedules, 'unreviewed x87 middle schedule')
        require(list(references) == [(5, 6, ZERO)] + schedules[middle]
                + [(0x46, 6, ZERO), (0x51, 6, ZERO)],
                'arrival constant sites, identities or addends changed')


def eager_targets(code, resolver):
    """Nine zero-acceleration guards, with eagerly evaluated target values."""
    _, references, _ = constant_roles(code, resolver)
    instructions = list(decode(code.data).values())
    by_offset = {instruction.offset: index for index, instruction in enumerate(instructions)}
    result = []
    for site, _, target in references:
        if target != ZERO or code.data[site - 2:site] != b'\xdc\x1d':
            continue
        index = by_offset[site - 2]
        window = []
        for instruction in instructions[index + 1:index + 7]:
            if instruction.mnemonic == 'fnstsw':
                break
            window.append(instruction)
        else:
            raise AssertionError('missing bounded x87 status capture')
        status_index = index + 1 + len(window)
        status, test, branch = instructions[status_index:status_index + 3]
        if test.operands != 'ah,0x40':
            continue  # Positive-velocity comparison uses mask 0x41.
        require(code.data[status.offset:branch.offset] == b'\xdf\xe0\xf6\xc4\x40'
                and code.data[branch.offset] == 0x74,
                'zero-acceleration predicate changed')
        target_loads = [i for i in window if i.mnemonic in ('fadd', 'fld')
                        and i.operands.startswith('QWORD PTR [ecx+')]
        require(len(target_loads) == 1, 'target not evaluated before acceleration guard')
        target_load = target_loads[0]
        zero_path = instructions[status_index + 3:status_index + 6]
        pops = 1 if target_load.mnemonic == 'fadd' else 2
        require(all(i.mnemonic == 'fstp' and i.operands == 'st(0)'
                    for i in zero_path[:pops]), 'zero-path target cleanup changed')
        require(zero_path[pops].mnemonic == 'fld'
                and zero_path[pops].operands.startswith('QWORD PTR [ecx+'),
                'zero acceleration does not return velocity')
        result.append((target_load.mnemonic, target_load.operands, pops,
                       zero_path[pops].operands))
    require(len(result) == 9, 'incomplete eager target family')
    return result


class MotionSquareConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe
        from gruntz.delink.coffx import Obj
        from gruntz.verify.assert_relocs import Resolver

        cls.pe = Pe()
        cls.resolver = Resolver()
        path = BUILD / 'objdiff/base/movinglogic.obj'
        cls.obj = Obj(path)
        cls.pairs = [(production(path, name),
                      Consumer('original ' + name, cls.pe.read(rva, size),
                               (), rva, cls.pe)) for name, rva, size in OWNERS]

    def test_all_ordered_step_constant_roles(self):
        check_step(*self.pairs[0], self.resolver)

    def test_complete_arrival_bodies_except_explicit_schedule(self):
        for pair in self.pairs[1:]:
            check_arrival(*pair, self.resolver)

    def test_nine_eager_targets_and_six_double_cleanup_paths(self):
        compiled, original = self.pairs[0]
        self.assertEqual(eager_targets(compiled, self.resolver),
                         eager_targets(original, self.resolver))
        for code in (compiled, original):
            # Removing one real bound load must not be hidden by equal ref counts.
            offset = code.data.index(bytes.fromhex('dd81 88000000'))
            with self.assertRaises(AssertionError):
                eager_targets(code.changed(offset, b'\x90' * 6), self.resolver)
            offset = code.data.index(bytes.fromhex('ddd8 ddd8 dd41'))
            with self.assertRaises(AssertionError):
                eager_targets(code.changed(offset, b'\x90\x90'), self.resolver)

    def test_actual_production_and_original_pool_payloads(self):
        for name, rva, value in POOLS:
            payload = struct.pack('<d', value)
            self.assertEqual(self.pe.read(rva, 8), payload)
            found = [(section, offset) for index, offset, section in self.obj.iter_symbols()
                     if section > 0 and self.obj.sym_name(index) == name]
            self.assertEqual(len(found), 1)
            section, offset = found[0]
            self.assertEqual(self.obj.section_payload(section)[offset:offset + 8], payload)

    def test_changed_arrival_member_square_or_guard_is_rejected(self):
        for compiled, original in self.pairs[1:]:
            for side, code in enumerate((compiled, original)):
                for offset, payload in ((0x24, b'\x20'), (0x28, b'\xcb'),
                                        (0x40, b'\x74'), (0x5a, b'\x75')):
                    damaged = code.changed(offset, payload)
                    pair = (damaged, original) if side == 0 else (compiled, damaged)
                    with self.subTest(side=side, offset=offset), self.assertRaises(AssertionError):
                        check_arrival(*pair, self.resolver)

    def test_wrong_missing_repeated_and_offset_references_are_rejected(self):
        for index, (compiled, original) in enumerate(self.pairs):
            check = check_step if index == 0 else check_arrival
            reference = next(ref for ref in compiled.references
                             if ref[2] == '?g_motionZero@@3NB')
            site, kind, _ = reference
            wrong = replace(compiled, references=tuple(
                (off, k, '?g_motionNegTwo@@3NB') if off == site else (off, k, name)
                for off, k, name in compiled.references))
            missing = replace(compiled, references=tuple(
                ref for ref in compiled.references if ref != reference))
            repeated = replace(compiled, references=compiled.references + (reference,))
            addend = compiled.changed(site, struct.pack('<I', 8))
            for damaged in (wrong, missing, repeated, addend):
                with self.subTest(owner=index), self.assertRaises(AssertionError):
                    check(damaged, original, self.resolver)
            original_site = next(off for off, _, target in facts(original, self.resolver)[1]
                                 if target == ZERO)
            wrong_original = original.changed(original_site,
                struct.pack('<I', self.pe.image_base + NEG_TWO))
            with self.subTest(original=index), self.assertRaises(AssertionError):
                check(compiled, wrong_original, self.resolver)


if __name__ == '__main__':
    unittest.main()
