"""gruntz.verify.selftest - NEGATIVE CONTROLS for the ported gates.

A gate nobody has seen FAIL is a green light, not a check (the ported
gate_selftest doctrine: the slot-binding gate once parsed its own baseline's
banner as the header row and passed everything, forever). Every ported gate
gets at least one demonstrated failure case AND a clean pass; the tests are
hermetic (tmpdir trees / synthetic models) except the DATA_COMPGEN control
set, which deliberately runs against real base objs (skipped loudly when the
tree is unbuilt).

    gruntz verify selftest             # every control, one dot each
    gruntz verify selftest -v          # name each control as it runs
    gruntz verify selftest -k Ledger   # only the controls matching a name
"""

from __future__ import annotations

import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


# --------------------------------------------------------------------------- #
# fast tier                                                                   #
# --------------------------------------------------------------------------- #
class BoardControls(unittest.TestCase):
    def test_ratcheted_metric_cannot_creep_up(self):
        from gruntz.verify import board
        base = board.load_baseline()
        floor = base.get("reinterpret_casts", 0)
        rows = [("reinterpret_casts", floor + 1)]
        self.assertTrue(board.gate(rows))            # a rise FAILS
        self.assertFalse(board.gate([("reinterpret_casts", floor)]))

    def test_non_ratcheted_metric_only_tracks(self):
        from gruntz.verify import board
        self.assertFalse(board.gate([("Unknown ids", 10_000)]))

    def test_address_derived_identifiers_are_counted(self):
        from gruntz.verify.board import _count_address_derived_identifiers
        src = ("i32 m_8; CMapStringToOb m_10map; CString local_14; "
               "DWORD g_ratingRaw_64da84;")
        self.assertEqual(_count_address_derived_identifiers(src), 4)
        clean = "i32 m_reserved; CMapStringToOb m_workersByName;"
        self.assertEqual(_count_address_derived_identifiers(clean), 0)

    def test_unmeasured_semantic_floor_survives_a_bless(self):
        from gruntz.verify import board
        with tempfile.TemporaryDirectory() as td:
            sem = Path(td) / "sem.tsv"
            sem.write_text("caller-callee FAKE-VIEW\t0\ntruncated masks\t0\n")
            with mock.patch.object(board, "SEMANTIC_BASELINE", sem), \
                 mock.patch.object(board, "TEXT_BASELINE", Path(td) / "t.tsv"):
                board.save_baseline([("nested static_casts", 25)],
                                    include_semantic=True)
                kept = board.load_baseline()
        self.assertEqual(kept.get("truncated masks"), 0)   # floor NOT dropped
        self.assertEqual(kept.get("nested static_casts"), 25)


class BansControls(unittest.TestCase):
    def _scan(self, text):
        from gruntz.verify import bans
        with tempfile.TemporaryDirectory() as td:
            f = Path(td) / "probe.h"
            f.write_text(text)
            with mock.patch("gruntz.verify.bans.source_files",
                            return_value=[f]):
                return list(bans.scan())

    def test_manual_vtable_idiom_fails(self):
        hits = self._scan("struct CFooVtbl { void* slots[4]; };\n"
                          "int use(CFoo* p) { return p->vtbl != 0; }\n")
        self.assertGreaterEqual(len(hits), 2)

    def test_prose_does_not_trip_the_ban(self):
        hits = self._scan("// the old struct CFooVtbl idiom is banned\n"
                          "class CFoo { virtual void Render(); };\n")
        self.assertEqual(hits, [])


class CastControls(unittest.TestCase):
    def test_seam_self_recursion_is_caught(self):
        from gruntz.verify import casts
        with tempfile.TemporaryDirectory() as td:
            (Path(td) / "src").mkdir()
            (Path(td) / "include").mkdir()
            f = Path(td) / "src/Probe.cpp"
            f.write_text("inline u16* Scratch16() { return Scratch16(); }\n")
            with mock.patch.object(casts, "REPO", Path(td)):
                self.assertEqual(len(casts.self_recursion()), 1)
                f.write_text("inline u16* Scratch16() { "
                             "return reinterpret_cast<u16*>(g_scratch); }\n")
                self.assertEqual(casts.self_recursion(), [])

    def test_unreasoned_cast_is_open_and_reasoned_is_parked(self):
        from gruntz.verify import casts
        with tempfile.TemporaryDirectory() as td:
            (Path(td) / "src").mkdir()
            (Path(td) / "include").mkdir()
            f = Path(td) / "src/Probe.cpp"
            with mock.patch.object(casts, "REPO", Path(td)):
                f.write_text("u16* p = reinterpret_cast<u16*>(g_x);\n")
                _forced, openv = casts.scan_ledger()
                self.assertEqual(sum(len(v) for v in openv.values()), 1)
                f.write_text("u16* p = reinterpret_cast<u16*>(g_x); "
                             "// byte-forced: no reloc, bare imm\n")
                _forced, openv = casts.scan_ledger()
                self.assertEqual(sum(len(v) for v in openv.values()), 0)

    def test_a_cast_in_prose_is_not_a_site(self):
        from gruntz.verify import casts
        with tempfile.TemporaryDirectory() as td:
            (Path(td) / "src").mkdir()
            (Path(td) / "include").mkdir()
            (Path(td) / "src/P.cpp").write_text(
                "// reinterpret_cast<u16*>(g_x) would be wrong here\n")
            with mock.patch.object(casts, "REPO", Path(td)):
                forced, openv = casts.scan_ledger()
                self.assertEqual((sum(forced.values()),
                                  sum(len(v) for v in openv.values())), (0, 0))


class EnumDomainControls(unittest.TestCase):
    def _audit(self, files):
        from gruntz.verify import enum_domains
        with tempfile.TemporaryDirectory() as td:
            paths = []
            for name, text in files.items():
                p = Path(td) / name
                p.write_text(text)
                paths.append(p)
            real = Path(td)
            with mock.patch.object(enum_domains, "source_files",
                                   lambda: iter(paths)), \
                 mock.patch.object(enum_domains, "REPO", real):
                return enum_domains.audit()

    def test_split_width_disagreement_is_fatal(self):
        fatal, _w, _d = self._audit({
            "A.h": "GZ_ENUM_BEGIN_SPLIT(Tool, u8)\nTOOL_GAUNTLETZ = 0,\n"
                   "GZ_ENUM_END_SPLIT(Tool)\n",
            "B.h": "GZ_ENUM_STORAGE(Tool, i32) m_tool;\n"})
        self.assertTrue(any("two beliefs" in f for f in fatal))

    def test_matching_split_width_passes(self):
        fatal, _w, _d = self._audit({
            "A.h": "GZ_ENUM_BEGIN_SPLIT(Tool, u8)\nTOOL_GAUNTLETZ = 0,\n"
                   "GZ_ENUM_END_SPLIT(Tool)\n",
            "B.h": "GZ_ENUM_STORAGE(Tool, u8) m_tool;\n"})
        self.assertEqual(fatal, [])

    def test_bare_header_enum_is_fatal_and_tag_type_exempt(self):
        fatal, _w, _d = self._audit({
            "A.h": "enum Tool { TOOL_A = 1, TOOL_B = 2 };\n"})
        self.assertTrue(any("bare `enum Tool`" in f for f in fatal))
        fatal, _w, _d = self._audit({
            "A.h": "enum ENoSeed { NO_SEED };\n"})
        self.assertEqual(fatal, [])

    def test_range_test_against_a_member_is_fatal(self):
        fatal, _w, _d = self._audit({
            "A.h": "GZ_ENUM_BEGIN(Pickup)\nPICKUP_WINGZ = 22,\n"
                   "GZ_ENUM_END(Pickup)\n",
            "B.cpp": "if (n > PICKUP_WINGZ) return 0;\n"})
        self.assertTrue(any("names a MEMBER" in f for f in fatal))


class LabelStyleControls(unittest.TestCase):
    def _scan(self, text):
        from gruntz.verify import label_style
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "Probe.cpp"
            p.write_text(text)
            return label_style.scan(p), label_style.compgen_order(p)

    def test_off_canon_and_volatile_ordinal_fail(self):
        hits, _o = self._scan("RVA(0x12345, 0x10)\n"           # 5-digit addr
                              "RVA_COMPGEN(0x00012345, 0x10, _$E123)\n")
        self.assertTrue(any("off-canon" in why for _l, why, _t in hits))
        self.assertTrue(any("volatile" in why for _l, why, _t in hits))

    def test_unblessed_marker_fails_and_blessed_passes(self):
        hits, _o = self._scan("// @fold-TODO: merge with CFoo\n")
        self.assertEqual(len(hits), 1)
        hits, _o = self._scan("// @early-stop cfg wall, branch counts differ\n")
        self.assertEqual(hits, [])

    def test_compgen_out_of_rva_order_fails(self):
        _h, order = self._scan("RVA(0x00001000, 0x10)\n"
                               "RVA_COMPGEN(0x00003000, 0x10, ??_GX@@UAEPAXI@Z)\n"
                               "RVA(0x00002000, 0x10)\n")
        self.assertEqual(len(order), 1)

    def test_canonical_labels_pass(self):
        hits, order = self._scan("RVA(0x00001000, 0x10)\n"
                                 "RVA_COMPGEN(0x00001400, 0x0, ??_GX@@UAEPAXI@Z)\n"
                                 "RVA(0x00002000, 0x1f)\n"
                                 "RVA_DYNINIT(0x00003000, 0x1f, "
                                 "CActRegPool<CGrunt>::s_table)\n")
        self.assertEqual((hits, order), ([], []))


class IncludeOrderControls(unittest.TestCase):
    def test_duplicate_and_disorder_are_caught_conservingly(self):
        from gruntz.verify import include_order as io
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "Probe.cpp"
            p.write_text("#include <rva.h>\n#include <Zed.h>\n"
                         "#include <Abc.h>\n#include <Zed.h>\n\nint x;\n")
            head, entries, tail = io.parse(p)
            headers = [h for _c, h in entries]
            dropped = [h for i, h in enumerate(headers) if h in headers[:i]]
            self.assertEqual(dropped, ["Zed.h"])
            want = io.render(head, entries, tail, None)
            self.assertNotEqual(want,
                                p.read_text().splitlines())   # out of order
            io.assert_conserved(p, p.read_text().splitlines(), want, dropped)

    def test_a_lost_line_aborts_the_rewrite(self):
        from gruntz.verify import include_order as io
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "Probe.cpp"
            p.write_text("#include <A.h>\n// @early-stop\nint x;\n")
            before = p.read_text().splitlines()
            butchered = ["#include <A.h>", "int x;"]     # marker vanished
            with self.assertRaises(SystemExit):
                io.assert_conserved(p, before, butchered, [])


# --------------------------------------------------------------------------- #
# normal tier (synthetic Models)                                              #
# --------------------------------------------------------------------------- #
def _binding(rva, name, unit="probe", channel="src", kind="", size=0x10,
             space="text", aliases=()):
    from gruntz.model import Binding
    return Binding(rva, size, kind, space, name, unit, channel,
                   tuple(aliases), ())


def _model(functions=(), data=(), violations=()):
    from gruntz.model import Model
    return Model(list(functions), list(data), list(violations))


