from pathlib import Path
import struct
import tempfile
import unittest

from gruntz.compare.test_function_sizes import obj
from gruntz.delink.coffx import Obj
from gruntz.walls.diagnose import _find_function
from gruntz.walls.pairscan import function_body, insns


class FunctionBodyTests(unittest.TestCase):
    def load(self, code, **kwargs):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = Path(directory.name) / 'sample.obj'
        path.write_bytes(obj(code, **kwargs))
        return Obj(path)

    def test_jump_operands_are_not_alignment_bytes(self):
        for operand in (0x90, 0xcc):
            with self.subTest(operand=operand):
                code = bytes((0xeb, operand))
                coff = self.load(code, relocs=())
                self.assertEqual(_find_function(coff, '_entry'), (code, {}, 2))
                decoded = insns(coff, 1, 0, 2)
                self.assertEqual(len(decoded), 1)
                self.assertEqual(decoded[0][1], 'jmp')

    def test_operand_is_kept_when_real_padding_follows(self):
        code = b'\xeb\x90'
        coff = self.load(code + b'\x90\xcc\x90', relocs=())
        self.assertEqual(function_body(coff, 1, 0, 5), code)

    def test_return_immediate_is_kept(self):
        code = b'\xc2\x04\xcc'
        coff = self.load(code + b'\xcc\xcc', relocs=())
        self.assertEqual(_find_function(coff, '_entry')[0], code)

    def test_fallthrough_nops_are_not_proven_padding(self):
        code = b'\x40\x90\x90'
        coff = self.load(code, relocs=())
        self.assertEqual(function_body(coff, 1, 0, len(code)), code)

    def test_branch_into_terminal_nop_keeps_it(self):
        code = b'\x74\x01\xc3\x90'
        coff = self.load(code, relocs=())
        self.assertEqual(function_body(coff, 1, 0, len(code)), code)

    def test_relocation_operand_in_tail_is_not_trimmed(self):
        code = b'\xc3\x90\x90\x90\x90'
        coff = self.load(code, relocs=((1, 1, 6),))
        self.assertEqual(function_body(coff, 1, 0, len(code)), code)

    def test_embedded_table_prevents_linear_padding_inference(self):
        code = b'\xb8\x00\x00\x00\x00\xc3\x90\x90'
        coff = self.load(code, relocs=((1, 0, 6),))
        self.assertEqual(function_body(coff, 1, 0, len(code)), code)

    def test_pointer_from_another_window_preserves_referenced_tail(self):
        code = b'\xc3\x90\x90\x90' + struct.pack('<I', 3)
        next_symbol = ('_next', 4, 1, 0x20, 2, b'')
        coff = self.load(code, extra=(next_symbol,), relocs=((4, 0, 6),))
        self.assertEqual(function_body(coff, 1, 0, 4), code[:4])

    def test_label_in_tail_preserves_its_extent(self):
        code = b'\xc3\x90\x90\x90'
        label = ('$L10', 2, 1, 0, 3, b'')
        coff = self.load(code, extra=(label,), relocs=())
        self.assertEqual(function_body(coff, 1, 0, 4), code)

    def test_explicit_extent_is_authoritative_even_with_terminal_nop(self):
        aux = struct.pack('<IIIIH', 0, 2, 0, 0, 0)
        coff = self.load(b'\xc3\x90\x90\x90', relocs=(), function_aux=aux)
        self.assertEqual(function_body(coff, 1, 0, 4), b'\xc3\x90')

    def test_invalid_extent_does_not_truncate_an_operand(self):
        aux = struct.pack('<IIIIH', 0, 99, 0, 0, 0)
        coff = self.load(b'\xeb\x90', relocs=(), function_aux=aux)
        self.assertEqual(function_body(coff, 1, 0, 2), b'\xeb\x90')


if __name__ == '__main__':
    unittest.main()
