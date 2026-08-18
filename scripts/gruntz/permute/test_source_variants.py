from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from gruntz.permute import batch_source_variants as batch
from gruntz.permute.generate_ast_variants import (
    AstEdit,
    AstMutation,
    candidate_payloads,
    crossed_candidate_payloads,
    marker_span,
    non_overlapping,
)
from gruntz.permute.topology import compare_topology, topology_rank


class BatchSourceVariantTests(unittest.TestCase):
    def test_exact_axes_form_a_cartesian_product(self):
        original = b"left + right\n"
        left = batch.Axis("left", 0, 4, b"left", (
            batch.AxisOption("keep", b"left"),
            batch.AxisOption("rename", b"first"),
        ))
        right = batch.Axis("right", 7, 12, b"right", (
            batch.AxisOption("keep", b"right"),
            batch.AxisOption("rename", b"second"),
        ))
        variants = list(batch.iter_variants(original, (left, right), ()))
        self.assertEqual(len(variants), 4)
        self.assertEqual(variants[-1][0], b"first + second\n")
        self.assertEqual(
            variants[-1][1], {"left": "rename", "right": "rename"}
        )

    def test_manifest_requires_unique_exact_spans_and_nonoverlap(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/unit.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("int value = left + right;\n")
            manifest = root / "axes.json"
            manifest.write_text(json.dumps({
                "schema": 1,
                "source": "src/unit.cpp",
                "rva": "0x1234",
                "axes": [{
                    "name": "order",
                    "find": "left + right",
                    "options": [
                        {"name": "keep"},
                        {"name": "swap", "replace": "right + left"},
                    ],
                }],
            }))
            _payload, _source, _original, axes, candidates, rva = \
                batch.load_manifest(manifest, root)
            self.assertEqual(rva, 0x1234)
            self.assertEqual(len(axes), 1)
            self.assertEqual(candidates, ())

            payload = json.loads(manifest.read_text())
            payload["axes"].append(dict(payload["axes"][0], name="overlap"))
            manifest.write_text(json.dumps(payload))
            with self.assertRaisesRegex(ValueError, "axes overlap"):
                batch.load_manifest(manifest, root)

    def test_axis_option_extra_edit_is_atomic(self):
        original = b"int helper;\nint result = old_call;\n"
        axis = batch.Axis("call", 25, 33, b"old_call", (
            batch.AxisOption("keep", b"old_call"),
            batch.AxisOption(
                "helper", b"new_call",
                (batch.Edit(0, 0, b"", b"static int new_call;\n"),),
            ),
        ))
        variants = list(batch.iter_variants(original, (axis,), ()))
        self.assertEqual(variants[1][1], {"call": "helper"})
        self.assertEqual(
            variants[1][0],
            b"static int new_call;\nint helper;\nint result = new_call;\n",
        )

    def test_result_rank_prefers_score_then_size_then_relocations(self):
        best = {"score": 99.0, "candidate_size": 10, "candidate_relocs": 2, "trial": 2}
        lower = {"score": 98.0, "candidate_size": 10, "candidate_relocs": 2, "trial": 1}
        self.assertLess(batch.result_rank(best, 10, 2), batch.result_rank(lower, 10, 2))

    def test_frontier_keeps_highest_distinct_states(self):
        with tempfile.TemporaryDirectory() as directory:
            scratch = Path(directory)
            candidate = scratch / "candidate.obj"
            frontier = {}
            for state, score in (("a", 90), ("b", 95), ("a", 92), ("c", 91)):
                candidate.write_bytes(state.encode())
                row = {"score": score}
                batch.retain_frontier_candidate(
                    frontier, 2, state, (-score,), row, state.encode(),
                    candidate, scratch,
                )
            self.assertEqual(set(frontier), {"a", "b"})
            self.assertEqual(frontier["a"]["row"]["score"], 92)


class AstVariantTests(unittest.TestCase):
    def test_marker_span_uses_real_rva_markers(self):
        blob = (
            b"// RVA(0x00123456, in a comment\n"
            b"RVA(0x00123456, 0x1)\nint Target() { return 1; }\n"
            b"RVA(0x00123460, 0x1)\nint Next() { return 2; }\n"
        )
        start, end = marker_span(blob, 0x123456)
        self.assertEqual(blob[start:start + 4], b"RVA(")
        self.assertTrue(blob[end:].lstrip().startswith(b"RVA("))

    def test_candidate_generation_rejects_overlapping_edits(self):
        blob = b"abcdef"
        mutations = [
            AstMutation("a", "first", (AstEdit(1, 3, b"XX"),)),
            AstMutation("b", "second", (AstEdit(2, 4, b"YY"),)),
        ]
        candidates, truncated = candidate_payloads(blob, mutations, 2, 20)
        self.assertFalse(truncated)
        self.assertEqual(len(candidates), 2)
        self.assertTrue(all(len(candidate["edits"]) == 1 for candidate in candidates))
        self.assertFalse(non_overlapping((mutations[0].edits[0], mutations[1].edits[0])))

    def test_source_shapes_are_crossed_with_all_tu_states(self):
        blob = b"abcdef"
        source = [AstMutation("source", "swap", (AstEdit(0, 1, b"A"),))]
        states = [
            AstMutation("tu_state_forest", "one", (AstEdit(6, 6, b"X"),)),
            AstMutation("tu_state_forest", "two", (AstEdit(6, 6, b"Y"),)),
        ]
        candidates, truncated, source_count, state_count = crossed_candidate_payloads(
            blob, source, states, max_depth=1, limit=6,
        )
        self.assertFalse(truncated)
        self.assertEqual((source_count, state_count, len(candidates)), (2, 3, 6))
        self.assertEqual(candidates[0]["name"], "baseline")
        self.assertIn("source:swap", candidates[-1]["name"])
        self.assertIn("tu_state_forest:two", candidates[-1]["name"])


class TopologyRankTests(unittest.TestCase):
    def test_structural_match_outranks_a_higher_fuzzy_wrong_shape(self):
        retail = {
            "instructions": 4, "branches": 1, "returns": 1,
            "flow": [["jne", 3], ["ret", None]],
        }
        exact_shape = compare_topology(dict(retail), retail)
        wrong_shape = compare_topology({
            "instructions": 5, "branches": 2, "returns": 1,
            "flow": [["jne", 3], ["jmp", 4], ["ret", None]],
        }, retail)
        self.assertLess(
            topology_rank(exact_shape, 95.0),
            topology_rank(wrong_shape, 99.0),
        )


if __name__ == "__main__":
    unittest.main()
