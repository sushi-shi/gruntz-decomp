"""Real-consumer emission and retail static-lifetime ownership controls."""

import os
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def guard_bodies(obj, object_name):
    from gruntz.walls.pairscan import functions, fn_relocs
    result = []
    for name, (section, start, end) in functions(obj).items():
        if not name.startswith('_$E'):
            continue
        refs = fn_relocs(obj, section, start, end)
        names = [target for _, target, _, _ in refs]
        if object_name not in names:
            continue
        guards = {target for target in names if target.startswith('_$S')}
        if guards:
            result.append((name, obj.section_payload(section)[start:end],
                           [(off - start, target, kind, addend)
                            for off, target, kind, addend in refs], guards))
    return result


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop and full build')
class TuOwnershipTests(unittest.TestCase):
    def test_bindings_live_in_natural_consumers(self):
        from gruntz.manifest import units
        from gruntz.model import resolve
        from gruntz.delink.coffx import Obj
        from gruntz.walls.pairscan import functions
        from gruntz.verify.compiler_artifacts import instantiation_only
        sources = {u['unit']: ROOT / u['source'] for u in units()}
        retired = {'zdarrayderived', 'arrayserialize', 'rezbufferobject',
                   'logicdispatchinit', 'stringstaticpool'}
        self.assertFalse(retired & sources.keys())
        for unit, source in sources.items():
            with self.subTest(unit=unit):
                self.assertFalse(instantiation_only(source.read_text()))
        bindings = {b.rva: b for b in resolve().functions}
        expected = {
            'actionarea': (0x8710, 0x8750, 0x8780),
            'creditsstate': (0x39f20, 0x39fa0, 0x3a1a0),
            'fader': (0x17f130, 0x17f300, 0x17f310, 0x17f330, 0x17f500),
        }
        for unit, addresses in expected.items():
            self.assertNotIn('template class ', sources[unit].read_text())
            emitted = functions(Obj(ROOT / 'build/objdiff/base' / (unit + '.obj')))
            for rva in addresses:
                with self.subTest(rva=hex(rva)):
                    binding = bindings[rva]
                    self.assertEqual(binding.unit, unit)
                    self.assertIn(binding.name, emitted)

    def test_static_guard_owners_reach_model_and_retail(self):
        from gruntz.core import msvc_names
        from gruntz.delink.coffx import Obj
        from gruntz.verify.assert_relocs import Resolver
        resolver = Resolver()
        data = {b.rva: b for b in resolver.model.data}
        functions = {b.rva: b for b in resolver.model.functions}
        for unit, object_rva, guard_rva, teardown_rva in (
            ('eyecandyani', 0x246060, 0x245f34, 0xacb80),
            ('frontcandyani', 0x2460b0, 0x245f30, 0xad180),
            ('splashstate', 0x24e25c, 0x24e218, 0xf9750),
        ):
            with self.subTest(unit=unit):
                datum, guard = data[object_rva], data[guard_rva]
                self.assertEqual((datum.unit, guard.unit, functions[teardown_rva].unit),
                                 (unit, unit, unit))
                obj = Obj(ROOT / 'build/objdiff/base' / (unit + '.obj'))
                candidates = guard_bodies(obj, datum.name)
                self.assertEqual(len(candidates), 1)
                _, body, refs, guards = candidates[0]
                self.assertEqual(len(guards), 1)
                emitted_guard = next(iter(guards))
                self.assertEqual(msvc_names.mask(emitted_guard), msvc_names.mask(guard.name))
                # COFF's next-symbol extent includes one alignment NOP.
                self.assertEqual(body[31:], b'\x90')
                base = bytearray(body[:31])
                retail = bytearray(resolver.img.read(teardown_rva, 31))
                self.assertEqual(body[6:8], b'\xb0\x01')
                for off, target, kind, addend in refs:
                    actual = resolver.retail_value(teardown_rva + off, kind)
                    if target == emitted_guard:
                        self.assertEqual(actual, guard_rva)
                        self.assertEqual(addend, 0)
                    elif target == datum.name:
                        self.assertEqual(actual, object_rva)
                        self.assertEqual(addend, 0)
                    else:
                        self.assertIn(actual, resolver.resolve_base(target, kind, addend))
                    base[off:off + 4] = retail[off:off + 4] = bytes(4)
                self.assertEqual(base, retail)

    def test_merging_registry_storage_shares_the_guard_and_changes_its_bit(self):
        from gruntz.tool import cl
        from gruntz.delink.coffx import Obj
        prefix = '''#include <Gruntz/ActReg.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/FrontCandyAni.h>
'''
        definitions = [
            'template<> CActReg CActRegPool<%s>::s_table(ACT_ID_FIRST, ACT_ID_LAST);\n' % cls
            for cls in ('CEyeCandyAni', 'CFrontCandyAni')
        ]
        with tempfile.TemporaryDirectory(prefix='gruntz-tu-guards-') as directory:
            folder = Path(directory)
            for label, rows in (('eye', definitions[:1]), ('front', definitions[1:]),
                                ('merged', definitions)):
                source, output = folder / (label + '.cpp'), folder / (label + '.obj')
                source.write_text(prefix + ''.join(rows))
                cl.compile(source, output, ['/nologo', '/c', '/O2', '/MT', '/GX', '/GR'])
                obj = Obj(output)
                members = [obj.sym_name(i) for i, _, sec in obj.iter_symbols()
                           if sec > 0 and obj.sym_name(i).startswith('?s_table@')]
                candidates = [row for member in members for row in guard_bodies(obj, member)]
                self.assertEqual(len(candidates), len(rows))
                self.assertEqual({row[1][7] for row in candidates},
                                 {1, 2} if label == 'merged' else {1})
                self.assertEqual(len(set.union(*(row[3] for row in candidates))), 1)


if __name__ == '__main__':
    unittest.main()
