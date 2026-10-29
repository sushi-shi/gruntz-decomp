#!/usr/bin/env python3
"""gruntz.walls.inline_solve - where and what kind of source difference an
inline wall needs.

    gruntz walls inline-solve <rva|name> [--max-edits 1|2|3] [--top N]
                                         [--seconds S] [--json]
    gruntz walls inline-solve <rva|name> --what-if [--cb N] [--add I:K]...
                                         [--remove I]... [--cost TEXT:CB]...
    gruntz walls inline-solve --check <unit>... | --check-all

`gruntz walls inline-trace` records every decision cl 5.0's inline pass
makes in a unit. The replay here reproduces those decisions from the traced
site trees alone (`--check` proves it per root), and the normalized pair
shows how many calls retail keeps to each traced callee. When the counts
differ, the search looks for the fewest, smallest edits whose replay gives
retail's count for every traced callee and changes no other:

- ``cb``      the root's own front-end cost (budget clamp(2 x cb, 1000,
              35000)): more or fewer statements in the caller;
- ``insert``  K free (cb <= 40) candidate sites after a site of one site
              list: in the root's list a caller edit, in a callee's list an
              edit of that helper's body (every expansion of it);
- ``remove``  one site retail does not have;
- ``callee``  a callee's cost moved by d at every site (a lighter or heavier
              helper body); a cost reaching 1000 leaves the candidate set.

Every fit carries the root-cb window over which it still holds, since a
real edit moves the caller's own cost too. Ranking: fewest edits, then
edits that do not just remove a site of a callee whose count must change,
then complete fits (no newly expanded site with unknown children), then
fits that hold at the current cb, then magnitude.

A fit is a search instruction, not source to adopt: it names the place and
kind of difference, and the matcher looks for the original structure there
(AGENTS.md forbids budget-steering code). Children of a site the trace
refused come from the callee's largest traced expansion in the unit; with
none they are unknown and the fit is marked incomplete.
"""

from __future__ import annotations

import argparse
import dataclasses
from dataclasses import dataclass, field
import itertools
import json
import sys
import time
from collections import Counter

from gruntz.walls.inline_model import (
    BUDGET_CAP, BUDGET_FLOOR, CANDIDACY_CB, RUNNING_CAP, SMALL_FREE, _tdiv,
)


def tree(sites: list[dict]) -> list[dict]:
    """Nest one root's sites (trace order) by depth."""
    top: dict = {"children": []}
    stack = [(0, top)]
    for site in sites:
        node = dict(site, children=[])
        while stack[-1][0] >= site["depth"]:
            stack.pop()
        stack[-1][1]["children"].append(node)
        stack.append((site["depth"], node))
    return top["children"]


def templates(roots: list[dict]) -> dict[str, dict]:
    """Callee -> its largest traced expansion in the unit. A callee's sites
    come from its one stored body, so any traced expansion shows the sites
    a refused occurrence would have had."""
    best: dict[str, tuple[int, dict]] = {}

    def size(nodes):
        return sum(1 + size(n["children"]) for n in nodes)

    def walk(nodes):
        for node in nodes:
            if node["expanded"]:
                n = size(node["children"])
                if n > best.get(node["symbol"], (-1, None))[0]:
                    best[node["symbol"]] = (n, node)
            walk(node["children"])
    for root in roots:
        walk(tree([dict(s, index=i) for i, s in enumerate(root["sites"])]))
    return {symbol: node for symbol, (_n, node) in best.items()}


def budget_of(cb: int) -> int:
    return min(BUDGET_CAP, max(BUDGET_FLOOR, 2 * cb))


