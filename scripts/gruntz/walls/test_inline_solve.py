"""The traced-tree replay and the inline-wall hypothesis search."""

import unittest

from gruntz.walls import inline_solve
from gruntz.walls.inline_solve import Edit, Model, Search


def site(symbol, cb, depth, budget, remaining, running, *, expanded=None,
         args_match=True, allow=8):
    allows = cb <= 40 or budget >= cb
    if expanded is None:
        expanded = args_match and depth <= allow and allows and running <= 35000
    return dict(symbol=symbol, owner="?", depth=depth, allow=allow, budget=budget,
                remaining=remaining, cb=cb, running=running, budget_allows=allows,
                args_match=args_match, admitted=expanded, charged=expanded,
                expanded=expanded)


def flat_root(cb, costs):
    """A root whose own list holds leaf sites of the given costs, traced by
    replaying the rule by hand."""
    budget, running, sites = min(35000, max(1000, 2 * cb)), cb, []
    for k, cost in enumerate(costs):
        s = site(f"?leaf{cost}@@YAXXZ", cost, 1, budget, len(costs) - k, running)
        sites.append(s)
        if s["expanded"]:
            budget -= cost if cost > 40 else 0
            running += cost
    return dict(symbol="?root@@YAXXZ", cb=cb, initial_budget=min(35000, max(1000, 2 * cb)),
                sites=sites)


class Replay(unittest.TestCase):
    def test_flat_budget_runs_out(self):
        root = flat_root(100, [300, 300, 300, 300])
        model = Model.build(root)
        self.assertEqual(model.mismatches(), [])
        expanded, counts, incomplete = model.run()
        self.assertEqual(expanded, [True, True, True, False])
        self.assertEqual(counts, {"?leaf300@@YAXXZ": 1})
        self.assertFalse(incomplete)

    def test_nested_share_is_budget_after_charge_over_remaining(self):
        # root cb 27 -> budget 1000; mid (cb 60, 2 sites left) charges 60 and
        # hands (1000 - 60) // 2 = 470 to its body. The body's 300 reaches
        # the running size twice: as the leaf's own cost, then as spent.
        sites = [site("?mid@@YAXXZ", 60, 1, 1000, 2, 27),
                 site("?leaf@@YAXXZ", 300, 2, 470, 2, 87),
                 site("?leaf@@YAXXZ", 300, 2, 170, 1, 387, expanded=False),
                 site("?leaf@@YAXXZ", 300, 1, 640, 1, 687)]
        root = dict(symbol="?root@@YAXXZ", cb=27, initial_budget=1000, sites=sites)
        self.assertEqual(Model.build(root).mismatches(), [])

    def test_argument_gate_refuses_without_charge(self):
        sites = [site("?a@@YAXXZ", 300, 1, 1000, 2, 50, args_match=False),
                 site("?b@@YAXXZ", 800, 1, 1000, 1, 50)]
        root = dict(symbol="?root@@YAXXZ", cb=50, initial_budget=1000, sites=sites)
        model = Model.build(root)
        self.assertEqual(model.mismatches(), [])
        self.assertEqual(model.run()[1], {"?a@@YAXXZ": 1})

    def test_context_refusals_replay_with_their_traced_charge(self):
        charged = dict(site("?a@@YAXXZ", 300, 1, 1000, 3, 50), expanded=False)
        free = dict(site("?b@@YAXXZ", 300, 1, 700, 2, 350), expanded=False, charged=False)
        sites = [charged, free, site("?c@@YAXXZ", 300, 1, 700, 1, 350)]
        root = dict(symbol="?root@@YAXXZ", cb=50, initial_budget=1000, sites=sites)
        model = Model.build(root)
        self.assertEqual(model.mismatches(), [])
        self.assertEqual(model.run()[1], {"?a@@YAXXZ": 1, "?b@@YAXXZ": 1})

    def test_refused_site_takes_children_from_a_traced_expansion(self):
        sites = [site("?mid@@YAXXZ", 300, 1, 1000, 3, 50),
                 site("?leaf@@YAXXZ", 20, 2, 233, 1, 350),
                 site("?big@@YAXXZ", 600, 1, 700, 2, 370),
                 site("?mid@@YAXXZ", 300, 1, 100, 1, 970, expanded=False)]
        root = dict(symbol="?root@@YAXXZ", cb=50, initial_budget=1000, sites=sites)
        model = Model.build(root)
        self.assertEqual(model.mismatches(), [])
        last = model.index.index(3)
        self.assertTrue(model.known[last])
        self.assertEqual([model.sym[k] for k in model.kids[last]], ["?leaf@@YAXXZ"])


class Solve(unittest.TestCase):
    def test_root_cost_window_reproduces_a_retained_call(self):
        # ours expands three 300-cost leaves; retail keeps one more call.
        root = flat_root(100, [300, 300, 300, 300])
        search = Search(Model.build(root), {"?leaf300@@YAXXZ": 1}, seconds=10)
        result = search.solve(max_edits=1)
        self.assertTrue(result["fits"])
        best = result["fits"][0]
        self.assertEqual(best.distance, 0)
        self.assertTrue(any(e.kind in ("insert", "callee", "remove", "cb") for e in best.edits))

    def test_heavier_caller_expands_the_last_site(self):
        root = flat_root(100, [300, 300, 300, 300])
        result = Search(Model.build(root), {"?leaf300@@YAXXZ": -1}, seconds=10).solve(max_edits=1)
        cb_fits = [f for f in result["fits"] if f.edits[0].kind == "cb"]
        self.assertTrue(cb_fits)
        # budget 2 x cb must reach 1200: cb >= 600.
        self.assertEqual(cb_fits[0].edits[0], Edit("cb", amount=500))

    def test_collapse_merges_a_run_of_cost_deltas(self):
        # Three leaves of cost c fit a budget of 1000 while 1000 - 2c >= c:
        # every c in 334..500 keeps one more call.
        root = flat_root(100, [300, 300, 300, 300])
        search = Search(Model.build(root), {"?leaf300@@YAXXZ": 1}, seconds=10)
        fits = [search.evaluate([Edit("callee", symbol="?leaf300@@YAXXZ", amount=d)])
                for d in range(30, 49)]
        merged = inline_solve.collapse([f for f in fits if f.distance == 0])
        self.assertEqual([f.deltas for f in merged], [(34, 48)])

    def test_what_if_reports_flips(self):
        root = flat_root(100, [300, 300, 300, 300])
        out = inline_solve.what_if(Model.build(root), cb=600)
        self.assertEqual(out["calls"], {"?leaf300@@YAXXZ": -1})
        self.assertEqual([f["index"] for f in out["flips"]], [3])


if __name__ == "__main__":
    unittest.main()
