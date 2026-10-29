"""Rule proposals and budget needs of the inline-decision forcing search."""

import unittest

from gruntz.walls import inline_reach


def site(symbol, expanded, *, cb=61, budget=60, running=100, depth=1, allow=8,
         args_match=True):
    return dict(symbol=symbol, owner="?root@@YAXXZ", expanded=expanded, cb=cb, budget=budget,
                running=running, depth=depth, allow=allow, args_match=args_match)


R, I = "??0R@@QAE@XZ", "??0I@@QAE@XZ"


class Candidates(unittest.TestCase):
    def test_a_call_retail_lacks_proposes_every_kept_site_ordinal_first(self):
        root = dict(sites=[site(R, False), site(R, False), site(R, True), site(R, False)])
        # ours R R R, retail R R: the third call is surplus.
        out = inline_reach.candidates(root, [R, R, R], [R, R], set(), 8)
        self.assertEqual(out[0], (3, "E"))
        self.assertEqual(sorted(out), [(0, "E"), (1, "E"), (3, "E")])

    def test_a_call_retail_adds_proposes_keeping_an_expansion(self):
        root = dict(sites=[site(R, True), site(I, True, depth=2), site(R, False)])
        out = inline_reach.candidates(root, [R], [R, I], set(), 8)
        self.assertIn((1, "K"), out)

    def test_unforceable_and_forced_sites_are_never_proposed(self):
        root = dict(sites=[site(R, False, args_match=False), site(R, False, depth=9, allow=8),
                           site(R, False)])
        self.assertEqual(inline_reach.candidates(root, [R, R, R], [], {2}, 8), [])

    def test_distance_counts_traced_differences_only(self):
        other = "?untraced@@YAXXZ"
        self.assertEqual(inline_reach.distance([R, R, other], [R, I], {R, I}), 2)
        self.assertEqual(inline_reach.distance([R, other], [R], {R}), 0)


class Need(unittest.TestCase):
    def test_expand_needs_the_missing_budget(self):
        self.assertTrue(inline_reach.need(site(R, False), "E").startswith("budget +1 "))

    def test_keep_of_an_exempt_callee_is_not_a_budget_change(self):
        self.assertIn("budget-exempt", inline_reach.need(site(I, True, cb=30), "K"))

    def test_expand_over_the_size_cap(self):
        self.assertIn("running size", inline_reach.need(site(R, False, running=35001), "E"))


if __name__ == "__main__":
    unittest.main()