@dataclass
class Model:
    """A root's site tree, flattened, with template children attached."""
    root: dict
    sym: list = field(default_factory=list)
    cost: list = field(default_factory=list)
    remaining: list = field(default_factory=list)
    gate: list = field(default_factory=list)       # argument count and depth admit it
    refusal: list = field(default_factory=list)    # traced context refusal: None, "free", "charged"
    index: list = field(default_factory=list)      # trace index, -1 synthetic
    anchor: list = field(default_factory=list)     # nearest traced ancestor
    known: list = field(default_factory=list)      # children are known
    kids: list = field(default_factory=list)
    top: list = field(default_factory=list)
    lists: list = field(default_factory=list)      # (owner symbol, nodes)
    siblings: list = field(default_factory=list)   # node -> (its list, position)
    parent: list = field(default_factory=list)

    @classmethod
    def build(cls, root: dict, *, unit_roots: list[dict] | None = None,
              template_depth: int = 6) -> "Model":
        model = cls(root)
        shapes = templates(unit_roots or [root])

        def add(node, anchor, path, synthetic, depth):
            n = len(model.sym)
            model.sym.append(node["symbol"])
            model.cost.append(node["cb"])
            model.remaining.append(node["remaining"])
            model.gate.append(node["args_match"] and depth <= node["allow"])
            model.refusal.append(None if synthetic or node["expanded"] or not node["admitted"]
                                 else "charged" if node["charged"] else "free")
            index = -1 if synthetic else node["index"]
            model.index.append(index)
            model.anchor.append(index if index >= 0 else anchor)
            model.kids.append([])
            children, known = node["children"], synthetic or node["expanded"]
            if not known and node["symbol"] in shapes and node["symbol"] not in path \
                    and len(path) < template_depth:
                children, known, synthetic = shapes[node["symbol"]]["children"], True, True
            model.known.append(known)
            kids = [add(c, model.anchor[n], path | {node["symbol"]}, synthetic, depth + 1)
                    for c in children]
            model.kids[n] = kids
            if kids:
                model.lists.append((node["symbol"], kids))
            return n
        model.top = [add(node, -1, frozenset(), False, 1)
                     for node in tree([dict(s, index=i) for i, s in enumerate(root["sites"])])]
        model.lists.insert(0, (None, model.top))
        model.siblings = [None] * len(model.sym)
        model.parent = [-1] * len(model.sym)
        for _owner, nodes in model.lists:
            for position, n in enumerate(nodes):
                model.siblings[n] = (nodes, position)
        for n, kids in enumerate(model.kids):
            for kid in kids:
                model.parent[kid] = n
        return model

    def run(self, cb: int | None = None, cost: dict | None = None,
            extra: dict | None = None, removed=frozenset(), calls=frozenset()):
        """cl 5.0's inline walk (c2.exe FUN_004249cb) under an edit.

        Per site, in order: the argument-count and depth gates, the budget
        test (cost <= 40 is exempt), and the running-size cap (refused above
        35000). An expanded site charges its cost (over 40 only), hands its
        body (budget / sites remaining) and charges what the body spent; the
        running size grows by the cost and by the body's spending. A site the
        trace shows refused by its call context after these tests stays
        refused, with or without the charge as traced. Returns (expanded per
        node, retained calls per callee, incomplete)."""
        cb = self.root["cb"] if cb is None else cb
        costs = self.cost if not cost else [cost.get(n, c) for n, c in enumerate(self.cost)]
        rem = self.remaining if not extra else [r + extra.get(n, 0)
                                                for n, r in enumerate(self.remaining)]
        kids, gate, refusal = self.kids, self.gate, self.refusal
        dropped = set(removed) | set(calls)
        expanded = [False] * len(costs)
        budgets = self.budgets = [None] * len(costs)
        sizes = self.sizes = [None] * len(costs)
        size = [cb]

        def walk(nodes, budget):
            left = budget
            for n in nodes:
                if n in dropped:
                    continue
                c = costs[n]
                budgets[n], sizes[n] = left, size[0]
                if not (gate[n] and (c <= SMALL_FREE or left >= c) and size[0] <= RUNNING_CAP) \
                        or refusal[n] == "free":
                    continue
                if c > SMALL_FREE:
                    left -= c
                size[0] += c
                if refusal[n] == "charged":
                    continue
                expanded[n] = True
                spent = walk(kids[n], _tdiv(left, max(1, rem[n])))
                left -= spent
                size[0] += spent
            return budget - left
        walk(self.top, budget_of(cb))
        self.expanded = expanded
        counts: dict[str, int] = {}
        incomplete = False
        reached = self.reached = [False] * len(costs)
        stack = list(self.top)
        while stack:
            n = stack.pop()
            if n in removed:
                continue
            reached[n] = True
            if expanded[n]:
                incomplete |= not self.known[n]
                stack.extend(kids[n])
            else:
                counts[self.sym[n]] = counts.get(self.sym[n], 0) + 1
        return expanded, counts, incomplete

    def mismatches(self) -> list[int]:
        """Trace indices whose replayed budget, running size or verdict
        differs from the trace."""
        expanded, _counts, _incomplete = self.run()
        sites = self.root["sites"]
        return [i for n, i in enumerate(self.index) if i >= 0 and (
            expanded[n] != sites[i]["expanded"] or self.budgets[n] != sites[i]["budget"]
            or self.sizes[n] != sites[i]["running"])]