class UniqueNamesControls(unittest.TestCase):
    def test_one_name_at_two_rvas_is_fatal(self):
        from gruntz.verify import unique_names
        m = _model([_binding(0x1000, "?F@@YAXXZ"),
                    _binding(0x2000, "?F@@YAXXZ")])
        with mock.patch("gruntz.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertTrue(any("name-injectivity" in b for b in bad))

    def test_model_violation_is_fatal_and_clean_passes(self):
        from gruntz.verify import unique_names
        m = _model([_binding(0x1000, "?F@@YAXXZ")], violations=["boom"])
        with mock.patch("gruntz.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertTrue(any("model violation" in b for b in bad))
        m = _model([_binding(0x1000, "?F@@YAXXZ"),
                    _binding(0x2000, "?G@@YAXXZ")])
        with mock.patch("gruntz.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertEqual(bad, [])

    def test_zero_claims_never_pass_vacuously(self):
        from gruntz.verify import unique_names
        with mock.patch("gruntz.model.resolve", return_value=_model()):
            bad, _n = unique_names.findings()
        self.assertTrue(any("vacuously" in b for b in bad))

    def test_a_fid_label_repeat_is_not_a_finding(self):
        from gruntz.verify import unique_names
        m = _model([_binding(0x1000, "??_G__non_rtti_object@@UAEPAXI@Z",
                             channel="functions_static_libs"),
                    _binding(0x2000, "??_G__non_rtti_object@@UAEPAXI@Z",
                             channel="functions_static_libs"),
                    _binding(0x3000, "?F@@YAXXZ")])
        with mock.patch("gruntz.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertEqual(bad, [])


class LibraryOverlapControls(unittest.TestCase):
    def test_src_claim_on_a_static_libs_row_is_loud(self):
        from gruntz.retail_labels import Claim
        from gruntz.verify import library_overlap
        alias = Claim(0x1000, "??0CFile@@QAE@XZ", "func",
                      "functions_static_libs", None, "", {})
        m = _model([_binding(0x1000, "?Open@CFileIO@@QAEHXZ",
                             aliases=[alias])])
        with mock.patch("gruntz.model.resolve", return_value=m):
            bad, _n = library_overlap.findings()
        self.assertEqual(len(bad), 1)
        m = _model([_binding(0x1000, "?Open@CFileIO@@QAEHXZ")])
        with mock.patch("gruntz.model.resolve", return_value=m):
            bad, _n = library_overlap.findings()
        self.assertEqual(bad, [])


class TuOrderControls(unittest.TestCase):
    def test_descending_file_order_is_a_violation(self):
        from gruntz.verify.tu_order import Entry, check_intra
        seq = [Entry(0x2000, 0x10, 1, "A::f", "t"),
               Entry(0x1000, 0x10, 2, "A::g", "t")]
        self.assertTrue(check_intra({"t": seq}))
        seq = [Entry(0x1000, 0x10, 1, "A::f", "t"),
               Entry(0x2000, 0x10, 2, "A::g", "t")]
        self.assertEqual(check_intra({"t": seq}), {})

    def test_overlapping_spans_are_a_violation(self):
        from gruntz.verify.tu_order import Entry, check_intra
        seq = [Entry(0x1000, 0x200, 1, "A::f", "t"),
               Entry(0x1100, 0x10, 2, "A::g", "t")]
        self.assertTrue(check_intra({"t": seq}))

    def test_interleaving_tus_are_a_violation(self):
        from gruntz.verify.tu_order import Entry, check_inter
        a = [Entry(0x1000, 0x10, 1, "A::f", "a"),
             Entry(0x3000, 0x10, 2, "A::g", "a")]
        b = [Entry(0x2000, 0x10, 1, "B::f", "b"),
             Entry(0x4000, 0x10, 2, "B::g", "b")]
        self.assertTrue(check_inter({"a": a, "b": b}))

    def test_the_tu_key_accepts_the_project_unit_spelling(self):
        """`--tu` keys on the .cpp STEM (`Grunt`) while every other --unit
        flag takes the unit (`grunt`); the correct unit name answered "no
        such TU"."""
        import contextlib
        import io

        from gruntz.verify import tu_order as to
        seq = [to.Entry(0x1000, 0x10, 1, "CGrunt::f", "src/Grunt.cpp")]
        with mock.patch.object(to, "load_in_file_order",
                               return_value={"Grunt": seq}), \
             mock.patch.object(to, "load_exiles", return_value={}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(to.main(["--tu", "grunt"]), 0)
            self.assertIn("Grunt  (1 functions", out.getvalue())
            with contextlib.redirect_stderr(io.StringIO()) as err:
                self.assertEqual(to.main(["--tu", "zzz"]), 2)
            self.assertIn("no such TU: zzz", err.getvalue())
            with contextlib.redirect_stderr(io.StringIO()) as err:
                self.assertEqual(to.main(["--tu", "grun"]), 2)
            self.assertIn("did you mean: Grunt", err.getvalue())

    def test_a_stale_exile_row_fails(self):
        from gruntz.verify.tu_order import verify_exiles
        exiles = {0x184610: ("MenuItem", "MenuPage", "CMenuItem::GetUpName")}
        bad = verify_exiles(exiles, {}, {}, {})     # nothing pins/emits/hosts
        self.assertEqual(len(bad), 2)               # no owner pin + no host span
        bad = verify_exiles(exiles, {0x184610: "MenuItem"},
                            {"MenuPage": (0x184000, 0x185000)}, {})
        self.assertEqual(bad, [])


class DataTuOrderControls(unittest.TestCase):
    def test_a_def_inside_a_foreign_band_is_a_crossing(self):
        from gruntz.verify import data_tu_order as dto
        m = _model(data=[
            _binding(0x1000, "?g_a@@3HA", unit="alpha", space="data"),
            _binding(0x3000, "?g_b@@3HA", unit="alpha", space="data"),
            _binding(0x2000, "?g_x@@3HA", unit="beta", space="data")])
        with mock.patch("gruntz.model.resolve", return_value=m), \
             mock.patch.object(dto, "_unit_basename",
                               return_value={"alpha": "Alpha.cpp",
                                             "beta": "Beta.cpp"}), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertTrue(dto.gate_findings())
        m = _model(data=[
            _binding(0x1000, "?g_a@@3HA", unit="alpha", space="data"),
            _binding(0x1100, "?g_b@@3HA", unit="alpha", space="data"),
            _binding(0x2000, "?g_x@@3HA", unit="beta", space="data")])
        with mock.patch("gruntz.model.resolve", return_value=m), \
             mock.patch.object(dto, "_unit_basename",
                               return_value={"alpha": "Alpha.cpp",
                                             "beta": "Beta.cpp"}), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertEqual(dto.gate_findings(), [])

    def test_different_storage_bands_do_not_cross(self):
        from gruntz.verify import data_tu_order as dto
        m = _model(data=[
            _binding(0x1000, "?g_a@@3HA", unit="alpha", space="data"),
            _binding(0x3000, "?g_b@@3HA", unit="alpha", space="data"),
            _binding(0x2000, "?g_x@@3HA", unit="beta", space="bss")])
        with mock.patch("gruntz.model.resolve", return_value=m), \
             mock.patch.object(dto, "_unit_basename", return_value={}), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertEqual(dto.gate_findings(), [])


class UndefinedClosureControls(unittest.TestCase):
    def _run(self, bdef, bund, tnames, baseline=frozenset()):
        from gruntz.verify import undefined_closure as uc
        with mock.patch.object(uc, "live_base_objs", return_value=["x"]), \
             mock.patch.object(uc, "_sym_sets",
                               side_effect=[(bdef, bund), (tnames, set())]), \
             mock.patch.object(uc, "lib_symbols", return_value=set()), \
             mock.patch.object(uc, "_rtti_classes", return_value=set()), \
             mock.patch.object(uc, "source_library_shadows",
                               return_value=[]), \
             mock.patch.object(uc, "_read_baseline",
                               return_value=set(baseline)):
            return uc.analyse()

    def test_pure_phantom_class_is_a_fake_view(self):
        phantom, _sh, _dec = self._run(
            bdef={"?Run@CReal@@QAEXXZ"},
            bund={"?M@CPhantomView@@QAEXXZ"},
            tnames=set())
        self.assertIn("CPhantomView", phantom)

    def test_a_real_class_with_bodies_is_not_a_phantom(self):
        phantom, _sh, _dec = self._run(
            bdef={"?Run@CGrunt@@QAEXXZ"},
            bund={"?Walk@CGrunt@@QAEXXZ"},     # unreconstructed, not fake
            tnames=set())
        self.assertEqual(dict(phantom), {})

    def test_declared_only_alias_is_debt_unless_retail_names_it(self):
        _p, _s, declared = self._run(
            bdef=set(), bund={"?Check4_2ce8@@YGHH@Z"}, tnames=set())
        self.assertIn("?Check4_2ce8@@YGHH@Z", declared)
        _p, _s, declared = self._run(
            bdef=set(), bund={"?Check4_2ce8@@YGHH@Z"},
            tnames={"?Check4_2ce8@@YGHH@Z"})   # retail namespace has it
        self.assertEqual(declared, set())


# --------------------------------------------------------------------------- #
# full tier                                                                   #
# --------------------------------------------------------------------------- #
class VtableTierControls(unittest.TestCase):
    def test_virtuality_is_read_off_the_mangled_name(self):
        from gruntz.verify.vtables import classify_storage, split_mangled
        nm, quals, st = split_mangled("?RenderFrame@CFaderFlat@@UAEXH@Z")
        self.assertEqual((nm, quals[0], classify_storage(st)),
                         ("RenderFrame", "CFaderFlat", "virtual"))
        nm, quals, st = split_mangled("?Gap_17f660@@YAXXZ")
        self.assertEqual(classify_storage(st), "free function")
        nm, quals, st = split_mangled("??1CGrunt@@UAE@XZ")
        self.assertEqual((nm, quals[0], classify_storage(st)),
                         ("??1", "CGrunt", "virtual"))
        self.assertIsNone(split_mangled("_memcpy"))

    def test_wiring_defect_fails_and_inherited_virtual_passes(self):
        from gruntz.verify.vtables import (base_closure, classify_storage,
                                           split_mangled)
        classes = {"CFaderFlat": (2, ["CFader"]), "CFader": (2, [])}
        allowed = base_closure("CFaderFlat", classes)
        # inherited slot bound to the base's virtual: allowed
        _n, quals, st = split_mangled("?OnInit@CFader@@UAEXXZ")
        self.assertIn(quals[0], allowed)
        self.assertEqual(classify_storage(st), "virtual")
        # a free function at the slot: the WIRING defect
        _n, _q, st = split_mangled("?Gap_17f660@@YAXXZ")
        self.assertNotEqual(classify_storage(st), "virtual")
        # a virtual of a FOREIGN class: MISBOUND
        _n, quals, st = split_mangled("?Render@CImage@@UAEXXZ")
        self.assertNotIn(quals[0], allowed)

    def test_under_virtualized_class_is_counted(self):
        from gruntz.verify.vtables import resolved_virtuals
        classes = {"CShell": (1, [])}
        self.assertLess(resolved_virtuals("CShell", classes), 5)
        classes = {"CReal": (3, ["CCmdTarget"])}   # 3 own + 8 credited
        self.assertGreaterEqual(resolved_virtuals("CReal", classes), 11)

    def test_slot_resolution_honours_the_raw_target_first(self):
        from gruntz.verify.vtables import resolve_slot
        syms = {0x1000: ("?Unload@CPool@@UAEXXZ", "pool"),
                0x2000: ("?FreeAll@CPool@@QAEXXZ", "pool")}
        # raw = a real 5-byte `jmp FreeAll` override: the override IS Unload
        self.assertEqual(resolve_slot(syms, 0x1000, 0x2000)[1],
                         "?Unload@CPool@@UAEXXZ")
        # anonymous ILT thunk at raw: chase to the body
        self.assertEqual(resolve_slot(syms, 0x5555, 0x2000)[1],
                         "?FreeAll@CPool@@QAEXXZ")
        self.assertIsNone(resolve_slot(syms, 0x5555, 0x6666))


class AssertRelocsControls(unittest.TestCase):
    def test_a_misdirected_referent_is_caught(self):
        """Poison ONE symbol's rva resolution and the multiset audit must
        report a WRONG row where the clean run reports none."""
        from gruntz.core.paths import BUILD
        from gruntz.verify import assert_relocs as ar
        if not (BUILD / "objdiff/base/butemgr.obj").is_file():
            self.skipTest("base objs absent (unbuilt tree)")
        clean, _seen = ar.audit(unit_filter="butemgr")
        if clean:
            self.skipTest("butemgr not clean on this tree state - control "
                          "needs a green substrate")
        real = ar.Resolver.resolve_base

        def poisoned(self, name, typ, addend):
            out = real(self, name, typ, addend)
            if "CButeMgr" in name and out:
                return {v + 0x10 for v in out}     # shift every CButeMgr ref
            return out
        with mock.patch.object(ar.Resolver, "resolve_base", poisoned):
            bad, _seen = ar.audit(unit_filter="butemgr")
        self.assertTrue(any("WRONG" in p for _u, _n, p in bad))

    def test_a_fabricated_symbol_is_fake(self):
        from gruntz.verify import assert_relocs as ar
        resolver = mock.Mock()
        resolver.rva_of = lambda name: set()
        self.assertTrue(ar._is_fake("?M@CPhantom@@QAEXXZ", set(), resolver,
                                    set()))
        self.assertFalse(ar._is_fake("?M@CPhantom@@QAEXXZ",
                                     {"?M@CPhantom@@QAEXXZ"}, resolver,
                                     set()))
        self.assertFalse(ar._is_fake("??_C@_01ABC@x?$AA@", set(), resolver,
                                     set()))


class DataRelocsControls(unittest.TestCase):
    def test_an_injected_wrong_vtable_slot_is_caught(self):
        """The ported negative control: redirect one vtable slot's relocation
        in a copy of a real normalized base obj - the sieve must return WRONG
        (and a deleted record must return MISSING)."""
        import shutil
        import struct as st

        from gruntz.delink.coffx import Obj
        from gruntz.verify import data_relocs as dr
        from gruntz.walls import pairscan
        src_pair = pairscan.pairs({"projectile"}).get("projectile")
        if not src_pair:
            self.skipTest("normalized projectile pair absent (unbuilt tree)")
        base_rows, *_rest = dr.scan("projectile")
        if base_rows:
            self.skipTest("projectile not clean on this tree state")
        with tempfile.TemporaryDirectory() as td:
            b = Path(td) / "base" / "projectile.obj"
            t = Path(td) / "target" / "projectile.c.obj"
            b.parent.mkdir()
            t.parent.mkdir()
            shutil.copy(src_pair[0], b)
            shutil.copy(src_pair[1], t)
            # find a vtable COMDAT DIR32 in the base copy and retarget it at
            # a different symbol index
            obj = Obj(b)
            edited = False
            for secnum in range(1, obj.nsec + 1):
                members = obj.section_members(secnum)
                if not any(n.startswith("??_7") for _v, n, _s in members):
                    continue
                sec = obj.section_table[secnum - 1]
                ptr, count = sec["reloc_offset"], sec["reloc_count"]
                if not ptr or count < 2:
                    continue
                buf = bytearray(obj.buf)
                # swap the symbol indices of the first two relocations
                s0 = st.unpack_from("<I", buf, ptr + 4)[0]
                s1 = st.unpack_from("<I", buf, ptr + 10 + 4)[0]
                if s0 == s1:
                    continue
                st.pack_into("<I", buf, ptr + 4, s1)
                st.pack_into("<I", buf, ptr + 10 + 4, s0)
                b.write_bytes(bytes(buf))
                edited = True
                break
            if not edited:
                self.skipTest("no editable vtable COMDAT found")
            with mock.patch.object(dr.pairscan, "pairs",
                                   lambda units=None: {"projectile": (b, t)}):
                rows, *_r = dr.scan("projectile")
        self.assertTrue(any(r.verdict in ("WRONG", "MISSING", "EXTRA")
                            for r in rows))


class AllocSizeControls(unittest.TestCase):
    def test_disp0_store_covers_the_ebp_form(self):
        from gruntz.verify.alloc_size import _disp0_store
        # C7 06 imm32  -> mov [esi], imm32 (mod=00)
        self.assertEqual(_disp0_store(b"\xc7\x06AAAA", 0), (True, 6))
        # C7 45 00 imm32 -> mov [ebp+0x0], imm32 (mod=01 disp8=0) - the form
        # whose omission mis-attributed three CSBI_ImageSet sites
        self.assertEqual(_disp0_store(b"\xc7\x45\x00AAAA", 0), (True, 7))
        # SIB and non-zero disp are NOT plain [reg]
        self.assertEqual(_disp0_store(b"\xc7\x04\x24AAAA", 0)[0], False)
        self.assertEqual(_disp0_store(b"\xc7\x45\x04AAAA", 0)[0], False)

    def test_most_derived_uses_the_rtti_spine(self):
        from gruntz.verify import alloc_size
        sw = object.__new__(alloc_size.Sweep)
        with mock.patch.object(alloc_size, "rtti_ancestors",
                               return_value={"CProjectile": {"CMovingLogic",
                                                             "CGameObject"},
                                             "CMovingLogic": {"CGameObject"}}):
            got = alloc_size.Sweep._most_derived(
                sw, {"CProjectile", "CMovingLogic"})
            self.assertEqual(got, "CProjectile")
            self.assertIsNone(alloc_size.Sweep._most_derived(
                sw, {"CProjectile", "CUnrelated"}))


class LinkTierControls(unittest.TestCase):
    def test_unresolved_txt_content_fails(self):
        from gruntz.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "GRUNTZ.candidate.EXE"
            cand.write_bytes(b"MZ")
            unres = Path(td) / "u.txt"
            unres.write_text("?Lost@CFoo@@QAEXXZ\n")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "UNRESOLVED", unres), \
                 mock.patch("gruntz.verify.undefined_closure._sym_sets",
                            return_value=(set(), set())), \
                 mock.patch("gruntz.verify.undefined_closure.live_base_objs",
                            return_value=[]), \
                 mock.patch("gruntz.verify.undefined_closure.lib_symbols",
                            return_value=set()):
                bad = lt.link_defect_findings()
        self.assertTrue(any("unresolved external" in b for b in bad))

    def test_a_stale_candidate_never_misattributes(self):
        import time as _t

        from gruntz.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "c.exe"
            cand.write_bytes(b"MZ")
            cmap = Path(td) / "c.map"
            cmap.write_text("x")
            objdir = Path(td) / "objdiff/base"
            objdir.mkdir(parents=True)
            newer = objdir / "a.obj"
            _t.sleep(0.01)
            newer.write_bytes(b"x")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "CMAP", cmap), \
                 mock.patch.object(lt, "BUILD", Path(td)):
                out = lt.image_diff_findings()
        self.assertTrue(out and "STALE" in out[0])


# --------------------------------------------------------------------------- #
# walls sieves                                                                #
# --------------------------------------------------------------------------- #
class PairscanControls(unittest.TestCase):
    def test_rep_movsd_folds_and_movsx_does_not_count(self):
        from gruntz.walls.aggregate_copies import REP_MOVS
        self.assertTrue(REP_MOVS.match("rep movs"))
        self.assertTrue(REP_MOVS.match("rep movsd"))
        self.assertFalse(REP_MOVS.match("movs"))       # bare table byte decode
        self.assertFalse(REP_MOVS.match("movsx"))      # sign-extend load
        # and the decoder really produces the folded spelling for F3 A5
        try:
            from gruntz.delink.coffx import Obj  # noqa: F401 (env probe)
            from gruntz.tool import objdump
            text = objdump.disassemble(b"\xf3\xa5\xc3", vma=0)
        except Exception as exc:  # noqa: BLE001
            self.skipTest(f"objdump unavailable: {exc}")
        self.assertIn("rep movs", text)

    def test_local_labels_are_not_function_boundaries(self):
        from gruntz.walls.pairscan import is_local_label
        self.assertTrue(is_local_label("$L27"))
        self.assertTrue(is_local_label("$loop_restart$32243"))
        self.assertFalse(is_local_label("?Advance@CAni@@QAEXXZ"))

    def test_canon_folds_static_suffix_and_vector_dtor(self):
        from gruntz.walls.pairscan import canon
        self.assertEqual(canon("_s_QUESTZ$Sdata_data_87db2c_0"), "_s_QUESTZ")
        self.assertEqual(canon("??_EzPTree@@UAEPAXI@Z"),
                         "??_GzPTree@@UAEPAXI@Z")


class EhFrameControls(unittest.TestCase):
    PROLOGUE = [
        (0, "push", "0xffffffff"),
        (2, "push", "0x0"),
        (7, "mov", "eax,DWORD PTR fs:0x0"),
        (13, "push", "eax"),
        (14, "mov", "DWORD PTR fs:0x0,esp"),
        (21, "sub", "esp,0x8"),
        (24, "push", "esi"),
    ]

    def test_the_full_sequence_is_required(self):
        from gruntz.walls.eh_frame import has_eh
        self.assertTrue(has_eh(self.PROLOGUE))
        no_push = [i for i in self.PROLOGUE if i[2] != "0xffffffff"]
        self.assertFalse(has_eh(no_push))              # fs store alone != EH
        no_store = [i for i in self.PROLOGUE
                    if not i[2].startswith("DWORD PTR fs:0x0,")]
        self.assertFalse(has_eh(no_store))

    def test_presence_mismatch_classifies(self):
        from gruntz.walls.eh_frame import classify
        plain = [(0, "push", "esi"), (1, "ret", "")]
        self.assertEqual(classify(plain, self.PROLOGUE), "TARGET_ONLY")
        self.assertEqual(classify(self.PROLOGUE, plain), "BASE_ONLY")
        self.assertEqual(classify(plain, plain), "NEITHER")

    def test_state_stores_are_counted_at_the_slot(self):
        from gruntz.walls.eh_frame import eh_states
        insns = self.PROLOGUE + [
            (30, "mov", "DWORD PTR [esp+0x14],0x0"),
            (38, "call", "0x100"),
            (43, "mov", "BYTE PTR [esp+0x14],0x1"),
            (48, "mov", "DWORD PTR [esp+0x14],0xffffffff"),
        ]
        slot, states = eh_states(insns)
        self.assertEqual(slot, "[esp+0x14]")
        self.assertEqual(len(states), 3)
        self.assertEqual(sorted(s for _o, s in states if s is not None),
                         [-1, 0, 1])

    def test_inline_cut_vs_exit_merge_vs_object(self):
        from gruntz.walls.eh_frame import cause, ctor_delta
        only_t, only_b, resited = ctor_delta(
            [(0, "??1CString@@QAE@XZ")],
            [(0, "??1CString@@QAE@XZ"), (5, "??0CRect@@QAE@XZ")])
        self.assertEqual(cause("TARGET_ONLY", 1, only_t, only_b, resited),
                         "INLINE_CUT")
        only_t, only_b, resited = ctor_delta(
            [(0, "??1CString@@QAE@XZ")] * 2,
            [(0, "??1CString@@QAE@XZ")] * 8)
        self.assertEqual(cause("TARGET_ONLY", 1, only_t, only_b, resited),
                         "EXIT_MERGE")
        self.assertEqual(cause("TARGET_ONLY", 1, [], [], []),
                         "MISSING_OBJECT")
        self.assertEqual(cause("BASE_ONLY", -1, [], [], []), "EXTRA_OBJECT")


class GlobalRefsControls(unittest.TestCase):
    def test_addend_past_the_symbol_is_dropped(self):
        import collections

        from gruntz.walls import global_refs as gr
        relocs = [(0x10, "_g_dir", gr.DIR32, 0x2B0),   # unsized-datum fallback
                  (0x20, "_g_dir", gr.DIR32, 0x0)]     # a real read
        obj = mock.Mock()
        with mock.patch.object(gr.pairscan, "fn_relocs",
                               return_value=relocs):
            dropped = collections.Counter()
            c = gr._refs(obj, 1, 0, 0x100, gr.DIR32, dropped,
                         {"_g_dir": 12})
        self.assertEqual(c["_g_dir"], 1)
        self.assertEqual(sum(dropped.values()), 1)


class StaleMarkerControls(unittest.TestCase):
    def test_marker_resolution_prefers_plain_rva(self):
        from gruntz.walls.stale_markers import RVA, RVA_COMPGEN
        self.assertTrue(RVA.search("RVA(0x00012340, 0x10)"))
        self.assertIsNone(RVA.search("RVA_COMPGEN(0x00012340, 0x10, X)"))
        self.assertTrue(RVA_COMPGEN.search("RVA_COMPGEN(0x00012340, 0x10, X)"))


class TierRunnerControls(unittest.TestCase):
    def test_a_crashing_gate_is_a_failure_not_a_skip(self):
        import contextlib
        import io

        from gruntz.verify import tiers

        def boom():
            raise RuntimeError("tool broke")
        with mock.patch.dict(tiers.TIERS, {"fast": [("probe", boom)]}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                failed = tiers.run(["fast"])
        self.assertEqual(failed, 1)
        self.assertIn("gate crashed", out.getvalue())

    def test_a_clean_gate_passes(self):
        import contextlib
        import io

        from gruntz.verify import tiers
        with mock.patch.dict(tiers.TIERS, {"fast": [("probe", lambda: [])]}):
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(tiers.run(["fast"]), 0)


# --------------------------------------------------------------------------- #
# the data-access map: layout oracle, every category, every suppression       #
# --------------------------------------------------------------------------- #
def _layout(types=()):
    from gruntz.verify.layout import Layout
    return Layout({"types": list(types), "units": {}, "tree_hash": "ctrl"})


def _prim(t, sz):
    return {"k": "prim", "t": t, "sz": sz}


def _arr(el, n, t="T[]"):
    return {"k": "arr", "t": t, "sz": el["sz"] * n, "n": n, "el": el}


def _claim(rva, node, name="?g_probe@@3HA", extent=None, channel="src",
           unit="probe"):
    from gruntz.verify.access_map import Claim
    return Claim(rva=rva, name=name, unit=unit, channel=channel, kind="",
                 section=".data", space="data",
                 extent=extent if extent is not None else (node or {}).get("sz")
                 or 4, node=node, pct=100.0)


def _access(target, width=4, form="direct", rw="r", mnemonic="mov", text="",
            insn=0x1000, fpu="", ext="", scale=0, base_reg="", disp=0):
    from gruntz.verify.access_map import Access
    return Access(insn_rva=insn, insn_len=6, mnemonic=mnemonic, site_rva=insn,
                  target_rva=target, width=width, rw=rw, form=form,
                  base_reg=base_reg, index_reg="eax" if scale else "",
                  scale=scale, disp=disp, fpu=fpu, ext=ext, origin="reloc",
                  text=text or f"{mnemonic} probe", owner=None)


def _findings(claims, accesses, rows=(), layout=None, cells=()):
    """Categories fired by one synthetic claim set - hermetic, no image."""
    from gruntz.verify import data_access as da
    img = mock.Mock()
    img.pe.data_regions.return_value = {"rdata": (0, 0), "data": (0, 1 << 30),
                                        "bss": (0, 0),
                                        "idata": (0x2C3000, 0x2C6C00)}
    img.section_name.return_value = ".data"
    img.u32.return_value = 0x2C4CCC
    model = mock.Mock()
    model.functions = []
    spine = da.Spine(img, model, layout or _layout(), sorted(claims,
                                                             key=lambda c: c.rva),
                     list(rows))
    owners = mock.Mock()
    owners.at.return_value = None
    found, _st = da.derive_findings(spine, list(accesses), list(cells), owners)
    return found


def _cats(rows):
    return {r[0] for r in rows}


class LayoutOracleControls(unittest.TestCase):
    def test_an_array_element_offset_is_absolute(self):
        """The measured own-goal: resolving +0x4 of `int[32]` returned the
        ELEMENT-relative 0, so every array offset past the first read as
        'lands INSIDE field [1]' - 42 fabricated width findings."""
        lay = _layout()
        node = _arr(_prim("int", 4), 32, "int[32]")
        f = lay.field_at(node, 4)
        self.assertEqual((f.off, f.size, f.path, f.tag), (4, 4, "[1]", ""))
        self.assertEqual(lay.field_at(node, 0x7C).off, 0x7C)
        self.assertEqual(lay.field_at(node, 0x80).tag, "out")

    def test_a_union_is_laid_out_but_never_adjudicated(self):
        lay = _layout()
        u = {"k": "rec", "t": "U", "sz": 8, "u": 1,
             "m": [[0, ".a", _prim("int", 4)], [0, ".b", _prim("float", 4)]]}
        self.assertFalse(lay.field_at(u, 0).resolved)

    def test_the_vptr_slot_is_tagged_not_reported_as_a_hole(self):
        lay = _layout()
        poly = {"k": "rec", "t": "CFoo", "sz": 8, "poly": 1,
                "m": [[0, ".__vfptr", _prim("void *", 4)],
                      [4, ".m_x", _prim("int", 4)]]}
        self.assertEqual(lay.field_at(poly, 0).path, ".__vfptr")
        plain = {"k": "rec", "t": "CBar", "sz": 8,
                 "m": [[4, ".m_x", _prim("int", 4)]]}
        self.assertEqual(lay.field_at(plain, 0).tag, "hole")

    def test_the_harvest_joins_every_src_claim_on_this_tree(self):
        """The oracle is only worth its verdicts if it actually reaches the
        claims: a silent join failure would look exactly like a clean tree."""
        from gruntz.core.paths import BUILD
        from gruntz.model import resolve
        from gruntz.verify.layout import CACHE, harvest
        if not CACHE.is_file() or not (BUILD / "objdiff/base").is_dir():
            self.skipTest("layout cache absent (unbuilt tree)")
        lay, _p = harvest()
        miss = [b.name for b in resolve().data
                if b.channel == "src" and not lay.var(b.unit, b.name)]
        self.assertEqual(miss[:5], [])


class DataAccessCategoryControls(unittest.TestCase):
    def test_a_narrow_declaration_is_caught(self):
        c = _claim(0x1000, _prim("unsigned char", 1), extent=1)
        self.assertIn("width", _cats(_findings([c], [_access(0x1000, 4)])))

    def test_the_and_mask_narrow_load_is_suppressed(self):
        """cl 5.0 reads a u16 global 4 bytes wide and masks the register; the
        2-byte STORE is what proves the field is 2 bytes (g_sfDeviceIndex)."""
        c = _claim(0x1000, _prim("unsigned short", 2), extent=2)
        acc = [_access(0x1000, 4, insn=0x1000),
               _access(0x1000, 2, rw="w", insn=0x1010)]
        self.assertNotIn("width", _cats(_findings([c], acc)))
        wide_store = [_access(0x1000, 4, insn=0x1000),
                      _access(0x1000, 4, rw="w", insn=0x1010)]
        self.assertIn("width", _cats(_findings([c], wide_store)))

    def test_a_double_moved_as_two_dwords_is_suppressed(self):
        c = _claim(0x1000, _prim("double", 8), extent=8)
        acc = [_access(0x1000, 4), _access(0x1004, 4, insn=0x1010)]
        self.assertNotIn("width", _cats(_findings([c], acc)))

    def test_a_string_op_width_is_not_a_field_width(self):
        c = _claim(0x1000, _prim("unsigned short", 2), extent=2)
        acc = [_access(0x1000, 4, mnemonic="stosd", rw="w",
                       text="rep stos DWORD PTR es:[edi],eax")]
        self.assertNotIn("width", _cats(_findings([c], acc)))

    def test_a_block_move_over_a_byte_ARRAY_is_suppressed_a_scalar_is_not(self):
        arr = _claim(0x1000, _arr(_prim("char", 1), 8, "char[8]"), extent=8)
        self.assertNotIn("width", _cats(_findings([arr], [_access(0x1000, 4)])))
        scalar = _claim(0x1000, _prim("char", 1), extent=1)
        self.assertIn("width", _cats(_findings([scalar], [_access(0x1000, 4)])))

    def test_an_x87_access_on_an_integer_field_is_caught(self):
        c = _claim(0x1000, _prim("int", 4), extent=4)
        acc = [_access(0x1000, 4, mnemonic="fld", fpu="f32")]
        self.assertIn("width", _cats(_findings([c], acc)))

    def test_an_index_into_a_single_element_array_is_an_undercount(self):
        one = _claim(0x1000, _arr(_prim("int", 4), 1, "int[1]"), extent=4)
        acc = [_access(0x1000, 4, form="indexed", scale=4,
                       text="mov eax,DWORD PTR [eax*4+0x401000]")]
        self.assertIn("undercount", _cats(_findings([one], acc)))
        many = _claim(0x1000, _arr(_prim("int", 4), 8, "int[8]"), extent=32)
        self.assertNotIn("undercount", _cats(_findings([many], acc)))

    def test_a_pair_array_declared_as_scalars_is_a_stride_finding(self):
        c = _claim(0x1000, _arr(_prim("int", 4), 32, "int[32]"), extent=128)
        acc = [_access(0x1000, 4, form="indexed", scale=8,
                       text="mov edx,DWORD PTR [edx*8+0x401000]")]
        self.assertIn("stride", _cats(_findings([c], acc)))

    def test_an_indexed_base_just_before_a_claim_is_not_unmodelled_data(self):
        """`[ecx + &g_buf - 4]` is g_buf's own 1-based spelling; the same
        address touched DIRECTLY is a real unclaimed byte."""
        c = _claim(0x1010, _arr(_prim("char", 1), 16, "char[16]"), extent=16)
        idx = [_access(0x100C, 1, form="indexed", rw="w", base_reg="ecx",
                       text="mov BYTE PTR [ecx+0x40100c],al")]
        self.assertNotIn("unclaimed", _cats(_findings([c], idx)))
        direct = [_access(0x100C, 1, rw="w", text="mov BYTE PTR ds:0x40100c,al")]
        self.assertIn("unclaimed", _cats(_findings([c], direct)))

    def test_a_source_claim_on_an_iat_slot_is_an_import(self):
        c = _claim(0x2C43C0, _prim("int", 4), name="?g_val_2c43c0@@3HA")
        acc = [_access(0x2C43C0, 4, form="iat", mnemonic="call",
                       text="call DWORD PTR ds:0x6c43c0")]
        self.assertIn("import-slot", _cats(_findings([c], acc)))
        lib = _claim(0x2C43C0, None, name="??_7CFoo@@6B@",
                     channel="data_vtables")
        self.assertNotIn("import-slot", _cats(_findings([lib], acc)))

    def test_a_claim_nothing_names_is_a_phantom_candidate(self):
        c = _claim(0x1000, _prim("int", 4))
        self.assertIn("unaccessed", _cats(_findings([c], [])))
        self.assertNotIn("unaccessed", _cats(_findings([c], [_access(0x1000)])))

    def test_one_access_crossing_two_claims_is_one_object(self):
        a = _claim(0x1000, _prim("int", 4), name="?g_a@@3HA")
        b = _claim(0x1004, _prim("int", 4), name="?g_b@@3HA")
        acc = [_access(0x1000, 8, mnemonic="fld", fpu="f64")]
        self.assertIn("adjacent", _cats(_findings([a, b], acc)))

    def test_the_gate_reports_gated_rows_and_honours_the_accept_list(self):
        """A report-only category must not break the build, a gated one must,
        and a documented exception must stay silent while its evidence holds
        (a STALE accept is itself reported)."""
        from gruntz.verify import data_access as da
        fired = [("shortfall", "high", 0x1000, "?g_x@@3HA", 0x1004, "d", "e"),
                 ("undercount", "high", 0x253C48, "?g_panTable@@3PAHA",
                  0x253C48, "d", "e"),
                 ("stride", "high", 0x2000, "?g_y@@3HA", 0x2000, "d", "e")]
        with mock.patch.object(da, "analysis",
                               return_value=(None, None, None, None, fired,
                                             None, None)):
            out = da.gate_findings()
            self.assertEqual(len(out), 1)
            self.assertIn("shortfall", out[0])
        with mock.patch.object(da, "analysis",
                               return_value=(None, None, None, None, [],
                                             None, None)):
            stale = da.gate_findings()
        self.assertTrue(any("STALE accept" in s for s in stale))

    def test_the_injected_defects_are_all_caught_on_the_real_tree(self):
        """The whole-tree harness: nine planted defects, each a class this
        campaign has shipped. A sieve returning 0 rows because it is BLIND is
        indistinguishable from a clean tree - this is what tells them apart."""
        from gruntz.core.paths import BUILD
        from gruntz.verify import data_access as da
        if not (BUILD / "gen/bindings.tsv").is_file():
            self.skipTest("no bindings (unbuilt tree)")
        missed = [f"{tag}->{want}" for tag, want, _label, caught
                  in da.run_selftest() if not caught]
        self.assertEqual(missed, [])


class DataCoverageControls(unittest.TestCase):
    def _row(self, **kw):
        row = {"rva": 0x1000, "length": 16, "section": ".data",
               "verdict": "NONZERO", "addressed": 1, "touched": 8, "sites": 2,
               "payload_nonzero": 8, "relocs": 0, "prev_object": "probe",
               "prev_name": "?g_a@@3HA", "next_object": "probe",
               "next_name": "?g_b@@3HA", "first_bytes": "01"}
        row.update(kw)
        return row

    def test_a_touched_nonzero_gap_inside_one_unit_fails(self):
        from gruntz.verify import data_coverage as dc
        self.assertEqual(len(dc.gate_rows([self._row()])), 1)

    def test_the_library_frontier_and_the_iat_do_not(self):
        from gruntz.verify import data_coverage as dc
        self.assertEqual(dc.gate_rows([self._row(prev_object="library_data",
                                                 next_object="library_data")]),
                         [])
        self.assertEqual(dc.gate_rows([self._row(prev_object="probe",
                                                 next_object="other")]), [])
        self.assertEqual(dc.gate_rows([self._row(section=".idata")]), [])
        self.assertEqual(dc.gate_rows([self._row(touched=0, sites=0)]), [])
        self.assertEqual(dc.gate_rows([self._row(verdict="ZERO-GAP",
                                                 payload_nonzero=0)]), [])

    def test_a_folded_comdat_is_not_an_overlap_but_two_extents_are(self):
        from gruntz.verify import data_coverage as dc
        folded = [{"rva": 0x1000, "size": 8, "name": "??_7C@@6B@",
                   "object": "a", "storage": "rdata"},
                  {"rva": 0x1000, "size": 8, "name": "??_7C@@6B@",
                   "object": "b", "storage": "rdata"}]
        self.assertEqual(dc.overlaps(folded), [])
        clash = folded + [{"rva": 0x1004, "size": 8, "name": "?g_x@@3HA",
                           "object": "c", "storage": "data"}]
        self.assertEqual(len(dc.overlaps(clash)), 1)


# --------------------------------------------------------------------------- #
# the DATA_COMPGEN negative-control set (real base objs)                      #
# --------------------------------------------------------------------------- #
class DataCompgenControls(unittest.TestCase):
    """The 16 controls from the extraction campaign: every malformed /
    unverifiable pin is REFUSED (never silently dropped, never admitted)."""

    def _cases(self):
        from gruntz.core.paths import BUILD
        from gruntz.model import resolve

        def owner_obj(rva, fallback):
            unit = next((b.unit for b in resolve().data
                         if b.rva == rva and b.unit), fallback)
            return BUILD / f"objdiff/base/{unit}.obj"

        # the owning TU is derived from the Model, not hardcoded - the pins
        # legitimately migrate when a fold re-homes their use sites
        obj = owner_obj(0x1E9A40, "grunt")
        str_obj = owner_obj(0x20D168, "fortconquered")
        return obj, str_obj, [
            ("positive f64", 'x = DATA_COMPGEN(0x001e9a40, 750.0);', obj, 1),
            ("positive str", 'x = DATA_COMPGEN(0x0020d168, "!");', str_obj, 1),
            ("wrong rva (right value, other slot)",
             'x = DATA_COMPGEN(0x001e9a48, 750.0);', obj, 0),
            ("value this TU never emitted",
             'x = DATA_COMPGEN(0x001e9a40, 751.0);', obj, 0),
            ("string this TU never emitted",
             'x = DATA_COMPGEN(0x0020d168, "nowhere");', str_obj, 0),
            ("string at the wrong rva",
             'x = DATA_COMPGEN(0x0020d16a, "!");', str_obj, 0),
            ("identifier value",
             'x = DATA_COMPGEN(0x001e9a40, g_thing);', obj, 0),
            ("integer value", 'x = DATA_COMPGEN(0x001e9a40, 750);', obj, 0),
            ("wide string", 'x = DATA_COMPGEN(0x0020d168, L"!");', str_obj, 0),
            ("non-canonical address",
             'x = DATA_COMPGEN(0x1e9a40, 750.0);', obj, 0),
            ("one argument", 'x = DATA_COMPGEN(0x001e9a40);', obj, 0),
            ("same rva, two values",
             'x = DATA_COMPGEN(0x001e9a40, 750.0) '
             '+ DATA_COMPGEN(0x001e9a40, 1.0);', obj, 1),
            ("f32 spelling of an f64 slot",
             'x = DATA_COMPGEN(0x001e9a40, 750.0f);', obj, 0),
            ("two pins on one line",
             'x = DATA_COMPGEN(0x001e9a40, 750.0) '
             '/ DATA_COMPGEN(0x001e9a58, 0.01);', obj, 2),
            ("wrapped over lines",
             'x = DATA_COMPGEN(0x001e9a58,\n        0.01);', obj, 1),
            ("commented out",
             '// x = DATA_COMPGEN(0x001e9a40, 750.0);', obj, 0),
        ]

    def test_the_sixteen_controls(self):
        from gruntz.retail_labels.source import blank_comments, compgen_claims
        obj, str_obj, cases = self._cases()
        if not obj.is_file() or not str_obj.is_file():
            self.skipTest("base objs absent (unbuilt tree) - the "
                          "DATA_COMPGEN controls need grunt.obj + "
                          "fortconquered.obj")
        bad = []
        for label, text, o, want in cases:
            claims, problems = compgen_claims(blank_comments(text), "ctrl", o)
            ok = len(claims) == want and (want
                                          or all("FATAL" in p
                                                 for p in problems))
            if not ok:
                bad.append(f"{label}: claims={len(claims)} want={want} "
                           f"problems={problems[:1]}")
        self.assertEqual(bad, [])


class LedgerPrecisionControls(unittest.TestCase):
    """Below-best must read the same on a live float and on its banked copy.

    The bank stores cur_pct to 4 decimals. While the regress test compared
    the raw float and the carried test the stored one, a row whose dip is
    exactly EPS was regressed AND never-carried at once - banking could not
    clear it and the build gate stayed red forever (seen live on
    StepDiggerBehavior, best 68.1947 vs cur 68.18469).
    """

    def test_raw_and_banked_readings_agree(self):
        from gruntz.verify.baseline import at_ledger_precision, below_best
        best = 68.1947
        for raw in (68.18469, 68.1847, 68.18471, 68.0, 68.1947, 70.0):
            self.assertEqual(
                below_best(raw, best),
                below_best(at_ledger_precision(raw), best),
                f"{raw} reads differently once banked - unclearable gate")

    def test_a_dip_the_ledger_cannot_see_is_not_a_regression(self):
        from gruntz.verify.baseline import below_best
        self.assertFalse(below_best(68.18469, 68.1947))   # rounds to the edge
        self.assertTrue(below_best(68.1840, 68.1947))     # genuinely below

    def test_a_real_dip_is_still_fresh_after_banking(self):
        from gruntz.verify.classify import currency
        base = {("u", "f"): {"best": 90.0, "cur": 90.0, "fp": "h", "tries": 1}}
        got = currency({("u", "f"): 80.0}, base, [("u", "f", 80.0, 90.0)])
        self.assertEqual((got["regress_fresh"], got["regress_carried"]), (1, 0))

    def test_a_banked_dip_is_carried(self):
        from gruntz.verify.classify import currency
        base = {("u", "f"): {"best": 90.0, "cur": 80.0, "fp": "h", "tries": 1}}
        got = currency({("u", "f"): 80.0}, base, [("u", "f", 80.0, 90.0)])
        self.assertEqual((got["regress_fresh"], got["regress_carried"]), (0, 1))


# --------------------------------------------------------------------------- #
# investigation surfaces (sema / walls / lsp / ghidra): a navigator that
# answers CONFIDENTLY and WRONGLY is worse than one that fails, so each of
# these is the control for a defect that shipped.
# --------------------------------------------------------------------------- #
class SemaReportSourceControls(unittest.TestCase):
    """sema read build/objdiff/report.json - the BANKED reference copy, not
    the report `gruntz compare` writes. `sema match 0x153810` answered 99.57%
    while walls/verify read 99.89% off the same build."""

    def test_sema_reads_the_same_reports_in_the_same_order_as_verify(self):
        from gruntz.sema.report import REPORTS as sema_reports
        from gruntz.verify.scores import REPORTS as verify_reports
        self.assertEqual(tuple(sema_reports), tuple(verify_reports))

    def test_the_current_report_wins_over_the_banked_one(self):
        from gruntz.sema import report as rep
        with tempfile.TemporaryDirectory() as td:
            new, old = Path(td) / "new.json", Path(td) / "old.json"
            old.write_text('{"units": [], "measures": {}}')
            with mock.patch.object(rep, "REPORTS", (new, old)):
                self.assertEqual(rep.report_path(), old)   # only the bank
                new.write_text('{"units": [], "measures": {}}')
                self.assertEqual(rep.report_path(), new)   # current wins


class SemaPipeControls(unittest.TestCase):
    """`gruntz sema vtable --list | head` exited 120 with 'Exception ignored
    on flushing sys.stdout' - the BrokenPipeError fires at interpreter exit,
    after run()'s handler could ever see it."""

    def test_a_closed_reader_is_a_clean_exit(self):
        import os

        from gruntz import sema
        r, w = os.pipe()
        os.close(r)                                  # the reader is gone
        saved = os.dup(1)
        try:
            os.dup2(w, 1)
            os.close(w)
            mod = mock.Mock()
            mod.main.side_effect = lambda argv: (print("x" * 200_000), 0)[1]
            with mock.patch("importlib.import_module", return_value=mod):
                rc = sema.run("gruntz.sema.rva", [])
        finally:
            os.dup2(saved, 1)
            os.close(saved)
        self.assertEqual(rc, 0)


class SemaMapControls(unittest.TestCase):
    """`sema map at <unmapped>` printed 'no admitted row covers this address'
    and exited 0, against sema's own rc convention (1 = answered-NO)."""

    def test_an_uncovered_address_is_answered_no(self):
        from gruntz.sema import map as smap
        idx = mock.Mock()
        idx.covering.return_value = None
        idx.preceding_func.return_value = None
        with mock.patch.object(smap, "index", return_value=idx), \
             mock.patch("gruntz.sema.image.retail") as retail:
            retail.return_value.section_of.return_value = None
            lines, rc = smap.at(0xDEADBEEF)
        self.assertEqual(rc, 1)
        self.assertIn("outside every section", "\n".join(lines))


class SemaGapControls(unittest.TestCase):
    """The same-file gap view must trim only edge padding and keep executable
    categories separate; otherwise the derived reconstruction queue can lose a
    tiny body or bury one under the repeated compiler/runtime bands."""

    def test_edge_padding_is_trimmed_without_eating_body_bytes(self):
        from gruntz.sema import gaps
        rva, body = gaps._trim(0x1000, b"\x90\xcc\xc3\x90")
        self.assertEqual((rva, body), (0x1002, b"\xc3"))

    def test_gap_kinds_preserve_tiny_and_substantive_bodies(self):
        from gruntz.sema import gaps
        with mock.patch.object(gaps, "_switch_table", return_value=False):
            self.assertEqual(gaps._kind(b"\xe9\x00\x00\x00\x00", None), "thunk")
            self.assertEqual(gaps._kind(b"\x33\xc0\xc3", None), "trivial")
            self.assertEqual(gaps._kind(b"\x55" * 9, None), "substantive")
            self.assertEqual(gaps._kind(b"\x55" * 0x100, None), "band")

    def test_aligned_padding_separates_multiple_missing_functions(self):
        from gruntz.sema import gaps
        payload = b"\xc3" + b"\x90" * 15 + b"\xcc" * 16 + b"\xc2\x04\x00\x90"
        self.assertEqual(
            gaps._split(0x17400, payload),
            [(0x17400, b"\xc3"), (0x17420, b"\xc2\x04\x00")],
        )


class WallsUnitFilterControls(unittest.TestCase):
    """A misspelt --unit answered '0 mismatches' / '0 function(s) below 100%'
    - a typo that reads as a clean sieve."""

    def test_an_unknown_unit_is_refused(self):
        from gruntz import walls
        with mock.patch("gruntz.manifest.units",
                        return_value=[{"unit": "cimage"}]):
            with self.assertRaises(SystemExit) as cm:
                walls.check_unit("cimag")
            self.assertEqual(cm.exception.code, 2)
            self.assertEqual(walls.check_unit("cimage"), "cimage")
            self.assertIsNone(walls.check_unit(None))


class WallsEmptyBuildControls(unittest.TestCase):
    """With no normalized pair on disk the sieves printed 'aggregate-copy
    mismatches: 0' / 'paired functions: 0' and exited 0 - an unbuilt tree
    reading as a clean sieve."""

    def test_no_pair_is_refused_not_answered(self):
        from gruntz.walls import pairscan
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(pairscan, "NORM", Path(td)):
                with self.assertRaises(SystemExit) as cm:
                    pairscan.require_pairs()
                self.assertEqual(cm.exception.code, 2)

    def test_a_real_pair_passes_through(self):
        from gruntz.walls import pairscan
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "base").mkdir()
            (root / "target").mkdir()
            (root / "base/cimage.obj").write_bytes(b"")
            (root / "target/cimage.c.obj").write_bytes(b"")
            with mock.patch.object(pairscan, "NORM", root):
                self.assertEqual(list(pairscan.require_pairs()), ["cimage"])


class EhFrameTsvControls(unittest.TestCase):
    """`--calibrate` returned before the `--tsv` writer, so asking for both
    silently produced no file."""

    ROW = dict(unit="u", name="?f@@YAXXZ", rva="0x001000", fuzzy=100.0,
               size=16, verdict="BOTH", cause="EXTRA_OBJECT", extra_ctors=[],
               our_ctors=[], resited=[], base_insn=4, tgt_insn=4,
               slot="[esp+0x4]", states=[0], base_states=1, tgt_states=1,
               first=0, last=4, unwind=True)

    def test_calibrate_still_writes_the_tsv(self):
        import contextlib
        import io

        from gruntz.walls import eh_frame
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "eh.tsv"
            with mock.patch.object(eh_frame, "scan", return_value=[self.ROW]), \
                 mock.patch.object(eh_frame.pairscan, "require_pairs"), \
                 contextlib.redirect_stdout(io.StringIO()):
                eh_frame.main(["--calibrate", "--tsv", str(out)])
            self.assertTrue(out.is_file(), "--tsv was swallowed by --calibrate")
            self.assertIn("verdict", out.read_text().splitlines()[0])


class WallsDiagnoseTargetControls(unittest.TestCase):
    """diagnose took only the rva or the exact mangled name, and answered
    every miss with the same 'no claimed function for X'."""

    def _model(self, *names):
        fns = [_binding(0x153810 + i * 0x10, n) for i, n in enumerate(names)]
        return mock.Mock(functions=fns)

    def test_a_readable_spelling_resolves(self):
        from gruntz.walls import diagnose as D
        name = "?RenderFrameClipped@CImage@@QAEXH@Z"
        with mock.patch("gruntz.model.resolve", return_value=self._model(name)):
            b, why = D._locate("CImage::RenderFrameClipped")
            self.assertIsNotNone(b, why)
            self.assertEqual(b.name, name)

    def test_a_miss_names_the_accepted_spellings(self):
        from gruntz.walls import diagnose as D
        with mock.patch("gruntz.model.resolve", return_value=self._model()):
            b, why = D._locate("NoSuchThing")
            self.assertIsNone(b)
            self.assertIn("CClass::Member", why)
            b, why = D._locate("0xdeadbeef")
            self.assertIsNone(b)
            self.assertIn("sema rva", why)

    def test_a_relocation_free_call_to_function_start_is_self_recursion(self):
        from gruntz.walls import diagnose as D
        asm = "   2ba:\te8 00 00 00 00\tcall 0x0\n"
        self.assertEqual(D._call_targets({}, asm, "?f@@YAXXZ"), [("?f@@YAXXZ", 0)])

    def test_a_relocated_call_is_not_counted_twice_as_self_recursion(self):
        from gruntz.walls import diagnose as D
        asm = "   2ba:\te8 00 00 00 00\tcall 0x0\n"
        rel = {0x2bb: ("?f@@YAXXZ", 0)}
        self.assertEqual(D._call_targets(rel, asm, "?f@@YAXXZ"), [("?f@@YAXXZ", 0)])

    def test_diagnose_does_not_route_relocation_free_self_recursion_to_inline(self):
        import contextlib
        import io
        from types import SimpleNamespace

        from gruntz.walls import diagnose as D
        name = "?ParseRecords@CSymParser@@QAEHXZ"
        binding = SimpleNamespace(unit="u", name=name, rva=0x13B300)
        asm = "   0:\te8 fb ff ff ff\tcall 0x0\n"
        with tempfile.TemporaryDirectory() as td:
            norm = Path(td)
            (norm / "base").mkdir()
            (norm / "target").mkdir()
            (norm / "base/u.obj").touch()
            (norm / "target/u.c.obj").touch()
            found = [
                (b"base", {1: (name, 0)}, 4),
                (b"target", {}, 4),
            ]
            skeletons = [
                (b"base", 1, 0, 0, 1, asm),
                (b"target", 1, 0, 0, 1, asm),
            ]
            with mock.patch.object(D, "NORM", norm), \
                 mock.patch.object(D, "_locate", return_value=(binding, "")), \
                 mock.patch.object(D, "Obj", side_effect=lambda path: path), \
                 mock.patch.object(D, "_find_function", side_effect=found), \
                 mock.patch.object(D, "_jump_table_bytes", return_value=set()), \
                 mock.patch.object(D, "_skeleton", side_effect=skeletons), \
                 contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(D.diagnose("0x13b300"), 0)
        self.assertIn("REGALLOC/SCHEDULING", out.getvalue())
        self.assertNotIn("INLINE/CALL-SET", out.getvalue())


class InlineModelFlagControls(unittest.TestCase):
    """--spec was documented, parsed, and then fell through to
    `error: need --selftest, --spec/--gap FILE, or --measure-cb TU`."""

    def test_spec_predicts_instead_of_erroring(self):
        import contextlib
        import io

        from gruntz.walls import inline_model
        with tempfile.TemporaryDirectory() as td:
            spec = Path(td) / "s.json"
            spec.write_text('{"caller_cb": 120, "sites": ['
                            + ",".join(['{"name": "fill", "cb": 150}'] * 9)
                            + "]}")
            with contextlib.redirect_stdout(io.StringIO()) as out:
                rc = inline_model.main(["--spec", str(spec)])
        text = out.getvalue()
        self.assertEqual(rc, 0)
        self.assertEqual(text.count("EXPAND fill"), 6)     # the oracle shape
        self.assertEqual(text.count("call   fill"), 3)

    def test_a_malformed_spec_is_explained(self):
        from gruntz.walls import inline_model
        with tempfile.TemporaryDirectory() as td:
            bad = Path(td) / "b.json"
            bad.write_text("{}")
            with self.assertRaises(SystemExit):
                inline_model.main(["--spec", str(bad)])
            with self.assertRaises(SystemExit):
                inline_model.main(["--gap", str(Path(td) / "absent.json")])


class ExeMapWriteControls(unittest.TestCase):
    """`python3 -m gruntz.sema.exe_map --help` ignored the flag and rewrote
    docs/exe-map/ - a help request with a side effect on the tracked tree."""

    def test_help_does_not_write(self):
        import contextlib
        import io

        from gruntz.sema import exe_map
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(SystemExit) as cm:
                exe_map.main(["--help"])
        self.assertEqual(cm.exception.code, 0)

    def test_check_writes_nothing(self):
        import contextlib
        import io

        from gruntz.sema import exe_map
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(exe_map, "core_rows",
                                   return_value=[{"unit": "u", "name": "n",
                                                  "rva": 0x1000, "size": 4}]), \
                 mock.patch.object(exe_map, "unit_sources", return_value={}), \
                 mock.patch.object(exe_map, "OUT_DIR", Path(td) / "out"), \
                 contextlib.redirect_stdout(io.StringIO()):
                exe_map.main(["--check"])
            self.assertFalse((Path(td) / "out").exists())


class ToolDriverMessageControls(unittest.TestCase):
    """A missing file / unparsable address reached the user as a traceback."""

    def test_objdump_refuses_a_missing_blob_and_a_bad_vma(self):
        from gruntz.tool import objdump
        with tempfile.TemporaryDirectory() as td:
            blob = Path(td) / "b.bin"
            blob.write_bytes(b"\x90\xc3")
            self.assertEqual(objdump.main([str(Path(td) / "absent.bin")]), 2)
            self.assertEqual(objdump.main([str(blob), "--vma", "zz"]), 2)

    def test_ghidra_verify_refuses_a_non_address(self):
        from gruntz.ghidra import project
        self.assertEqual(project.main(["verify", "zzz"]), 2)

    def test_lsp_point_names_the_missing_file(self):
        from gruntz.lsp.query import parse_point
        with self.assertRaises(SystemExit) as cm:
            parse_point("include/NoSuchHeader.h:12")
        self.assertIn("no such file", str(cm.exception))
        self.assertIsNone(parse_point("CGrunt::GetAI"))   # still a symbol


# --------------------------------------------------------------------------- #
# the runner and the CLI surface (2026-08-16 review)                          #
# --------------------------------------------------------------------------- #
class TierRunnerExitControls(unittest.TestCase):
    """SystemExit is a BaseException: a gate reporting a missing input by
    raising it used to abort the whole tier, silently skipping every gate
    after it - the run looked short, not failed."""

    def _run(self, gates):
        import contextlib
        import io

        from gruntz.verify import tiers
        with mock.patch.dict(tiers.TIERS, {"fast": gates}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                failed = tiers.run(["fast"])
        return failed, out.getvalue()

    def test_a_gate_that_raises_systemexit_is_a_failure_not_an_abort(self):
        def bail():
            raise SystemExit("no report.json - run `gruntz compare` first")
        seen = []
        failed, text = self._run([("probe", bail),
                                  ("after", lambda: seen.append(1) or [])])
        self.assertEqual(failed, 1)
        self.assertIn("could not run", text)
        self.assertIn("no report.json", text)
        self.assertEqual(seen, [1])          # the NEXT gate still ran

    def test_the_rerun_hint_names_a_command_that_exists(self):
        from gruntz.verify import _ALIASES, _GATES, tiers
        for tier, rows in tiers.TIERS.items():
            for name, _fn in rows:
                hint = tiers._rerun_command(name)
                gate = hint.rsplit(" ", 1)[-1]
                self.assertIn(gate, _GATES,
                              f"[{tier}] {name}: hint {hint!r} names no gate")
        # the row this got wrong: the `vtable-bans` label runs verify.BANS, so
        # the mechanical name.replace('-','_') named a module that does not
        # exist (`python3 -m gruntz.verify.vtable_bans`).
        self.assertEqual(_ALIASES["vtable-bans"], "bans")
        self.assertEqual(tiers._rerun_command("vtable-bans"),
                         "gruntz verify bans")

    def test_every_tier_label_is_runnable_as_a_verb(self):
        """A tier label nobody can type is a dead end at the exact moment the
        gate fails."""
        from gruntz.verify import _ALIASES, _GATES, _QUERY_ONLY, tiers
        for tier, rows in tiers.TIERS.items():
            for name, _fn in rows:
                verb = _ALIASES.get(name, name)
                self.assertIn(verb, _GATES,
                              f"[{tier}] {name} runs in a tier but "
                              f"`gruntz verify {name}` reaches nothing")
        # and the help listing's own claim: every non-oracle gate IS tier-run
        labels = {n for rows in tiers.TIERS.values() for n, _f in rows}
        in_a_tier = {_ALIASES.get(n, n) for n in labels}
        for gate in _GATES:
            if gate in _QUERY_ONLY:
                continue
            self.assertIn(gate, in_a_tier,
                          f"{gate} is advertised as tier-run but no tier "
                          f"lists it (add it to a tier, or declare it in "
                          f"_QUERY_ONLY)")

    def test_the_alias_verb_actually_dispatches(self):
        import contextlib
        import io

        from gruntz import verify
        with mock.patch("gruntz.verify.bans.main", return_value=0) as m:
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(verify.main(["vtable-bans"]), 0)
        m.assert_called_once()


class ReportInputControls(unittest.TestCase):
    """A bad --report is an operator error, not a traceback."""

    def test_missing_malformed_and_foreign_json_all_say_what_to_do(self):
        from gruntz.verify import scores
        with tempfile.TemporaryDirectory() as td:
            gone = Path(td) / "gone.json"
            with self.assertRaises(SystemExit) as e:
                scores.load(gone)
            self.assertIn("gone.json", str(e.exception))

            trunc = Path(td) / "trunc.json"
            trunc.write_text('{"units": [')
            with self.assertRaises(SystemExit) as e:
                scores.load(trunc)
            self.assertIn("not valid JSON", str(e.exception))
            self.assertIn("gruntz compare", str(e.exception))

            foreign = Path(td) / "other.json"
            foreign.write_text('{"hello": 1}')
            with self.assertRaises(SystemExit) as e:
                scores.load(foreign)
            self.assertIn("not an objdiff report", str(e.exception))

    def test_an_unknown_tier_is_rejected_before_any_work(self):
        """`check --tier bogus` used to run the whole MAX gate first, so the
        typo surfaced behind a wall of regression output."""
        from gruntz.verify import verbs
        with mock.patch.object(verbs, "_report",
                               side_effect=AssertionError("ran the gate")):
            with self.assertRaises(SystemExit) as e:
                verbs.cmd_check(["--tier", "bogus"])
        self.assertIn("unknown tier", str(e.exception))

    def test_a_real_report_still_loads(self):
        from gruntz.verify import scores
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "r.json"
            p.write_text('{"units": [], "measures": {}}')
            self.assertEqual(scores.load(p)["units"], [])


class GateCliSurfaceControls(unittest.TestCase):
    """Every `gruntz verify <gate>` answers --help and REJECTS a typo; none
    of them treats an unknown flag as 'run anyway'."""

    def test_every_gate_module_parses_its_argv(self):
        import contextlib
        import importlib
        import io

        from gruntz.verify import _GATES
        for gate, module in sorted(_GATES.items()):
            mod = importlib.import_module(module)
            with contextlib.redirect_stdout(io.StringIO()) as out:
                with self.assertRaises(SystemExit) as e:
                    mod.main(["--help"])
            self.assertEqual(e.exception.code, 0, f"{gate} --help")
            # RENDERING it is the check: argparse %-expands help strings, so
            # an unescaped `%` in one (`100%-clean`) raises inside --help.
            self.assertIn("usage:", out.getvalue(), f"{gate} --help")
            self.assertIn("options:", out.getvalue(), f"{gate} --help")
            with contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as e:
                    mod.main(["--definitely-not-a-flag"])
            self.assertEqual(e.exception.code, 2, f"{gate} unknown flag")

    def test_no_gate_flag_is_a_silent_no_op(self):
        """A flag argparse accepts and nothing ever READS is worse than no
        flag: it looks like it worked. (`verify vtables --list` was one.)"""
        import ast
        import importlib
        import re

        from gruntz.verify import _GATES

        def unread(src):
            """dests argparse defines that no `<ns>.<dest>` ever reads."""
            tree = ast.parse(src)
            dests, namespaces = [], set()
            for node in ast.walk(tree):
                if isinstance(node, ast.Call) \
                        and isinstance(node.func, ast.Attribute) \
                        and node.func.attr == "add_argument":
                    opts = [a.value for a in node.args
                            if isinstance(a, ast.Constant)]
                    dest = next((kw.value.value for kw in node.keywords
                                 if kw.arg == "dest"
                                 and isinstance(kw.value, ast.Constant)), None)
                    if dest is None and opts:
                        longs = [o for o in opts if o.startswith("--")] or opts
                        dest = longs[0].lstrip("-").replace("-", "_")
                    if dest and dest != "help":
                        dests.append(dest)
                if isinstance(node, ast.Assign) \
                        and isinstance(node.value, ast.Call) \
                        and isinstance(node.value.func, ast.Attribute) \
                        and node.value.func.attr == "parse_args":
                    namespaces |= {t.id for t in node.targets
                                   if isinstance(t, ast.Name)}
            # `args` covers the do_*(args) handlers data_access hands off to
            names = namespaces | {"args"}
            return dests, [d for d in dests
                           if not any(re.search(rf"\b{n}\.{d}\b", src)
                                      for n in names)]

        # the detector must actually SEE flags, or an empty `dead` proves
        # nothing (this scan found `vtables --list` before it was removed)
        probe = ('import argparse\nap = argparse.ArgumentParser()\n'
                 'ap.add_argument("--used", action="store_true")\n'
                 'ap.add_argument("--dead", action="store_true")\n'
                 'a = ap.parse_args()\nprint(a.used)\n')
        self.assertEqual(unread(probe), (["used", "dead"], ["dead"]))

        seen, dead = 0, []
        for gate, module in sorted(_GATES.items()):
            src = Path(importlib.import_module(module).__file__).read_text()
            dests, bad = unread(src)
            seen += len(dests)
            dead += [f"{gate} --{d.replace('_', '-')}" for d in bad]
        self.assertGreater(seen, 30, "the flag scan found almost nothing")
        self.assertEqual(dead, [])

    def test_every_gate_flag_documents_itself(self):
        """`--help` listing a bare flag name tells the reader nothing; the
        one place a flag's meaning is guaranteed to be found is `help=`."""
        import ast
        import importlib

        from gruntz.verify import _GATES
        bare = []
        for gate, module in sorted(_GATES.items()):
            src = Path(importlib.import_module(module).__file__).read_text()
            for node in ast.walk(ast.parse(src)):
                if isinstance(node, ast.Call) \
                        and isinstance(node.func, ast.Attribute) \
                        and node.func.attr == "add_argument" \
                        and not any(kw.arg == "help" for kw in node.keywords):
                    opts = [a.value for a in node.args
                            if isinstance(a, ast.Constant)]
                    bare.append(f"{gate} {'/'.join(opts)}")
        self.assertEqual(bare, [])

    def test_model_help_does_not_run_the_join(self):
        """`gruntz model --help` used to resolve the whole Model and REWRITE
        build/gen/bindings.tsv as a side effect of asking for help."""
        import contextlib
        import io

        import gruntz.model as model
        with mock.patch.object(model, "resolve") as res, \
             mock.patch.object(model, "serialize") as ser:
            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(SystemExit) as e:
                    model.main(["--help"])
        self.assertEqual(e.exception.code, 0)
        res.assert_not_called()
        ser.assert_not_called()


# --------------------------------------------------------------------------- #
# never vacuous: a gate that measured NOTHING may not report OK               #
# --------------------------------------------------------------------------- #
class VacuityControls(unittest.TestCase):
    def test_tu_order_refuses_an_empty_scan(self):
        from gruntz.verify import tu_order as to
        with mock.patch.object(to, "load_in_file_order", return_value={}), \
             mock.patch.object(to, "load_exiles", return_value={}), \
             mock.patch.object(to, "load_emitted_claims", return_value={}), \
             mock.patch.object(to, "_load_baseline", return_value=({}, 0)):
            findings, _s = to.gate_findings()
        self.assertTrue(any("vacuous" in f for f in findings))

    def test_data_tu_order_refuses_zero_defs(self):
        from gruntz.verify import data_tu_order as dto
        with mock.patch.object(dto, "crossings", return_value=(set(), 0, [])), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertTrue(any("vacuous" in f for f in dto.gate_findings()))
        with mock.patch.object(dto, "crossings", return_value=(set(), 42, [])), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertEqual(dto.gate_findings(), [])

    def test_caller_callee_refuses_an_empty_call_graph(self):
        from gruntz.verify import caller_callee as cc
        rc = mock.Mock()
        rc.tgt = set()
        with mock.patch.object(cc, "_summary", return_value=(rc, [], {})):
            self.assertTrue(any("vacuous" in f for f in cc.gate_findings()))

    def test_caller_callee_follows_inline_forwarding_members(self):
        from gruntz.verify.caller_callee import _resolve_source_calls
        wrapper = "?ApplyName@CWapX@@QAEXPBD@Z"
        real = "?ApplyName@CWwdGameObjectA@@QAEXPBD@Z"
        rvas, leaves = _resolve_source_calls(
            wrapper, {wrapper: {real}}, {real: 0x150540})
        self.assertEqual(rvas, {0x150540})
        self.assertEqual(leaves, set())

    def test_caller_callee_keeps_unresolved_forwarding_leaves(self):
        from gruntz.verify.caller_callee import _resolve_source_calls
        wrapper = "?Run@CRealInline@@QAEXXZ"
        phantom = "?Run@CPhantomView@@QAEXXZ"
        rvas, leaves = _resolve_source_calls(
            wrapper, {wrapper: {phantom}}, {})
        self.assertEqual(rvas, set())
        self.assertEqual(leaves, {phantom})

    def test_caller_callee_consumer_reconciles_an_inline_forwarder(self):
        from gruntz.verify import caller_callee as cc
        caller = "?Ctor@COwner@@QAEXXZ"
        wrapper = "?ApplyName@CWapX@@QAEXPBD@Z"
        real = "?ApplyName@CWwdGameObjectA@@QAEXPBD@Z"
        ir = {caller: {wrapper}, wrapper: {real}, real: set()}
        rc = cc.Recon.__new__(cc.Recon)
        rc.m2rva = {caller: 0x1000, real: 0x150540}
        with mock.patch("gruntz.tool.clang.compdb",
                        return_value={"/probe/src/Probe.cpp": []}), \
             mock.patch.object(cc, "_tu_edges",
                               return_value=("/probe/src/Probe.cpp", ir)):
            edges, defined, unresolved, failed = rc._base_graph(jobs=1)
        self.assertEqual(edges, {(0x1000, 0x150540)})
        self.assertEqual(defined, {0x1000, 0x150540})
        self.assertEqual(unresolved, {})
        self.assertEqual(failed, [])

    def test_assert_relocs_refuses_zero_audited_functions(self):
        from gruntz.verify import assert_relocs as ar
        with mock.patch.object(ar, "audit", return_value=([], 0)):
            self.assertTrue(any("NOTHING was audited" in f
                                for f in ar.gate_findings()))
        with mock.patch.object(ar, "audit", return_value=([], 1200)):
            self.assertEqual(ar.gate_findings(), [])

    def test_data_relocs_refuses_zero_scanned_pairs(self):
        from collections import Counter

        from gruntz.verify import data_relocs as dr
        with mock.patch.object(dr, "scan",
                               return_value=([], [], [], Counter(), {}, [])), \
             mock.patch.object(dr, "units_without_a_target", return_value=[]), \
             mock.patch.object(dr, "orphan_payloads", return_value=[]):
            self.assertTrue(any("0 base/target pairs" in f
                                for f in dr.gate_findings()))

    def test_alloc_size_refuses_an_empty_layout_harvest(self):
        from gruntz.verify import alloc_size as az
        sw = mock.Mock()
        sw.rows.return_value = []
        with mock.patch.object(az, "Sweep", return_value=sw), \
             mock.patch.object(az, "computed_sizes", return_value=({}, set())), \
             mock.patch.object(az, "def_counts", return_value={}), \
             mock.patch.object(az, "classify_rows",
                               return_value=([], [], [], [], [], [])):
            self.assertTrue(any("0 class sizes" in f
                                for f in az.gate_findings()))
        with mock.patch.object(az, "Sweep", return_value=sw), \
             mock.patch.object(az, "computed_sizes",
                               return_value=({"CFoo": 8}, set())), \
             mock.patch.object(az, "def_counts", return_value={}), \
             mock.patch.object(az, "classify_rows",
                               return_value=([], [], [], [], [], [])):
            self.assertEqual(az.gate_findings(), [])

    def test_library_overlap_does_not_call_its_vacuity_guard_a_double_claim(self):
        import contextlib
        import io

        from gruntz.verify import library_overlap as lo
        with mock.patch.object(lo, "findings",
                               return_value=(["parsed 0 src claims"], 0)):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                self.assertEqual(lo.main([]), 1)
        self.assertIn("0 src claims", err.getvalue())
        self.assertNotIn("double-claim(s). Each", err.getvalue())


class LibSymbolCacheControls(unittest.TestCase):
    """The toolchain .LIB symbol cache is keyed on the ARCHIVE SET.

    Measured 2026-08-16: an unkeyed cache written under toolchain r2 was
    still answering under r3 (46,866 live symbols vs 56,474 cached), so
    `verify link-tier` called 42 resolvable Win32 imports "a guaranteed
    unresolved external" while the candidate link reported ZERO unresolved.
    """

    def _archive(self, path: Path, names: list[str]) -> None:
        import struct as st
        body = st.pack(">I", len(names)) + b"\0" * (4 * len(names)) \
            + b"".join(n.encode() + b"\0" for n in names)
        head = b"/" + b" " * 15 + b"0" * 12 + b" " * 20 \
            + f"{len(body):<10d}".encode() + b"`\n"
        path.write_bytes(b"!<arch>\n" + head + body)

    def test_a_toolchain_change_invalidates_the_cache(self):
        from gruntz.verify import undefined_closure as uc
        with tempfile.TemporaryDirectory() as td:
            r2, r3 = Path(td) / "r2/lib", Path(td) / "r3/lib"
            r2.mkdir(parents=True)
            r3.mkdir(parents=True)
            self._archive(r2 / "OLD.LIB", ["_OnlyInR2@4"])
            self._archive(r3 / "NEW.LIB", ["_OnlyInR3@4"])
            cache = Path(td) / "lib_symbols.txt"
            with mock.patch.object(uc, "LIB_CACHE", cache):
                with mock.patch.dict("os.environ",
                                     {"MSVC_DIR": str(r2.parent)},
                                     clear=False):
                    os.environ.pop("DXSDK_DIR", None)
                    first = uc.lib_symbols()
                self.assertIn("_OnlyInR2@4", first)
                with mock.patch.dict("os.environ",
                                     {"MSVC_DIR": str(r3.parent)},
                                     clear=False):
                    os.environ.pop("DXSDK_DIR", None)
                    second = uc.lib_symbols()
        self.assertIn("_OnlyInR3@4", second)
        self.assertNotIn("_OnlyInR2@4", second)   # the r2 answer is retired

    def test_an_unreachable_toolchain_keeps_the_last_answer(self):
        """Outside the dev shell $MSVC_DIR is unset; returning an empty set
        would make every consumer call the whole CRT unresolvable."""
        from gruntz.verify import undefined_closure as uc
        with tempfile.TemporaryDirectory() as td:
            cache = Path(td) / "lib_symbols.txt"
            cache.write_text("# libs deadbeef\n_CloseHandle@4\n")
            with mock.patch.object(uc, "LIB_CACHE", cache), \
                 mock.patch.object(uc, "_toolchain_libs", return_value=[]):
                self.assertEqual(uc.lib_symbols(), {"_CloseHandle@4"})


class LinkTierRunnabilityControls(unittest.TestCase):
    def test_a_missing_map_is_a_finding_not_a_silent_skip(self):
        """With no .map the image diff cannot run at all - and main()'s
        success line claims every exact body is byte-identical in the linked
        image, which would then be a claim about a check that never ran."""
        from gruntz.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "c.exe"
            cand.write_bytes(b"MZ")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "CMAP", Path(td) / "absent.map"):
                out = lt.image_diff_findings()
        self.assertTrue(out and "could not run" in out[0])

    def test_a_missing_retail_section_is_fatal_and_reloc_is_exempt(self):
        """The third link-tier check had no control at all."""
        from gruntz.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "c.exe"
            cand.write_bytes(b"MZ")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "census",
                                   return_value=[(".text", 0x1000, 0x1000),
                                                 (".rsrc", 0x2000, 0),
                                                 (".reloc", 0x400, 0)]):
                bad = lt.census_findings()
        self.assertEqual(len(bad), 1)
        self.assertIn(".rsrc", bad[0])       # .reloc is deliberately exempt

    def test_no_candidate_at_all_stays_one_finding(self):
        from gruntz.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(lt, "CAND", Path(td) / "none.exe"), \
                 mock.patch.object(lt, "CMAP", Path(td) / "none.map"):
                self.assertEqual(lt.image_diff_findings(), [])
                self.assertEqual(lt.census_findings(), [])


# --------------------------------------------------------------------------- #
# function source fingerprint name bridge                                     #
# --------------------------------------------------------------------------- #
class FingerprintNameControls(unittest.TestCase):
    def test_function_pointer_return_keeps_the_declared_method_name(self):
        from gruntz.verify.fingerprints import _qualified_of
        demangled = ("public: void (__cdecl * __thiscall "
                     "CVariantSlot::Add(class zErrHandling *, "
                     "void (__cdecl *)(char *, int)))(char *, int)")
        self.assertEqual(_qualified_of(demangled), "CVariantSlot::Add")

    def test_ordinary_method_name_is_unchanged(self):
        from gruntz.verify.fingerprints import _qualified_of
        self.assertEqual(
            _qualified_of("public: int __thiscall CFileIO::Open(char const *)"),
            "CFileIO::Open")


# --------------------------------------------------------------------------- #
# the MAX ledger: the bank rules themselves (the file is project state)       #
# --------------------------------------------------------------------------- #
class BankRatchetControls(unittest.TestCase):
    """bank_rows is what edits config/match_baseline.tsv. Every rule that
    protects a banked MAX gets a control here; nothing writes the ledger."""

    def _bank(self, cur, base, fps=None, rvas=None, library=()):
        from gruntz.verify import verbs
        fps = fps or {}
        with mock.patch.object(verbs, "library_rvas", return_value=set(library)):
            return verbs.bank_rows(cur, base,
                                   lambda u, f: fps.get((u, f), "h1"),
                                   rvas or {})

    def _row(self, best=90.0, cur=90.0, fp="h1", addr=0x1000, hist=None,
             state="", tries=1):
        return {"best": best, "cur": cur, "tries": tries, "fp": fp,
                "addr": addr, "hist": best if hist is None else hist,
                "state": state}

    def test_a_dip_never_lowers_a_best_while_the_source_is_unchanged(self):
        key = ("u", "f")
        new, stats, reset, _drop = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, cur=90.0)},
            rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 90.0)     # MAX held
        self.assertEqual(new[key]["cur"], 70.0)
        self.assertEqual(new[key]["hist"], 90.0)
        self.assertEqual(reset, [])

    def test_a_real_source_edit_resets_best_but_never_hist(self):
        key = ("u", "f")
        new, _s, reset, _d = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, fp="old")},
            fps={key: "new"}, rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 70.0)
        self.assertEqual(new[key]["hist"], 90.0)     # the all-time peak holds
        self.assertEqual(len(reset), 1)

    def test_a_fallback_fingerprint_is_not_an_edit(self):
        from gruntz.verify.fingerprints import FALLBACK
        key = ("u", "f")
        new, _s, reset, _d = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, fp="real")},
            fps={key: FALLBACK + "abc"}, rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 90.0)
        self.assertEqual(new[key]["fp"], "real")     # the real hash is kept
        self.assertEqual(reset, [])

    def test_the_rva_moving_is_the_only_rva_keyed_reset(self):
        key = ("u", "f")
        new, stats, _r, _d = self._bank(
            {key: 55.0}, {key: self._row(best=90.0, addr=0x1000)},
            rvas={key: 0x2000})
        self.assertEqual(stats["rebounds"], 1)
        self.assertEqual(new[key]["best"], 55.0)     # a different BODY
        self.assertEqual(new[key]["hist"], 90.0)

    def test_a_unit_move_migrates_the_high_water_by_rva(self):
        old, new_key = ("olda", "f"), ("newb", "f")
        new, stats, _r, _d = self._bank(
            {new_key: 80.0}, {old: self._row(best=95.0, addr=0x1000)},
            rvas={new_key: 0x1000})
        self.assertEqual(stats["moved"], 1)
        self.assertEqual(new[new_key]["best"], 95.0)
        self.assertNotIn(old, new)

    def test_an_unscored_body_is_preserved_absent_and_round_trips(self):
        from gruntz.verify import baseline as bl
        key = ("u", "f")
        new, stats, _r, dropped = self._bank(
            {}, {key: self._row(best=100.0, addr=0x1000)}, rvas={})
        self.assertEqual(stats["preserved_absent"], 1)
        self.assertEqual(new[key]["state"], "absent")
        self.assertEqual(new[key]["best"], 100.0)
        self.assertEqual(dropped, [])
        self.assertEqual(bl.load(bl.render(new)), new)     # survives the file

    def test_an_absent_row_is_dropped_once_its_rva_is_claimed_elsewhere(self):
        key, other = ("u", "f"), ("u", "g")
        new, stats, _r, dropped = self._bank(
            {other: 100.0}, {key: self._row(best=100.0, addr=0x1000)},
            rvas={other: 0x1000})
        # 0x1000 is now claimed under another name: keeping the row would pin
        # a phantom, and the high-water travelled with the body (moved).
        self.assertEqual(stats["moved"] + len(dropped), 1)
        self.assertNotIn("absent", {r.get("state") for r in new.values()})

    def test_banking_twice_changes_nothing(self):
        key = ("u", "f")
        base = {key: self._row(best=90.0, cur=90.0)}
        first, _s, _r, _d = self._bank({key: 95.0}, base, rvas={key: 0x1000})
        second, stats, _r, _d = self._bank({key: 95.0}, first,
                                           rvas={key: 0x1000})
        self.assertEqual(first, second)
        self.assertEqual(stats["raised"], 0)

    def test_the_ledger_round_trips_through_render_and_load(self):
        from gruntz.verify import baseline as bl
        rows = {("u", "f"): self._row(best=99.1234, cur=98.7654, hist=100.0),
                ("u", "g"): self._row(best=100.0, cur=100.0, addr=None,
                                      state="absent")}
        self.assertEqual(bl.load(bl.render(rows)), rows)


