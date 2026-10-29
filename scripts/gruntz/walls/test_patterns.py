"""gruntz walls patterns: decoding, signature, cleanup, loop, store-order,
substitution and naming helpers on hand-assembled x86 bodies."""

from __future__ import annotations

import struct
import unittest

from gruntz.walls import patterns as P
from gruntz.walls.pairscan import DIR32, REL32

ME = "?Fn@C@@QAEHH@Z"


def _signature(hexbody: str):
    return P.signature(P.decode(bytes.fromhex(hexbody)))


class SignatureTests(unittest.TestCase):
    def test_cdecl_body_pops_nothing_and_reads_no_register(self):
        # mov eax,[esp+4]; ret
        self.assertEqual(_signature("8b442404c3"), (frozenset({0}), frozenset()))

    def test_fastcall_body_reads_ecx_and_edx_and_pops_stack_arguments(self):
        # mov eax,ecx; add eax,edx; add eax,[esp+4]; ret 4
        pops, live = _signature("8bc1" "03c2" "03442404" "c20400")
        self.assertEqual(pops, frozenset({4}))
        self.assertEqual(live, frozenset({"ecx", "edx"}))

    def test_written_register_and_frame_push_are_not_arguments(self):
        # push ecx; xor ecx,ecx; mov [esp],ecx; pop ecx; ret
        self.assertEqual(_signature("51" "33c9" "890c24" "59" "c3")[1], frozenset())

    def test_a_call_clobbers_the_argument_registers(self):
        # call +0; mov eax,ecx; ret
        self.assertEqual(_signature("e800000000" "8bc1" "c3")[1], frozenset())

    def test_shift_count_and_division_read_their_implicit_registers(self):
        # shl eax,cl; ret / div ebx; ret
        self.assertEqual(_signature("d3e0c3")[1], frozenset({"ecx"}))
        self.assertEqual(_signature("f7f3c3")[1], frozenset({"edx"}))
        # cdq; idiv ebx; ret - cdq writes edx first
        self.assertEqual(_signature("99f7fbc3")[1], frozenset())

    def test_liveness_follows_both_branch_arms(self):
        # test eax,eax; je +2; mov eax,ecx; ret
        self.assertEqual(_signature("85c0" "7402" "8bc1" "c3")[1], frozenset({"ecx"}))


def _switch_body():
    """cmp eax,1; ja default; jmp [eax*4+T]; arm0: mov eax,10; ret;
    arm1: mov eax,20; ret; default: xor eax,eax; ret; then the table."""
    code = bytearray(bytes.fromhex("83f801" "770f" "ff2485" "00000000"
                                   "b80a000000c3" "b814000000c3" "33c0c3"))
    table = len(code)
    struct.pack_into("<I", code, 8, table)
    body = bytes(code) + struct.pack("<II", 12, 18)
    relocs = {8: (ME, DIR32, table), table: (ME, DIR32, 12), table + 4: (ME, DIR32, 18)}
    return body, relocs, table


class DecodeTests(unittest.TestCase):
    def test_jump_table_is_data_and_its_arms_are_code(self):
        body, relocs, table = _switch_body()
        insns = P.decode(body, relocs, ME)
        self.assertTrue(all(at < table for at in insns))
        self.assertIn(12, insns)
        self.assertIn(18, insns)
        self.assertEqual(P.signature(insns)[0], frozenset({0}))

    def test_without_the_relocations_only_the_fall_through_is_code(self):
        body, _relocs, _table = _switch_body()
        self.assertNotIn(12, P.decode(body))

    def test_batched_decode_equals_one_body_at_a_time(self):
        switch, relocs, _table = _switch_body()
        # the second body ends inside a five-byte call
        bodies = [(switch, relocs, ME), (bytes.fromhex("85c975fc" "e8"), {}, "?g@@YAXXZ"),
                  (bytes.fromhex("d3e0c3"), {}, "?h@@YAXXZ")]
        batched = P.decode_many(bodies)
        for (body, rel, own), got in zip(bodies, batched):
            self.assertEqual(got, P.decode(body, rel, own))
        self.assertEqual(P.branch_target(batched[1][2]), 0)