# -- edits ----------------------------------------------------------------

@dataclass(frozen=True)
class Edit:
    kind: str                  # cb | insert | remove | callee
    owner: str | None = None   # insert/remove: list owner (None = the root)
    position: int = 0          # insert/remove: position in that list
    symbol: str | None = None  # callee
    amount: int = 0            # cb/callee delta, insert count

    def magnitude(self) -> float:
        if self.kind == "insert":
            return float(self.amount)
        if self.kind == "remove":
            return 1.0
        return abs(self.amount) / 8.0


def _apply(model: Model, edits) -> dict:
    """Edits -> replay arguments."""
    cb = model.root["cb"]
    cost: dict[int, int] = {}
    extra: dict[int, int] = {}
    removed: set[int] = set()
    calls: set[int] = set()
    for edit in edits:
        if edit.kind == "cb":
            cb += edit.amount
        elif edit.kind == "callee":
            for n, symbol in enumerate(model.sym):
                if symbol == edit.symbol:
                    cost[n] = max(1, model.cost[n] + edit.amount)
                    if cost[n] >= CANDIDACY_CB:
                        # No longer collected: one fewer remaining site for
                        # every earlier sibling.
                        calls.add(n)
                        nodes, position = model.siblings[n]
                        for earlier in nodes[:position]:
                            extra[earlier] = extra.get(earlier, 0) - 1
        else:
            for owner, nodes in model.lists:
                if owner != edit.owner or edit.position >= len(nodes):
                    continue
                if edit.kind == "insert":
                    for n in nodes[:edit.position + 1]:
                        extra[n] = extra.get(n, 0) + edit.amount
                else:
                    removed.add(nodes[edit.position])
                    for n in nodes[:edit.position]:
                        extra[n] = extra.get(n, 0) - 1
    return dict(cb=cb, cost=cost, extra=extra, removed=removed, calls=calls)


def _shapes(model: Model) -> list[tuple[str | None, list[int]]]:
    """One representative list per owner whose expansions agree in shape."""
    seen: dict = {}
    for owner, nodes in model.lists:
        shape = tuple(model.sym[n] for n in nodes)
        if owner in seen:
            if seen[owner] is not None and seen[owner][0] != shape:
                seen[owner] = None
            continue
        seen[owner] = (shape, nodes)
    return [(owner, value[1]) for owner, value in seen.items() if value is not None]


@dataclass
class Fit:
    edits: tuple
    distance: int
    incomplete: bool
    flips: list
    window: tuple | None = None
    span: tuple | None = None       # insert: first and last fitting position
    trivial: bool = False           # removes a site of a callee it must change
    deltas_by_callee: dict | None = None
    counts: tuple | None = None     # insert: fewest and most fitting sites
    deltas: tuple | None = None     # callee: fitting cost deltas

    def key(self):
        holds = (self.window is not None and self.window[0] <= 0 <= self.window[1]) \
            or all(e.kind == "cb" for e in self.edits)
        return (self.distance, len(self.edits), self.trivial, self.incomplete, not holds,
                sum(e.magnitude() for e in self.edits))