class BankPreconditionControls(unittest.TestCase):
    def test_an_unstaged_build_input_refuses_and_names_the_paths(self):
        from gruntz.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs",
                               return_value=["src/Gruntz/Grunt.cpp"]):
            with self.assertRaises(SystemExit) as e:
                verbs.require_bankable_tree("write the baseline")
        msg = str(e.exception)
        self.assertIn("src/Gruntz/Grunt.cpp", msg)
        self.assertIn("--dirty", msg)

    def test_dirty_warns_loudly_and_proceeds(self):
        import contextlib
        import io

        from gruntz.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs",
                               return_value=["include/Gruntz/Grunt.h"]):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                verbs.require_bankable_tree("write the baseline",
                                            allow_dirty=True)
        self.assertIn("WARNING", err.getvalue())
        self.assertIn("include/Gruntz/Grunt.h", err.getvalue())

    def test_a_clean_tree_is_silent(self):
        from gruntz.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs", return_value=[]):
            verbs.require_bankable_tree("write the baseline")

    def test_a_stale_report_is_called_out(self):
        import contextlib
        import io

        from gruntz.verify import verbs
        with tempfile.TemporaryDirectory() as td:
            report = Path(td) / "report.json"
            report.write_text("{}")
            objs = Path(td) / "build/objdiff/base"
            objs.mkdir(parents=True)
            obj = objs / "a.obj"
            obj.write_bytes(b"x")
            os.utime(obj, (report.stat().st_mtime + 600,) * 2)
            with mock.patch.object(verbs, "REPO", Path(td)):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    verbs._warn_stale_report(report)
        self.assertIn("STALE", err.getvalue())
        self.assertIn("gruntz build", err.getvalue())


