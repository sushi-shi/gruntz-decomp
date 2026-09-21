"""Real-VC5 controls for leaf omission and a load-bearing authored base."""
import os
from pathlib import Path
import tempfile
import tomllib
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(os.environ.get('MSVC_DIR'), 'requires pinned nix develop and full build')
class ImplicitLifetimeControls(unittest.TestCase):
    def test_seven_generated_destructors_match_complete_normalized_retail_pairs(self):
        from gruntz.delink.coffx import Obj
        from gruntz.model import resolve
        from gruntz.walls.diagnose import _find_function
        from gruntz.walls.pairscan import pairs
        bindings = {b.rva: b for b in resolve().functions}
        for address in (0xb940, 0xbb40, 0x13bd0, 0x161370, 0x161460, 0x163a10, 0x1682a0):
            with self.subTest(rva=hex(address)):
                binding = bindings[address]
                values = []
                for path in pairs({binding.unit})[binding.unit]:
                    body, refs, size = _find_function(Obj(path), binding.name)
                    self.assertIsNotNone(body)
                    masked = bytearray(body)
                    for offset in refs:
                        masked[offset:offset+4] = bytes(4)
                    values.append((masked, refs, size))
                self.assertEqual(values[0], values[1])

    def test_user_logic_authored_destructor_is_not_an_omittable_leaf(self):
        from gruntz.tool import cl
        from gruntz.tool.wine import winepath
        from gruntz.delink.coffx import Obj
        from gruntz.walls.diagnose import _find_function
        config = tomllib.loads((ROOT / 'config/units.toml').read_text())
        unit = next(u for u in config['unit'] if u['unit'] == 'serialobjectfactory')
        header = ROOT / 'include/Gruntz/UserLogic.h'
        source = header.read_text()
        declaration = '    virtual ~CUserLogic() OVERRIDE {}\n'
        self.assertEqual(source.count(declaration), 1)
        original_flags = cl.repo_include_flags()
        results = []
        with tempfile.TemporaryDirectory(prefix='gruntz-lifetime-negative-') as directory:
            folder = Path(directory)
            overlay = folder / 'Gruntz/UserLogic.h'
            overlay.parent.mkdir()
            for name, text in (('authored', source), ('omitted', source.replace(declaration, ''))):
                overlay.write_text(text)
                output = folder / (name + '.obj')
                with patch.object(cl, 'repo_include_flags', return_value=[f'/I{winepath(folder)}', *original_flags]):
                    cl.compile(ROOT / unit['source'], output, config['flags'][unit['flags']])
                body, refs, size = _find_function(Obj(output), '??1CUserLogic@@UAE@XZ')
                self.assertIsNotNone(body)
                results.append((size, {target for target, _ in refs.values()}))
        self.assertEqual([result[0] for result in results], [68, 62])
        self.assertIn('??_7CUserLogic@@6B@', results[0][1])
        self.assertNotIn('??_7CUserLogic@@6B@', results[1][1])


if __name__ == '__main__':
    unittest.main()