class Search:
    """The bounded hypothesis search for one root."""

    def __init__(self, model: Model, wanted: dict[str, int], *, max_insert: int = 12,
                 callee_range: int = 48, seeds: int = 40, seconds: float = 60.0):
        self.model, self.wanted = model, {k: v for k, v in wanted.items() if v}
        self.max_insert, self.callee_range, self.seeds = max_insert, callee_range, seeds
        self.deadline = time.monotonic() + seconds
        self.timed_out = False
        self.evaluated = 0
        self.base_expanded, self.base_counts, _ = model.run()
        self.base_reached = list(model.reached)
        self.shapes = dict(_shapes(model))

    def distance(self, counts: dict) -> int:
        names = set(counts) | set(self.base_counts) | set(self.wanted)
        return sum(abs(counts.get(s, 0) - self.base_counts.get(s, 0) - self.wanted.get(s, 0))
                   for s in names)

    def evaluate(self, edits) -> Fit:
        self.evaluated += 1
        args = _apply(self.model, edits)
        expanded, counts, incomplete = self.model.run(**args)
        reached, budgets = self.model.reached, self.model.budgets
        flips = [(n, budgets[n] if n not in args["calls"] else "non-candidate")
                 for n, (a, b) in enumerate(zip(expanded, self.base_expanded))
                 if a != b and (reached[n] or self.base_reached[n])
                 and n not in args["removed"]]
        trivial = any(e.kind == "remove" and self._removed_symbol(e) in self.wanted
                      for e in edits)
        changed = {k: counts.get(k, 0) - self.base_counts.get(k, 0)
                   for k in set(counts) | set(self.base_counts)
                   if counts.get(k, 0) != self.base_counts.get(k, 0)}
        return Fit(tuple(edits), self.distance(counts), incomplete, flips,
                   trivial=trivial, deltas_by_callee=changed)

    def _removed_symbol(self, edit: Edit) -> str | None:
        nodes = self.shapes.get(edit.owner, [])
        return self.model.sym[nodes[edit.position]] if edit.position < len(nodes) else None

    def window(self, fit: Fit, span: int) -> tuple | None:
        """Root-cb deltas around zero (within span) for which `fit` holds."""
        others = [e for e in fit.edits if e.kind != "cb"]
        shift = sum(e.amount for e in fit.edits if e.kind == "cb")

        def holds(delta):
            return self.evaluate(others + [Edit("cb", amount=shift + delta)]).distance == 0
        if not holds(0):
            return None
        low = high = 0
        while low > -span and holds(low - 1):
            low -= 1
        while high < span and holds(high + 1):
            high += 1
        return (low + shift, high + shift)

    def partial(self, scored: list[Fit], top: int) -> dict:
        """Per wanted callee, the smallest edits that give exactly its change
        and move no other callee away from retail: independent pieces of a
        wall no single small edit set closes."""
        out = {}
        for symbol, delta in self.wanted.items():
            pieces = []
            for fit in scored:
                got = fit.deltas_by_callee or {}
                if got.get(symbol, 0) != delta or any(k not in self.wanted for k in got):
                    continue
                if any(abs(got.get(k, 0) - self.wanted.get(k, 0)) > abs(self.wanted.get(k, 0))
                       for k in got if k != symbol):
                    continue
                pieces.append(fit)
            pieces = collapse(pieces[:top * 20])
            pieces.sort(key=lambda f: f.key()[1:])
            out[symbol] = pieces[:top]
        return out

    def singles(self) -> list[Edit]:
        model, out = self.model, []
        for owner, nodes in self.shapes.items():
            for position in range(len(nodes)):
                out.append(Edit("remove", owner, position))
                out.extend(Edit("insert", owner, position, amount=k)
                           for k in range(1, self.max_insert + 1))
        for symbol in sorted(set(model.sym)):
            low = min(c for c, s in zip(model.cost, model.sym) if s == symbol)
            out.extend(Edit("callee", symbol=symbol, amount=d)
                       for d in range(-min(low - 1, self.callee_range), self.callee_range + 1)
                       if d)
        return out

    def cb_fits(self) -> list[Fit]:
        """Root-cb-only windows."""
        cb = self.model.root["cb"]
        runs: list[list] = []
        for value in range(max(1, cb // 2), min(17600, max(cb * 2 + 200, cb + 1000))):
            fit = self.evaluate([Edit("cb", amount=value - cb)])
            if fit.distance == 0:
                if runs and runs[-1][1] == value - 1:
                    runs[-1][1] = value
                else:
                    runs.append([value, value, fit])
        out = []
        for low, high, fit in runs:
            nearest = min(range(low, high + 1), key=lambda v: abs(v - cb))
            out.append(Fit((Edit("cb", amount=nearest - cb),), 0, fit.incomplete, fit.flips,
                           window=(low - cb, high - cb)))
        return out

    def _expired(self) -> bool:
        if time.monotonic() > self.deadline:
            self.timed_out = True
        return self.timed_out

    def solve(self, *, max_edits: int = 2, top: int = 8, span: int = 60) -> dict:
        base = self.distance(self.base_counts)
        result = dict(base_distance=base, fits=[], best=[], partial={}, evaluated=0,
                      timed_out=False, total_fits=0)
        if base == 0:
            return result
        fits = self.cb_fits()
        scored: list[Fit] = []
        candidates = self.singles()
        alone: dict = {}
        for edit in candidates:
            if self._expired():
                break
            fit = self.evaluate([edit])
            alone[edit] = fit.distance
            if fit.distance == 0:
                fits.append(fit)
            elif fit.distance < base:
                scored.append(fit)
        scored.sort(key=Fit.key)
        if max_edits >= 2 and not self.timed_out:
            pool = [f.edits[0] for f in scored[:self.seeds]]
            pool += [f.edits[0] for f in fits if f.edits[0].kind != "cb"][:self.seeds]
            for a, b in itertools.combinations(dict.fromkeys(pool), 2):
                if self._expired():
                    break
                if (a.kind, a.owner, a.position, a.symbol) == (b.kind, b.owner, b.position, b.symbol):
                    continue
                fit = self.evaluate([a, b])
                if fit.distance == 0:
                    fits.append(fit)
                elif fit.distance < min(alone.get(a, base), alone.get(b, base)):
                    scored.append(fit)          # both edits matter
            # A seed plus the root's own mass: an edit that adds or drops
            # sites moves the caller's cost too.
            reach = min(self.model.root["cb"] // 4, 300)
            scored.sort(key=Fit.key)
            for seed in [f for f in scored if len(f.edits) == 1][:self.seeds]:
                for delta in sorted(range(-reach, reach + 1), key=abs):
                    if not delta:
                        continue
                    if self._expired():
                        break
                    fit = self.evaluate([seed.edits[0], Edit("cb", amount=delta)])
                    if fit.distance == 0:
                        fits.append(fit)
                        break
        if max_edits >= 3 and not self.timed_out \
                and not any(f.distance == 0 and len(f.edits) < 3 for f in fits):
            # Greedy third edit on the best pairs: a wall that decomposes into
            # independent pieces needs one edit per piece.
            scored.sort(key=Fit.key)
            for pair in [f for f in scored if len(f.edits) == 2][:8]:
                for edit in candidates:
                    if self._expired():
                        break
                    if any((edit.kind, edit.owner, edit.position, edit.symbol)
                           == (e.kind, e.owner, e.position, e.symbol) for e in pair.edits):
                        continue
                    fit = self.evaluate(list(pair.edits) + [edit])
                    if fit.distance == 0:
                        fits.append(fit)
                    elif fit.distance < pair.distance:
                        scored.append(fit)
        unique: dict = {}
        for fit in fits:
            unique.setdefault(frozenset(fit.edits), fit)
        fits = list(unique.values())
        for fit in fits:
            if fit.window is None:
                fit.window = self.window(fit, span)
        fits = collapse(fits)
        fits.sort(key=Fit.key)
        kept: list[Fit] = []
        for fit in fits:
            # Drop fits that only add a further edit to a smaller fit.
            if not any(set(k.edits) < set(fit.edits) for k in kept):
                kept.append(fit)
        scored.sort(key=Fit.key)
        best = collapse(scored[:top * 20])
        best.sort(key=Fit.key)
        result.update(fits=kept[:top], best=best[:top] if not kept else [],
                      partial=self.partial(scored, top) if not kept else {},
                      evaluated=self.evaluated, timed_out=self.timed_out,
                      total_fits=len(kept))
        return result


def collapse(fits: list[Fit]) -> list[Fit]:
    """Merge fits that differ only along one dimension of one edit: an
    insertion's position in its list, then its count, then a callee's cost
    delta. Only fits with the same outcome merge."""
    def runs(items, dimension, key, done):
        groups: dict = {}
        out = []
        for fit in items:
            varying = [e for e in fit.edits if dimension(e) is not None]
            if len(varying) != 1:
                out.append(fit)
                continue
            edit = varying[0]
            rest = tuple(sorted((e for e in fit.edits if e is not edit), key=repr))
            outcome = (fit.distance, tuple(sorted((fit.deltas_by_callee or {}).items())))
            group = (edit.kind, edit.owner, edit.symbol, rest, outcome, key(edit, fit))
            groups.setdefault(group, []).append((dimension(edit), fit))
        for members in groups.values():
            members.sort(key=lambda m: m[0])
            run: list = []
            for value, fit in members:
                if run and value != run[-1][0] + 1:
                    out.append(done(run))
                    run = []
                run.append((value, fit))
            out.append(done(run))
        return out

    fits = runs(fits, lambda e: e.position if e.kind == "insert" else None,
                lambda edit, fit: edit.amount,
                lambda run: dataclasses.replace(run[0][1], span=(run[0][0], run[-1][0])))
    fits = runs(fits, lambda e: e.amount if e.kind == "insert" else None,
                lambda edit, fit: (edit.position, fit.span),
                lambda run: dataclasses.replace(run[0][1], counts=(run[0][0], run[-1][0])))
    return runs(fits, lambda e: e.amount if e.kind == "callee" else None,
                lambda edit, fit: edit.amount > 0,
                lambda run: dataclasses.replace(
                    min((f for _v, f in run),
                        key=lambda f: sum(abs(e.amount) for e in f.edits if e.kind == "callee")),
                    deltas=(run[0][0], run[-1][0])))


def what_if(model: Model, *, cb: int | None = None, adds=(), removes=(), costs=()) -> dict:
    """One manual what-if over trace indices: `adds` (index, count) inserts
    free sites after that site in its own list, `removes` drops sites,
    `costs` (text, cb) sets every callee whose name contains text."""
    by_index = {i: n for n, i in enumerate(model.index) if i >= 0}
    extra: dict[int, int] = {}
    removed: set[int] = set()
    calls: set[int] = set()
    cost: dict[int, int] = {}

    def shift(n, amount, inclusive):
        nodes, position = model.siblings[n]
        for earlier in nodes[:position + (1 if inclusive else 0)]:
            extra[earlier] = extra.get(earlier, 0) + amount
    for index, count in adds:
        if index not in by_index:
            raise ValueError(f"no trace site {index}")
        shift(by_index[index], count, True)
    for index in removes:
        if index not in by_index:
            raise ValueError(f"no trace site {index}")
        removed.add(by_index[index])
        shift(by_index[index], -1, False)
    for needle, value in costs:
        for n, symbol in enumerate(model.sym):
            if needle in symbol:
                cost[n] = value
                if value >= CANDIDACY_CB:
                    calls.add(n)
                    shift(n, -1, False)
    base_expanded, base_counts, _ = model.run()
    base_budgets, base_reached = list(model.budgets), list(model.reached)
    expanded, counts, incomplete = model.run(cb=cb, cost=cost, extra=extra,
                                             removed=removed, calls=calls)
    flips = [dict(index=model.index[n], anchor=model.anchor[n], depth=depth(model, n),
                  symbol=model.sym[n], cb=cost.get(n, model.cost[n]),
                  before=base_budgets[n], after=model.budgets[n],
                  verdict="expands" if a else "refused")
             for n, (a, b) in enumerate(zip(expanded, base_expanded))
             if a != b and n not in removed and (model.reached[n] or base_reached[n])]
    changed = {k: counts.get(k, 0) - base_counts.get(k, 0)
               for k in set(counts) | set(base_counts)
               if counts.get(k, 0) != base_counts.get(k, 0)}
    return dict(flips=flips, calls=changed, incomplete=incomplete)


def depth(model: Model, n: int) -> int:
    level = 1
    while model.parent[n] >= 0:
        n, level = model.parent[n], level + 1
    return level


# -- retail's targets -----------------------------------------------------

def call_counts(unit: str, symbol: str) -> tuple[Counter, Counter]:
    """(ours, retail) call counts per callee of one function, from the
    normalized pair `walls diagnose` reads."""
    from gruntz.delink.coffx import Obj
    from gruntz.walls.diagnose import (
        NORM, _call_targets, _find_function, _jump_table_bytes, _skeleton,
    )
    base = NORM / "base" / f"{unit}.obj"
    target = next((p for p in (NORM / "target" / f"{unit}.c.obj",
                               NORM / "target" / f"{unit}.obj") if p.is_file()), None)
    if not base.is_file() or target is None:
        raise SystemExit(f"inline-solve: normalized pair missing for {unit} - run `gruntz build`")
    counts = []
    for path in (base, target):
        payload, rel, _size = _find_function(Obj(path), symbol)
        if payload is None:
            raise SystemExit(f"inline-solve: {path.name} does not define {symbol}")
        *_rest, asm = _skeleton(payload, rel, data=_jump_table_bytes(rel, symbol))
        counts.append(Counter(n for n, _a in _call_targets(rel, asm, symbol)))
    return counts[0], counts[1]


# -- rendering ------------------------------------------------------------

class Renderer:
    def __init__(self, model: Model):
        self.model = model
        self.names: dict[str, str] = {}
        try:
            from gruntz.verify.fingerprints import demangle_map
            self.names = demangle_map(set(model.sym) | {model.root["symbol"]})
        except Exception:
            pass

    def name(self, symbol: str | None) -> str:
        if symbol is None:
            return "the root"
        return self.names.get(symbol) or symbol

    def site(self, n: int) -> str:
        m = self.model
        index = m.index[n]
        where = f"#{index}" if index >= 0 else f"inside #{m.anchor[n]}"
        return f"{self.name(m.sym[n])} ({where}, cb {m.cost[n]})"

    def edit(self, edit: Edit, fit: Fit | None = None) -> str:
        m = self.model
        if edit.kind == "cb":
            return f"root cb {edit.amount:+d}"
        if edit.kind == "callee":
            span = f" (any of {fit.deltas[0]:+d}..{fit.deltas[1]:+d})" if fit and fit.deltas else ""
            return f"cost of {self.name(edit.symbol)} {edit.amount:+d}{span}"
        nodes = dict(_shapes(m)).get(edit.owner, [])
        owner = "the root's list" if edit.owner is None else f"{self.name(edit.owner)}'s body"
        if edit.kind == "remove":
            return f"remove {self.site(nodes[edit.position])} from {owner}"
        count = (f"{fit.counts[0]}..{fit.counts[1]}" if fit and fit.counts
                 else str(edit.amount))
        first = nodes[edit.position]
        text = f"add {count} free site(s) to {owner} after {self.site(first)}"
        if fit and fit.span and fit.span[1] != fit.span[0]:
            text += f" ... or as late as after {self.site(nodes[fit.span[1]])}"
        return text


def fit_json(fit: Fit, render: Renderer) -> dict:
    return dict(edits=[render.edit(e, fit) for e in fit.edits], distance=fit.distance,
                window=fit.window, incomplete=fit.incomplete, trivial=fit.trivial,
                calls=fit.deltas_by_callee)


def _print_fit(fit: Fit, render: Renderer) -> None:
    tags = []
    if fit.window is not None:
        tags.append(f"holds for root cb {fit.window[0]:+d}..{fit.window[1]:+d}")
    if fit.incomplete:
        tags.append("incomplete: a newly expanded site's children are unknown")
    if fit.trivial:
        tags.append("trivial: removes a call whose count must change")
    print("  - " + "; ".join(render.edit(e, fit) for e in fit.edits))
    if fit.distance:
        print(f"      leaves {fit.distance} call(s) off; changes "
              + ", ".join(f"{render.name(k)} {v:+d}"
                          for k, v in sorted((fit.deltas_by_callee or {}).items())))
    if tags:
        print("      " + "; ".join(tags))


# -- driver ---------------------------------------------------------------

def _root(report: dict, symbol: str) -> dict:
    roots = [r for r in report["roots"] if r["symbol"] == symbol]
    if not roots:
        raise SystemExit(f"inline-solve: the trace of {report['unit']} has no root {symbol} "
                         "(an inline body never visited as a root?)")
    return roots[0]


def check(units: list[str]) -> int:
    """Trace each unit and replay every root; report the roots whose replay
    differs from the trace."""
    from gruntz.walls import inline_trace
    total = failed = sites = 0
    for unit in units:
        try:
            report = inline_trace.load(unit)
        except SystemExit as error:
            print(f"{unit}: {error}")
            failed += 1
            continue
        bad = []
        for root in report["roots"]:
            if not root["sites"]:
                continue
            total += 1
            sites += len(root["sites"])
            off = Model.build(root, unit_roots=report["roots"]).mismatches()
            if off:
                bad.append((root["symbol"], off))
        failed += len(bad)
        print(f"{unit}: {len(bad)} of {sum(1 for r in report['roots'] if r['sites'])} "
              "root(s) differ from the replay")
        for symbol, off in bad[:5]:
            print(f"    {symbol}: trace sites {off[:8]}")
    print(f"replayed {total} roots ({sites} sites); {failed} differ")
    return 1 if failed else 0


def solve(token: str, args) -> int:
    from gruntz.walls.diagnose import _locate
    from gruntz.walls import inline_trace

    located, why = _locate(token)
    if located is None:
        raise SystemExit(f"inline-solve: {why}")
    report = inline_trace.load(located.unit, refresh=args.refresh)
    root = _root(report, located.name)
    model = Model.build(root, unit_roots=report["roots"])
    render = Renderer(model)
    if args.what_if:
        adds = [tuple(int(x) for x in a.split(":")) for a in args.add]
        costs = [(c.rsplit(":", 1)[0], int(c.rsplit(":", 1)[1])) for c in args.cost]
        out = what_if(model, cb=args.cb, adds=adds, removes=args.remove, costs=costs)
        if args.json:
            json.dump(out, sys.stdout, indent=1)
            print()
            return 0
        for flip in out["flips"]:
            print(f"  #{flip['index'] if flip['index'] >= 0 else 'inside #%d' % flip['anchor']}"
                  f" depth {flip['depth']} {render.name(flip['symbol'])} cb {flip['cb']}: "
                  f"budget {flip['before']} -> {flip['after']}, now {flip['verdict']}")
        print("  calls: " + (", ".join(f"{render.name(k)} {v:+d}"
                                       for k, v in sorted(out["calls"].items())) or "unchanged")
              + ("  (incomplete)" if out["incomplete"] else ""))
        return 0
    off = model.mismatches()
    ours, retail = call_counts(located.unit, located.name)
    traced = set(model.sym)
    wanted = {s: retail[s] - ours[s] for s in traced if retail[s] != ours[s]}
    _expanded, replayed, _incomplete = model.run()
    unexplained = {s: (ours[s], replayed.get(s, 0)) for s in traced
                   if ours[s] != replayed.get(s, 0)}
    untraced = {s: retail[s] - ours[s] for s in set(ours) | set(retail)
                if s not in traced and retail[s] != ours[s]}
    search = Search(model, wanted, max_insert=args.max_insert, seconds=args.seconds)
    result = search.solve(max_edits=args.max_edits, top=args.top) if wanted else None
    if args.json:
        json.dump(dict(root=root["symbol"], unit=located.unit, cb=root["cb"],
                       replay_mismatches=off, wanted=wanted, untraced=untraced,
                       unexplained=unexplained,
                       fits=[fit_json(f, render) for f in (result or {}).get("fits", [])],
                       best=[fit_json(f, render) for f in (result or {}).get("best", [])],
                       partial={k: [fit_json(f, render) for f in v]
                                for k, v in (result or {}).get("partial", {}).items()},
                       evaluated=(result or {}).get("evaluated", 0),
                       timed_out=(result or {}).get("timed_out", False)),
                  sys.stdout, indent=1)
        print()
        return 0
    print(f"[inline-solve] {render.name(root['symbol'])}  [{located.unit}]  "
          f"rva 0x{located.rva:06x}  cb {root['cb']}  budget {budget_of(root['cb'])}  "
          f"{len(root['sites'])} traced sites")
    if off:
        print(f"  WARNING: the replay differs from the trace at sites {off[:8]}; "
              "the fits below rest on an unreproduced model")
    for symbol, (have, model_calls) in sorted(unexplained.items()):
        print(f"  note: our object calls {render.name(symbol)} {have}x, the replay "
              f"retains {model_calls} (calls outside the traced sites)")
    if untraced:
        print("  call-set delta outside the traced sites (no budget edit reaches it): "
              + ", ".join(f"{render.name(k)} {v:+d}" for k, v in sorted(untraced.items())))
    if not wanted:
        print("  traced callees: retail keeps the same calls as ours - no inline wall here")
        return 0
    print("  retail minus ours, traced callees: "
          + ", ".join(f"{render.name(k)} {v:+d}" for k, v in sorted(wanted.items())))
    if result["fits"]:
        print(f"  fits ({result['total_fits']}):")
        for fit in result["fits"]:
            _print_fit(fit, render)
    else:
        print("  no fit within the search bounds; nearest:")
        for fit in result["best"]:
            _print_fit(fit, render)
        for symbol, pieces in result["partial"].items():
            if pieces:
                print(f"  piece for {render.name(symbol)}:")
                for fit in pieces[:3]:
                    _print_fit(fit, render)
    print(f"  ({result['evaluated']} replays"
          + (", time limit reached" if result["timed_out"] else "") + ")")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls inline-solve",
                                 description=__doc__.split("\n\n")[1])
    ap.add_argument("function", nargs="?", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--check", nargs="+", metavar="UNIT",
                    help="replay every traced root of these units against their trace")
    ap.add_argument("--check-all", action="store_true", help="--check over every unit")
    ap.add_argument("--max-edits", type=int, default=2, choices=(1, 2, 3))
    ap.add_argument("--max-insert", type=int, default=12)
    ap.add_argument("--top", type=int, default=8)
    ap.add_argument("--seconds", type=float, default=60.0)
    ap.add_argument("--refresh", action="store_true", help="re-trace the unit")
    ap.add_argument("--what-if", action="store_true")
    ap.add_argument("--cb", type=int)
    ap.add_argument("--add", action="append", default=[], metavar="INDEX:COUNT")
    ap.add_argument("--remove", action="append", default=[], type=int, metavar="INDEX")
    ap.add_argument("--cost", action="append", default=[], metavar="TEXT:CB")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    if args.check_all:
        from gruntz.graph.emit import load_units
        return check([u["unit"] for u in load_units()[1]])
    if args.check:
        return check(args.check)
    if not args.function:
        ap.error("name a function, or pass --check/--check-all")
    return solve(args.function, args)


if __name__ == "__main__":
    raise SystemExit(main())