# --------------------------------------------------------------------------- #
# the CONSUMER, not the recognizer: what the tier actually calls              #
# --------------------------------------------------------------------------- #
class GateConsumerControls(unittest.TestCase):
    """A recognizer control proves a helper; the tier calls the GATE. These
    drive the exact entry point tiers.TIERS holds."""

    def test_include_order_the_tier_calls_audit_not_parse(self):
        from gruntz.verify import include_order as io
        from gruntz.verify import tiers

        def with_file(text):
            with tempfile.TemporaryDirectory() as td:
                root = Path(td)
                (root / "src").mkdir()
                p = root / "src/Probe.cpp"
                p.write_text(text)
                with mock.patch.object(io, "repo_files", return_value=[p]), \
                     mock.patch.object(io, "REPO", root):
                    return tiers._include_order()
        dirty = with_file("#include <rva.h>\n#include <Zed.h>\n"
                          "#include <Abc.h>\n#include <Zed.h>\n\nint x;\n")
        self.assertTrue(any("duplicate include" in f for f in dirty))
        clean = with_file("#include <rva.h>\n\n#include <Abc.h>\n"
                          "#include <Zed.h>\n\nint x;\n")
        self.assertEqual(clean, [])

    def test_caller_callee_fires_above_the_floor_and_is_silent_at_it(self):
        from gruntz.verify import caller_callee as cc
        rc = mock.Mock()
        rc.tgt = {(1, 2)}
        rc.name = lambda r: "?F@@YAXXZ"
        rc.unit = lambda r: "u"
        miss = [(0x1000 + i, 0x2000, "FAKE-VIEW", "CView") for i in range(5)]
        summary = (rc, miss, {"FAKE-VIEW": 5})
        with mock.patch.object(cc, "_summary", return_value=summary), \
             mock.patch("gruntz.verify.board.load_baseline",
                        return_value={"caller-callee FAKE-VIEW": 4}):
            over = cc.gate_findings()
        self.assertTrue(over and "exceeds the committed floor 4" in over[0])
        with mock.patch.object(cc, "_summary", return_value=summary), \
             mock.patch("gruntz.verify.board.load_baseline",
                        return_value={"caller-callee FAKE-VIEW": 5}):
            self.assertEqual(cc.gate_findings(), [])

    def test_vtables_gate_renders_every_defect_class(self):
        from gruntz.verify import vtables as vt
        wiring = [("WIRING", 0x100, "CFader", 3, 0x2000,
                   "?Gap_17f660@@YAXXZ", "u", "not a virtual")]
        with mock.patch.object(vt, "analyse",
                               return_value=([], [], [], wiring, [], 1, 1)):
            self.assertTrue(any("vtable-slot-binding [WIRING]" in f
                                for f in vt.gate_findings()))
        gaps = [(0x1234, 16, "rtti", "CGhost", 0)]
        with mock.patch.object(vt, "analyse",
                               return_value=(gaps, [], [], [], [], 1, 1)):
            self.assertTrue(any("vtable-coverage" in f
                                for f in vt.gate_findings()))
        virt = [("CShell", 0x300, 9, 2, "under-virtualized")]
        with mock.patch.object(vt, "analyse",
                               return_value=([], virt, [], [], [], 1, 1)):
            self.assertTrue(any("vtable-virtuality" in f
                                for f in vt.gate_findings()))
        with mock.patch.object(vt, "analyse",
                               return_value=([], [], [], [], [], 1, 1)):
            self.assertEqual(vt.gate_findings(), [])

    def test_alloc_size_catches_a_planted_sizeof_error_on_the_real_tree(self):
        """The whole-tree control: take a class whose retail `push <n>`
        immediate ALREADY agrees with clang's sizeof, shift the computed side
        by 4, and require the live gate to name it. A gate that is blind
        returns the same 0 rows as a clean tree."""
        from gruntz.core.paths import BUILD
        from gruntz.verify import alloc_size as az
        if not (BUILD / "gen/class_sizes.json").is_file():
            self.skipTest("class_sizes cache absent (unbuilt tree)")
        comp, conflicts = az.computed_sizes()
        if not comp:
            self.skipTest("libclang harvest empty")
        rows = az.Sweep().rows()
        _b, _s, _m, _u, _un, ok = az.classify_rows(rows, comp, conflicts,
                                                   az.def_counts())
        if not ok:
            self.skipTest("no agreeing class to poison on this tree state")
        victim = sorted(o[0] for o in ok)[0]
        poisoned = dict(comp)
        poisoned[victim] += 4
        with mock.patch.object(az, "computed_sizes",
                               return_value=(poisoned, conflicts)):
            out = az.gate_findings()
        self.assertTrue(any(victim in f for f in out),
                        f"a 4-byte sizeof error on {victim} was not caught")


