from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from gruntz import cli
from gruntz.permute import tu_state_noise as noise


class TuStateNoiseTests(unittest.TestCase):
    def target(self, text: str) -> noise.Target:
        marker = "RVA(0x00123456,"
        marker_offset = text.index(marker)
        insertion = noise._leading_metadata_offset(text, marker_offset)
        return noise.Target(
            Path("unit.cpp"), "unit", 0x123456, 0x523456,
            "?Target@@YAHXZ", 1, marker_offset, insertion,
            noise.logical_line_at(text, insertion),
        )

    def test_target_and_top_insertions_restore_logical_line(self):
        original = (
            "#include <a.h>\n\nint predecessor;\n\n// evidence\n"
            "RVA(0x00123456, 0x1)\nint Target() { return 1; }\n"
        )
        target = self.target(original)
        variant = noise.Variant(1, "typedef", "tag", "typedef int PROBE;\n")
        beside = noise.insert_variant(original, target, variant, "target")
        top = noise.insert_variant(original, target, variant, "top")
        self.assertLess(beside.index("PROBE"), beside.index("// evidence"))
        self.assertLess(top.index("PROBE"), top.index("int predecessor"))
        self.assertIn("#line 3\nint predecessor", top)

    def test_forest_is_deterministic_broad_and_replayable(self):
        left = noise.make_variants(12, noise.DEFAULT_FAMILIES, 123)
        right = noise.make_variants(12, noise.DEFAULT_FAMILIES, 123)
        self.assertEqual(left, right)
        self.assertTrue(all(variant.family == "forest" for variant in left))
        self.assertGreaterEqual(left[0].body.count("typedef "), 10)
        self.assertGreaterEqual(left[0].body.count("class "), 10)
        self.assertGreaterEqual(left[0].body.count("PROTOTYPE_"), 10)
        self.assertGreaterEqual(left[0].body.count("FUNCTION_"), 10)
        self.assertEqual(noise.select_variants(left, (4, 11), 12), [left[3], left[10]])
        with self.assertRaisesRegex(ValueError, "exceeds --trials"):
            noise.select_variants(left, (13,), 12)

    def test_state_identity_masks_only_compiler_private_ordinals(self):
        base = {
            "objdiff_size": 4,
            "text_sha": "same",
            "reloc_stream": ["00000000:0006:$SG123:00000000"],
        }
        renumbered = dict(base)
        renumbered["reloc_stream"] = ["00000000:0006:$SG999:00000000"]
        self.assertEqual(
            noise.target_state_identity(base),
            noise.target_state_identity(renumbered),
        )
        changed = dict(base, text_sha="different")
        self.assertNotEqual(
            noise.target_state_identity(base),
            noise.target_state_identity(changed),
        )

    def test_exact_closure_requires_score_size_and_ordered_relocations(self):
        metrics = {
            "reloc_stream_complete": True,
            "reloc_stream": ["00000001:0006:_target:04000000"],
        }
        self.assertEqual(
            noise.exact_closure_rejections(100.0, 6, 6, metrics, metrics), []
        )
        self.assertIn(
            "unrounded objdiff score is not exactly 100.0",
            noise.exact_closure_rejections(99.999, 6, 6, metrics, metrics),
        )
        other = dict(metrics, reloc_stream=["00000001:0006:_other:04000000"])
        self.assertIn(
            "ordered relocation offsets/types/identities/addends differ from retail",
            noise.exact_closure_rejections(100.0, 6, 6, metrics, other),
        )

    def test_resolve_target_reads_current_model_serialization(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/unit.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("RVA(0x00123456, 0x1)\nint Target() { return 1; }\n")
            (root / "config").mkdir()
            (root / "config/units.toml").write_text(
                '[flags]\ncpp = ["/c"]\n\n[[unit]]\nunit = "unit"\n'
                'source = "src/unit.cpp"\nflags = "cpp"\n'
            )
            (root / "build/gen").mkdir(parents=True)
            (root / "build/gen/bindings.tsv").write_text(
                "# generated\n"
                "rva\tsize\tkind\tspace\tname\tunit\tchannel\talso_units\taliases\n"
                "0x00123456\t0x1\t\ttext\t?Target@@YAHXZ\tunit\tsrc\t\t\n"
            )
            target, flags = noise.resolve_target(root, Path("src/unit.cpp"), 0x123456)
        self.assertEqual(target.symbol, "?Target@@YAHXZ")
        self.assertEqual(target.retail_size, 1)
        self.assertEqual(flags, ["/c"])


class PermuteCliGateTests(unittest.TestCase):
    def run_cli(self, diagnosis: str, hist: float = 99.0):
        binding = SimpleNamespace(rva=0x123456, unit="unit", name="?Target@@YAHXZ")

        def diagnose(_token):
            print(diagnosis)
            return 0

        with mock.patch("gruntz.walls.diagnose.diagnose", side_effect=diagnose), \
             mock.patch("gruntz.model.resolve", return_value=SimpleNamespace(
                 functions=[binding]
             )), \
             mock.patch("gruntz.verify.baseline.load", return_value={
                 (binding.unit, binding.name): {"hist": hist}
             }), \
             mock.patch("gruntz.permute.tu_state_noise.main", return_value=17) as run:
            result = cli.main([
                "permute", "state", "--source", "src/unit.cpp",
                "--rva", "0x123456",
            ])
        return result, run

    def test_public_command_forwards_only_regalloc_wall_below_max(self):
        result, run = self.run_cli("class: REGALLOC/SCHEDULING")
        self.assertEqual(result, 17)
        run.assert_called_once()

    def test_public_command_refuses_cfg_and_historical_exact(self):
        result, run = self.run_cli("class: CFG")
        self.assertEqual(result, 2)
        run.assert_not_called()
        result, run = self.run_cli("class: REGALLOC/SCHEDULING", hist=100.0)
        self.assertEqual(result, 2)
        run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
