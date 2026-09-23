from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest import mock

from gruntz.lineage import main
from gruntz.lineage import objects


def record(kind, data=b""):
    return struct.pack("<HH", len(data) + 2, kind) + data


def string(text):
    data = text.encode("latin-1")
    return bytes([len(data)]) + data


def proc(name):
    return record(0x100B, struct.pack("<IIIIIIIIHB", 0, 0, 0, 16, 0, 16, 0x1000, 0, 1, 0)
                  + string(name))


def local(name, offset=-4, type_index=0x74):
    return record(0x1006, struct.pack("<iI", offset, type_index) + string(name))


def coff(sections):
    """Small actual i386 COFF, exercising the same Obj consumer as real libs."""
    offset = 20 + 40 * len(sections)
    headers, content = b"", b""
    for name, data, chars in sections:
        headers += struct.pack("<8sIIIIIIHHI", name.encode().ljust(8, b"\0"),
                               0, 0, len(data), offset, 0, 0, 0, 0, chars)
        content += data
        offset += len(data)
    symbol = struct.pack("<8sIhHBB", b"_run\0\0\0\0", 0, 1, 0x20, 2, 0)
    return (struct.pack("<HHIIIHH", 0x14C, len(sections), 0, offset, 1, 0, 0)
            + headers + content + symbol + struct.pack("<I", 4))


class CodeViewTests(unittest.TestCase):
    def test_scope_identity_distinguishes_sibling_locals_with_shared_slot(self):
        block = record(0x207, struct.pack("<IIIIH", 0, 0, 8, 0, 1) + string(""))
        data = (b"\x02\0\0\0" + proc("Run") + local("outer")
                + block + local("left", -8) + record(6)
                + block + local("right", -8) + record(6) + record(6))
        rows = objects.symbols(data, signature=True)
        locals_ = [r for r in rows if r["record"] == "local"]
        self.assertEqual([r["name"] for r in locals_], ["outer", "left", "right"])
        self.assertNotEqual(locals_[1]["scope"], locals_[2]["scope"])
        self.assertEqual(locals_[1]["frame_offset"], locals_[2]["frame_offset"])

    def test_unknown_record_is_retained_not_evidence_of_absence(self):
        rows = objects.symbols(record(0x7FFF, b"abc"), signature=False)
        self.assertEqual(rows[0]["payload_hex"], "616263")

    def test_types_keep_raw_payload_and_type_index(self):
        rows = objects.types(b"\x02\0\0\0" + record(0x1003, b"array"))
        self.assertEqual(rows[0]["index"], "0x1000")
        self.assertEqual(rows[0]["payload_hex"], b"array".hex())

    def test_malformed_streams_fail_loudly(self):
        for data in (b"x", b"\x01\0\x06\0", b"\x20\0\x06\0"):
            with self.subTest(data=data), self.assertRaises(ValueError):
                list(objects.records(data))
        for data in (record(6), proc("Run"), record(0x1006, b"x")):
            with self.subTest(data=data), self.assertRaises(ValueError):
                objects.symbols(data, signature=False)
        with self.assertRaises(ValueError):
            objects.symbols(b"\x04\0\0\0", signature=True)
        with self.assertRaises(ValueError):
            objects.pstring(b"\x05a", 0)

    def test_external_pdb_is_not_fabricated_as_local_type_1000(self):
        data = (b"\x02\0\0\0" + record(0x16, struct.pack("<II", 123, 2) + string("vc60.pdb"))
                + record(0x1002, b"raw"))
        rows = objects.types(data)
        self.assertEqual(rows[0]["record"], "type_server")
        self.assertEqual(rows[0]["name"], "vc60.pdb")
        self.assertNotIn("index", rows[0])
        self.assertIsNone(rows[1]["index"])

    def test_signature_change_is_not_an_exact_match(self):
        bank = {"?Save@Owner@@QAE_NXZ": {"historical": 90}}
        later = {"?Save@Owner@@QAE_NPBD@Z"}
        result = objects.overlap(later, later, bank)
        self.assertFalse(result["exact_name_overlap"])
        self.assertEqual(len(result["signature_different_candidates"]), 1)

    def test_symbol_absent_from_release_is_not_a_pair(self):
        bank = {"_run": {"historical": 90}}
        self.assertFalse(objects.overlap({"_run"}, set(), bank)["exact_name_overlap"])

    def test_ambiguous_member_basename_is_rejected(self):
        with mock.patch("subprocess.check_output", return_value=b"a\\foo.obj\nb\\FOO.OBJ\n"):
            with self.assertRaisesRegex(ValueError, "ambiguous"):
                objects.members(Path("archive.lib"))

    def test_cli_archive_to_coff_to_continuation_stream(self):
        primary = b"\x02\0\0\0" + proc("Run") + local("count") + record(6)
        continuation = proc("Inline") + local("this", type_index=0x1001) + record(6)
        data = coff([(".text", b"\xc3", 0x20), (".debug$S", primary, 0),
                     (".debug$S", continuation, 0),
                     (".debug$T", b"\x02\0\0\0" + record(0x1002, b"raw"), 0)])

        def ar(argv):
            return b".\\Build\\sample.obj\n" if argv[1] == "t" else data

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ("debug.lib", "release.lib"):
                (root / name).write_bytes(b"fixture")
            bank = root / "bank.tsv"
            bank.write_text("unit\t_run\t90\t89\t1\thash\t0x1000\t95\t\n")
            args = ["objects", "--debug", str(root / "debug.lib"), "--release",
                    str(root / "release.lib"), "--baseline", str(bank), "--member", "sample.obj"]
            with mock.patch("subprocess.check_output", side_effect=ar), contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(main(args), 0)
            pair = json.loads(out.getvalue())["pairs"][0]
            member = pair["members"][0]["debug"]
            self.assertEqual(member["local_records"], 2)
            self.assertEqual(member["type_records"], 1)
            self.assertEqual(pair["below_historical_100"][0]["rva"], "0x1000")
            data = coff([(".text", b"\xc3", 0x20), (".debug$S", primary[:-1], 0)])
            with mock.patch("subprocess.check_output", side_effect=ar), contextlib.redirect_stderr(io.StringIO()) as err:
                self.assertEqual(main(args), 1)
            self.assertIn("library evidence failed", err.getvalue())


if __name__ == "__main__":
    unittest.main()
