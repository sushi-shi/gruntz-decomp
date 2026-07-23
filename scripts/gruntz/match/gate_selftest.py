#!/usr/bin/env python3
"""gruntz.match.gate_selftest - NEGATIVE CONTROLS for the build gates.

A gate nobody has seen FAIL is a green light, not a check.

Every defect this suite pins was a gate that looked healthy and reported a number
nobody had ever falsified:

  * ``vtable_slot_binding`` read its own baseline through ``csv.DictReader``, which
    ate the ``#`` banner as the header row - so the frozen backlog parsed as EMPTY
    and the gate passed everything, forever. Found by writing a negative control.
  * ``class_sizes`` matched ``SIZE(C, N)`` in PROSE, and ``out[name]`` let the last
    writer win - so a COMMENT beat the real declaration and turned a FATAL gate red
    against correct code.
  * the MAX-% high-water absorbed a 0/0 report's published 100% via ``max(prev, cur)``
    and pinned config/match-max.tsv at 100.0000 permanently (bb4d94cef).
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
import sys
import tempfile
import unittest
from pathlib import Path

from gruntz.cleanliness import board as cleanliness
from gruntz.cleanliness import class_sizes
from gruntz.core import class_meta
from gruntz.core import library_labels
from gruntz.build import canonicalize_data_symbols
from gruntz.cleanliness import vtable_slot_binding as vsb
from gruntz.match import high_water, status
from gruntz.match import verify_unique_names as vun


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


# --------------------------------------------------------------------------- #
# COFF normalization: MSVC's x86 `$E<n>` functions carry a leading underscore #
# --------------------------------------------------------------------------- #
class TestCompilerPrivateFunctionNames(unittest.TestCase):
    def test_x86_dynamic_initializer_is_content_addressed(self):
        self.assertEqual(canonicalize_data_symbols._family("_$E28"), ("e", None))


# --------------------------------------------------------------------------- #
# class_sizes: PROSE is not CODE, and a duplicate is not a race                #
# --------------------------------------------------------------------------- #
class TestClassSizesProse(unittest.TestCase):
    def test_size_in_a_comment_is_not_a_declaration(self):
        """THE ORIGINAL BUG: a note mentioning SIZE(C, 0x30) beat the real SIZE(C, 0x34)."""
        with _Tree({"a.h": "class CUserLogic {};\nSIZE(0x34);\n",
                    "b.cpp": "// (2) SIZE(0x30) under CUserLogic revisited - a base at +0x34 ...\n"}):
            self.assertEqual(class_sizes._declared_sizes(), {"CUserLogic": 0x34},
                             "a SIZE() written in prose was read as a declaration")

    def test_conflicting_real_declarations_are_an_error_not_a_race(self):
        """Two REAL decls disagreeing must FAIL, not silently pick the last writer."""
        # The gate reports the conflict on stderr; swallow it so this EXPECTED failure
        # does not read as a real one in the build log it now runs inside.
        with _Tree({"a.h": "struct CFoo {};\nSIZE(0x34);\n", "b.h": "struct CFoo;\nSIZE(0x30);\n"}):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                with self.assertRaises(SystemExit) as cm:
                    class_sizes._declared_sizes()
            self.assertEqual(cm.exception.code, 1)
            self.assertIn("CONFLICTING", err.getvalue())
            self.assertIn("0x34", err.getvalue())   # both sides named, no winner picked
            self.assertIn("0x30", err.getvalue())

    def test_agreeing_duplicates_are_fine(self):
        with _Tree({"a.h": "struct CFoo {};\nSIZE(0x34);\n", "b.h": "struct CFoo;\nSIZE(0x34);\n"}):
            self.assertEqual(class_sizes._declared_sizes(), {"CFoo": 0x34})

    def test_loadbearing_ignores_english_prose(self):
        """`new` is an English word. Prose must not make a class load-bearing -
        that escalates a documented partial model into a FATAL byte-bug report."""
        with _Tree({"a.cpp": "// allocate a new CGameMgr for the new tile when the\n"
                             "// new fader runs; sizeof(CObject) is discussed here.\n"
                             "void f() { CReal* p = new CReal(); }\n"}):
            lb = class_sizes._loadbearing()
        self.assertIn("CReal", lb, "a real `new CReal()` must be load-bearing")
        for prose in ("CGameMgr", "CObject", "tile", "fader"):
            self.assertNotIn(prose, lb, f"prose word {prose!r} counted as load-bearing")


# --------------------------------------------------------------------------- #
# high_water: a 0/0 report publishes 100% - it must never become the peak      #
# --------------------------------------------------------------------------- #
class TestHighWater(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.f = Path(self._tmp.name) / "match-max.tsv"

    def tearDown(self):
        self._tmp.cleanup()

    def test_degenerate_empty_report_cannot_poison_the_peak(self):
        """THE ORIGINAL BUG: total_functions == 0 -> objdiff publishes 100% ->
        max(prev, cur) locked config/match-max.tsv at 100.0000 forever."""
        high_water.write(72.7691, 3975, self.f)
        peak, note, wrote = high_water.update(100.0, 0, self.f)
        self.assertAlmostEqual(peak, 72.7691, places=3)
        self.assertIn("REFUSED", note)
        self.assertFalse(wrote)
        self.assertAlmostEqual(high_water.read(self.f)[0], 72.7691, places=3)

    def test_partial_build_cannot_raise_the_peak(self):
        """A report covering a fraction of the tree is not comparable."""
        high_water.write(72.7691, 3975, self.f)
        peak, note, wrote = high_water.update(99.9, 120, self.f)
        self.assertAlmostEqual(peak, 72.7691, places=3)
        self.assertIn("REFUSED", note)
        self.assertFalse(wrote)

    def test_a_real_improvement_still_raises_the_peak(self):
        """The guard must not brick the ratchet: a comparable reading still counts."""
        high_water.write(72.7691, 3975, self.f)
        peak, note, wrote = high_water.update(73.1693, 3975, self.f)
        self.assertAlmostEqual(peak, 73.1693, places=3)
        self.assertIn("NEW HIGH", note)
        self.assertTrue(wrote)
        self.assertAlmostEqual(high_water.read(self.f)[0], 73.1693, places=3)

    def test_a_dip_never_lowers_the_peak(self):
        high_water.write(73.1693, 3975, self.f)
        peak, _note, _w = high_water.update(70.0, 3975, self.f)
        self.assertAlmostEqual(peak, 73.1693, places=3)

    def test_growing_the_tree_is_comparable(self):
        """Adding units RAISES total_functions - that must not trip the guard."""
        high_water.write(73.0, 3975, self.f)
        peak, note, _w = high_water.update(74.0, 4200, self.f)
        self.assertAlmostEqual(peak, 74.0, places=3)
        self.assertIn("NEW HIGH", note)

    def test_legacy_one_token_file_is_adopted_not_trusted(self):
        self.f.write_text("72.7691\n")
        self.assertEqual(high_water.read(self.f), (72.7691, None))
        peak, _note, _w = high_water.update(73.1693, 3975, self.f)
        self.assertAlmostEqual(peak, 72.7691, places=3)   # cannot compare -> no raise
        self.assertEqual(high_water.read(self.f)[1], 3975)  # scale adopted for next time

    def test_peak_is_still_readable_by_a_split_zero_reader(self):
        """Back-compat: anything doing `read_text().split()[0]` keeps working."""
        high_water.write(73.1693, 3975, self.f)
        self.assertAlmostEqual(float(self.f.read_text().split()[0]), 73.1693, places=3)


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
            with contextlib.redirect_stdout(io.StringIO()):
                status.cmd_update(args)
        finally:
            status.fingerprinter, status.func_rvas = saved_fpr, saved_rvas
        return status.load_baseline()[("u", "?F@@QAEHXZ")]

    def _seed(self, best, cur, fp, rva, tries=3):
        status.write_baseline({("u", "?F@@QAEHXZ"): {
            "best": best, "cur": cur, "tries": tries, "fp": fp, "addr": rva}})

    def test_editing_a_function_cannot_erode_its_best(self):
        """THE BUG, minimal: a row with best > current, an EDIT, and a plain update."""
        self._seed(best=98.8947, cur=98.8947, fp="aaaa", rva=0xe7440)
        row = self._run_update(87.0526, fp="bbbb", rva=0xe7440)  # source changed, % fell
        self.assertAlmostEqual(row["best"], 98.8947, places=3)   # old code: 87.0526
        self.assertAlmostEqual(row["cur"], 87.0526, places=3)
        self.assertEqual(row["tries"], 4)  # the edit is still RECORDED, just not charged

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

    def test_an_unvouchable_rva_ratchets_rather_than_erodes(self):
        """A pre-rva baseline row (or one with no symbol_names entry) cannot be checked
        for identity. Fail LOUD (a false REGRESS a human reads), never silent erosion."""
        self._seed(best=93.3127, cur=93.3127, fp="aaaa", rva=None)
        row = self._run_update(89.5636, fp="bbbb", rva=None)
        self.assertAlmostEqual(row["best"], 93.3127, places=3)

    def test_accept_regressions_is_the_ONE_way_to_lower_a_best(self):
        """The deliberate, reviewed escape hatch must still work."""
        self._seed(best=98.8947, cur=98.8947, fp="aaaa", rva=0xe7440)
        row = self._run_update(87.0526, fp="bbbb", rva=0xe7440, accept=True)
        self.assertAlmostEqual(row["best"], 87.0526, places=3)


# --------------------------------------------------------------------------- #
# cleanliness: an UNMEASURED metric is not a measurement of zero               #
# --------------------------------------------------------------------------- #
class TestCleanlinessRatchet(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self._saved = cleanliness.BASELINE
        cleanliness.BASELINE = Path(self._tmp.name) / "cleanliness-baseline.tsv"

    def tearDown(self):
        cleanliness.BASELINE = self._saved
        self._tmp.cleanup()

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

    def test_prose_does_not_inflate_the_metrics(self):
        code = cleanliness._strip('// a )this cast and a void* m_x live in this comment\n'
                                  'const char* s = "void* m_y and )this";\n'
                                  'int real = (int)x;\n')
        self.assertNotIn(")this", code)
        self.assertNotIn("m_y", code)
        self.assertIn("(int)", code)


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


# --------------------------------------------------------------------------- #
# class_meta: the shared scanner both halves of class_sizes depend on          #
# --------------------------------------------------------------------------- #
class TestClassMetaScanner(unittest.TestCase):
    def test_a_comment_never_declares_anything(self):
        with _Tree({"a.cpp": "// class CGhost { };  SIZE(0x10);  VTBL(CGhost, 0x1)\n"
                             "/* class CBlock {}; SIZE(0x20); */\n"
                             "class CReal {};\nSIZE(0x30);\n"}):
            self.assertEqual(set(class_meta.unique_class_defs()), {"CReal"})
            self.assertEqual(class_meta.size_annotated_names(), {"CReal"})

    def test_a_double_slash_inside_a_string_is_not_a_comment(self):
        """The state machine must not treat "http://x" as starting a comment and eat
        the real declaration that follows on the same line."""
        with _Tree({"a.cpp": 'const char* u = "http://x";\nstruct CAfter {};\nSIZE(0x40);\n'}):
            self.assertIn("CAfter", class_meta.size_annotated_names())


def main() -> int:
    v = 2 if "-v" in sys.argv or "--verbose" in sys.argv else 1
    suite = unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    res = unittest.TextTestRunner(verbosity=v).run(suite)
    return 0 if res.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
