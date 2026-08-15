"""gruntz.verify.selftest - NEGATIVE CONTROLS for the ported gates.

A gate nobody has seen FAIL is a green light, not a check (the ported
gate_selftest doctrine: the slot-binding gate once parsed its own baseline's
banner as the header row and passed everything, forever). Every ported gate
gets at least one demonstrated failure case AND a clean pass; the tests are
hermetic (tmpdir trees / synthetic models) except the DATA_COMPGEN control
set, which deliberately runs against real base objs (skipped loudly when the
tree is unbuilt).

    python3 -m gruntz.verify selftest [-v]
"""

from __future__ import annotations

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


def main(argv=None) -> int:
    argv = list(argv or [])
    verbosity = 2 if "-v" in argv else 1
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromModule(sys.modules[__name__])
    runner = unittest.TextTestRunner(verbosity=verbosity)
    result = runner.run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
