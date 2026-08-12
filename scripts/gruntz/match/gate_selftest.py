#!/usr/bin/env python3
"""gruntz.match.gate_selftest - NEGATIVE CONTROLS for the build gates.

A gate nobody has seen FAIL is a green light, not a check.

Every defect this suite pins was a gate that looked healthy and reported a number
nobody had ever falsified:

  * ``vtable_slot_binding`` read its own baseline through ``csv.DictReader``, which
    ate the ``#`` banner as the header row - so the frozen backlog parsed as EMPTY
    and the gate passed everything, forever. Found by writing a negative control.
  * ``cleanliness`` dropped a ratcheted metric's row whenever its subprocess failed,
    and the next ``save_baseline`` deleted that metric's floor.

Each test below feeds a gate a KNOWN violation and asserts it FAILS, and feeds it clean
input and asserts it PASSES. Both halves matter: a gate that always fails is as useless
as one that never does. The tests are hermetic - they build tiny synthetic trees in a
tmpdir and never read build/ or the real src/ - so they run in ~a second with no build.

    python -m gruntz.match.gate_selftest          # run them all
    python -m gruntz.match.gate_selftest -v       # per-test detail
"""
from __future__ import annotations

import argparse
import contextlib
import io
import json
import os
import subprocess
import struct
import sys
import tempfile
import unittest
from unittest import mock
from pathlib import Path

from gruntz.audit import aggregate_copies, data_denominator, data_integrity, image_diff
from gruntz.audit import rename_member, tu_layout, tu_order_check
from gruntz.audit import nested_static_casts
from gruntz.cleanliness import board as cleanliness
from gruntz.cleanliness import view_debt
from gruntz.core import class_meta
from gruntz.core import branches
from gruntz.core import data_universe
from gruntz.core import function_universe
from gruntz.core import report
from gruntz.core import library_labels
from gruntz.build import canonicalize_data_symbols, data_manifest, labels, synth_pdb
from gruntz.cleanliness import vtable_slot_binding as vsb
from gruntz.match import residual_queue, status
from gruntz.permute import permute_sweep, tu_state_noise
from gruntz.match import verify_unique_names as vun


class RenameMemberToolTests(unittest.TestCase):
    def test_whole_tree_rename_has_no_file_count_cap(self):
        self.assertIn("--rename-file-limit=0", rename_member.clangd_command())


class SourceIdentityContractTests(unittest.TestCase):
    def test_address_and_stack_derived_identifiers_are_counted(self):
        source = """
            i32 m_8;
            CMapStringToOb m_10map;
            CString local_14;
            DWORD g_ratingRaw_64da84;
        """
        self.assertEqual(cleanliness._count_address_derived_identifiers(source), 4)
        self.assertEqual(
            cleanliness._count_address_derived_identifiers(
                "i32 m_reserved; CMapStringToOb m_workersByName; CString voiceSection;"
            ),
            0,
        )

    def test_data_compgen_accepts_no_source_identity_argument(self):
        _claims, errors = labels.compgen_tu(
            "double x = DATA_COMPGEN(0x001ea410, 1.0);", "sample.cpp", "sample", None
        )
        self.assertEqual(errors, [])

        _claims, errors = labels.compgen_tu(
            "double x = DATA_COMPGEN(0x001ea410, fp_1ea410, 1.0);",
            "sample.cpp",
            "sample",
            None,
        )
        self.assertEqual(len(errors), 1)
        self.assertIn("takes (addr, value)", errors[0][2])


class DataExtentAuthorityTests(unittest.TestCase):
    def test_pylibclang_sizes_current_tu_without_structs_cache(self):
        with tempfile.TemporaryDirectory() as td:
            source = Path(td) / "extent.cpp"
            source.write_text(
                "struct Rect { int left, top, right, bottom; };\n"
                "Rect g_rect;\n"
            )
            sizes = labels.clang_var_sizes(
                str(source), ["--target=i386-pc-windows-msvc"]
            )
            self.assertIsNotNone(sizes)
            self.assertIn(16, sizes.values())

    def test_pylibclang_omits_incomplete_extent(self):
        with tempfile.TemporaryDirectory() as td:
            source = Path(td) / "incomplete.cpp"
            source.write_text("extern char g_unknown[];\n")
            sizes = labels.clang_var_sizes(
                str(source), ["--target=i386-pc-windows-msvc"]
            )
            self.assertIsNotNone(sizes)
            self.assertFalse(any("g_unknown" in name for name in sizes))


class TuOrderExileControls(unittest.TestCase):
    EXILES = {0x1000: ("Owner", "Host", "Fn")}
    SPANS = {"Host": (0x1000, 0x1100)}

    def test_header_inline_owner_emission_proves_exile_pin(self):
        self.assertEqual(
            tu_order_check.verify_exiles(
                self.EXILES, {}, self.SPANS, {0x1000: {"owner", "host"}}
            ),
            [],
        )

    def test_host_only_emission_does_not_impersonate_owner(self):
        bad = tu_order_check.verify_exiles(
            self.EXILES, {}, self.SPANS, {0x1000: {"host"}}
        )
        self.assertEqual(len(bad), 1)
        self.assertIn("no owner RVA() pin/emission", bad[0])


class AggregateCopyAuditTests(unittest.TestCase):
    def test_plain_movsb_from_inline_data_is_not_an_aggregate_copy(self):
        self.assertIsNone(aggregate_copies.REP_MOVS.match("movsb"))
        self.assertIsNotNone(aggregate_copies.REP_MOVS.match("rep movsb"))
        self.assertIsNotNone(aggregate_copies.REP_MOVS.match("rep movsl"))


