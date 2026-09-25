"""Whole-body controls for the four exact ambient scaler consumers.

Read existing production COFF and original retail only; no compile/game run.
Compare full instruction payloads with relocation words masked, then every
ordered fixup offset, kind and resolved original target, including multiplicity.
FadePlayback is intentionally excluded: its whole-body mismatch remains open.
"""

from dataclasses import replace
import struct
import unittest

from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer


OWNERS = (
    ('?ApplyMasterVolume@CAmbientSound@@QAEXH@Z', 0xbf10, 0x72, 2),
    ('?StartPlayback@CAmbientSound@@QAEXXZ', 0xbfb0, 0xa9, 3),
    ('?SetVolumeLevel@CAmbientSound@@QAEHHHH@Z', 0xc200, 0x7e, 2),
    ('?Update@CAmbientPosSound@@UAEXHHH@Z', 0xc5b0, 0x1df, 7),
)
SET_VOLUME = '?SetVolumePercent@SoundBuffer@@QAEHH@Z'


def load_production():
    """Read one consistent object, not four files potentially rebuilt between reads."""
    from gruntz.core.paths import BUILD
    from gruntz.delink.coffx import Obj
    from gruntz.walls.pairscan import functions, fn_relocs

    path = BUILD / 'objdiff/base/worldsoundset.obj'
    require(path.is_file(), 'missing production worldsoundset.obj; run gruntz build')
    obj = Obj(path)
    defined = functions(obj)
    result = []
    for name, _, _, _ in OWNERS:
        require(name in defined, f'missing actual production owner {name}')
        section, start, end = defined[name]
        data = obj.section_payload(section)[start:end]
        # These four original owners end in RET/RET imm16, never padding-valued
        # instruction operands. Trim only conventional post-function alignment.
        data = data.rstrip(b'\xcc').rstrip(b'\x90')
        references = tuple((off - start, kind, symbol)
                           for off, symbol, kind, _ in fn_relocs(obj, section, start, end))
        result.append(Consumer(f'production {name}', data, references))
    return result


def facts(code, resolver):
    instructions = decode(code.data)
    references = []
    if code.pe is None:
        for site, kind, name in code.references:
            require(kind in (6, 0x14), 'unsupported production relocation kind')
            require(0 <= site <= len(code.data) - 4, 'production fixup outside owner')
            addend = struct.unpack_from('<I', code.data, site)[0]
            targets = resolver.resolve_base(name, kind, addend)
            require(len(targets) == 1, f'ambiguous/missing production target {name}: {targets}')
            references.append((site, kind, next(iter(targets))))
    else:
        for absolute, _ in resolver.img.relocs_in(code.rva, code.rva + len(code.data)):
            site = absolute - code.rva
            target = struct.unpack_from('<I', code.data, site)[0] - code.pe.image_base
            references.append((site, 6, resolver.chase(target)))
        for instruction in instructions.values():
            if instruction.mnemonic != 'call':
                continue
            require(code.data[instruction.offset] == 0xe8,
                    'unsupported indirect original call in scaler owner')
            site = instruction.offset + 1
            target = (code.rva + instruction.after
                      + struct.unpack_from('<i', code.data, site)[0])
            references.append((site, 0x14, resolver.chase(target)))
    references.sort()
    require(len({site for site, _, _ in references}) == len(references),
            'duplicate relocation sites')
    masked = bytearray(code.data)
    for site, kind, _ in references:
        require(0 <= site <= len(masked) - 4, 'fixup outside complete owner')
        if kind == 0x14:
            require(site - 1 in instructions
                    and instructions[site - 1].mnemonic == 'call'
                    and code.data[site - 1] == 0xe8,
                    'REL32 no longer belongs to a direct call')
        masked[site:site + 4] = b'\0' * 4
    return bytes(masked), tuple(references)


def check_pair(compiled, original, resolver):
    actual_bytes, actual_refs = facts(compiled, resolver)
    original_bytes, original_refs = facts(original, resolver)
    require(actual_bytes == original_bytes,
            f'{compiled.label}: whole masked body differs from original')
    require(actual_refs == original_refs,
            f'{compiled.label}: ordered fixup offsets/kinds/targets or multiplicity differ')
    return actual_refs


class AmbientScalerConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.pe import Pe
        from gruntz.verify.assert_relocs import Resolver

        cls.resolver = Resolver()
        pe = Pe()
        compiled = load_production()
        require(len(compiled) == len(OWNERS), 'incomplete scaler production census')
        cls.pairs = []
        for code, (name, rva, size, count) in zip(compiled, OWNERS):
            data = pe.read(rva, size)
            require(data is not None, f'missing original scaler owner {rva:x}')
            cls.pairs.append((code, Consumer(f'original {name}', data, (), rva, pe), count))

    def test_complete_real_owners_and_all_fourteen_ordered_references(self):
        total = 0
        for compiled, original, count in self.pairs:
            with self.subTest(owner=compiled.label):
                refs = check_pair(compiled, original, self.resolver)
                self.assertEqual(len(refs), count)
                total += len(refs)
        self.assertEqual(total, 14)

    def test_changed_arithmetic_factor_or_signed_guard_is_rejected(self):
        for compiled, original, _ in self.pairs:
            for side, code in enumerate((compiled, original)):
                # First signed /100 reciprocal, master>5 threshold, and second
                # scale>0 signed guard; all are actual consumer instructions.
                reciprocal = code.data.index(bytes.fromhex('b8 1f 85 eb 51'))
                threshold = code.data.index(bytes.fromhex('83 f8 05'))
                factor = code.data.index(bytes.fromhex('85 c0 7e 14'))
                boundaries = decode(code.data)
                for start in (reciprocal, threshold, factor):
                    self.assertIn(start, boundaries)
                controls = ((reciprocal + 1, b'\x20'),
                            (threshold + 2, b'\x06'),
                            (factor + 2, b'\x76'))  # JBE replaces signed JLE.
                for offset, payload in controls:
                    damaged = code.changed(offset, payload)
                    pair = (damaged, original) if side == 0 else (compiled, damaged)
                    with self.subTest(owner=code.label, offset=offset):
                        with self.assertRaisesRegex(AssertionError, 'whole masked body'):
                            check_pair(*pair, self.resolver)

    def test_wrong_callee_with_unchanged_production_bytes_is_rejected(self):
        for compiled, original, _ in self.pairs:
            site = next(site for site, kind, name in compiled.references
                        if kind == 0x14 and name == SET_VOLUME)
            refs = tuple((off, kind, '_rand' if off == site else name)
                         for off, kind, name in compiled.references)
            damaged = replace(compiled, references=refs)
            self.assertEqual(compiled.data, damaged.data)
            with self.subTest(owner=compiled.label):
                with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
                    check_pair(damaged, original, self.resolver)
            call = decode(original.data)[site - 1]
            damaged_original = original.changed(
                site, struct.pack('<i', 0x11fee0 - original.rva - call.after))
            with self.subTest(owner=original.label):
                with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
                    check_pair(compiled, damaged_original, self.resolver)

    def test_lost_repeated_call_fixup_is_not_hidden_by_deduplication(self):
        tested = 0
        for compiled, original, _ in self.pairs:
            repeated = [ref for ref in compiled.references if ref[2] == SET_VOLUME]
            if len(repeated) != 2:
                continue
            tested += 1
            damaged = replace(compiled, references=tuple(
                ref for ref in compiled.references if ref != repeated[0]))
            before_bytes, before_refs = facts(compiled, self.resolver)
            after_bytes, after_refs = facts(damaged, self.resolver)
            self.assertEqual(before_bytes, after_bytes)
            self.assertEqual({(kind, target) for _, kind, target in before_refs},
                             {(kind, target) for _, kind, target in after_refs})
            self.assertEqual(len(before_refs) - 1, len(after_refs))
            with self.subTest(owner=compiled.label):
                with self.assertRaisesRegex(AssertionError, 'multiplicity'):
                    check_pair(damaged, original, self.resolver)
        self.assertEqual(tested, 2)  # ApplyMasterVolume and positional Update.


if __name__ == '__main__':
    unittest.main()
