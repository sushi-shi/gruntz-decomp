"""Controls for the FP-pool width the canonicalizer names a `$T` constant by."""

import struct
import unittest

from gruntz.compare.canonicalize import canonicalize_coff

MILLI = bytes.fromhex("6f12833a")  # 0.001f
FLD_M32 = b"\xd9\x05"
FLD_M64 = b"\xdd\x05"
FMUL_M32_SIB = b"\xd8\x0c\x85"  # fmul dword [eax*4+disp32]
MOV_EAX_MOFFS = b"\xa1"


def obj(pool: bytes, *operands: bytes) -> bytes:
    """One `.text` whose operands each name `$T1`, the sole `.rdata` datum."""
    code, sites = bytearray(), []
    for opcode in operands:
        code += opcode
        sites.append(len(code))
        code += bytes(4)
    code += b"\xc3"
    rawptr = 20 + 2 * 40
    relptr = rawptr + len(code)
    poolptr = relptr + 10 * len(sites)
    symptr = poolptr + len(pool)
    strings = bytearray(bytes(4))
    symbols = bytearray()
    for name, section, typ, storage in (("$T1", 2, 0, 3), ("_entry", 1, 0x20, 2)):
        symbols += struct.pack("<II", 0, len(strings))
        strings += name.encode("latin1") + b"\0"
        symbols += struct.pack("<IhHBB", 0, section, typ, storage, 0)
    struct.pack_into("<I", strings, 0, len(strings))
    header = struct.pack("<HHIIIHH", 0x14c, 2, 0, symptr, 2, 0, 0)
    text = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(code), rawptr,
                       relptr, 0, len(sites), 0, 0x60500020)
    rdata = struct.pack("<8sIIIIIIHHI", b".rdata", 0, 0, len(pool), poolptr,
                        0, 0, 0, 0, 0x40400040)
    relocs = b"".join(struct.pack("<IIH", site, 0, 6) for site in sites)
    return header + text + rdata + bytes(code) + relocs + pool + symbols + strings


def canonical(data: bytes) -> str:
    row = next(row for row in canonicalize_coff(data).rows
               if row.original_name == "$T1")
    return row.canonical_name


class FloatPoolWidthControls(unittest.TestCase):
    def test_zero_tail_read_as_dword_is_the_packed_float(self):
        packed = canonical(obj(MILLI, FLD_M32))
        self.assertEqual(packed, "$anon_f32_3a83126f_0")
        self.assertEqual(canonical(obj(MILLI + bytes(4), FLD_M32)), packed)
        self.assertEqual(
            canonical(obj(MILLI + bytes(4), FLD_M32, FMUL_M32_SIB)), packed)

    def test_zero_tail_read_as_qword_stays_a_double(self):
        self.assertEqual(canonical(obj(MILLI + bytes(4), FLD_M64)),
                         "$anon_f64_000000003a83126f_0")

    def test_operands_that_prove_no_width_leave_the_span_ambiguous(self):
        for operands in ((MOV_EAX_MOFFS,), (FLD_M32, FLD_M64), ()):
            with self.subTest(operands=operands):
                name = canonical(obj(MILLI + bytes(4), *operands))
                self.assertTrue(name.startswith("$anon_data_"), name)


if __name__ == "__main__":
    unittest.main()


def calls(*names: str) -> bytes:
    """One `.text` calling each named undefined external in turn."""
    code = b"".join(b"\xe8" + bytes(4) for _ in names) + b"\xc3"
    rawptr = 20 + 40
    relptr = rawptr + len(code)
    symptr = relptr + 10 * len(names)
    strings = bytearray(bytes(4))
    symbols = bytearray()
    for name, section, typ in (("_entry", 1, 0x20), *((n, 0, 0x20) for n in names)):
        symbols += struct.pack("<II", 0, len(strings))
        strings += name.encode("latin1") + b"\0"
        symbols += struct.pack("<IhHBB", 0, section, typ, 2, 0)
    struct.pack_into("<I", strings, 0, len(strings))
    header = struct.pack("<HHIIIHH", 0x14c, 1, 0, symptr, 1 + len(names), 0, 0)
    text = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(code), rawptr,
                       relptr, 0, len(names), 0, 0x60500020)
    relocs = b"".join(struct.pack("<IIH", 5 * k + 1, k + 1, 0x14) for k in range(len(names)))
    return header + text + bytes(code) + relocs + symbols + strings


class OldnamesAliasControls(unittest.TestCase):
    def setUp(self):
        from unittest import mock
        from gruntz.compare import canonicalize
        patch = mock.patch.object(canonicalize, "_oldnames_aliases",
                                  return_value={"_stricmp": "__stricmp"})
        patch.start()
        self.addCleanup(patch.stop)

    def targets(self, data: bytes) -> list[str]:
        from gruntz.compare.canonicalize import CoffObject
        coff = CoffObject(canonicalize_coff(data).data)
        return [coff.symbols[r.symbol_index].name for r in coff.relocations]

    def test_an_old_name_call_names_the_real_crt_function(self):
        self.assertEqual(self.targets(calls("_stricmp")), ["__stricmp"])

    def test_both_spellings_in_one_object_reach_one_symbol(self):
        self.assertEqual(self.targets(calls("_stricmp", "__stricmp")),
                         ["__stricmp", "__stricmp"])

    def test_other_names_are_untouched(self):
        self.assertEqual(self.targets(calls("_strlen")), ["_strlen"])
