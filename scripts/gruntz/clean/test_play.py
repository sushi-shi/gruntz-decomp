"""Controls for the exported launcher's data ownership and build-before-run flow."""

from contextlib import redirect_stderr
import importlib.util
import io
from pathlib import Path
import subprocess
import tempfile
from types import ModuleType
import unittest
from unittest.mock import Mock, patch

SPEC = importlib.util.spec_from_file_location('source_play', Path(__file__).parent / 'project/play.py')
play = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(play)


class LaunchControls(unittest.TestCase):
    def fixture(self, root):
        data = root / 'original data'
        data.mkdir()
        for name in play.ASSETS:
            (data / name.lower()).write_bytes(b'original asset')
        build = root / 'build'
        build.mkdir()
        executable = build / 'GRUNTZ.EXE'
        executable.write_bytes(b'compiled game')
        dll = root / 'real.dll'
        dll.write_bytes(b'real runtime')
        return data, executable, dll

    def test_case_insensitive_assets_and_missing_data(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data, _, _ = self.fixture(root)
            self.assertEqual(set(play.assets_in(data)), set(play.ASSETS))
            (data / 'tiny.fnt').unlink()
            with self.assertRaisesRegex(ValueError, 'TINY.FNT'):
                play.assets_in(data)

    def test_install_preserves_saves_and_never_modifies_original_data(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data, executable, dll = self.fixture(root)
            game = root / 'game'
            game.mkdir()
            saved = game / 'Gruntz.sav'
            saved.write_bytes(b'progress')
            assets = play.assets_in(data)
            for _ in range(2):
                play.install_files(game, assets, executable, {'MSS32.DLL': dll})
            self.assertEqual(saved.read_bytes(), b'progress')
            self.assertEqual((game / 'GRUNTZ.EXE').read_bytes(), b'compiled game')
            self.assertEqual((data / 'gruntz.rez').read_bytes(), b'original asset')
            self.assertTrue((game / 'MSS32.DLL').is_symlink())
            (game / 'MSS32.DLL').unlink()
            (game / 'MSS32.DLL').write_bytes(b'local file')
            with self.assertRaisesRegex(ValueError, 'refusing to replace'):
                play.install_files(game, assets, executable, {'MSS32.DLL': dll})

    def test_build_then_launch_and_remember_data_folder(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data, _, dll = self.fixture(root)
            runner = root / 'play.sh'
            helper = ModuleType('gruntzbuild.play')
            helper.write_play_sh = Mock(return_value=runner)
            with patch.object(play, 'ROOT', root), patch.object(play, 'prepare_prefix') as prepare, \
                    patch.dict('os.environ', {'GRUNTZ_MSS32': str(dll), 'GRUNTZ_SMACKW32': str(dll)}), \
                    patch.dict('sys.modules', {'gruntzbuild.play': helper}), \
                    patch.object(play.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0)) as run:
                self.assertEqual(play.main(['--data', str(data)]), 0)
                self.assertEqual(play.main([]), 0)
                self.assertEqual(prepare.call_count, 2)
                commands = [call.args[0] for call in run.call_args_list]
                self.assertEqual(commands[0][1], str(root / 'build.py'))
                self.assertEqual(commands[1], [str(runner)])
                self.assertEqual(commands[2:], commands[:2])

    def test_failed_build_does_not_install_or_launch(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            data, _, dll = self.fixture(root)
            with patch.object(play, 'ROOT', root), patch.object(play, 'prepare_prefix') as prepare, \
                    patch.dict('os.environ', {'GRUNTZ_MSS32': str(dll), 'GRUNTZ_SMACKW32': str(dll)}), \
                    patch.object(play.subprocess, 'run', side_effect=subprocess.CalledProcessError(1, 'build')) as run, \
                    redirect_stderr(io.StringIO()):
                self.assertEqual(play.main(['--data', str(data)]), 1)
                self.assertEqual(run.call_count, 1)
                prepare.assert_not_called()
                self.assertFalse((root / 'build/game').exists())


if __name__ == '__main__':
    unittest.main()
