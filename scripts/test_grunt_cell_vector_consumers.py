"""Cell numeric-storage dependency controls for DoubleVector2 source changes.

Read existing production COFF and the original PE; no compilation/game execution.
Constructor/destructor checks compare complete extents, masked bytes and ordered
fixup offsets/kinds/resolved targets. Serializers check their real final motion
transfer and call identities/slots, not complete serializer or EH equivalence.
"""

from dataclasses import replace
import re
import struct
import unittest

from test_ambient_scaler_consumers import check_pair
from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer


CTORS = (
    ('??0CGruntCellRec@@QAE@XZ', 0xf400, 0x1b,
     ((2, 6, 0x1b9cde), (7, 6, 0x1b9b93), (0x13, 0x14, 0x11f5a0))),
    ('??1CGruntCellRec@@QAE@XZ', 0xf430, 0x10,
     ((1, 6, 0x1b9cde), (0xb, 0x14, 0x11f640))),
)
SERIALIZERS = (
    ('?SerializeStrings@CGruntCellRec@@QAEHPAVCFileMemBase@@@Z',
     0x56da0, 0xc7, 'write'),
    ('?DeserializeStrings@CGruntCellRec@@QAEHPAVCFileMemBase@@@Z',
     0x56eb0, 0x94, 'read'),
)
CTOR, DTOR = '??0CString@@QAE@XZ', '??1CString@@QAE@XZ'

# Pin only the original local argument/receiver protocol, not register choices
# in arbitrary equivalent implementations. Changed shapes require review.
TRANSFER = {
    'write': bytes.fromhex('8b 13 83 c6 48 6a 20 56 8b cb ff 52 30'),
    'read': bytes.fromhex('8b 16 83 c5 48 6a 20 55 8b ce ff 52 2c'),
}


def production_owners(path, names):
    """Load each object once so a concurrent replacement cannot mix its owners."""
    from gruntz.delink.coffx import Obj
    from gruntz.walls.pairscan import functions, fn_relocs

    require(path.is_file(), f'missing production object {path}; run gruntz build')
    obj = Obj(path)
    defined = functions(obj)
    result = {}
    for name in names:
        require(name in defined, f'missing production owner {name}')
        section, start, end = defined[name]
        data = obj.section_payload(section)[start:end].rstrip(b'\xcc').rstrip(b'\x90')
        refs = tuple((off - start, kind, symbol)
                     for off, symbol, kind, _ in fn_relocs(obj, section, start, end))
        result[name] = Consumer('production ' + name, data, refs)
    return result


def check_lifetime(compiled, original, resolver, size, references):
    require(len(compiled.data) == len(original.data) == size,
            'complete cell lifetime owner extent changed')
    actual = check_pair(compiled, original, resolver)
    require(actual == references, 'cell CString lifetime identities changed')


def call_targets(code, resolver):
    """Retain direct-call order/multiplicity; virtual calls are checked below."""
    insns = decode(code.data)
    calls = [i for i in insns.values() if i.mnemonic == 'call']
    direct = [i for i in calls if code.data[i.offset] == 0xe8]
    if code.pe is None:
        require(len(code.references) == len(direct),
                'serializer fixup/call count changed')
        targets = []
        for i in direct:
            refs = [(kind, name) for site, kind, name in code.references
                    if site == i.offset + 1]
            require(len(refs) == 1 and refs[0][0] == 0x14,
                    'serializer missing/duplicate direct-call fixup')
            kind, name = refs[0]
            addend = struct.unpack_from('<I', code.data, i.offset + 1)[0]
            resolved = resolver.resolve_base(name, kind, addend)
            require(len(resolved) == 1, 'unresolved serializer direct callee')
            targets.append(next(iter(resolved)))
    else:
        require(not resolver.img.relocs_in(code.rva, code.rva + len(code.data)),
                'unexpected original serializer absolute fixup')
        targets = [
            resolver.chase(code.rva + i.after
                           + struct.unpack_from('<i', code.data, i.offset + 1)[0])
            for i in direct
        ]
    return insns, calls, targets


def check_motion_transfer(code, resolver, mode):
    insns, calls, targets = call_targets(code, resolver)
    require(targets == ([] if mode == 'write' else [0x1b9e74]),
            'serializer ordered direct callee identities changed')
    slot = 0x30 if mode == 'write' else 0x2c
    indirect = [i for i in calls if code.data[i.offset] != 0xe8]
    require(len(indirect) == 5, 'serializer virtual-call multiplicity changed')
    require(all(re.fullmatch(r'DWORD PTR \[(?:eax|edx)\+0x%x\]' % slot,
                             i.operands) for i in indirect),
            'serializer Read/Write virtual slot changed')
    sequence = TRANSFER[mode]
    require(code.data.count(sequence) == 1,
            'motion transfer address/length/receiver/slot changed')
    start = code.data.index(sequence)
    require(start in insns and start + 10 in insns,
            'motion transfer is not an instruction sequence')
    require(calls[-1].offset == start + 10,
            'motion transfer is not the final call')

    # Verify the cell base reaching the final ADD, using these two original
    # straight-line register protocols. This is not a general dataflow engine.
    before = list(insns.values())
    if mode == 'write':
        # Entry stores incoming this before three later callee-save pushes.
        require(code.data[0:7] == bytes.fromhex('81 ec 88 00 00 00 53')
                and code.data[0x10:0x14] == bytes.fromhex('89 4c 24 08')
                and code.data[0x22:0x25] == bytes.fromhex('57 56 55'),
                'unsupported write owner-home protocol')
        reloads = [i for i in before if i.offset < start
                   and i.mnemonic == 'mov' and i.operands == 'esi,DWORD PTR [esp+0x14]']
        require(len(reloads) == 1, 'missing write cell-base reload')
        require(all(not i.operands.startswith('esi,')
                    for i in before if reloads[0].after <= i.offset < start),
                'write cell base changed before motion transfer')
    else:
        require(code.data[0:8] == bytes.fromhex('81 ec 80 00 00 00 55 56')
                and code.data[0xf:0x11] == bytes.fromhex('8b e9'),
                'unsupported read owner-register protocol')
        require(all(not i.operands.startswith('ebp,')
                    for i in before if 0x11 <= i.offset < start),
                'read cell base changed before motion transfer')
    return start


class GruntCellVectorConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe
        from gruntz.verify.assert_relocs import Resolver

        cls.resolver = Resolver()
        pe = Pe()
        cls.lifetimes = []
        cls.serializers = []
        owners = production_owners(BUILD / 'objdiff/base/grunt.obj',
                                   [r[0] for r in CTORS])
        for name, rva, size, refs in CTORS:
            data = pe.read(rva, size)
            require(data is not None, 'missing original cell lifetime owner')
            cls.lifetimes.append(
                (owners[name], Consumer('original ' + name, data, (), rva, pe), size, refs))
        owners = production_owners(BUILD / 'objdiff/base/gruntdatarecord.obj',
                                   [r[0] for r in SERIALIZERS])
        for name, rva, size, mode in SERIALIZERS:
            data = pe.read(rva, size)
            require(data is not None, 'missing original cell serializer')
            cls.serializers.append(
                (owners[name], Consumer('original ' + name, data, (), rva, pe), mode))

    def test_complete_actual_lifetime_bodies_and_all_five_fixups(self):
        for compiled, original, size, refs in self.lifetimes:
            with self.subTest(owner=compiled.label):
                check_lifetime(compiled, original, self.resolver, size, refs)

    def test_wrong_string_count_or_stride_is_rejected(self):
        for compiled, original, size, refs in self.lifetimes:
            for side, code in enumerate((compiled, original)):
                start = code.data.index(bytes.fromhex('6a 05 6a 04'))
                for off, value in ((start + 1, b'\x09'), (start + 3, b'\x10')):
                    pair = (code.changed(off, value), original) if side == 0 else (
                        compiled, code.changed(off, value))
                    with self.subTest(owner=code.label, off=off):
                        with self.assertRaisesRegex(AssertionError, 'whole masked body'):
                            check_lifetime(*pair, self.resolver, size, refs)

    def test_wrong_lifetime_identity_addend_or_multiplicity_is_rejected(self):
        for compiled, original, size, refs in self.lifetimes:
            off, _, name = next(r for r in compiled.references if r[1] == 6)
            wrong_name = CTOR if name == DTOR else DTOR
            renamed = replace(compiled, references=tuple(
                (site, kind, wrong_name if site == off else target)
                for site, kind, target in compiled.references))
            addend = compiled.changed(off, struct.pack('<I', 4))
            missing = replace(compiled, references=compiled.references[1:])
            repeated = replace(compiled, references=compiled.references + (compiled.references[0],))
            for damaged in (renamed, addend, missing, repeated):
                with self.subTest(owner=compiled.label):
                    with self.assertRaises(AssertionError):
                        check_lifetime(damaged, original, self.resolver, size, refs)
            value = struct.unpack_from('<I', original.data, off)[0]
            with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
                check_lifetime(compiled, original.changed(off, struct.pack('<I', value + 4)),
                               self.resolver, size, refs)

    def test_actual_final_motion_transfers(self):
        for compiled, original, mode in self.serializers:
            for code in (compiled, original):
                with self.subTest(owner=code.label):
                    check_motion_transfer(code, self.resolver, mode)

    def test_wrong_motion_address_length_or_virtual_slot_is_rejected(self):
        for compiled, original, mode in self.serializers:
            for code in (compiled, original):
                start = check_motion_transfer(code, self.resolver, mode)
                for off, value in ((start + 4, b'\x44'), (start + 6, b'\x10'),
                                   (start + 12, b'\x2c' if mode == 'write' else b'\x30')):
                    with self.subTest(owner=code.label, off=off):
                        with self.assertRaises(AssertionError):
                            check_motion_transfer(code.changed(off, value), self.resolver, mode)

    def test_wrong_motion_owner_home_is_rejected(self):
        for compiled, original, mode in self.serializers:
            for code in (compiled, original):
                # Both changes remain valid MOV instructions: write saves this
                # in the wrong home; read takes its cell pointer from EAX.
                off, value = (0x13, b'\x0c') if mode == 'write' else (0x10, b'\xe8')
                with self.subTest(owner=code.label):
                    with self.assertRaisesRegex(AssertionError, 'owner-'):
                        check_motion_transfer(code.changed(off, value), self.resolver, mode)

    def test_wrong_serializer_callee_and_lost_fixup_is_rejected(self):
        compiled, original, mode = next(p for p in self.serializers if p[2] == 'read')
        off, kind, _ = compiled.references[0]
        renamed = replace(compiled, references=((off, kind, CTOR),))
        missing = replace(compiled, references=())
        repeated = replace(compiled, references=compiled.references * 2)
        for code in (renamed, missing, repeated):
            with self.assertRaises(AssertionError):
                check_motion_transfer(code, self.resolver, mode)
        damaged = original.changed(off, struct.pack('<i', 0x1b9b93 - original.rva - off - 4))
        with self.assertRaisesRegex(AssertionError, 'identities'):
            check_motion_transfer(damaged, self.resolver, mode)


if __name__ == '__main__':
    unittest.main()