class PipelineErrorControls(unittest.TestCase):
    """The pipeline verbs answer a broken environment with a MESSAGE.

    `gruntz delink` used to let the delinker's ToolError escape as a
    traceback, and the tool's own words name a symptom ("relocation alias
    owner is absent: _length_code$S") whose real cause is a stale
    vostok-delinker on $PATH.
    """

    def _delink(self, exc):
        import contextlib
        import io

        from gruntz.delink import run as dr
        with mock.patch.object(dr, "run", side_effect=exc):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                rc = dr.main([])
        return rc, err.getvalue()

    def test_a_stale_delinker_is_named_not_just_echoed(self):
        from gruntz.tool import ToolError
        rc, text = self._delink(ToolError(
            "vostok-delinker failed (rc=1):\nrelocation alias owner is "
            "absent: _length_code$S"))
        self.assertEqual(rc, 1)
        self.assertIn("_length_code$S", text)      # the tool's own words kept
        self.assertIn("STALE vostok-delinker", text)
        self.assertIn("which vostok-delinker", text)

    def test_a_missing_delinker_binary_is_a_message_not_a_traceback(self):
        rc, text = self._delink(
            FileNotFoundError(2, "No such file or directory",
                              "vostok-delinker"))
        self.assertEqual(rc, 1)
        self.assertIn("nix develop", text)

    def test_an_unhinted_failure_still_prints_the_tools_words(self):
        from gruntz.tool import ToolError
        rc, text = self._delink(ToolError("vostok-delinker failed (rc=9)"))
        self.assertEqual(rc, 1)
        self.assertIn("rc=9", text)

    def test_rsrc_separates_could_not_run_from_a_real_deviation(self):
        """A missing era rc.exe is not a verdict on Gruntz.rc: reporting it as
        `check FAILED` reads as 'the resources diverge from retail'."""
        import contextlib
        import io

        from gruntz.rsrc import check as rc_check
        from gruntz.tool import ToolError
        with mock.patch.object(rc_check.rc_tool, "compile",
                               side_effect=ToolError("rc.exe not found")):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                code = rc_check.check()
        self.assertEqual(code, 2)                  # 2 = could not run
        self.assertIn("COULD NOT RUN", err.getvalue())
        self.assertIn("not a verdict", err.getvalue())
        with mock.patch.object(rc_check.rc_tool, "compile",
                               side_effect=PermissionError(13, "denied")):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                code = rc_check.check(out="/root/denied.res")
        self.assertEqual(code, 2)
        self.assertIn("--out", err.getvalue())

    def test_rsrc_rejects_an_unknown_subcommand_by_name(self):
        import contextlib
        import io

        from gruntz import rsrc
        with contextlib.redirect_stderr(io.StringIO()) as err:
            self.assertEqual(rsrc.main(["bogus"]), 2)
        self.assertIn("unknown subcommand 'bogus'", err.getvalue())
        with contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(rsrc.main(["--help"]), 0)
        self.assertIn("rsrc check", out.getvalue())


