"""Real-VC5 controls for the distinct CRT and credits random helpers."""

import os
from pathlib import Path
import tempfile
import unittest


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop')
class GameRandTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from gruntz.tool import cl
        from gruntz.delink.coffx import Obj
        from gruntz.walls.pairscan import functions

        cls.directory = tempfile.TemporaryDirectory(prefix='gruntz-random-header-')
        cls.addClassCleanup(cls.directory.cleanup)
        folder = Path(cls.directory.name)
        source = folder / 'probe.cpp'
        source.write_text('''#include <Gruntz/GameRand.h>
int range(int lo, int hi) { return GetRandom(lo, hi); }
int fixed() { return GetRandom(0, 2); }
int empty() { return GetRandom(3, 2); }
int credits() { return GetRandomNumber(3, 9); }
''')
        output = folder / 'probe.obj'
        cl.compile(source, output, ['/nologo', '/c', '/O2', '/MT'])
        cls.obj = Obj(output)
        cls.functions = functions(cls.obj)

    def body(self, caller):
        from gruntz.walls.pairscan import fn_relocs
        from gruntz.walls.diagnose import _skeleton

        name = next(n for n in self.functions if n.startswith('?' + caller + '@@'))
        section, start, end = self.functions[name]
        refs = list(fn_relocs(self.obj, section, start, end))
        code = bytearray(self.obj.section_payload(section)[start:end])
        for offset, _, _, _ in refs:
            code[offset-start:offset-start+4] = b'\0' * 4
        return [ref[1] for ref in refs], _skeleton(bytes(code), {})[-1]

    def test_crt_ranges_do_not_use_credits_seed(self):
        for caller in ('range', 'fixed', 'empty'):
            with self.subTest(caller=caller):
                refs, _ = self.body(caller)
                self.assertTrue(refs)
                self.assertEqual(set(refs), {'_rand'})

    def test_empty_range_uses_coin_without_dividing(self):
        refs, assembly = self.body('empty')
        self.assertEqual(refs, ['_rand'])
        self.assertNotIn('idiv', assembly)

    def test_nonempty_range_keeps_signed_remainder(self):
        refs, assembly = self.body('fixed')
        self.assertEqual(refs, ['_rand'])
        self.assertIn('idiv', assembly)

    def test_credits_range_does_not_call_crt_rand(self):
        refs, _ = self.body('credits')
        self.assertNotIn('_rand', refs)
        self.assertTrue(any('holdrand' in ref for ref in refs), refs)


if __name__ == '__main__':
    unittest.main()
