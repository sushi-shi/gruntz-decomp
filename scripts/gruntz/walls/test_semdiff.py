from __future__ import annotations

import struct
import unittest

from gruntz.walls.semdiff import (Line, _decode, ebp_is_frame, exclusive,
                                  features)

ME = "?Fn@C@@QAEHXZ"


def _lines(asms):
    return [Line(i * 4, a, None) for i, a in enumerate(asms)]


def _keys(base, target):
    ebp = ebp_is_frame(base, target)
    return {(k, key) for k, key, _u, _v in exclusive(
        features(base, ME, ebp), features(target, ME, ebp))}


def _switch_body(pad: int):
    """xor edx,edx / mov dl,[eax+I] / jmp [edx*4+T] / two `mov eax,K; ret`
    arms, `pad` nops, the jump table, then a byte index table - the same
    function with its tables at a different offset."""
    code = bytes.fromhex("33d2" "8a90" "00000000" "ff2495" "00000000"
                         "b80a000000c3" "b814000000c3") + b"\x90" * pad
    table = len(code)
    index = table + 8
    body = bytearray(code + struct.pack("<II", 0x0F, 0x15)
                     + bytes([1, 0, 0, 1, 1, 0]))
    struct.pack_into("<I", body, 4, index)
    struct.pack_into("<I", body, 0xB, table)
    rel = {4: (ME, 6), 0xB: (ME, 6), table: (ME, 6), table + 4: (ME, 6)}
    return bytes(body), rel, table


class JumpTableControls(unittest.TestCase):
    def test_tables_at_another_offset_screen_clean(self):
        """The table operands and the reloc-less index table are not keys:
        before the fix `[eax+0x24]` vs `[eax+0x28]` read as a member swap."""
        (ba, ra, ta), (bb, rb, tb) = _switch_body(1), _switch_body(5)
        la, lb = _decode(ba, ra, ME), _decode(bb, rb, ME)
        self.assertTrue(all(ln.addr < ta for ln in la))
        self.assertTrue(all(ln.addr < tb for ln in lb))
        self.assertEqual(_keys(la, lb), set())


class WidthMirrorControls(unittest.TestCase):
    def test_narrow_logic_equals_its_32_bit_mirror(self):
        self.assertEqual(_keys(_lines(["and al,0xe0", "or ah,0xc"]),
                               _lines(["and ecx,0xffffffe0", "or eax,0xc00"])),
                         set())

    def test_a_different_mask_is_still_flagged(self):
        """`and al,0x1f` keeps the upper bytes; `and eax,0x1f` clears them."""
        self.assertEqual(_keys(_lines(["and al,0x1f"]),
                               _lines(["and eax,0x1f"])),
                         {("imm", "0xffffff1f"), ("imm", "0x1f")})


if __name__ == "__main__":
    unittest.main()