class TuStateNoiseControls(unittest.TestCase):
    @staticmethod
    def _reloc_metrics(stream):
        return {"reloc_stream_complete": True, "reloc_stream": stream}

    def test_exact_closure_folds_only_producer_specific_eh_relocations(self):
        candidate = self._reloc_metrics([
            "00000002:0006:__except_list:00000000",
            "00000009:0006:$L123:00000000",
            "00000020:0014:?Call@@YAXXZ:00000000",
        ])
        retail = self._reloc_metrics([
            "00000009:0006:__ehreg$?Fn@@YAXXZ:00000000",
            "00000020:0014:?Call@@YAXXZ:00000000",
        ])
        self.assertEqual(
            tu_state_noise.exact_closure_rejections(100.0, 32, 32, candidate, retail),
            [],
        )

    def test_exact_closure_rejects_a_real_referent_difference(self):
        candidate = self._reloc_metrics(["00000020:0014:?Wrong@@YAXXZ:00000000"])
        retail = self._reloc_metrics(["00000020:0014:?Right@@YAXXZ:00000000"])
        self.assertIn(
            "ordered relocation offsets/types/identities/addends differ from retail",
            tu_state_noise.exact_closure_rejections(100.0, 32, 32, candidate, retail),
        )

    def test_current_source_hash_uses_supported_fingerprinter_api(self):
        fingerprint = mock.Mock(return_value="range-hash")
        with mock.patch.object(
            status,
            "fingerprinter",
            return_value=(fingerprint, mock.Mock(), set()),
        ):
            self.assertEqual(tu_state_noise.current_source_hash("u", "?Fn@@YAXXZ"), "range-hash")
        fingerprint.assert_called_once_with("u", "?Fn@@YAXXZ")

    def test_record_exact_updates_best_and_historical_max_in_current_schema(self):
        original = (
            "# baseline\n"
            "u\t?Fn@@YAXXZ\t99.5000\t99.2500\t7\trange-hash\t0x1000\t99.7500\n"
            "u\t?Other@@YAXXZ\t80.0000\t79.0000\t3\tother-hash\t0x1100\t81.0000\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "match_baseline.tsv"
            path.write_text(original)
            result = tu_state_noise.record_target_max(
                path, "u", "?Fn@@YAXXZ", "range-hash", 100.0
            )
            lines = path.read_text().splitlines()
        self.assertTrue(result["updated"])
        self.assertEqual(
            lines[1],
            "u\t?Fn@@YAXXZ\t100.0000\t99.2500\t7\trange-hash\t0x1000\t100.0000",
        )
        self.assertEqual(lines[2], original.splitlines()[2])

    def test_record_exact_rejects_stale_source_hash(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "match_baseline.tsv"
            path.write_text(
                "u\t?Fn@@YAXXZ\t99.5000\t99.2500\t7\told-hash\t0x1000\t99.7500\n"
            )
            with self.assertRaisesRegex(tu_state_noise.BaselineUpdateError, "source hash mismatch"):
                tu_state_noise.record_target_max(path, "u", "?Fn@@YAXXZ", "new-hash", 100.0)

    def test_curated_includes_are_gruntz_or_toolchain_headers(self):
        self.assertFalse(any("BASE/" in header for header in tu_state_noise.CURATED_INCLUDES))

    def test_include_guard_searches_the_active_pinned_toolchain(self):
        with tempfile.TemporaryDirectory() as tmp, mock.patch.dict(
            os.environ, {"MSVC_DIR": str(Path(tmp) / "msvc")}
        ):
            roots = tu_state_noise.include_search_roots(Path(tmp) / "repo")
        self.assertIn(Path(tmp) / "msvc" / "include", roots)


class DataManifestAlignmentControls(unittest.TestCase):
    def test_absolute_rva_mismatch_is_a_placement_adjustment_not_a_gate(self):
        row = {"name": "?validContributionMember@@3PAY0BA@E",
               "object": "probe.c", "rva": 0x1004, "size": 16,
               "storage": "bss", "provenance": "test"}
        with mock.patch.object(data_manifest, "declared_types",
                               return_value={0x1004: "unsigned char[16]"}):
            adjusted = []
            manifest = data_manifest.manifest_bytes([row], adjusted).decode()
        self.assertEqual(adjusted[0][4:], (8, 4))
        self.assertIn("\tbss\t0x4\t-\t-\texternal\ttest\n", manifest)


class DataIntegrityRatchetTests(unittest.TestCase):
    @staticmethod
    def _run(current, limits):
        with mock.patch.object(data_integrity, "measures", return_value=current), \
                mock.patch.object(data_integrity, "_read", return_value=(limits, {})), \
                mock.patch.object(sys, "argv", ["data_integrity", "--gate"]), \
                contextlib.redirect_stdout(io.StringIO()):
            return data_integrity.main()

    def test_increase_and_unbanked_improvement_both_fail(self):
        baseline = {"unclaimed_runs": 5, "wrong_referent_regions": 7,
                    "ordering_only_regions": 2, "multiplicity_only_regions": 3}
        self.assertEqual(self._run({**baseline, "unclaimed_runs": 6}, baseline), 1)
        self.assertEqual(
            self._run({**baseline, "wrong_referent_regions": 6}, baseline), 1)
        self.assertEqual(self._run(dict(baseline), baseline), 0)

    def test_referent_gate_links_the_checked_retail_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            order = Path(tmp) / "order.txt"
            cand = Path(tmp) / "integrity.exe"
            mapfile = Path(tmp) / "integrity.map"
            with (mock.patch.object(data_integrity, "ORDER_LIST", order),
                  mock.patch.object(data_integrity, "INTEGRITY_CAND", cand),
                  mock.patch.object(data_integrity, "INTEGRITY_MAP", mapfile),
                  mock.patch.object(data_integrity.link_line, "check", return_value=0),
                  mock.patch.object(data_integrity.link_line, "objlist_text",
                                    return_value="# order\na\nb\n"),
                  mock.patch.object(data_integrity.subprocess, "run") as run):
                data_integrity._ensure_candidate()
            self.assertEqual(order.read_text(), "# order\na\nb\n")
            cmd = run.call_args.args[0]
            self.assertEqual(cmd[-3:], ["--order", str(order), "--engine-lib"])
            self.assertIn(str(cand), cmd)
            self.assertIn(str(mapfile), cmd)
            self.assertNotIn("ninja", cmd)


class DataCoverageReachabilityTests(unittest.TestCase):
    def test_library_boundary_stops_transitive_reachability(self):
        direct = {0: {"direct game/compiler code": 1}}
        enrolled = {4: 1}
        edges = {
            0: {1: 1},       # game-visible data follows its pointer
            2: {3: 1},       # library-private source has no game-side root
            4: {5: 1},       # enrolled data is an independent game-side root
        }
        visible, _ = data_denominator.propagate_reachability(
            direct, enrolled, edges)
        self.assertEqual(visible, {0, 1, 4, 5})

    def test_reachability_overrides_library_ownership(self):
        self.assertEqual(
            data_denominator.coverage_verdict(data_denominator.GUIDV, True),
            (data_denominator.VISIBLE, True))
        self.assertEqual(
            data_denominator.coverage_verdict(data_denominator.GUIDV, False),
            (data_denominator.GUIDV, False))
        self.assertEqual(
            data_denominator.coverage_verdict(data_denominator.UNK, False),
            (data_denominator.UNK, True))

    def test_scoreboard_rejects_a_stale_partition(self):
        header = ("rva\tend\tsize\tsection\tverdict\tcoverage\tattribution\t"
                  "evidence\treachability\n")
        rows = (
            "0x0000006e\t0x00000074\t6\t.rdata\tprivate\texcluded\tprivate\t-\t\n"
            "0x00000074\t0x00000078\t4\t.rdata\tunknown\teligible\tunknown\t-\t\n"
            "0x000000d2\t0x000000d7\t5\t.data\tprivate\texcluded\tprivate\t-\t\n"
            "0x000000d7\t0x000000dc\t5\t.data\tunknown\teligible\tunknown\t-\t\n"
            "0x000000dc\t0x000000f0\t20\t.bss\tunknown\teligible\tunknown\t-\t\n")
        regs = {"rdata": (100, 120), "data": (200, 220), "bss": (220, 240)}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "partition.tsv"
            path.write_text(header + rows)
            with mock.patch.object(data_universe, "PARTITION", path):
                enrolled = [(100, 110), (200, 210)]
                got = data_universe._partition(regs, enrolled)
                self.assertEqual(got["regions"]["rdata"]["excluded"], 6)
                self.assertEqual(
                    got["regions"]["bss"]["eligible_unenrolled"], 20)
                # Same byte totals at shifted addresses are stale too; range
                # identity matters, not just arithmetic coverage.
                path.write_text(header + rows.replace(
                    "0x0000006e\t0x00000074", "0x0000006d\t0x00000073"))
                self.assertIsNone(data_universe._partition(regs, enrolled))
                path.write_text(header + rows.splitlines(keepends=True)[0])
                self.assertIsNone(
                    data_universe._partition(regs, enrolled))


class ScoreBankingProvenanceTests(unittest.TestCase):
    def test_unstaged_and_untracked_inputs_are_both_reported(self):
        results = [
            subprocess.CompletedProcess([], 0, stdout="src/Live.cpp\n", stderr=""),
            subprocess.CompletedProcess([], 0, stdout="include/New.h\n", stderr=""),
        ]
        with mock.patch.object(status.subprocess, "run", side_effect=results):
            self.assertEqual(status.unstaged_bank_inputs(),
                             ["include/New.h", "src/Live.cpp"])

    def test_staged_source_is_an_explicit_bankable_snapshot(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            source = root / "src" / "Measured.cpp"
            source.write_text("int measured;\n")
            subprocess.run(["git", "init", "-q"], cwd=root, check=True)
            subprocess.run(["git", "add", "src/Measured.cpp"], cwd=root, check=True)
            with mock.patch.object(status, "REPO", root):
                self.assertEqual(status.unstaged_bank_inputs(), [])
                source.write_text("int measured = 1;\n")
                self.assertEqual(status.unstaged_bank_inputs(), ["src/Measured.cpp"])

    def test_update_refuses_before_reading_or_writing_the_baseline(self):
        args = argparse.Namespace(report="missing-report.json", accept_regressions=False,
                                  keep_max=False, verbose=False)
        with mock.patch.object(status, "unstaged_bank_inputs",
                               return_value=["src/Live.cpp"]):
            with self.assertRaisesRegex(SystemExit, "refusing to write"):
                status.cmd_update(args)

    def test_readme_write_refuses_before_refreshing_the_baseline(self):
        args = argparse.Namespace(report="missing-report.json", write_readme=True,
                                  json=False, per_unit=False)
        with mock.patch.object(status, "unstaged_bank_inputs",
                               return_value=["include/Live.h"]), \
                mock.patch.object(status, "cmd_update") as update:
            with self.assertRaisesRegex(SystemExit, "refusing to write"):
                status.cmd_summary(args)
        update.assert_not_called()


class ReferentEvidenceTriageTests(unittest.TestCase):
    def test_a_later_string_cannot_downgrade_symbol_evidence(self):
        divs = [(["?KnownRetail@@YAXXZ"], []),
                ([], ['"plausible but not decisive"'])]
        self.assertEqual(image_diff._referent_evidence(divs), "symbol")

    def test_literal_only_and_weak_regions_keep_their_classes(self):
        self.assertEqual(image_diff._referent_evidence(
            [(['"retail"'], ['"candidate"'])]), "string literal")
        self.assertEqual(image_diff._referent_evidence(
            [(["<0011223344556677>"], ["0x12345678"])]),
            "weak / content only")


class OrderingWorklistTests(unittest.TestCase):
    def test_a_moved_referent_reads_as_two_one_sided_segments(self):
        rows = image_diff._seq_divergences(["A", "B", "C"], ["B", "C", "A"])
        self.assertEqual(sum(len(rr) + len(cc) for rr, cc in rows), 2)
        self.assertTrue(all(not rr or not cc for rr, cc in rows))

    def test_equal_sequences_yield_no_segments(self):
        self.assertEqual(image_diff._seq_divergences(["A", "B"], ["A", "B"]), [])

    def test_candidate_tail_only_fills_retail_identity_deficits(self):
        self.assertEqual(
            image_diff._reconcile_candidate_tail(
                ["A", "B", "A"], ["A"], ["X", "B", "A", "A"]),
            ["A", "B", "A"],
        )

    def test_wrong_candidate_tail_cannot_hide_a_missing_referent(self):
        self.assertEqual(
            image_diff._reconcile_candidate_tail(["A", "B"], ["A"], ["X"]),
            ["A"],
        )


class ViewDebtLibraryShadowTests(unittest.TestCase):
    def _run(self, definition):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "include").mkdir()
            (root / "src").mkdir()
            objs = root / "build/objdiff/base"
            objs.mkdir(parents=True)
            (objs / "probe.obj").touch()
            (root / "include/Probe.h").write_text(definition)
            patches = (
                mock.patch.object(view_debt, "REPO", root),
                mock.patch.object(view_debt, "OBJS", objs),
                mock.patch.object(view_debt, "_nm_symbols", return_value=(set(), set())),
                mock.patch.object(view_debt, "_rtti_classes", return_value=set()),
                mock.patch.object(sys, "argv", ["view_debt", "--fatal"]),
            )
            with patches[0], patches[1], patches[2], patches[3], patches[4], \
                    contextlib.redirect_stdout(io.StringIO()), \
                    contextlib.redirect_stderr(io.StringIO()):
                return view_debt.main()

    def test_library_class_definition_fails_the_full_gate(self):
        self.assertEqual(self._run("struct CRect : public tagRECT { int x; };\n"), 1)

    def test_library_class_forward_declaration_is_allowed(self):
        self.assertEqual(self._run("class CRect;\n"), 0)


class EhBandScoreboardTests(unittest.TestCase):
    """`split_eh_band` must leave the reconstruction-target scoreboard bit-identical.

    The carved EH funclets are scored but are NOT reconstruction targets: the function
    universe classes the whole band `eh` and the README divides by a denominator that
    never had them. Leaving them in objdiff's aggregate put 3,034 symbols in the
    NUMERATOR alone - the headline read `5,137 / 4,314 functions exact`. The removal is
    arithmetic over the per-function rows (objdiff's measures are exact sums over them),
    so it is checkable, and a drifting formula would silently move every published score.
    """

    def _doc(self):
        return {
            "measures": {"total_functions": 3, "matched_functions": 2,
                         "total_code": "300", "matched_code": "200",
                         "fuzzy_match_percent": (100.0 * 100 + 100.0 * 100 + 50.0 * 100) / 300},
            "units": [{"name": "u",
                       "measures": {"total_functions": 3, "matched_functions": 2,
                                    "total_code": "300", "matched_code": "200",
                                    "fuzzy_match_percent": 250.0 / 3},
                       "functions": [
                           {"name": "?Real@@QAEHXZ", "size": "100",
                            "fuzzy_match_percent": 100.0},
                           {"name": "__ehunwind$?Real@@QAEHXZ$0", "size": "100",
                            "fuzzy_match_percent": 100.0},
                           {"name": "__ehreg$?Real@@QAEHXZ", "size": "100",
                            "fuzzy_match_percent": 50.0},
                       ]}],
        }

    def test_band_rows_leave_the_target_scoreboard_untouched(self):
        doc = self._doc()
        removed = report.split_eh_band(doc)

        self.assertEqual([row[1] for row in removed],
                         ["__ehunwind$?Real@@QAEHXZ$0", "__ehreg$?Real@@QAEHXZ"])
        for measures in (doc["measures"], doc["units"][0]["measures"]):
            self.assertEqual(measures["total_functions"], 1)
            self.assertEqual(measures["matched_functions"], 1)
            self.assertEqual(measures["total_code"], "100")
            self.assertEqual(measures["matched_code"], "100")
            self.assertAlmostEqual(measures["fuzzy_match_percent"], 100.0)
        self.assertEqual([row["name"] for row in doc["units"][0]["functions"]],
                         ["?Real@@QAEHXZ"])

    def test_a_real_symbol_is_never_mistaken_for_a_band_symbol(self):
        self.assertTrue(report.is_eh_band("__ehreg$?Foo@@QAEHXZ"))
        self.assertTrue(report.is_eh_band("__ehunwind$?Foo@@QAEHXZ$3"))
        self.assertFalse(report.is_eh_band("?Foo@@QAEHXZ"))
        self.assertFalse(report.is_eh_band("___CxxFrameHandler"))
        self.assertFalse(report.is_eh_band("?__ehreg$Nested@@QAEHXZ"))

    def test_band_symbol_names_agree_with_the_carve(self):
        from gruntz.build import eh_band
        self.assertTrue(report.is_eh_band(eh_band.registration_symbol("?Foo@@QAEHXZ")))
        self.assertTrue(report.is_eh_band(eh_band.unwind_symbol("?Foo@@QAEHXZ", 0)))
        self.assertTrue(eh_band.is_band_symbol(eh_band.unwind_symbol("?Foo@@QAEHXZ", 7)))

    def test_a_group_carves_one_record_per_funclet_plus_the_stub(self):
        from gruntz.build import eh_band
        group = eh_band.Group(owner_rva=0x1000, owner="?Foo@@QAEHXZ", unit="u",
                              funclets=(0x1D7D20, 0x1D7D28, 0x1D7D33), stub=0x1D7D3B)
        self.assertEqual(
            eh_band.records([group]),
            [(0x1D7D20, "__ehunwind$?Foo@@QAEHXZ$0", "u", 8),
             (0x1D7D28, "__ehunwind$?Foo@@QAEHXZ$1", "u", 11),
             (0x1D7D33, "__ehunwind$?Foo@@QAEHXZ$2", "u", 8),
             (0x1D7D3B, "__ehreg$?Foo@@QAEHXZ", "u", 10)])
        self.assertEqual((group.start, group.end), (0x1D7D20, 0x1D7D45))


class ResidualQueueTests(unittest.TestCase):
    def test_campaign_starts_at_residual_weighted_middle(self):
        report_data = {"units": [{"name": "unit", "functions": [
            {"name": "best", "fuzzy_match_percent": 90.0, "size": 100},
            {"name": "middle", "fuzzy_match_percent": 50.0, "size": 100},
            {"name": "worst", "size": 40},
        ]}]}
        symbols = {
            ("unit", "best"): {"rva": "0x1000"},
            ("unit", "middle"): {"rva": "0x1100"},
            ("unit", "worst"): {"rva": "0x1200"},
        }

        rows, library_count = residual_queue.residual_rows(report_data, symbols)
        campaign = residual_queue.campaign_rows(rows)

        self.assertEqual(library_count, 0)
        self.assertEqual([row["name"] for row in campaign], ["middle", "worst"])
        self.assertAlmostEqual(rows[0]["residual_bytes"], 10.0)
        self.assertAlmostEqual(rows[1]["cumulative_residual_pct"], 60.0)
        self.assertEqual([row["campaign_rank"] for row in campaign], [1, 2])

    def test_empty_report_has_empty_campaign(self):
        rows, _ = residual_queue.residual_rows({"units": []}, {})
        self.assertEqual(residual_queue.campaign_rows(rows), [])


class _Tree:
    """A throwaway src/ tree that class_meta.source_files() will walk."""

    def __init__(self, files: dict[str, str]):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        for name, text in files.items():
            p = self.root / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(text)

    def __enter__(self):
        self._saved = (class_meta.SRC, class_meta.INC, class_meta.RVA_H)
        class_meta.SRC = self.root
        class_meta.INC = self.root / "_none"
        class_meta.RVA_H = self.root / "_no_rva.h"
        return self

    def __exit__(self, *a):
        class_meta.SRC, class_meta.INC, class_meta.RVA_H = self._saved
        self._tmp.cleanup()


class TestBranchDisassembly(unittest.TestCase):
    def test_msvc_local_labels_stay_in_the_owning_function(self):
        parsed = branches.parse_objdump(
            "00000000 <?Switch@@YAHH@Z>:\n"
            "       0: jne 0x10 <?Switch@@YAHH@Z+0x10>\n"
            "00000010 <$L123>:\n"
            "      10: movl %eax, %ebx\n"
            "      12: retl\n"
            "00000000 <?Next@@YAHXZ>:\n"
            "       0: retl\n"
        )
        self.assertEqual(set(parsed), {"?Switch@@YAHH@Z", "?Next@@YAHXZ"})
        self.assertEqual([offset for offset, _, _ in parsed["?Switch@@YAHH@Z"]],
                         [0, 0x10, 0x12])


# --------------------------------------------------------------------------- #
# library_labels: LOW is evidence to investigate, never carve-out authority    #
# --------------------------------------------------------------------------- #
class TestLibraryLabels(unittest.TestCase):
    def test_low_rows_are_not_active_library_claims(self):
        with tempfile.TemporaryDirectory() as tmp:
            labels = Path(tmp) / "library_labels.csv"
            labels.write_text(
                "rva,name,lib,confidence,source\n"
                "0x1000,high,LIBCMT,HIGH,test\n"
                "0x2000,ambig,NAFXCW,AMBIG,test\n"
                "0x3000,low,LIBCMT,LOW,test\n"
            )
            self.assertEqual(library_labels.active_rvas(labels), {0x1000, 0x2000})


class TestFunctionUniverse(unittest.TestCase):
    def test_every_filter_uses_typed_source_claims_and_tracked_evidence(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "build/gen").mkdir(parents=True)
            (root / "config/retail").mkdir(parents=True)
            (root / "config/retail/functions.tsv").write_text(
                "rva\tsize\tkind\n"
                "0x8100\t8\t\n0x8200\t8\t\n0x8300\t10\t\n0x8400\t20\t\n"
                "0x8500\t20\t\n0x8600\t8\teh\n0x1600\t5\tthunk\n0x8700\t5\t\n")
            (root / "build/gen/symbol_names.csv").write_text(
                "rva,name,unit,size,kind\n"
                "0x8100,data_label,u,,data\n"
                "0x8200,source_function,u,0x8,func\n")
            (root / "config/retail/library_labels.csv").write_text(
                "rva,name,lib,confidence,source\n"
                "0x8400,library_high,LIBCMT,HIGH,test\n"
                "0x8500,library_low,LIBCMT,LOW,test\n")
            (root / "config/retail/compiler-generated-functions.tsv").write_text(
                "0x00008300\t0xa\t_$E1\tu\ttest\n")
            (root / "config/retail/compiler-helper-functions.tsv").write_text(
                "0x00008700\t0x5\t0x00008800\tforward\ttest\n")

            rows, meta = function_universe.classify(root, strict=False)
            cats = {row["rva"]: (row["category"], row["claimed"]) for row in rows}
            self.assertEqual(cats[0x8100], ("target", False))
            self.assertEqual(cats[0x8200], ("target", True))
            self.assertEqual(cats[0x8300], ("compiler", False))
            self.assertEqual(cats[0x8400], ("library", False))
            self.assertEqual(cats[0x8500], ("target", False))
            self.assertEqual(cats[0x8600], ("eh", False))
            self.assertEqual(cats[0x1600], ("thunk", False))
            self.assertEqual(cats[0x8700], ("compiler", False))
            self.assertEqual([row["rva"] for row in meta["unmatched"]],
                             [0x8100, 0x8500])


# --------------------------------------------------------------------------- #
# COFF normalization: MSVC's x86 `$E<n>` functions carry a leading underscore #
# --------------------------------------------------------------------------- #
class TestCompilerPrivateFunctionNames(unittest.TestCase):
    @staticmethod
    def _text_helper_coff(raw_size):
        body = b"\xb8\x01\x00\x00\x00\xc3"
        raw = body + b"\x90" * (raw_size - len(body))
        raw_offset = 20 + 40
        symbol_offset = raw_offset + len(raw)
        header = struct.pack(
            "<HHIIIHH", 0x14C, 1, 0, symbol_offset, 1, 0, 0)
        section = struct.pack(
            "<8sIIIIIIHHI",
            b".text\0\0\0", 0, 0, len(raw), raw_offset, 0, 0, 0, 0,
            canonicalize_data_symbols.MEM_EXECUTE,
        )
        symbol = b"_$E28\0\0\0" + struct.pack(
            "<IhHBB",
            0, 1, canonicalize_data_symbols.FUNCTION_TYPE, 2, 0,
        )
        return header + section + raw + symbol + struct.pack("<I", 4)

    def test_curated_body_name_propagates_through_leading_ilt_thunk(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / "tiny.exe"
            functions = Path(tmp) / "functions.tsv"

            image = bytearray(0x400)
            struct.pack_into("<I", image, 0x3C, 0x80)
            struct.pack_into("<H", image, 0x80 + 6, 1)
            struct.pack_into("<H", image, 0x80 + 20, 0)
            section = 0x80 + 24
            image[section:section + 8] = b".text\0\0\0"
            struct.pack_into("<IIII", image, section + 8, 0x200, 0x1000, 0x200, 0x200)
            image[0x200] = 0xE9
            struct.pack_into("<i", image, 0x201, 0x2000 - (0x1000 + 5))
            exe.write_bytes(image)
            functions.write_text(
                "rva\tsize\tkind\n"
                "0x1000\t5\t\n"
            )

            saved_bounds = synth_pdb.TEXT_BASE, synth_pdb.TEXT_END
            synth_pdb.TEXT_BASE, synth_pdb.TEXT_END = 0x1000, 0x3000
            try:
                names = synth_pdb.read_ilt_thunk_names(
                    exe,
                    functions,
                    {0x2000: ("?CuratedCtor@@QAE@XZ", "owner", 0x15)},
                )
                self.assertEqual(names, {0x1000: "?CuratedCtor@@QAE@XZ"})
                self.assertEqual(
                    synth_pdb.read_functions(functions, thunk_names=names),
                    [(0x1000, 5, "?CuratedCtor@@QAE@XZ")],
                )
            finally:
                synth_pdb.TEXT_BASE, synth_pdb.TEXT_END = saved_bounds

    @staticmethod
    def _text_helper_alias_coff(parent_targets_alias):
        child = b"\xb8\x01\x00\x00\x00\xc3"
        parent = b"\xe8\x00\x00\x00\x00\xc3"
        raw = child + parent
        raw_offset = 20 + 40
        reloc_offset = raw_offset + len(raw)
        symbol_offset = reloc_offset + 10
        header = struct.pack(
            "<HHIIIHH", 0x14C, 1, 0, symbol_offset, 3, 0, 0)
        section = struct.pack(
            "<8sIIIIIIHHI",
            b".text\0\0\0", 0, 0, len(raw), raw_offset, reloc_offset, 0, 1, 0,
            canonicalize_data_symbols.MEM_EXECUTE,
        )
        relocation = struct.pack(
            "<IIH", len(child) + 1, 1 if parent_targets_alias else 0, 0x14)

        def symbol(name, value, section_ordinal):
            return name.encode("ascii").ljust(8, b"\0") + struct.pack(
                "<IhHBB", value, section_ordinal,
                canonicalize_data_symbols.FUNCTION_TYPE, 2, 0,
            )

        symbols = (
            symbol("_$E100", 0, 1)
            + symbol("_$E100", 0, 0)
            + symbol("_$E200", len(child), 1)
        )
        return header + section + raw + relocation + symbols + struct.pack("<I", 4)

    def test_x86_dynamic_initializer_is_content_addressed(self):
        self.assertEqual(canonicalize_data_symbols._family("_$E28"), ("e", None))

    def test_dynamic_initializer_text_definition_is_a_candidate(self):
        section = canonicalize_data_symbols.Section(
            1, 0, ".text", 0x20, 0, 0, 0,
            canonicalize_data_symbols.MEM_EXECUTE,
        )
        for storage_class in (2, 3):
            symbol = canonicalize_data_symbols.Symbol(
                0, 0, "_$E28", 0, 1,
                canonicalize_data_symbols.FUNCTION_TYPE, storage_class, 0,
            )
            definition = canonicalize_data_symbols.Definition(
                symbol, section, "text", 0, 0x20,
            )
            self.assertTrue(
                canonicalize_data_symbols._is_canonical_candidate(definition))

    def test_ordinary_text_function_is_not_a_candidate(self):
        section = canonicalize_data_symbols.Section(
            1, 0, ".text", 0x20, 0, 0, 0,
            canonicalize_data_symbols.MEM_EXECUTE,
        )
        symbol = canonicalize_data_symbols.Symbol(
            0, 0, "_ordinary", 0, 1,
            canonicalize_data_symbols.FUNCTION_TYPE, 2, 0,
        )
        definition = canonicalize_data_symbols.Definition(
            symbol, section, "text", 0, 0x20,
        )
        self.assertFalse(
            canonicalize_data_symbols._is_canonical_candidate(definition))

    def test_text_identity_excludes_alignment_padding(self):
        self.assertEqual(
            canonicalize_data_symbols._identity_span("text", 0x20, 0x1a),
            0x1a,
        )
        self.assertEqual(
            canonicalize_data_symbols._identity_span("data", 0x20, 0x1a),
            0x20,
        )

    def test_full_coff_path_pairs_different_text_alignment_spans(self):
        wide = canonicalize_data_symbols.canonicalize_coff(
            self._text_helper_coff(0x20))
        packed = canonicalize_data_symbols.canonicalize_coff(
            self._text_helper_coff(0x1c))
        self.assertEqual(len(wide.rows), 1)
        self.assertEqual(len(packed.rows), 1)
        self.assertEqual(wide.rows[0].storage, "text")
        self.assertEqual(wide.rows[0].meaningful_size, 6)
        self.assertEqual(
            wide.rows[0].canonical_name,
            packed.rows[0].canonical_name,
        )

    def test_helper_relocation_names_normalize_ctor_dtor_and_atexit_aliases(self):
        stable = canonicalize_data_symbols._stable_relocation_name
        self.assertEqual(stable("??0CPtrList@@QAE@H@Z"), "CPtrList")
        self.assertEqual(stable("??1CPtrList@@UAE@XZ"), "~CPtrList")
        self.assertEqual(stable("_atexit"), "atexit")
        self.assertEqual(stable("CPtrList"), "CPtrList")
        self.assertEqual(stable("~CPtrList"), "~CPtrList")

    def test_template_static_dtor_guard_relocations_are_role_identified(self):
        body = (
            b"\x8a\x0d\x00\x00\x00\x00"
            b"\xb0\x02\x84\xc8\x75\x12\x0a\xc8"
            b"\x88\x0d\x00\x00\x00\x00"
            b"\xb9\x00\x00\x00\x00"
            b"\xe9\x00\x00\x00\x00\xc3"
        )
        is_guard = canonicalize_data_symbols._guarded_static_dtor_guard_site
        self.assertTrue(is_guard(body, 0x2))
        self.assertTrue(is_guard(body, 0x10))
        self.assertFalse(is_guard(body, 0x15))
        self.assertFalse(is_guard(body, 0x1A))

    def test_undefined_helper_alias_resolves_to_unique_same_object_definition(self):
        direct = canonicalize_data_symbols.canonicalize_coff(
            self._text_helper_alias_coff(False))
        through_alias = canonicalize_data_symbols.canonicalize_coff(
            self._text_helper_alias_coff(True))
        direct_parent = next(
            row for row in direct.rows if row.original_name == "_$E200")
        alias_parent = next(
            row for row in through_alias.rows if row.original_name == "_$E200")
        self.assertEqual(direct_parent.canonical_name, alias_parent.canonical_name)

        normalized = canonicalize_data_symbols.CoffObject(through_alias.data)
        self.assertEqual(normalized.symbols[0].name, normalized.symbols[1].name)
        alias_row = next(
            row for row in through_alias.rows
            if row.original_name == "_$E100"
            and row.proof == "alias-of-definition"
        )
        self.assertEqual(alias_row.canonical_name, normalized.symbols[0].name)


# --------------------------------------------------------------------------- #
# unnamed-function queue: current source claims leave the raw Ghidra inventory #
# --------------------------------------------------------------------------- #
class TestUnnamedFunctionQueue(unittest.TestCase):
    def test_volatile_rva_compgen_ordinal_does_not_claim_function(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "Static.cpp").write_text(
                "RVA_COMPGEN(0x00001000, 0xa, _$E4096)\n"
            )
            funcs = tu_layout.load_funcs(src)
            boundaries = [
                (0x1000, 0xA, "FUN_00401000"),
                (0x2000, 0xA, "FUN_00402000"),
            ]
            _, remaining = tu_layout.attribute(
                funcs, boundaries, tu_layout.DEFAULT_GAP
            )
            self.assertEqual(remaining, 2)

    def test_header_rva_claim_is_not_reported_as_still_unnamed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            inc = root / "include"
            src.mkdir()
            inc.mkdir()
            (src / "Known.cpp").write_text("RVA(0x00002000, 0xa)\nvoid Known() {}\n")
            (inc / "Inline.h").write_text(
                "RVA(0x00001000, 0x8)\ninline void ClaimedInline() {}\n"
            )
            funcs = tu_layout.load_funcs(src)
            claims = tu_layout.load_claimed_extents((src, inc))
            boundaries = [
                (0x1000, 0x8, "FUN_00401000"),
                (0x3000, 0xA, "FUN_00403000"),
            ]
            _, remaining = tu_layout.attribute(
                funcs, boundaries, tu_layout.DEFAULT_GAP, claims
            )
            self.assertEqual(remaining, 1)

    def test_interior_ghidra_split_is_not_reported_as_unnamed(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "src"
            src.mkdir()
            (src / "Whole.cpp").write_text(
                "RVA(0x00001000, 0x100)\nvoid Whole() {}\n"
            )
            funcs = tu_layout.load_funcs(src)
            boundaries = [
                (0x1080, 0x20, "FUN_00401080"),
                (0x2000, 0x20, "FUN_00402000"),
            ]
            _, remaining = tu_layout.attribute(
                funcs, boundaries, tu_layout.DEFAULT_GAP
            )
            self.assertEqual(remaining, 1)


# --------------------------------------------------------------------------- #
# label extraction: clang and VC5 disagree on static-data mangling.            #
# The rewrites are MECHANICAL but never SPECULATIVE - a spelling is returned    #
# only when the compiled object actually defines it, so a guess can never bind. #
# --------------------------------------------------------------------------- #
class TestMsvc5DataSymbols(unittest.TestCase):
    def test_message_entries_resolve_to_vc5_spelling_present_in_object(self):
        clang = "?_messageEntries@CMultiHelpDlg@@0QBUAFX_MSGMAP_ENTRY@@B"
        vc5 = "?_messageEntries@CMultiHelpDlg@@0PBUAFX_MSGMAP_ENTRY@@B"
        self.assertEqual(labels.msvc5_data_symbol(clang, {vc5}), vc5)

    def test_message_entries_are_not_rewritten_without_object_authority(self):
        clang = "?_messageEntries@CMultiHelpDlg@@0QBUAFX_MSGMAP_ENTRY@@B"
        self.assertIsNone(labels.msvc5_data_symbol(clang, set()))
        self.assertIsNone(labels.msvc5_data_symbol(clang, {"?unrelated@@3HA"}))

    def test_any_const_array_Q_resolves_to_the_vc5_P_in_the_object(self):
        """THE ORIGINAL BUG: the Q->P rewrite was hard-coded to AFX_MSGMAP_ENTRY, so an
        ordinary `const u8 g_guid1[16]` / `const char s_rb[]` DATA() bound NOTHING.
        The storage-class difference is generic."""
        for clang, vc5 in (("?g_guid1@@3QBEB", "?g_guid1@@3PBEB"),
                           ("?s_rb@@3QBDB", "?s_rb@@3PBDB"),
                           ("?value@CMultiHelpDlg@@0QBUOtherType@@B",
                            "?value@CMultiHelpDlg@@0PBUOtherType@@B")):
            self.assertEqual(labels.msvc5_data_symbol(clang, {vc5}), vc5)
            self.assertIsNone(labels.msvc5_data_symbol(clang, {"?unrelated@@3HA"}))

    def test_internal_linkage_static_resolves_its_volatile_pool_id(self):
        """cl decorates every internal-linkage file-scope variable `_x$S<n>`; clang
        reports the plain `_x`. Matched by prefix (the ordinal renumbers on any symbol
        churn), and ONLY when exactly one symbol matches."""
        self.assertEqual(
            labels.msvc5_data_symbol("_s_fmtNotFound", {"_s_fmtNotFound$S19047"}),
            "_s_fmtNotFound$S19047")
        # a longer name is not a prefix hit: `_s_key` must not grab `_s_keyA$S..`
        self.assertIsNone(labels.msvc5_data_symbol("_s_key", {"_s_keyA$S12"}))
        # ambiguity is refused, never guessed
        self.assertIsNone(
            labels.msvc5_data_symbol("_s_x", {"_s_x$S1", "_s_x$S2"}))


# --------------------------------------------------------------------------- #
# verify_unique_names: one NAME per rva, and one stretch of .text per CLAIM     #
# --------------------------------------------------------------------------- #
class TestClaimExtents(unittest.TestCase):
    """THE ORIGINAL BUG: the gate enforced one RVA per NAME but never looked at
    EXTENTS, so two names could claim overlapping ranges and both be scored forever.
    ?Serialize@CMapMgr@@ RVA(0x9356c,0x38) sat entirely inside ?BroadcastCmd@CGruntzMgr@@
    RVA(0x93460,0x15c) - it was not a function at all, but BroadcastCmd's tail, parked
    @early-stop at ~38% behind a fabricated ABI note. Deleting it RAISED the metric."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.csv = Path(self._tmp.name) / "symbol_names.csv"

    def tearDown(self):
        self._tmp.cleanup()

    def _run(self, rows):
        self.csv.write_text("rva,name,unit,size,kind\n" + "".join(rows))
        argv = sys.argv[:]
        sys.argv = ["x", "--csv", str(self.csv)]
        try:
            with contextlib.redirect_stdout(io.StringIO()) as out:
                rc = vun.main()
            return rc, out.getvalue()
        finally:
            sys.argv = argv

    def test_a_claim_contained_in_another_is_fatal(self):
        """The 0x9356c shape: a fragment modelled as its own function."""
        rc, out = self._run(["0x093460,?BroadcastCmd@@,gruntzmgr,0x15c,func\n",
                             "0x09356c,?Serialize@@,brickz,0x38,func\n"])
        self.assertEqual(rc, 1)
        self.assertIn("CONTAINS", out)

    def test_an_over_declared_size_is_fatal(self):
        """The LaunchWebBrowser/InitMode shape: a size arg reaching into its neighbour."""
        rc, out = self._run(["0x17c3f0,?InitMode@@,ddpagemgr,0x14e,func\n",
                             "0x17c510,?Teardown@@,ddpagemgr,0x5e,func\n"])
        self.assertEqual(rc, 1)
        self.assertIn("overlaps", out)

    def test_abutting_claims_are_fine(self):
        """A function ending exactly where the next begins is NORMAL, not an overlap."""
        rc, _ = self._run(["0x001000,?A@@,u,0x20,func\n", "0x001020,?B@@,u,0x10,func\n"])
        self.assertEqual(rc, 0)

    def test_unsized_rows_are_skipped_not_guessed(self):
        """A missing size is not evidence of an overlap - an RVA_COMPGEN pin may have none."""
        rc, out = self._run(["0x001000,?A@@,u,,func\n", "0x001000,?B@@,u,,func\n"])
        self.assertEqual(rc, 0)
        self.assertIn("2 unsized", out)

    def test_data_rows_do_not_trip_the_extent_check(self):
        """DATA globals are exempt: multi-TU extern pins of one object are legitimate."""
        rc, _ = self._run(["0x001000,_g,u,0x100,data\n", "0x001004,_h,u,0x4,data\n"])
        self.assertEqual(rc, 0)


# --------------------------------------------------------------------------- #
# status update: best_pct is a RATCHET, and only IDENTITY may reset it         #
# --------------------------------------------------------------------------- #
class TestBestEverRatchet(unittest.TestCase):
    """THE ORIGINAL BUG: `update` reset best<-current whenever a function's source
    FINGERPRINT changed ("the old peak belonged to different source"), keeping the max
    only behind a non-default --keep-max. So a plain `status update` - the documented
    bless, the one every lane runs - silently LOWERED best-evers.

    It failed in the worst possible direction: editing a body is exactly when the
    high-water has to hold, because structure-over-current-% asks lanes to take
    proven-correct shapes at a %-cost. So it erased the peak precisely when a lane did
    the thing we ask for. When it was fixed, 59 of 3982 rows had best > cur - 217.3
    best-% one edit away from being destroyed, four of them proven byte-exact 100.0000s.

    best now measures a BODY, keyed by rva; the fingerprint drives `tries` only.
    """

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        self.baseline = self.root / "match_baseline.tsv"
        self.report = self.root / "report.json"
        self._saved = status.BASELINE
        status.BASELINE = self.baseline

    def tearDown(self):
        status.BASELINE = self._saved
        self._tmp.cleanup()

    def _run_update(self, cur_pct, *, fp, rva, accept=False):
        """Drive the REAL cmd_update against one synthetic row; return its new best."""
        self.report.write_text(json.dumps({
            "measures": {},
            "units": [{"name": "u", "measures": {},
                       "functions": [{"name": "?F@@QAEHXZ",
                                      "fuzzy_match_percent": cur_pct}]}],
        }))
        saved_fpr, saved_rvas = status.fingerprinter, status.func_rvas
        status.fingerprinter = lambda: ((lambda u, f: fp), {}, set())
        status.func_rvas = lambda: {("u", "?F@@QAEHXZ"): rva}
        try:
            args = argparse.Namespace(report=str(self.report), accept_regressions=accept,
                                      keep_max=False, verbose=False)
            with mock.patch.object(status, "require_bankable_tree"), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.cmd_update(args)
        finally:
            status.fingerprinter, status.func_rvas = saved_fpr, saved_rvas
        return status.load_baseline()[("u", "?F@@QAEHXZ")]

    def _seed(self, best, cur, fp, rva, tries=3):
        status.write_baseline({("u", "?F@@QAEHXZ"): {
            "best": best, "cur": cur, "tries": tries, "fp": fp, "addr": rva}})

    def test_addressless_rows_do_not_emit_trailing_whitespace(self):
        status.write_baseline({("u", "$anon_data_hash_0"): {
            "best": 100.0, "cur": 100.0, "tries": 1, "fp": "aaaa", "addr": None}})
        rows = self.baseline.read_text().splitlines()
        self.assertEqual(rows[-1],
                         "u\t$anon_data_hash_0\t100.0000\t100.0000\t1\taaaa\t\t100.0000")

    def test_editing_a_function_resets_best_and_hist_keeps_the_peak(self):
        """best_pct is scoped to the IMPLEMENTATION. A changed src_hash means the peak
        was scored by source that no longer exists, so best resets to cur; hist_pct
        keeps the all-time number so the headroom is still visible. (Until 2026-08-08
        this preserved best, which let the ledger assert a peak nothing could reproduce.)"""
        self._seed(best=98.8947, cur=98.8947, fp="aaaa", rva=0xe7440)
        row = self._run_update(87.0526, fp="bbbb", rva=0xe7440)  # source changed, % fell
        self.assertAlmostEqual(row["best"], 87.0526, places=3)
        self.assertAlmostEqual(row["cur"], 87.0526, places=3)
        self.assertAlmostEqual(row["hist"], 98.8947, places=3)   # the peak is NOT lost
        self.assertEqual(row["tries"], 4)

    def test_a_fallback_fingerprint_is_not_an_edit(self):
        """NOT a fix - a property that was already right, pinned so it stays right. A cold
        clangd cache degrades the fingerprint to a whole-.cpp hash; treating that as an
        edit would reset every function in a touched file. real_edit() already refuses,
        which is why the cold cache was NOT the erosion mechanism (the six eroded rows all
        carried real fingerprints). Guard it anyway: it is one `is_fallback` from being."""
        self._seed(best=57.3034, cur=57.3034, fp="aaaa", rva=0xb1ee0)
        row = self._run_update(56.1798, fp="cpp:deadbeef", rva=0xb1ee0)
        self.assertAlmostEqual(row["best"], 57.3034, places=3)
        self.assertEqual(row["tries"], 3)  # a fallback must not bump tries either

    def test_a_real_improvement_still_raises_the_best(self):
        """The guard must not brick the ratchet."""
        self._seed(best=90.0, cur=90.0, fp="aaaa", rva=0xd12b0)
        row = self._run_update(96.5, fp="bbbb", rva=0xd12b0)
        self.assertAlmostEqual(row["best"], 96.5, places=3)

    def test_a_rebound_name_starts_fresh(self):
        """LEGITIMATE CARRY: a best measures a BODY. When the name at a row moves to a
        different rva it labels a different body, so the old peak is not its floor.
        Real case: main's shift-by-one attribution fix slid ?RegisterActs@CCheckpointTrigger
        from 0x10ebe0 (best 91.1739) to 0x10f340 - and 91.1739 correctly stayed at
        0x10ebe0, under the name that now sits there (?RegisterActs@CBrickz)."""
        self._seed(best=91.1739, cur=91.1739, fp="aaaa", rva=0x10ebe0)
        row = self._run_update(78.5970, fp="bbbb", rva=0x10f340)  # SAME name, NEW body
        self.assertAlmostEqual(row["best"], 78.5970, places=3)
        self.assertEqual(row["tries"], 1)  # a different body has its own try count

    def test_a_unit_move_preserves_the_same_rva_best(self):
        status.write_baseline({("synthetic", "?F@@QAEHXZ"): {
            "best": 100.0, "cur": 100.0, "tries": 3, "fp": "aaaa",
            "addr": 0x138D0,
        }})
        self.report.write_text(json.dumps({
            "measures": {},
            "units": [{"name": "natural", "measures": {}, "functions": [{
                "name": "?F@@QAEHXZ", "fuzzy_match_percent": 91.0,
            }]}],
        }))
        saved_fpr, saved_rvas = status.fingerprinter, status.func_rvas
        status.fingerprinter = lambda: ((lambda u, f: "bbbb"), {}, set())
        status.func_rvas = lambda: {("natural", "?F@@QAEHXZ"): 0x138D0}
        try:
            args = argparse.Namespace(report=str(self.report), accept_regressions=False,
                                      keep_max=False, verbose=False)
            with mock.patch.object(status, "require_bankable_tree"), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.cmd_update(args)
        finally:
            status.fingerprinter, status.func_rvas = saved_fpr, saved_rvas
        rows = status.load_baseline()
        self.assertNotIn(("synthetic", "?F@@QAEHXZ"), rows)
        # The row SURVIVES the unit move (that is what this test is for). Its fp also
        # changed, so best is scoped to the new implementation while hist keeps the peak.
        self.assertAlmostEqual(rows[("natural", "?F@@QAEHXZ")]["best"], 91.0)
        self.assertAlmostEqual(rows[("natural", "?F@@QAEHXZ")]["hist"], 100.0)

    def test_an_absent_body_preserves_its_historical_max(self):
        status.write_baseline({("synthetic", "?F@@QAEHXZ"): {
            "best": 100.0, "cur": 100.0, "tries": 3, "fp": "aaaa",
            "addr": 0x58CD0,
        }})
        self.report.write_text(json.dumps({"measures": {}, "units": []}))
        saved_fpr, saved_rvas = status.fingerprinter, status.func_rvas
        status.fingerprinter = lambda: ((lambda u, f: "bbbb"), {}, set())
        status.func_rvas = lambda: {}
        try:
            args = argparse.Namespace(report=str(self.report), accept_regressions=False,
                                      keep_max=False, verbose=False)
            with mock.patch.object(status, "require_bankable_tree"), \
                    contextlib.redirect_stdout(io.StringIO()):
                status.cmd_update(args)
        finally:
            status.fingerprinter, status.func_rvas = saved_fpr, saved_rvas
        row = status.load_baseline()[("synthetic", "?F@@QAEHXZ")]
        self.assertAlmostEqual(row["best"], 100.0)
        self.assertEqual(row["addr"], 0x58CD0)

    def test_an_unvouchable_rva_ratchets_rather_than_erodes(self):
        """A pre-rva baseline row (or one with no symbol_names entry) cannot be checked
        for identity. Fail LOUD (a false REGRESS a human reads), never silent erosion."""
        self._seed(best=93.3127, cur=93.3127, fp="aaaa", rva=None)
        row = self._run_update(89.5636, fp="bbbb", rva=None)
        # The rva is unvouchable, so identity cannot erode it - but the FINGERPRINT
        # changed, and that is what now scopes best. hist keeps the peak.
        self.assertAlmostEqual(row["best"], 89.5636, places=3)
        self.assertAlmostEqual(row["hist"], 93.3127, places=3)

    def test_accept_regressions_is_the_ONE_way_to_lower_a_best(self):
        """The deliberate, reviewed escape hatch must still work."""
        self._seed(best=98.8947, cur=98.8947, fp="aaaa", rva=0xe7440)
        row = self._run_update(87.0526, fp="bbbb", rva=0xe7440, accept=True)
        self.assertAlmostEqual(row["best"], 87.0526, places=3)


class TestCheckReportsWhatUpdateAlreadyKnows(unittest.TestCase):
    """`check` must classify a row the same way `update` does. Where the two disagreed,
    `check` invented losses and hid regressions (both measured on main, 2026-08-08)."""

    @staticmethod
    def _buckets(cur, base, rvas, fps=None):
        """Classify with UNEDITED source by default: an unlisted function reports the
        fingerprint its baseline row already carries, so only `fps` marks a real edit."""
        fps = fps or {}

        def fp(unit, fn):
            row = base.get((unit, fn))
            return fps.get((unit, fn), row["fp"] if row else "unseen")

        kinds: dict[str, set] = {}
        for kind, unit, fn, _pct, _best in status.classify(
                cur, base, fp, frozenset(), rvas):
            kinds.setdefault(kind, set()).add((unit, fn))
        return kinds

    def test_an_edited_function_below_its_best_is_a_REGRESS_not_a_TOUCHED(self):
        """THE MASK. `real_edit` used to be tested BEFORE `pct < best`, so editing a body
        hid its drop below the high-water - inverting the module's own doctrine (an edit
        is exactly when the ratchet must hold). Twelve rows were hidden this way at main,
        the worst -38.24 (?Blowfish_decipher@@YAXPAI0@Z, 99.88 -> 61.64)."""
        base = {("blowfish", "?Blowfish_decipher@@YAXPAI0@Z"): {
            "best": 99.8750, "cur": 99.8750, "tries": 2, "fp": "aaaa", "addr": 0x1a2b30}}
        kinds = self._buckets(
            {("blowfish", "?Blowfish_decipher@@YAXPAI0@Z"): 61.6375}, base,
            {("blowfish", "?Blowfish_decipher@@YAXPAI0@Z"): 0x1a2b30},
            {("blowfish", "?Blowfish_decipher@@YAXPAI0@Z"): "bbbb"})
        self.assertIn(("blowfish", "?Blowfish_decipher@@YAXPAI0@Z"),
                      kinds.get("REGRESS", set()))
        self.assertNotIn(("blowfish", "?Blowfish_decipher@@YAXPAI0@Z"),
                         kinds.get("TOUCHED", set()))

    def test_an_edited_function_at_its_best_is_still_a_TOUCHED(self):
        """The mask fix must not swallow the bucket: an edit that held is TOUCHED."""
        base = {("u", "?F@@QAEHXZ"): {"best": 90.0, "cur": 90.0, "tries": 2,
                                      "fp": "aaaa", "addr": 0x1000}}
        kinds = self._buckets({("u", "?F@@QAEHXZ"): 90.0}, base,
                              {("u", "?F@@QAEHXZ"): 0x1000},
                              {("u", "?F@@QAEHXZ"): "bbbb"})
        self.assertEqual(kinds.get("TOUCHED"), {("u", "?F@@QAEHXZ")})
        self.assertNotIn("REGRESS", kinds)

    def test_a_comdat_that_migrates_units_is_MOVED_not_LOST(self):
        """A COMDAT inline ctor/dtor legitimately changes emitting unit. cmd_update
        already carries its high-water by rva; cmd_check keyed the same test on
        (unit, rva) and so called the transfer a LOSS. Five of main's seven "LOST"
        were this - every one of them sitting at 100.00% under its new unit."""
        base = {("typekeycoll", "??_GCButeTree@@UAEPAXI@Z"): {
            "best": 45.5833, "cur": 45.5833, "tries": 4, "fp": "aaaa", "addr": 0x16e9c0}}
        cur = {("buteglobals", "??_GCButeTree@@UAEPAXI@Z"): 100.0}
        kinds = self._buckets(cur, base,
                              {("buteglobals", "??_GCButeTree@@UAEPAXI@Z"): 0x16e9c0})
        self.assertNotIn("LOST", kinds)
        self.assertNotIn("NEW", kinds)      # nor an ungated fresh row at the new unit
        self.assertEqual(kinds.get("MOVED"), {("buteglobals", "??_GCButeTree@@UAEPAXI@Z")})

    def test_a_migrated_comdat_that_dropped_is_gated_at_its_new_home(self):
        """The transfer must not become an ungated NEW row - that would launder the drop."""
        base = {("old", "??0X@@QAE@XZ"): {"best": 100.0, "cur": 100.0, "tries": 1,
                                          "fp": "aaaa", "addr": 0x15b300}}
        kinds = self._buckets({("new", "??0X@@QAE@XZ"): 72.5}, base,
                              {("new", "??0X@@QAE@XZ"): 0x15b300})
        self.assertEqual(kinds.get("REGRESS"), {("new", "??0X@@QAE@XZ")})
        self.assertNotIn("NEW", kinds)

    def test_a_body_with_no_emitter_anywhere_is_still_a_real_LOST(self):
        """The fix must not turn every loss into a transfer: an rva nothing claims is
        a genuine loss (main's two, ??1CRezBufferObject and its ??_G, are exactly this)."""
        base = {("fader", "??1CRezBufferObject@@UAE@XZ"): {
            "best": 100.0, "cur": 100.0, "tries": 1, "fp": "aaaa", "addr": 0x17f330}}
        kinds = self._buckets({}, base, {})
        self.assertEqual(kinds.get("LOST"), {("fader", "??1CRezBufferObject@@UAE@XZ")})

    def test_currency_splits_inherited_dips_from_the_ones_a_lane_caused(self):
        """A stale cur_pct snapshot cannot corrupt the MAX ratchet, but it does bury the
        one dip a lane owns among the ones it inherited. Name the split."""
        base = {
            ("u", "?carried@@QAEHXZ"): {"best": 95.0, "cur": 76.6, "tries": 1,
                                        "fp": "a", "addr": 0x1},
            ("u", "?fresh@@QAEHXZ"): {"best": 100.0, "cur": 100.0, "tries": 1,
                                      "fp": "a", "addr": 0x2},
        }
        cur = {("u", "?carried@@QAEHXZ"): 76.6, ("u", "?fresh@@QAEHXZ"): 66.4}
        regress = [("u", "?carried@@QAEHXZ", 76.6, 95.0),
                   ("u", "?fresh@@QAEHXZ", 66.4, 100.0)]
        c = status.baseline_currency(cur, base, regress)
        self.assertEqual((c["regress_carried"], c["regress_fresh"]), (1, 1))
        self.assertEqual(c["snapshot_drift"], 1)   # only ?fresh moved off the snapshot


# --------------------------------------------------------------------------- #
# cleanliness: an UNMEASURED metric is not a measurement of zero               #
# --------------------------------------------------------------------------- #
class TestCleanlinessRatchet(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self._saved = (cleanliness.TEXT_BASELINE, cleanliness.SEMANTIC_BASELINE)
        cleanliness.TEXT_BASELINE = Path(self._tmp.name) / "cleanliness-text-baseline.tsv"
        cleanliness.SEMANTIC_BASELINE = Path(self._tmp.name) / "cleanliness-semantic-baseline.tsv"

    def tearDown(self):
        cleanliness.TEXT_BASELINE, cleanliness.SEMANTIC_BASELINE = self._saved
        self._tmp.cleanup()

    def test_baseline_rows_are_partitioned_by_measurement_mechanism(self):
        cleanliness.save_baseline(
            [("m_<hex> fields", 7), ("caller-callee FAKE-VIEW", 2)]
        )
        self.assertIn("m_<hex> fields\t7", cleanliness.TEXT_BASELINE.read_text())
        self.assertNotIn("caller-callee", cleanliness.TEXT_BASELINE.read_text())
        self.assertEqual(
            cleanliness.SEMANTIC_BASELINE.read_text(),
            "caller-callee FAKE-VIEW\t2\n",
        )
        self.assertEqual(
            cleanliness.load_baseline(),
            {"m_<hex> fields": 7, "caller-callee FAKE-VIEW": 2},
        )

    def test_default_count_does_not_run_semantic_collectors(self):
        with mock.patch.object(cleanliness, "ROOTS", ()), mock.patch.object(
            cleanliness, "semantic_count",
            side_effect=AssertionError("semantic collector ran in the fast text path"),
        ):
            rows = dict(cleanliness.count())
        self.assertNotIn("caller-callee FAKE-VIEW", rows)

        with mock.patch.object(cleanliness, "ROOTS", ()), mock.patch.object(
            cleanliness, "semantic_count",
            return_value=[("caller-callee FAKE-VIEW", 3)],
        ):
            rows = dict(cleanliness.count(include_semantic=True))
        self.assertEqual(rows["caller-callee FAKE-VIEW"], 3)

    def test_nested_static_casts_are_ast_pairs_not_adjacent_statements(self):
        root = Path(self._tmp.name)
        source = root / "src" / "casts.cpp"
        source.parent.mkdir(parents=True)
        source.write_text(
            "int nested(int x) { return static_cast<int>(static_cast<short>(x)); }\n"
            "int separate(int x) { int y = static_cast<int>(x); "
            "return static_cast<short>(y); }\n"
            "template<typename T> int nested_template(T x) { "
            "return static_cast<int>(static_cast<short>(x)); }\n"
            "#define GZ_STRICT_ENUMS 0\n"
            "#if __cplusplus >= 202002L\n"
            "template<typename T> int strict_template(T x) { "
            "return static_cast<long>(static_cast<unsigned>(x)); }\n"
            "#endif\n"
        )
        compdb = root / "compile_commands.json"
        compdb.write_text(json.dumps([{
            "directory": str(root),
            "file": str(source),
            "arguments": ["clang-cl", "/c", str(source)],
        }]))
        hits = nested_static_casts.scan(root, compdb)
        self.assertEqual(len(hits), 3)
        self.assertEqual(hits[0][0:3], ("src/casts.cpp", 1, 28))
        self.assertEqual(hits[0][3:], ("int", "short", "int"))
        self.assertEqual(hits[1][0:3], ("src/casts.cpp", 3, 56))
        self.assertEqual(hits[1][3:], ("int", "short", "T"))
        self.assertEqual(hits[2][0:3], ("src/casts.cpp", 6, 56))
        self.assertEqual(hits[2][3:], ("long", "unsigned int", "T"))

    def test_a_failed_submetric_does_not_erase_its_floor(self):
        """THE ORIGINAL BUG: _caller_callee_counts() swallows every exception and
        returns {}, so count() omitted the row and save_baseline deleted the floor."""
        cleanliness.save_baseline([("m_<hex> fields", 10561), ("caller-callee FAKE-VIEW", 0)])
        rows_without_it = [("m_<hex> fields", 10561)]          # the tool failed this run
        merged = dict(cleanliness.merge_baseline_downonly(rows_without_it))
        self.assertIn("caller-callee FAKE-VIEW", merged,
                      "an unmeasured ratcheted metric lost its baseline floor")
        self.assertEqual(merged["caller-callee FAKE-VIEW"], 0)

    def test_ratcheted_metric_cannot_creep_up(self):
        cleanliness.save_baseline([(".cpp-local views", 34)])
        merged = dict(cleanliness.merge_baseline_downonly([(".cpp-local views", 41)]))
        self.assertEqual(merged[".cpp-local views"], 34, "the ratchet blessed a regression")

    def test_ratcheted_metric_still_goes_down(self):
        cleanliness.save_baseline([(".cpp-local views", 34)])
        merged = dict(cleanliness.merge_baseline_downonly([(".cpp-local views", 20)]))
        self.assertEqual(merged[".cpp-local views"], 20)

    def test_cpp_external_prototypes_include_implicit_externs(self):
        code = cleanliness._strip(
            """
            void GlobalDecl(int value);
            namespace NetLobby {
                void __stdcall AppendEditLine(HWND__* edit, char* str);
                void Definition(int value) { Local local(value); }
                class LocalView { void MemberDecl(); };
            }
            """
        )
        self.assertEqual(cleanliness._count_cpp_external_prototypes(code), 2)
        self.assertIn("cpp external prototypes", cleanliness._RATCHET)
        cleanliness.save_baseline([("cpp external prototypes", 2)])
        merged = dict(
            cleanliness.merge_baseline_downonly([("cpp external prototypes", 3)])
        )
        self.assertEqual(merged["cpp external prototypes"], 2)

    def test_header_extern_names_covers_every_spelling_in_the_tree(self):
        # The `duplicate header externs` metric is only as good as this extractor;
        # `cpp extern decls` sat at 0 for months while 52 symbols were declared in
        # 2+ headers, so the failure mode here is a SILENT zero, not a loud one.
        names = cleanliness._header_extern_names(
            cleanliness._strip(
                """
                extern "C" u32 g_frameTime;
                extern const double g_movingLogicMax;
                extern i32 g_a, g_b, g_c;
                extern "C" char g_slots[TINT_COUNT];
                extern "C" void ButeParseErrorSink(const char* msg);
                extern "C" BOOL __stdcall
                NetEnumCb(u32 dpId, NetDPName* lpName, CNetMgr* ctx);
                extern "C" __declspec(dllimport) unsigned long WINAPI timeGetTime(void);
                extern "C" {
                    extern i32 g_lastNow;
                    i32 g_bareInsideBlock;
                }
                """
            )
        )
        self.assertEqual(
            names,
            ["g_frameTime", "g_movingLogicMax", "g_a", "g_b", "g_c", "g_slots",
             "ButeParseErrorSink", "NetEnumCb", "timeGetTime", "g_lastNow",
             "g_bareInsideBlock"],
        )
        self.assertIn("duplicate header externs", cleanliness._RATCHET)

    def test_duplicate_header_externs_ratchets_and_exempts_the_umbrella_pair(self):
        cleanliness.save_baseline([("duplicate header externs", 1)])
        merged = dict(
            cleanliness.merge_baseline_downonly([("duplicate header externs", 9)])
        )
        self.assertEqual(merged["duplicate header externs"], 1)
        # Mfc.h and Win32.h can never both be included (MFC's C1189), so a symbol
        # declared once in each is not a duplicate; a third declarer still is.
        self.assertEqual(cleanliness._UMBRELLA_PAIR,
                         {"include/Mfc.h", "include/Win32.h"})

    def test_cpp_external_prototypes_exclude_qualified_direct_initialized_data(self):
        code = cleanliness._strip(
            """
            template <>
            DATA(0x0024aca8)
            CPtrList CPtrListPool<CNetCmdPacket>::s_freeList(0xa);
            """
        )
        self.assertEqual(cleanliness._count_cpp_external_prototypes(code), 0)

    def test_prose_does_not_inflate_the_metrics(self):
        code = cleanliness._strip('// a )this cast and a void* m_x live in this comment\n'
                                  'const char* s = "void* m_y and )this";\n'
                                  'int real = (int)x;\n')
        self.assertNotIn(")this", code)
        self.assertNotIn("m_y", code)
        self.assertIn("(int)", code)

    def test_reinterpret_casts_are_a_down_only_metric(self):
        code = cleanliness._strip(
            "// reinterpret_cast<CFake*>(x)\n"
            "auto* real = reinterpret_cast<CReal*>(value);\n"
        )
        self.assertEqual(len(cleanliness._REINTERPRET_CAST.findall(code)), 1)
        self.assertIn("reinterpret_casts", cleanliness._RATCHET)
        cleanliness.save_baseline([("reinterpret_casts", 7)])
        merged = dict(cleanliness.merge_baseline_downonly([("reinterpret_casts", 8)]))
        self.assertEqual(merged["reinterpret_casts"], 7)

    def test_forced_comdat_emitters_are_a_down_only_metric(self):
        code = cleanliness._strip(
            "// ForceEmitComment and COMMENT_OOL_CTOR do not count\n"
            "#define PLAYER_OOL_DTOR\n"
            "void ForceEmitPlayerDtor() {}\n"
        )
        self.assertEqual(len(cleanliness._FORCED_COMDAT_EMITTER.findall(code)), 2)
        self.assertIn("forced COMDAT emitters", cleanliness._RATCHET)



# --------------------------------------------------------------------------- #
# enum_domains: split-width agreement, header discipline, tag-type exemption   #
# --------------------------------------------------------------------------- #
class TestEnumDomainGate(unittest.TestCase):
    """Negative controls for gruntz.audit.enum_domains - a gate nobody has watched
    FAIL reports success whether or not it is true."""

    def _audit(self, files):
        from gruntz.audit import enum_domains
        tmp = tempfile.TemporaryDirectory()
        root = Path(tmp.name)
        for name, text in files.items():
            fp = root / name
            fp.parent.mkdir(parents=True, exist_ok=True)
            fp.write_text(text)
        saved = enum_domains.REPO
        enum_domains.REPO = root
        try:
            return enum_domains.audit()
        finally:
            enum_domains.REPO = saved
            tmp.cleanup()

    def test_split_width_disagreement_is_caught(self):
        fatal, _w, _d = self._audit({
            "include/D.h": "GZ_ENUM_BEGIN_SPLIT(Dom, u8)\n A = 0\nGZ_ENUM_END_SPLIT(Dom, u8)\n",
            "src/a.cpp": "struct S { GZ_ENUM_STORAGE(Dom, i16) m_x; };\n"})
        self.assertTrue(any("field width" in f for f in fatal), fatal)

    def test_matching_split_width_passes(self):
        fatal, _w, _d = self._audit({
            "include/D.h": "GZ_ENUM_BEGIN_SPLIT(Dom, u8)\n A = 0\nGZ_ENUM_END_SPLIT(Dom, u8)\n",
            "src/a.cpp": "struct S { GZ_ENUM_STORAGE(Dom, u8) m_x; };\n"})
        self.assertEqual([f for f in fatal if "field width" in f], [])

    def test_storage_naming_an_undeclared_domain_is_caught(self):
        fatal, _w, _d = self._audit({"src/a.cpp": "GZ_ENUM_STORAGE(NoSuchDomain, u8) m_x;\n"})
        self.assertTrue(any("undeclared domain" in f for f in fatal), fatal)

    def test_bare_header_enum_is_caught(self):
        fatal, _w, _d = self._audit({"include/D.h": "enum Raw { A = 0, B = 1 };\n"})
        self.assertTrue(any("bare `enum Raw`" in f for f in fatal), fatal)

    def test_single_enumerator_tag_type_is_exempt(self):
        fatal, _w, _d = self._audit({"include/D.h": "enum ENoSeed { NO_SEED };\n"})
        self.assertEqual([f for f in fatal if "bare `enum" in f], [])

    def test_a_cpp_local_enum_is_not_a_header_defect(self):
        fatal, _w, _d = self._audit({"src/a.cpp": "enum Local { A = 0, B = 1 };\n"})
        self.assertEqual([f for f in fatal if "bare `enum" in f], [])

    def test_implicit_enumerator_value_is_warned_not_fatal(self):
        fatal, warn, _d = self._audit({
            "include/D.h": "GZ_ENUM_BEGIN(Dom)\n    A_ONE,\n    A_TWO = 1\nGZ_ENUM_END(Dom)\n"})
        self.assertTrue(any("A_ONE" in w for w in warn), warn)
        self.assertEqual([f for f in fatal if "A_ONE" in f], [])


# --------------------------------------------------------------------------- #
# vtable_slot_binding: the baseline must not read as empty (the found bug)     #
# --------------------------------------------------------------------------- #
class TestSlotBindingBaseline(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self._saved = vsb.BASELINE
        vsb.BASELINE = Path(self._tmp.name) / "vtable-slot-binding-baseline.tsv"

    def tearDown(self):
        vsb.BASELINE = self._saved
        self._tmp.cleanup()

    def test_banner_is_not_eaten_as_the_header(self):
        """THE ORIGINAL BUG: csv.DictReader took the `#` banner as the header row, so
        every lookup missed and the whole frozen backlog read as EMPTY - passing all."""
        vsb.write_baseline([("WIRING", 0x1eadbc, "CFoo", 1, 0xe8e00,
                             "?Gap_0e8e00@@YAXXZ", "someunit", "why", Path("x.cpp"), 1)])
        base = vsb.load_baseline()
        self.assertEqual(base, {(0x1eadbc, 1, "?Gap_0e8e00@@YAXXZ")},
                         "the baseline did not round-trip (banner eaten as header?)")

    def test_missing_baseline_is_fail_closed_not_vacuous(self):
        self.assertEqual(vsb.load_baseline(), set())

    def test_virtuality_is_read_off_the_mangled_name(self):
        self.assertEqual(vsb.classify_storage(vsb.split_mangled(
            "?RenderFrame@CFaderFlat@@UAEXH@Z")[2]), "virtual")
        self.assertEqual(vsb.classify_storage(vsb.split_mangled(
            "?Gap_17f660@@YAXXZ")[2]), "free function")
        self.assertEqual(vsb.classify_storage(vsb.split_mangled(
            "?Setup@CFoo@@QAEXXZ")[2]), "non-virtual")

    def test_external_mfc_ancestry_is_in_the_base_closure(self):
        classes = {"CGameDialog": (0, ["CDialog"])}
        self.assertEqual(
            vsb.base_closure("CGameDialog", classes),
            {"CGameDialog", "CDialog", "CWnd", "CCmdTarget", "CObject"},
        )


# --------------------------------------------------------------------------- #
# class_meta: comments and strings do not create class definitions             #
# --------------------------------------------------------------------------- #
class TestClassMetaScanner(unittest.TestCase):
    def test_a_comment_never_declares_anything(self):
        with _Tree({"a.cpp": "// class CGhost { };\n"
                             "/* class CBlock {}; */\n"
                             "class CReal {};\n"}):
            self.assertEqual(set(class_meta.unique_class_defs()), {"CReal"})

    def test_a_double_slash_inside_a_string_is_not_a_comment(self):
        """The state machine must not treat "http://x" as starting a comment and eat
        the real declaration that follows on the same line."""
        with _Tree({"a.cpp": 'const char* u = "http://x";\nstruct CAfter {};\n'}):
            self.assertIn("CAfter", class_meta.unique_class_defs())


class TestOmittedZeroFuzzyPercent(unittest.TestCase):
    """A function scored at exactly 0.0% has NO `fuzzy_match_percent` key.

    objdiff serializes report.json with serde's skip-the-default rule, so a true 0.0%
    is indistinguishable BY KEY PRESENCE from a function it never diffed - and it is
    NOT the latter: objdiff's one-shot `diff` carries `"match_percent": 0.0` with a
    live `target_symbol` link for exactly these. Measured 2026-07-27: 8 tree functions
    sit at a true 0.0%.

    Two readers guessed the missing key and both guessed wrong, silently:
      * `permute_sweep._pcts` defaulted it to 100.0, so its "<100%" worklist SKIPPED
        every 0%-matching function - the ones most in need of permuting.
      * `Report.fn_pct` returned None, so `gruntz sema rva` printed no match line at
        all for them (a dossier that omits the worst score is worse than no dossier).
    """

    _ZERO = {"name": "?Zero@@QAEHXZ", "size": "100", "address": "0"}  # key omitted == 0.0
    _HALF = {"name": "?Half@@QAEHXZ", "size": "100", "address": "100",
             "fuzzy_match_percent": 50.0}

    def _report(self, root):
        (root / "report.json").write_text(json.dumps({
            "measures": {}, "units": [{"name": "u", "measures": {},
                                       "functions": [self._ZERO, self._HALF]}]}))
        return root / "report.json"

    def test_fn_fuzzy_reads_the_omitted_key_as_zero(self):
        self.assertEqual(report.fn_fuzzy(self._ZERO), 0.0)
        self.assertEqual(report.fn_fuzzy(self._HALF), 50.0)
        self.assertNotEqual(report.fn_fuzzy(self._ZERO), 100.0)

    def test_fn_pct_distinguishes_scored_zero_from_absent(self):
        with tempfile.TemporaryDirectory() as tmp:
            saved = report.REPORT
            report.REPORT = self._report(Path(tmp))
            try:
                r = report.Report()
                # present-and-zero is a NUMBER, so `sema rva` prints "0.00% fuzzy" ...
                self.assertEqual(r.fn_pct("?Zero@@QAEHXZ"), 0.0)
                self.assertIsNotNone(r.fn_pct("?Zero@@QAEHXZ"))
                self.assertEqual(r.fn_pct("?Half@@QAEHXZ"), 50.0)
                # ... and only a name the report never lists is None.
                self.assertIsNone(r.fn_pct("?Missing@@QAEHXZ"))
            finally:
                report.REPORT = saved

    def _sweep_tree(self, tmp):
        """A minimal tree `permute_sweep` can read: report.json + symbol_names.csv + src."""
        root = Path(tmp)
        (root / "build" / "objdiff").mkdir(parents=True)
        (root / "build" / "gen").mkdir(parents=True)
        (root / "build" / "objdiff" / "report.json").write_text(json.dumps({
            "units": [{"name": "u", "functions": [self._ZERO, self._HALF]}]}))
        (root / "build" / "gen" / "symbol_names.csv").write_text(
            "rva,name,unit,size,kind\n"
            "0x00001000,?Zero@@QAEHXZ,u,0x64,func\n"
            "0x00002000,?Half@@QAEHXZ,u,0x64,func\n")
        (root / "u.cpp").write_text("RVA(0x00001000, 0x64)\nx\nRVA(0x00002000, 0x64)\ny\n")
        return root

    def test_permute_sweep_does_not_read_a_zero_as_a_hundred(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = self._sweep_tree(tmp)
            cwd = Path.cwd()
            os.chdir(root)
            try:
                pcts = permute_sweep._pcts("u")
            finally:
                os.chdir(cwd)
            self.assertEqual(pcts["?Zero@@QAEHXZ"], 0.0)
            self.assertEqual(pcts["?Half@@QAEHXZ"], 50.0)
            self.assertLess(pcts["?Zero@@QAEHXZ"], 100.0)

    def test_the_sweep_worklist_contains_the_zero_percent_function(self):
        """THE BUG, end to end: `_ordered` selects the unit's <100% functions, and the
        0%-matching one - the function most in need of the permuter - must be in it."""
        with tempfile.TemporaryDirectory() as tmp:
            root = self._sweep_tree(tmp)
            cwd = Path.cwd()
            os.chdir(root)
            try:
                got = permute_sweep._ordered("u", str(root / "u.cpp"))
            finally:
                os.chdir(cwd)
            self.assertEqual(got, [("?Zero@@QAEHXZ", 0.0), ("?Half@@QAEHXZ", 50.0)])

    def test_importing_the_sweep_neither_exits_nor_chdirs(self):
        """An importable module must not sys.exit()/chdir() at module scope - that is
        what made the bug above untestable in the first place."""
        import importlib
        cwd = Path.cwd()
        importlib.reload(permute_sweep)          # would SystemExit if argv were parsed here
        self.assertEqual(Path.cwd(), cwd)


def main() -> int:
    v = 2 if "-v" in sys.argv or "--verbose" in sys.argv else 1
    suite = unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    res = unittest.TextTestRunner(verbosity=v).run(suite)
    return 0 if res.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