class FloorAbsenceControls(unittest.TestCase):
    """A ratchet with no floor must FAIL, never pass.

    board and casts used to fail OPEN: deleting the two files in
    config/cleanliness/ made the whole fast tier permanently green, while
    caller-callee and undefined-closure already refused to pass vacuously.
    """

    def test_board_reports_a_ratcheted_metric_with_no_floor(self):
        from gruntz.verify import board
        with mock.patch.object(board, "load_baseline", return_value={}):
            found = board.gate([("reinterpret_casts", 9999)])
        self.assertTrue(found, "a ratcheted metric with no floor passed")
        self.assertIn("no committed floor", found[0])

    def test_board_still_ratchets_when_the_floor_exists(self):
        from gruntz.verify import board
        with mock.patch.object(board, "load_baseline",
                               return_value={"reinterpret_casts": 10}):
            self.assertTrue(board.gate([("reinterpret_casts", 11)]))
            self.assertEqual(board.gate([("reinterpret_casts", 10)]), [])

    def test_casts_reports_a_missing_floor(self):
        from gruntz.verify import board, casts
        with mock.patch.object(board, "load_baseline", return_value={}), \
             mock.patch.object(casts, "self_recursion", return_value=[]), \
             mock.patch.object(casts, "scan_ledger", return_value=({}, {"a.cpp": [1]})):
            found = casts.gate_findings()
        self.assertTrue(found, "an absent cast floor passed the gate")
        self.assertIn("no committed floor", found[0])