class CleanupTests(unittest.TestCase):
    def test_caller_cleanup_forms(self):
        body = bytes.fromhex("e800000000" "83c40c" "e800000000" "59" "e800000000" "5959"
                             "e800000000" "c3")
        relocs = {1: ("_f", REL32, 0), 9: ("_f", REL32, 0), 15: ("_g", REL32, 0),
                  22: ("_f", REL32, 0)}
        self.assertEqual(P.call_sites(body, relocs), {"_f": [12, 4, 0], "_g": [8]})


class LoopTests(unittest.TestCase):
    def test_rotated_and_unrotated_back_edges(self):
        # top: dec ecx; jne top; ret          (rotated)
        rotated = P.decode(bytes.fromhex("4975fdc3"))
        # top: test ecx,ecx; je out; dec ecx; jmp top; out: ret
        plain = P.decode(bytes.fromhex("85c9740349ebf9c3"))
        self.assertEqual(P.loop_shape(rotated), {"jcc_back": 1, "ret": 1})
        self.assertEqual(P.loop_shape(plain), {"jcc_fwd": 1, "jmp_back": 1, "ret": 1})


class StoreOrderTests(unittest.TestCase):
    def _rows(self, hexbody):
        return P.masked_rows(P.decode(bytes.fromhex(hexbody)))

    def test_adjacent_swapped_stores_through_one_base(self):
        ours = self._rows("83c8ff" "8945b8" "8945bc" "c3")
        retail = self._rows("83c8ff" "8945bc" "8945b8" "c3")
        found = P.store_permutations(ours, retail)
        self.assertEqual(len(found["blocks"]), 1)
        self.assertEqual(found["permuted"], 2)

    def test_different_values_are_not_a_permutation(self):
        ours = self._rows("8945b8" "895dbc" "c3")
        retail = self._rows("8945b8" "8945bc" "c3")
        self.assertEqual(P.store_permutations(ours, retail)["blocks"], [])


class X87Tests(unittest.TestCase):
    def test_store_reload_pairs_and_fabs(self):
        # fstp qword [ebp-8]; fld qword [ebp-8]; fabs; fstp qword [ebp-16]; ret
        body = bytes.fromhex("dd5df8" "dd45f8" "d9e1" "dd5df0" "c3")
        shape = P.x87_profile(P.decode(body), body, {})
        self.assertEqual((shape["reloads"], shape["fabs"]), (1, 1))

    def test_integer_body_has_no_x87_profile(self):
        body = bytes.fromhex("8bc1c3")
        self.assertIsNone(P.x87_profile(P.decode(body), body, {}))


class SubstitutionTests(unittest.TestCase):
    def test_one_callee_replaced_by_another(self):
        self.assertEqual(P.substitutions(["a", "x", "b"], ["a", "y", "b"]), [("x", "y")])

    def test_reordered_calls_are_not_substitutions(self):
        self.assertEqual(P.substitutions(["a", "b"], ["b", "a"]), [])

    def test_an_expansion_is_not_a_substitution(self):
        self.assertEqual(P.substitutions(["a", "x", "b"], ["a", "b"]), [])


class NameTests(unittest.TestCase):
    def test_c_conventions_from_decoration(self):
        self.assertEqual(P.convention("@adler32@12", {}), "__fastcall")
        self.assertEqual(P.convention("_MessageBoxA@16", {}), "__stdcall")
        self.assertEqual(P.convention("_strlen", {}), "__cdecl")

    def test_qualified_keeps_template_arguments(self):
        self.assertEqual(P.qualified("public: class CString & __thiscall "
                                     "zDArray<class CString>::operator[](int)"),
                         "zDArray<class CString>::operator[]")
        self.assertEqual(P.qualified("public: void __thiscall FreeNodePool<struct Coord>"
                                     "::Push(struct Coord *)"),
                         "FreeNodePool<struct Coord>::Push")

    def test_ob1_profiles(self):
        self.assertTrue(P.ob1(["/O2", "/MT"]))
        self.assertTrue(P.ob1(["/O1"]))
        self.assertFalse(P.ob1(["/O1", "/Ob2"]))
        self.assertFalse(P.ob1(["/Od"]))


if __name__ == "__main__":
    unittest.main()
