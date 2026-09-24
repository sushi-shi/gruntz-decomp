"""Exact Activate and motion-constructor controls for shared double-vector macros.

Compare complete production COFF to original PE bytes and all 99 ordered fixups
(offset, kind, resolved address including addend). No game execution. The three
anonymous FP literals are resolved by actual COFF payload and independently
checked original pool slots, never by compiler ordinal or guessed data label.
The optional isolated header test checks API/layout, not whole-family identity.
"""

from dataclasses import replace
import os
from pathlib import Path
import re
import struct
import tempfile
import unittest

from test_ambient_scaler_consumers import check_pair, facts
from test_entrance_player_guard import decode, require
from test_rng_helper_consumers import Consumer, production


OWNER = '?Activate@CGrunt@@UAEXXZ'
NORTH = '?g_gruntDirNorth@@3UGruntDirectionCell@@A'
EAST = '?g_gruntDirEast@@3UGruntDirectionCell@@A'
RVA, SIZE = 0x5caa0, 0x5e4
FP_POOL = {0x1e9a28: struct.pack('<d', 2.0),
           0x1e9a30: struct.pack('<d', 1.0),
           0x1e9a38: struct.pack('<d', -1.0)}


class ActivateResolver:
    """Add only this actual object's anonymous double-literal payload oracle."""

    def __init__(self, path, original):
        from gruntz.delink.coffx import Obj
        from gruntz.verify.assert_relocs import Resolver

        self.inner = Resolver()
        self.img = self.inner.img
        for target, value in FP_POOL.items():
            require(original.pe.read(target, 8) == value,
                    'original Activate FP slot payload changed')
        self.fp = {}
        obj = Obj(path)
        for index, offset, section in obj.iter_symbols():
            name = obj.sym_name(index)
            if section < 1 or not re.fullmatch(r'\$T[0-9]+', name):
                continue
            payload = obj.section_payload(section)[offset:offset + 8]
            matches = [target for target, value in FP_POOL.items() if payload == value]
            if len(matches) == 1:
                require(name not in self.fp, 'ambiguous anonymous FP symbol')
                self.fp[name] = matches[0]

    def chase(self, target):
        return self.inner.chase(target)

    def resolve_base(self, name, kind, addend):
        if re.fullmatch(r'\$T[0-9]+', name):
            require(kind == 6 and addend == 0,
                    'unsupported Activate FP literal kind/addend')
            require(name in self.fp, 'missing or wrong double FP literal payload')
            return {self.fp[name]}
        return self.inner.resolve_base(name, kind, addend)


def check_activate(compiled, original, resolver):
    require(len(compiled.data) == len(original.data) == SIZE,
            'Activate complete owner extent changed')
    refs = check_pair(compiled, original, resolver)
    require(len(refs) == 75 and all(kind == 6 for _, kind, _ in refs),
            'Activate ordered 75 DIR32 fixups changed')
    fp_refs = [(off, target) for off, _, target in refs if target in FP_POOL]
    require(len(fp_refs) == 3 and {target for _, target in fp_refs} == set(FP_POOL),
            'Activate three double literal identities changed')
    return refs


class ActivateVectorConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe

        path = BUILD / 'objdiff/base/gruntcombat.obj'
        pe = Pe()
        data = pe.read(RVA, SIZE)
        require(data is not None, 'missing original Activate body')
        cls.original = Consumer('original Activate', data, (), RVA, pe)
        cls.compiled = production(path, OWNER)
        cls.resolver = ActivateResolver(path, cls.original)

    def test_complete_actual_consumer_and_all_ordered_references(self):
        check_activate(self.compiled, self.original, self.resolver)

    def test_wrong_direction_identity_with_unchanged_bytes_is_rejected(self):
        site = next(off for off, kind, name in self.compiled.references
                    if kind == 6 and name == NORTH)
        changed = replace(self.compiled, references=tuple(
            (off, kind, EAST if off == site else name)
            for off, kind, name in self.compiled.references))
        self.assertEqual(changed.data, self.compiled.data)
        with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
            check_activate(changed, self.original, self.resolver)
        target = struct.unpack_from('<I', self.original.data, site)[0]
        changed = self.original.changed(site, struct.pack('<I', target + 0x10))
        with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
            check_activate(self.compiled, changed, self.resolver)

    def test_wrong_direction_member_addend_is_rejected(self):
        site = next(off for off, kind, name in self.compiled.references
                    if kind == 6 and name == NORTH
                    and struct.unpack_from('<I', self.compiled.data, off)[0] == 4)
        for side, code in enumerate((self.compiled, self.original)):
            old = struct.unpack_from('<I', code.data, site)[0]
            damaged = code.changed(site, struct.pack('<I', old + 4))
            pair = (damaged, self.original) if side == 0 else (self.compiled, damaged)
            with self.subTest(owner=code.label):
                with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
                    check_activate(*pair, self.resolver)

    def test_narrowed_fp_store_is_rejected(self):
        for side, code in enumerate((self.compiled, self.original)):
            stores = [i for i in decode(code.data).values()
                      if i.mnemonic in ('fst', 'fstp')
                      and i.operands.startswith('QWORD PTR [ecx+')]
            self.assertTrue(stores, 'missing real double vector result store')
            store = stores[0]
            self.assertEqual(code.data[store.offset], 0xdd)
            # DD /2 or /3 stores a double; D9 /2 or /3 stores a float at
            # the same address, with identical instruction length/relocations.
            damaged = code.changed(store.offset, b'\xd9')
            pair = (damaged, self.original) if side == 0 else (self.compiled, damaged)
            with self.subTest(owner=code.label):
                with self.assertRaisesRegex(AssertionError, 'whole masked body'):
                    check_activate(*pair, self.resolver)

    def test_lost_repeated_direction_fixup_is_not_deduplicated(self):
        repeat = next(ref for ref in self.compiled.references
                      if ref[2] == NORTH
                      and struct.unpack_from('<I', self.compiled.data, ref[0])[0] == 0)
        damaged = replace(self.compiled, references=tuple(
            ref for ref in self.compiled.references if ref != repeat))
        before_bytes, before_refs = facts(self.compiled, self.resolver)
        after_bytes, after_refs = facts(damaged, self.resolver)
        self.assertEqual(before_bytes, after_bytes)
        self.assertEqual({(kind, target) for _, kind, target in before_refs},
                         {(kind, target) for _, kind, target in after_refs})
        self.assertEqual(len(before_refs) - 1, len(after_refs))
        with self.assertRaisesRegex(AssertionError, 'multiplicity'):
            check_activate(damaged, self.original, self.resolver)



class MotionVectorConsumerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.core.paths import BUILD
        from gruntz.core.pe import Pe
        from gruntz.verify.assert_relocs import Resolver

        pe = Pe()
        data = pe.read(0x136d0, 0x184)
        require(data is not None, 'missing original motion constructor')
        cls.original = Consumer('original motion constructor', data, (), 0x136d0, pe)
        cls.compiled = production(BUILD / 'objdiff/base/serialobjectfactory.obj',
                                  '??0CMotionState@@QAE@XZ')
        cls.resolver = Resolver()

    def check(self, compiled, original):
        require(len(compiled.data) == len(original.data) == 0x184,
                'motion constructor complete owner extent changed')
        refs = check_pair(compiled, original, self.resolver)
        require(len(refs) == 24 and all(kind == 6 for _, kind, _ in refs),
                'motion constructor ordered 24 DIR32 fixups changed')
        return refs

    def test_complete_constructor_and_all_ordered_references(self):
        self.check(self.compiled, self.original)

    def test_wrong_bound_identity_or_high_word_addend_is_rejected(self):
        minimum = '?g_movingLogicMin@@3NB'
        maximum = '?g_movingLogicMax@@3NB'
        low = next(off for off, kind, name in self.compiled.references
                   if name == minimum
                   and struct.unpack_from('<I', self.compiled.data, off)[0] == 0)
        high = next(off for off, kind, name in self.compiled.references
                    if name == minimum
                    and struct.unpack_from('<I', self.compiled.data, off)[0] == 4)
        damaged = replace(self.compiled, references=tuple(
            (off, kind, maximum if off == low else name)
            for off, kind, name in self.compiled.references))
        self.assertEqual(damaged.data, self.compiled.data)
        with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
            self.check(damaged, self.original)
        for side, code in enumerate((self.compiled, self.original)):
            old = struct.unpack_from('<I', code.data, high)[0]
            damaged = code.changed(high, struct.pack('<I', old - 4))
            pair = (damaged, self.original) if side == 0 else (self.compiled, damaged)
            with self.subTest(owner=code.label):
                with self.assertRaisesRegex(AssertionError, 'ordered fixup'):
                    self.check(*pair)

    def test_lost_repeated_bound_fixup_is_not_deduplicated(self):
        repeat = next(ref for ref in self.compiled.references
                      if ref[2] == '?g_movingLogicMax@@3NB'
                      and struct.unpack_from('<I', self.compiled.data, ref[0])[0] == 0)
        damaged = replace(self.compiled, references=tuple(
            ref for ref in self.compiled.references if ref != repeat))
        before_bytes, before_refs = facts(self.compiled, self.resolver)
        after_bytes, after_refs = facts(damaged, self.resolver)
        self.assertEqual(before_bytes, after_bytes)
        self.assertEqual({(kind, target) for _, kind, target in before_refs},
                         {(kind, target) for _, kind, target in after_refs})
        self.assertEqual(len(before_refs) - 1, len(after_refs))
        with self.assertRaisesRegex(AssertionError, 'multiplicity'):
            self.check(damaged, self.original)

    def test_wrong_zero_store_destination_is_rejected(self):
        for side, code in enumerate((self.compiled, self.original)):
            # Actual first position low-dword zero store at +4; redirecting it
            # into previousPosition would change an untouched member's state.
            self.assertEqual(code.data[4:7], bytes.fromhex('89 48 40'))
            self.assertIn(4, decode(code.data))
            damaged = code.changed(6, b'\x58')
            pair = (damaged, self.original) if side == 0 else (self.compiled, damaged)
            with self.subTest(owner=code.label):
                with self.assertRaisesRegex(AssertionError, 'whole masked body'):
                    self.check(*pair)


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop')
class DoubleVectorHeaderTests(unittest.TestCase):
    def test_shared_header_layout_and_declared_api_compile(self):
        from gruntz.tool import cl

        with tempfile.TemporaryDirectory(prefix='gruntz-double-vector-api-') as directory:
            root = Path(directory)
            source = root / 'probe.cpp'
            source.write_text('''#include <Gruntz/DoubleVector.h>
#include <stddef.h>
typedef char V2Layout[(sizeof(DoubleVector2) == 16
    && offsetof(DoubleVector2, m_x) == 0
    && offsetof(DoubleVector2, m_y) == 8) ? 1 : -1];
typedef char V3Layout[(sizeof(DoubleVector3) == 24
    && offsetof(DoubleVector3, m_x) == 0
    && offsetof(DoubleVector3, m_y) == 8
    && offsetof(DoubleVector3, m_z) == 16) ? 1 : -1];
void (DoubleVector2::*init2)(const double, const double) = &DoubleVector2::Init;
void (DoubleVector3::*init3)(double, double, double) = &DoubleVector3::Init;
typedef const DoubleVector2 (DoubleVector2::*Difference)(const DoubleVector2&) const;
Difference difference = &DoubleVector2::operator-;
DoubleVector2 use_values(const DoubleVector2& input) {
    DoubleVector2 empty;
    DoubleVector2 position(12.0, 34.0);
    DoubleVector2 copy(position);
    empty = copy;
    return input - empty;
}
void use_macro(DoubleVector2* value, double x, double y) {
    VEC2_SET(*value, x, y)
}
void use_macro3(DoubleVector3* value, double x, double y, double z) {
    VEC3_SET(*value, x, y, z)
}
void use_defaults(DoubleVector2* a, DoubleVector3* b) {
    a->Init();
    b->Init();
}
''')
            cl.compile(source, root / 'probe.obj', ['/nologo', '/c', '/O2', '/Ob0', '/MT'])

    def test_named_motion_preserves_complete_cell_layout(self):
        from gruntz.tool import cl

        with tempfile.TemporaryDirectory(prefix='gruntz-cell-vector-layout-') as directory:
            root = Path(directory)
            source = root / 'probe.cpp'
            source.write_text('''#include <Mfc.h>
#include <Gruntz/Grunt.h>
#include <stddef.h>
typedef char MotionLayout[(sizeof(CGruntCellRec::Motion) == 32
    && offsetof(CGruntCellRec::Motion, m_direction) == 0
    && offsetof(CGruntCellRec::Motion, m_step) == 16) ? 1 : -1];
typedef char CellLayout[(sizeof(CGruntCellRec) == 104
    && offsetof(CGruntCellRec, m_names) == 0
    && offsetof(CGruntCellRec, m_rects) == 20
    && offsetof(CGruntCellRec, m_motion) == 72) ? 1 : -1];
void use_cell(CGruntCellRec& cell) {
    cell.m_motion.m_direction.Init(1.0, 0.0);
    cell.m_motion.m_step.Init(32.0, 0.0);
}
''')
            cl.compile(source, root / 'probe.obj', ['/nologo', '/c', '/O2', '/MT', '/GX'])


if __name__ == '__main__':
    unittest.main()