class LinkClosureScanSetControls(unittest.TestCase):
    """The closure check must scan the link line we actually use.

    graph.link substitutes our synthesized import libs into LINK_LIBS, so
    mss32/smackw32 imports resolve at link time; scanning only the
    toolchain reported 26 of them as guaranteed-unresolved.
    """

    def test_synthesized_import_libs_are_in_the_scan_set(self):
        from gruntz.graph import implib
        from gruntz.verify import undefined_closure as uc
        made = [p for p in implib.on_disk() if p.is_file()]
        if not made:
            self.skipTest("no synthesized import libs on disk")
        scanned = set(uc._toolchain_libs())
        for p in made:
            self.assertIn(p, scanned, f"{p.name} is on the link line but unscanned")


class TsvAtomicWriteControls(unittest.TestCase):
    """A concurrent reader never sees a half-written table.

    Observed live: a gate crashed with `no header row` while a build edge
    rewrote build/gen/claims/grunt.tsv in place.
    """

    def test_write_replaces_atomically(self):
        import os
        from gruntz.core import tsv
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "t.tsv"
            tsv.write(path, ["# b"], ["a", "b"], [["1", "2"]])
            before = path.read_text()
            seen = []
            real_replace = os.replace

            def spy(src, dst):
                # the destination must still hold the OLD table right up to
                # the instant of replacement - never a truncated one
                seen.append(Path(dst).read_text())
                return real_replace(src, dst)

            with mock.patch("os.replace", spy):
                tsv.write(path, ["# b"], ["a", "b"], [["3", "4"]])
            self.assertEqual(seen, [before])
            self.assertIn("3\t4", path.read_text())

    def test_no_temp_file_survives(self):
        from gruntz.core import tsv
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "t.tsv"
            tsv.write(path, ["# b"], ["a"], [["1"]])
            self.assertEqual([p.name for p in Path(d).iterdir()], ["t.tsv"])


class SourceNameRewriteControls(unittest.TestCase):
    """The rewrite rules are COMPLETE, proven once per build over the corpus.

    Labelling spells every claim from source (core.msvc_names), so a rule gap
    can no longer hide as a silent per-claim drop - it has to fail here. The
    control is the same assertion the dropped per-claim authority check made,
    lifted to the whole claim set: for EVERY extracted source claim, the name
    equals the emitting base object's own symbol modulo the volatile ordinals
    both sides mask. Reading the objects is fine HERE; it is a test, not the
    extraction path.
    """

    @staticmethod
    def _corpus():
        from gruntz.core.paths import BUILD
        from gruntz.retail_labels import fragments
        base = BUILD / "objdiff/base"
        claims = [c for c in fragments.all_claims() if c.channel == "src"]
        return base, claims

    #: every decoration cl 5.0 could have chosen instead - if one of THESE is
    #: in the object, the claim named the right body and spelled it wrong.
    @staticmethod
    def _alternate_spellings(name: str) -> set[str]:
        import re
        out = set()
        for n in {name, re.sub(r"@@([0-9])P", r"@@\1Q", name)}:
            for m in {n, n.removesuffix("$S"), n + "$S"}:
                out |= {m, "_" + m, m.removeprefix("_")}
        return out - {name}

    def test_every_source_claim_is_cls_own_spelling(self):
        from gruntz.core.coff import Coff
        from gruntz.core.msvc_names import mask
        from gruntz.model import unmaterialized
        base, claims = self._corpus()
        if not claims:
            self.skipTest("no extracted claims - run `gruntz labels --all`")
        objs: dict[str, tuple[set[str], set[str]] | None] = {}
        for unit in {c.unit for c in claims}:
            path = base / f"{unit}.obj"
            if not path.is_file():
                objs[unit] = None
                continue
            coff = Coff(path)
            objs[unit] = ({mask(n) for n in coff.code_names()},
                          {mask(n) for n in coff.all_names()})
        absent = sorted(u for u, v in objs.items() if v is None)
        if absent:
            self.skipTest(f"{len(absent)} unit(s) have no base obj "
                          f"(e.g. {absent[0]}) - run `gruntz build`")
        # A header inline's macro reaches every including TU, but cl
        # materializes the COMDAT only where it is odr-used - so a claim with
        # no symbol in ITS OWN unit is expected. The rewrite is in question
        # only when NO unit claiming that (kind, rva, name) carries it.
        claimed, proven = {}, set()
        for c in claims:
            code, every = objs[c.unit]
            key = (c.kind, c.rva, mask(c.name))
            claimed.setdefault(key, []).append(c)
            if key[2] in (code if c.kind == "func" else every):
                proven.add(key)
        # ... and a gap is a SPELLING defect only if some other decoration of
        # the same claim IS in one of those objects. A gap with no spelling at
        # all is a missing body - a modelling question the Model reports.
        misspelled, bodiless = [], []
        for key in sorted(set(claimed) - proven):
            cs = claimed[key]
            alts = self._alternate_spellings(cs[0].name)
            hit = next((a for c in cs for a in sorted(alts)
                        if a in objs[c.unit][1]), None)
            row = f"{key[0]} 0x{key[1]:06x} {cs[0].name}"
            (misspelled if hit else bodiless).append(
                f"{row} -> cl spells it {hit}" if hit else row)
        self.assertFalse(
            misspelled,
            f"{len(misspelled)} of {len(claimed)} source claim(s) are spelled "
            f"differently by cl - the rewrite rules are incomplete "
            f"(first: {misspelled[0] if misspelled else ''})")
        # the bodiless class must stay LOUD somewhere: a claim with no other
        # spelling at its rva is a Model violation, one WITH another spelling
        # is recorded as that binding's alias. Nothing may be silent.
        gaps = {(c.kind, c.rva, mask(c.name)) for c in unmaterialized(
            [c for cs in claimed.values() for c in cs])}
        aliased = {rva for kind, rva, _n in proven if kind == "func"}
        unreported = sorted(
            k for k in set(claimed) - proven - gaps
            if k[0] == "func" and k[1] not in aliased)
        self.assertFalse(
            unreported,
            f"{len(unreported)} claim(s) match no object symbol and are "
            f"reported by nothing (first: {unreported[0] if unreported else ''})")

    def test_a_missing_rewrite_rule_fails_that_control(self):
        """The negative control: undo two rules, the corpus control must fail.

        A gate that would pass an incomplete rewrite is not a gate."""
        import io
        import re
        from gruntz.retail_labels import fragments

        _base, claims = self._corpus()
        if not claims:
            self.skipTest("no extracted claims - run `gruntz labels --all`")

        def poisoned():
            out = []
            for c in claims:
                name = re.sub(r"@@([0-9])P", r"@@\1Q", c.name)   # undo Q -> P
                if name.endswith("$S"):                          # undo _x$S
                    name = (name[1:] if name.startswith("_") else name)[:-2]
                out.append(c._replace(name=name))
            return out

        with mock.patch.object(fragments, "all_claims", poisoned):
            case = SourceNameRewriteControls(
                "test_every_source_claim_is_cls_own_spelling")
            result = unittest.TextTestRunner(stream=io.StringIO()).run(
                unittest.TestSuite([case]))
        self.assertEqual(len(result.failures), 1,
                         "a broken rewrite rule did not fail the corpus control")
        self.assertIn("rewrite rules are incomplete", result.failures[0][1])

    def test_masking_never_merges_two_object_symbols(self):
        """The mask is only sound while it is injective per object."""
        from gruntz.core.coff import Coff
        from gruntz.core.msvc_names import mask
        base, _claims = self._corpus()
        objs = sorted(base.glob("*.obj"))
        if not objs:
            self.skipTest("no base objs")
        collisions = []
        for path in objs:
            try:
                names = Coff(path).all_names()
            except ValueError:
                continue
            seen: dict[str, str] = {}
            for name in sorted(names):
                other = seen.setdefault(mask(name), name)
                if other != name:
                    collisions.append(f"{path.stem}: {other} / {name}")
        self.assertFalse(collisions,
                         f"{len(collisions)} object symbol pair(s) mask "
                         f"together (first: {collisions[0] if collisions else ''})")

    def test_the_rewrite_rules_are_the_measured_ones(self):
        from gruntz.core import msvc_names as m
        # the i386 COFF global prefix, applied to what LLVM did not mangle
        self.assertEqual(m.func("?Foo@C@@QAEXXZ"), "?Foo@C@@QAEXXZ")
        self.assertEqual(m.func("_stdcall_thing@8", decorated=True),
                         "_stdcall_thing@8")
        self.assertEqual(m.func("ordinary"), "_ordinary")
        # clang's array storage class
        self.assertEqual(m.data("?g_cmdBitTable@@3QBGB", internal=False),
                         "?g_cmdBitTable@@3PBGB")
        # TU-local storage: `_` and `$S` arrive together, whatever the mangling
        self.assertEqual(m.data("s_MAIN", internal=True), "_s_MAIN$S")
        self.assertEqual(m.data("_kDegToRad", internal=True, decorated=True),
                         "_kDegToRad$S")
        self.assertEqual(m.data("?s_x@?1??F@@QAEHXZ@4HA", internal=True),
                         "_?s_x@?1??F@@QAEHXZ@4HA$S")
        # the mask meets cl's own object on both ordinals
        self.assertEqual(m.mask("_?s_x@?BA@??F@@QAEHXZ@4HA$S35536"),
                         "_?s_x@?1??F@@QAEHXZ@4HA$S")
        self.assertEqual(m.mask("_?$S47@?1??G@@QAEHXZ@4EA$S20267"),
                         "_?$S@?1??G@@QAEHXZ@4EA$S")
        # an rva-keyed name is NOT an ordinal: it must survive masking
        self.assertEqual(m.mask("$S2277272"), "$S2277272")
        self.assertEqual(m.mask(m.discriminate("_s_x$S", 0x244970)), "_s_x$S")


class ReadmeFreshnessControls(unittest.TestCase):
    """README's derived block must not be able to go stale.

    It is a pure function of the current report + the banked ledger, but it
    used to move only at `bank` (a deliberate manual act), so ordinary builds
    left it describing an older tree and readers quoted numbers that were no
    longer true - three times in one session. `check` runs on every build and
    now re-renders it write-if-changed; the ledger stays manual.
    """

    def test_check_rewrites_a_stale_block(self):
        from gruntz.verify import readme as rm, verbs
        if not rm.README.is_file():
            self.skipTest("no README")
        before = rm.README.read_text()
        # Anchor on a token the block ALWAYS carries. The original anchor was
        # the `(unmatched)` row, which stopped existing the day the last
        # unclaimed reconstruction target got modelled - a freshness control
        # must not assert a row that only appears while work is outstanding.
        anchor = "Overall (vs full engine)"
        self.assertIn(anchor, before, "the block lost its headline line")
        try:
            rm.README.write_text(before.replace(anchor, anchor + "-STALE", 1))
            verbs.refresh_readme_block()
            fresh = rm.README.read_text()
            self.assertIn(anchor, fresh)
            self.assertNotIn(anchor + "-STALE", fresh)   # the stale text is GONE
        finally:
            rm.README.write_text(before)

    def test_refresh_is_idempotent(self):
        from gruntz.verify import readme as rm, verbs
        if not rm.README.is_file():
            self.skipTest("no README")
        before = rm.README.read_text()
        try:
            verbs.refresh_readme_block()
            self.assertFalse(verbs.refresh_readme_block(),
                             "a second refresh reported a change")
        finally:
            rm.README.write_text(before)

    def test_readme_is_not_a_bank_input(self):
        # writing it must never be able to block `bank`
        from gruntz.verify.verbs import BANK_INPUT_PATHS
        self.assertNotIn("README.md", BANK_INPUT_PATHS)


# --------------------------------------------------------------------------- #
# the build loop (2026-08-16 review)                                          #
# --------------------------------------------------------------------------- #
def _coff(nsec: int = 0, symptr: int = 0, nsym: int = 0, machine: int = 0x14C,
          sections: list[tuple[int, int]] = (), tail: bytes = b"") -> bytes:
    """A hand-built COFF header (+ `sections` as (size, ptr) pairs)."""
    import struct
    out = bytearray(struct.pack("<HHIIIHH", machine, nsec, 0, symptr, nsym, 0, 0))
    for size, ptr in sections:
        raw = bytearray(40)
        struct.pack_into("<II", raw, 16, size, ptr)
        out += raw
    return bytes(out) + tail


class ClEdgeObjectIntegrityControls(unittest.TestCase):
    """The `cl` edge published an incomplete object and called the edge built.

    Two `gruntz build` runs in one tree are not serialised by anything, and
    the driver staged every unit at the SAME `.tmp/<unit>.obj` and installed
    through the same `<unit>.obj.install`. Measured 3/3: one run's cl deleted
    the other's staged object between its `compile()` and its `install()` ("cl
    produced no object (rc=0)", with an EMPTY diagnostic - the compiler blamed
    for a collision) and the shared `.install` rename raised FileNotFoundError
    as a traceback. The shared temp also makes `os.replace` non-atomic across
    processes, and a real build did fail in gruntz.compare.normalize with
    "COFF string table is not final" over an object that a census then found
    invalid. Nothing checked the payload before installing it either.
    """

    def test_a_truncated_object_is_refused_not_installed(self):
        from gruntz.graph import cc
        from gruntz.tool import ToolError
        # one section header promising 0x1000 bytes at 0x40 in a 60-byte file
        torn = _coff(nsec=1, sections=[(0x1000, 0x40)])
        self.assertIsNotNone(cc.coff_defect(torn))
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            with self.assertRaises(ToolError):
                cc.install(torn, out)
            self.assertFalse(out.exists(), "a torn object was installed anyway")

    def test_a_truncated_string_table_is_refused(self):
        from gruntz.graph import cc
        import struct
        # symbol table present, string-table length dword promises past EOF
        body = _coff(nsec=0, symptr=20, nsym=1,
                     tail=b"\0" * 18 + struct.pack("<I", 0x1000))
        self.assertIn("string table", cc.coff_defect(body) or "")

    def test_a_complete_object_passes(self):
        from gruntz.graph import cc
        self.assertIsNone(cc.coff_defect(_coff()))
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            self.assertTrue(cc.install(_coff(), out))
            self.assertFalse(cc.install(_coff(), out), "rewrote unchanged bytes")
            self.assertEqual(list(Path(td).iterdir()), [out])   # no temp left

    def test_install_does_not_use_the_shared_temp_name(self):
        """A sibling build holding `<unit>.obj.install` must not break us."""
        from gruntz.graph import cc
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            (Path(td) / "unit.obj.install").mkdir()       # the old, shared name
            self.assertTrue(cc.install(_coff(), out))
            self.assertTrue(out.is_file())

    def test_the_staging_path_is_per_process(self):
        from gruntz.graph import cc
        seen = {}

        def fake(src, staged, flags):
            seen["staged"] = Path(staged)
            Path(staged).write_bytes(_coff())
            return ""

        with tempfile.TemporaryDirectory() as td:
            src = Path(td) / "u.c"
            src.write_text("int main(void){return 0;}\n")
            with mock.patch("gruntz.tool.cl.compile", fake):
                cc.compile_unit(src, Path(td) / "base" / "u.obj", [])
        parts = seen["staged"].parts
        self.assertIn(str(os.getpid()), parts, f"shared staging dir: {parts}")
        self.assertIn(".tmp", parts)

    def test_an_unwritable_object_tree_is_a_message(self):
        import contextlib
        import io
        from gruntz.graph import cc
        with tempfile.TemporaryDirectory() as td:
            ro = Path(td) / "ro"
            ro.mkdir()
            ro.chmod(0o500)
            try:
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    rc = cc.main(["--out", str(ro / "u.obj"), "--src", str(ro),
                                  "--unit", "u", "--", "/c"])
            finally:
                ro.chmod(0o700)
        self.assertEqual(rc, 1)          # a message + rc 1, never a traceback
        self.assertIn("cannot write", err.getvalue())


