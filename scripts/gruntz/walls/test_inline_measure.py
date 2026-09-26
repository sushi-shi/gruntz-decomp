"""Real compiler controls for the measure-cb command's inference boundary."""

import contextlib
import io
import os
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from gruntz.walls import inline_model


@unittest.skipUnless(os.environ.get("MSVC_DIR"), "requires the pinned VC5 environment")
class InlineMeasureControls(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.directory = Path(self.tmp.name)
        self.patch = mock.patch.object(inline_model, "_MODEL_SCRATCH", self.directory / "out")
        self.patch.start()
        self.addCleanup(self.patch.stop)
        self.source = self.directory / "constructor_sites.cpp"
        self.source.write_text('''#include <StdAfx.h>
#include <new>
struct ConstructorSiteControl : CObject {
    int id, flags;
    void* owner;
    virtual ~ConstructorSiteControl() {}
    ConstructorSiteControl(void* o, int x, int f) { id=x; flags=f; owner=o; }
};
void consume(ConstructorSiteControl*);
void heapSite(void* o, int x, int f) { consume(new ConstructorSiteControl(o,x,f)); }
void placementSite(ConstructorSiteControl* p, void* o, int x, int f) {
    new(p) ConstructorSiteControl(o,x,f);
}
''')

    def run_measure(self, caller, *extra):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            try:
                rc = inline_model.main([
                    "--measure-cb", str(self.source),
                    "--fn", "??0ConstructorSiteControl@@QAE@PAXHH@Z",
                    "--caller", caller, "--sites", "1", *extra])
            except SystemExit as exc:
                rc = exc.code
        return rc, out.getvalue(), err.getvalue()

    def test_mfc_placement_rejection_does_not_disprove_inline_eligibility(self):
        rc, heap, err = self.run_measure("heapSite")
        self.assertEqual(rc, 0, err)
        self.assertIn("1 expanded, 0 rejected", heap)
        self.assertIn("cb not determined", heap)
        rc, placement, err = self.run_measure("placementSite", "--flat-floor-budget")
        self.assertEqual(rc, 0, err)
        self.assertIn("0 expanded, 1 rejected", placement)
        self.assertIn("check site eligibility", placement)
        self.assertNotIn("NOT an inline candidate", placement)

    def test_missing_caller_cannot_report_all_sites_expanded(self):
        rc, out, err = self.run_measure("absentCaller")
        self.assertNotEqual(rc, 0)
        self.assertIn("matched 0 procedures", err)
        self.assertNotIn("expanded", out)

    def test_ambiguous_caller_cannot_select_the_first_procedure(self):
        rc, out, err = self.run_measure("Site@@YAX")
        self.assertNotEqual(rc, 0)
        self.assertIn("matched 2 procedures", err)
        self.assertNotIn("expanded", out)

    def test_flat_harness_intervals_require_explicit_calibration(self):
        source = self.directory / "flat.cpp"
        source.write_text(inline_model.gen_harness(8, 30, 0))
        args = ["--measure-cb", str(source), "--fn", "leaf",
                "--caller", "callerX", "--sites", "30"]
        for extra, expected in [([], "cb not determined"),
                                (["--flat-floor-budget"], "conditional cb in")]:
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(inline_model.main(args + extra), 0)
            self.assertIn(expected, out.getvalue())
