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

import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

from gruntz.match import class_meta, class_sizes, cleanliness, high_water
from gruntz.match import vtable_slot_binding as vsb


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
# class_sizes: PROSE is not CODE, and a duplicate is not a race                #
# --------------------------------------------------------------------------- #
class TestClassSizesProse(unittest.TestCase):
    def test_size_in_a_comment_is_not_a_declaration(self):
        """THE ORIGINAL BUG: a note mentioning SIZE(C, 0x30) beat the real SIZE(C, 0x34)."""
        with _Tree({"a.h": "class CUserLogic {};\nSIZE(CUserLogic, 0x34);\n",
                    "b.cpp": "// (2) SIZE(CUserLogic, 0x30) revisited - a base at +0x34 ...\n"}):
            self.assertEqual(class_sizes._declared_sizes(), {"CUserLogic": 0x34},
                             "a SIZE() written in prose was read as a declaration")

    def test_conflicting_real_declarations_are_an_error_not_a_race(self):
        """Two REAL decls disagreeing must FAIL, not silently pick the last writer."""
        # The gate reports the conflict on stderr; swallow it so this EXPECTED failure
        # does not read as a real one in the build log it now runs inside.
        with _Tree({"a.h": "SIZE(CFoo, 0x34);\n", "b.h": "SIZE(CFoo, 0x30);\n"}):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                with self.assertRaises(SystemExit) as cm:
                    class_sizes._declared_sizes()
            self.assertEqual(cm.exception.code, 1)
            self.assertIn("CONFLICTING", err.getvalue())
            self.assertIn("0x34", err.getvalue())   # both sides named, no winner picked
            self.assertIn("0x30", err.getvalue())

    def test_agreeing_duplicates_are_fine(self):
        with _Tree({"a.h": "SIZE(CFoo, 0x34);\n", "b.h": "SIZE(CFoo, 0x34);\n"}):
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
        with _Tree({"a.cpp": "// class CGhost { };  SIZE(CGhost, 0x10);  VTBL(CGhost, 0x1)\n"
                             "/* class CBlock {}; SIZE(CBlock, 0x20); */\n"
                             "class CReal {};\nSIZE(CReal, 0x30);\n"}):
            self.assertEqual(set(class_meta.unique_class_defs()), {"CReal"})
            self.assertEqual(class_meta.size_annotated_names(), {"CReal"})

    def test_a_double_slash_inside_a_string_is_not_a_comment(self):
        """The state machine must not treat "http://x" as starting a comment and eat
        the real declaration that follows on the same line."""
        with _Tree({"a.cpp": 'const char* u = "http://x"; \nSIZE(CAfter, 0x40);\n'}):
            self.assertIn("CAfter", class_meta.size_annotated_names())


def main() -> int:
    v = 2 if "-v" in sys.argv or "--verbose" in sys.argv else 1
    suite = unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    res = unittest.TextTestRunner(verbosity=v).run(suite)
    return 0 if res.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