class ToolDriverEnvironmentControls(unittest.TestCase):
    """Outside `nix develop` the drivers raised bare Python exceptions.

    `$MSVC_DIR` unset reached the user as a RuntimeError traceback from
    gruntz.core.paths (no tool main() catches it), and an absent
    wine/winepath/llvm-pdbutil/vostok-delinker as a FileNotFoundError out of
    subprocess - including from `gruntz tool wine --verify`, whose whole job
    is to answer "is wine set up?".
    """

    def _no_path(self):
        return mock.patch("shutil.which", lambda _p: None)

    def test_missing_msvc_dir_is_a_toolerror(self):
        from gruntz.tool import ToolError, wine
        with mock.patch.dict(os.environ, {}, clear=False):
            os.environ.pop("MSVC_DIR", None)
            with self.assertRaises(ToolError) as cm:
                wine.era_tool("cl.exe")
        self.assertIn("MSVC_DIR", str(cm.exception))

    def test_the_rc_release_hint_is_only_on_rc(self):
        from gruntz.tool import ToolError, wine
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(wine, "toolchain_root", lambda: Path(td)):
                with self.assertRaises(ToolError) as rc_exc:
                    wine.era_tool("rc.exe")
                with self.assertRaises(ToolError) as cl_exc:
                    wine.era_tool("cl.exe")
        self.assertIn("r3", str(rc_exc.exception))
        self.assertNotIn("rc.exe", str(cl_exc.exception))

    def test_absent_wine_is_named_not_a_traceback(self):
        from gruntz.tool import ToolError, wine
        with self._no_path():
            for call in (lambda: wine.winepath("/tmp"),
                         lambda: wine.require("wine")):
                with self.assertRaises(ToolError) as cm:
                    call()
                self.assertIn("nix develop", str(cm.exception))

    def test_wine_verify_without_wine_returns_a_message(self):
        import contextlib
        import io
        from gruntz.tool import wine
        with self._no_path(), mock.patch.object(sys, "argv",
                                                ["gruntz tool wine", "--verify"]):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                rc = wine.main()
        self.assertEqual(rc, 1)
        self.assertIn("not found on PATH", err.getvalue())

    def test_wine_with_no_action_is_not_a_silent_success(self):
        import contextlib
        import io
        from gruntz.tool import wine
        for argv in (["gruntz tool wine"], ["gruntz tool wine", "--force"]):
            with mock.patch.object(sys, "argv", argv):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    rc = wine.main()
            self.assertEqual(rc, 2, argv)
            self.assertIn("--init", err.getvalue())

    def test_absent_native_tools_are_named(self):
        from gruntz.tool import ToolError, delinker, pdbutil
        with self._no_path():
            with self.assertRaises(ToolError) as cm:
                delinker.delink("a.pdb", "b.exe", "/tmp/out")
            self.assertIn("vostok-delinker", str(cm.exception))
            with self.assertRaises(ToolError) as cm:
                pdbutil.dump("a.pdb")
            self.assertIn("llvm-pdbutil", str(cm.exception))

    def test_a_stalled_native_tool_is_a_message(self):
        import subprocess
        from gruntz.tool import ToolError, pdbutil
        with mock.patch("shutil.which", lambda p: "/bin/" + p), \
                mock.patch("subprocess.run",
                           side_effect=subprocess.TimeoutExpired("x", 1)):
            with self.assertRaises(ToolError) as cm:
                pdbutil.dump("a.pdb")
        self.assertIn("did not finish", str(cm.exception))

    def test_implib_without_the_retail_image_is_a_message(self):
        import contextlib
        import io
        from gruntz.graph import implib
        with mock.patch.object(sys, "argv", ["implib", "--list"]), \
                mock.patch.object(implib, "import_table",
                                  side_effect=FileNotFoundError(
                                      2, "No such file or directory",
                                      "GRUNTZ.EXE")):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                rc = implib.main()
        self.assertEqual(rc, 1)
        self.assertIn("retail image", err.getvalue())


class LinkVerbTargetControls(unittest.TestCase):
    """`gruntz link <anything>` died on `unknown target build/gen/gruntz.res`.

    The `.res` edge exists only when the toolchain carried rc.exe at CONFIGURE
    time; the verb asked ninja for it unconditionally, so on a pre-r3 pin every
    flagged form - `--help` included - failed before reaching the parser, and
    `--help` also ran a full build first.
    """

    def test_manifest_targets_reads_the_emitted_edges(self):
        from gruntz.graph import verbs
        with tempfile.TemporaryDirectory() as td:
            man = Path(td) / "build.ninja"
            man.write_text("rule cl\n  command = x\n"
                           "build a/b.obj: cl src.c | dep.h\n"
                           "build one two: phony $\n    three\n")
            with mock.patch.object(verbs.graph, "NINJA", str(man)), \
                    mock.patch.object(verbs, "REPO", Path("/")):
                got = verbs.manifest_targets()
        self.assertEqual(got, {"a/b.obj", "one", "two"})
        self.assertNotIn("build/gen/gruntz.res", got)

    def test_a_missing_manifest_is_an_empty_set_not_a_traceback(self):
        from gruntz.graph import verbs
        with mock.patch.object(verbs.graph, "NINJA", "no/such/build.ninja"):
            self.assertEqual(verbs.manifest_targets(), set())

    def test_help_answers_the_parser_without_building(self):
        import contextlib
        import io
        from gruntz.graph import verbs
        with mock.patch.object(verbs, "ninja",
                               side_effect=AssertionError("built for --help")), \
                mock.patch.object(verbs, "configure_if_needed",
                                  side_effect=AssertionError("configured")):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                with self.assertRaises(SystemExit) as cm:
                    verbs.link_main(["--help"])
        self.assertEqual(cm.exception.code, 0)
        self.assertIn("--engine-lib", out.getvalue())

    def test_the_res_target_is_only_requested_when_it_exists(self):
        import contextlib
        import io
        from gruntz import graph
        from gruntz.graph import verbs
        asked = []
        with mock.patch.object(verbs, "configure_if_needed", lambda *a, **k: None), \
                mock.patch.object(verbs, "ninja",
                                  lambda t, **k: asked.append(list(t)) or 1):
            with mock.patch.object(verbs, "manifest_targets", lambda: {"base"}):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    verbs.link_main(["--dry-run"])
            self.assertEqual(asked[-1], ["base"])
            self.assertIn("no build/gen/gruntz.res edge", err.getvalue())
            with mock.patch.object(verbs, "manifest_targets",
                                   lambda: {"base", graph.RESOURCE_RES}):
                verbs.link_main(["--dry-run"])
            self.assertEqual(asked[-1], ["base", graph.RESOURCE_RES])

    def test_a_resourceless_candidate_says_so(self):
        """A candidate with no .rsrc has no MFC dialogs; the only note used to
        live in a generated manifest comment nobody reads."""
        import contextlib
        import io
        from gruntz.graph import implib, link as gl
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "cand.EXE"
            obj = Path(td) / "u.obj"
            obj.write_bytes(_coff())

            def fake_link(args, **kw):
                out.write_bytes(b"MZ")
                out.with_suffix(".map").write_text("")
                return ""

            with mock.patch.object(gl, "winepath", str), \
                    mock.patch.object(implib, "ensure_all", lambda *a, **k: []), \
                    mock.patch("gruntz.tool.link.link", fake_link):
                with contextlib.redirect_stdout(io.StringIO()) as sout:
                    gl.candidate(out, Path(td), explicit=[str(obj)])
        self.assertIn("NO .rsrc", sout.getvalue())


class ImplibScaffoldControls(unittest.TestCase):
    """The synthesis cleaned up `<stem>.exp`, a name link.exe never writes.

    The .exp is named after the /IMPLIB path, and synthesis links through
    `<stem>.lib.tmp`, so the real leftovers were `mss32.lib.exp` and
    `smackw32.lib.exp` - both still sitting in build/lib/ weeks later.
    """

    def test_the_cleanup_names_the_exp_link_actually_writes(self):
        import inspect
        from gruntz.graph import implib
        src = inspect.getsource(implib.synthesize)
        self.assertIn('tmp_lib.with_suffix(".exp")', src)

    def test_the_temp_lib_derives_the_observed_exp_name(self):
        from gruntz.graph import implib
        lib = Path(implib.OUT_DIR) / "mss32.lib"
        tmp_lib = lib.with_suffix(".lib.tmp")
        self.assertEqual(tmp_lib.with_suffix(".exp").name, "mss32.lib.exp")


class ConfigureWriteControls(unittest.TestCase):
    """`gruntz configure --out <unwritable>` raised FileNotFoundError from the
    manifest mkdir instead of naming the path it could not write."""

    def test_an_unwritable_manifest_path_is_a_message(self):
        import contextlib
        import io
        from gruntz.graph import emit
        with contextlib.redirect_stderr(io.StringIO()) as err:
            rc = emit.main(["--out", "/proc/no/such/dir/build.ninja"])
        self.assertEqual(rc, 1)
        self.assertIn("cannot write", err.getvalue())


class CompdbStalenessControls(unittest.TestCase):
    """`--check` answered "coverage: 300/300" for a database whose toolchain
    include dirs no longer exist. A toolchain re-pin moves $MSVC_DIR and no
    ninja edge can see it, so the stored /nix/store path outlives its store
    entry; every clang consumer then silently loses its headers."""

    def test_a_dead_include_dir_is_reported(self):
        from gruntz.graph import compdb
        db = {"/x/a.cpp": ["/imsvc", "/nix/store/gone-toolchain/msvc/include",
                           "/I", "/nix/store/gone-toolchain/dx/Include"]}
        self.assertEqual(
            compdb.dead_include_dirs(db),
            ["/nix/store/gone-toolchain/dx/Include",
             "/nix/store/gone-toolchain/msvc/include"])

    def test_live_dirs_are_not_reported(self):
        from gruntz.graph import compdb
        with tempfile.TemporaryDirectory() as td:
            self.assertEqual(compdb.dead_include_dirs({"x": ["/imsvc", td]}), [])

    def test_a_flag_without_its_operand_is_not_a_crash(self):
        from gruntz.graph import compdb
        self.assertEqual(compdb.dead_include_dirs({"x": ["/imsvc"]}), [])


class MatchReferenceControls(unittest.TestCase):
    """`gruntz match --reference <bad path>` raised FileNotFoundError AFTER a
    full build - the work was done and the run ended in a traceback."""

    def test_an_unreadable_reference_is_a_message(self):
        import contextlib
        import io
        from gruntz.graph import verbs
        with tempfile.TemporaryDirectory() as td:
            missing = Path(td) / "nope.json"
            with mock.patch.object(verbs, "configure_if_needed", lambda *a, **k: None), \
                    mock.patch.object(verbs, "ninja", lambda *a, **k: 0), \
                    mock.patch.object(verbs, "object_census", dict), \
                    mock.patch("gruntz.tool.objdiff.load",
                               lambda p: (_ for _ in ()).throw(FileNotFoundError(p))
                               if str(p) == str(missing) else {"measures": {},
                                                               "units": []}), \
                    mock.patch("gruntz.compare.run.print_summary",
                               lambda *a, **k: None), \
                    mock.patch.object(Path, "exists", lambda self: True):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    with contextlib.redirect_stdout(io.StringIO()):
                        rc = verbs.match_main(["--reference", str(missing)])
        self.assertEqual(rc, 2)
        self.assertIn("--reference", err.getvalue())


class ToolchainIsADeclaredInput(unittest.TestCase):
    """The era toolchain and the delinker used to be pure environment.

    Consequence chain, all silent: re-pinning $MSVC_DIR recompiled only the
    units that happened to be dirty and left the rest built by the previous
    cl (a mixed object set, the worst failure a byte-matching project has),
    swapping the delinker gave `ninja: no work to do`, and because the `rc`
    edge only exists when rc.exe was present at CONFIGURE time, a pre-r3 pin
    silently produced a candidate image with no `.rsrc` at all.
    """

    def test_the_identity_names_all_three_inputs(self):
        from gruntz.graph.emit import toolchain_id
        with mock.patch.dict(os.environ, {"MSVC_DIR": "/m", "DXSDK_DIR": "/d"}):
            got = toolchain_id()
        self.assertIn("MSVC_DIR=/m", got)
        self.assertIn("DXSDK_DIR=/d", got)
        self.assertIn("delinker=", got)

    def test_an_unset_variable_is_recorded_not_skipped(self):
        """Unset -> set is itself a change the edges must see."""
        from gruntz.graph.emit import toolchain_id
        with mock.patch.dict(os.environ, {"DXSDK_DIR": "/d"}, clear=True):
            got = toolchain_id()
        self.assertIn("MSVC_DIR=-", got)

    def test_the_id_is_written_if_changed(self):
        """It is an input of all 300 cl edges: an unconditional rewrite would
        recompile the tree every time anything else re-ran configure."""
        from gruntz.graph.emit import write_toolchain_id
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "toolchain.id"
            self.assertTrue(write_toolchain_id(p))     # created
            before = p.stat().st_mtime_ns
            self.assertFalse(write_toolchain_id(p))    # unchanged
            self.assertEqual(p.stat().st_mtime_ns, before)
            p.write_text("MSVC_DIR=/elsewhere\n")
            self.assertTrue(write_toolchain_id(p))     # moved

    def test_a_repin_is_detected(self):
        from gruntz.graph import verbs
        from gruntz.graph.emit import toolchain_id
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "toolchain.id"
            with mock.patch.dict(os.environ, {"MSVC_DIR": "/one"}):
                p.write_text(toolchain_id())
                with mock.patch.object(verbs, "REPO", Path(td)), \
                        mock.patch.object(verbs.graph, "TOOLCHAIN_ID", "toolchain.id"):
                    self.assertFalse(verbs.toolchain_repinned())
                    with mock.patch.dict(os.environ, {"MSVC_DIR": "/two"}):
                        self.assertTrue(verbs.toolchain_repinned())

    def test_the_emitted_manifest_declares_it_on_the_cl_edges(self):
        """The integration control. Recognising the file is not the claim -
        the claim is that the COMPILE edges depend on it, which is what makes
        a re-pin invalidate the object set."""
        from gruntz import graph
        from gruntz.core.paths import REPO
        ninja = (REPO / graph.NINJA)
        if not ninja.exists():
            self.skipTest("no emitted manifest (run `gruntz configure`)")
        text = ninja.read_text()
        self.assertIn(graph.TOOLCHAIN_ID, text)
        cl_edges = [ln for ln in text.splitlines()
                    if ln.startswith("build build/objdiff/base/") and ": cl " in ln]
        self.assertTrue(cl_edges, "no cl edges in the manifest")
        # Edge lines are wrapped by the writer, so join the continuations.
        joined, buf = [], ""
        for ln in text.splitlines():
            buf += ln.rstrip("$")
            if not ln.endswith("$"):
                joined.append(buf); buf = ""
        declaring = [ln for ln in joined
                     if ln.startswith("build build/objdiff/base/")
                     and ": cl " in ln and graph.TOOLCHAIN_ID in ln]
        self.assertEqual(len(declaring), len(cl_edges),
                         "every cl edge must declare the toolchain identity")


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="gruntz verify selftest", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="name every control as it runs")
    ap.add_argument("-k", dest="patterns", action="append", default=[],
                    help="run only controls whose name contains this "
                         "substring (repeatable; comma-separated also works)")
    a = ap.parse_args(list(argv or []))
    verbosity = 2 if a.verbose else 1
    loader = unittest.TestLoader()
    wanted = [p.strip() for spec in a.patterns for p in spec.split(",")
              if p.strip()]
    if wanted:
        loader.testNamePatterns = [f"*{p}*" for p in wanted]
    suite = loader.loadTestsFromModule(sys.modules[__name__])
    runner = unittest.TextTestRunner(verbosity=verbosity)
    result = runner.run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
