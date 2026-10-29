#!/usr/bin/env python3
"""gruntz.walls.inline_reach - force retail's inline decisions and score.

    gruntz walls inline-reach <rva|name> [--max-rules N] [--json]
    gruntz walls inline-reach --all [--jobs N]     every function below MAX 100

Classifies an inline/call-set wall by forcing it. The unit compiles through
the traced c2.exe (`gruntz walls inline-trace`) with a rule file that
overrides chosen decisions at C2's budget compare: `E` takes the admit path,
`K` the decline path. An empty rule list must reproduce the build object (the
inertness gate). Each round aligns our call stream with retail's, proposes rules
at the differences (expand the site of a call retail lacks; keep as a call an
expansion where retail calls), compiles each, and keeps the one that most
shrinks the call-stream difference (then the score against the canonicalized
retail object), until the traced calls agree or no rule helps.

Verdicts:
- reached: retail's decisions alone make the function exact; the wall is an
  inline-budget wall and each rule's `need` is the source target;
- partial: forcing moves the score toward retail; the rest is another class;
- not inline: the traced callees' calls already agree, or forcing does not
  improve the score.

Forced objects are diagnostics, never source or banked state. A rule's need
is the budget change at that site that would make C2 decide the same way:
`gruntz walls inline-solve` searches the source edits that produce it.
Work files go to build/inline-reach/<unit>/<rva>/; `--all` writes
build/inline-reach/all.jsonl and a verdict summary.
"""

from __future__ import annotations

import argparse
import difflib
import json
import sys
from collections import Counter
from pathlib import Path

from gruntz.core.paths import BUILD, REPO
from gruntz.walls.inline_model import SMALL_FREE

OUT = BUILD / "inline-reach"


def call_stream(path: Path, symbol: str) -> list[str]:
    """The function's call referents in address order."""
    from gruntz.delink.coffx import Obj
    from gruntz.walls.diagnose import _call_targets, _find_function, _jump_table_bytes, _skeleton

    payload, rel, _size = _find_function(Obj(path), symbol)
    if payload is None:
        raise SystemExit(f"inline-reach: {path.name} does not define {symbol}")
    *_rest, asm = _skeleton(payload, rel, data=_jump_table_bytes(rel, symbol))
    return [name for name, _offset in _call_targets(rel, asm, symbol)]


def _forceable(site: dict) -> bool:
    """Whether the site reaches C2's budget compare, where the shim forces."""
    return site["args_match"] and site["depth"] <= site["allow"]


def candidates(root: dict, ours: list[str], retail: list[str], forced: set[int],
               limit: int) -> list[tuple[int, str]]:
    """Rules worth trying next, as (trace index, E|K).

    A call retail lacks becomes E on a site of that callee that stayed a
    call: first the one with the same per-callee ordinal, then the rest
    (block layout need not emit a callee's sites in trace order). A call
    retail adds becomes K on the expansions of that callee, nearest the
    expected ordinal first."""
    sites = root["sites"]
    traced = {s["symbol"] for s in sites}
    ours = [name for name in ours if name in traced]
    retail = [name for name in retail if name in traced]
    kept: dict[str, list[int]] = {}
    for index, site in enumerate(sites):
        if not site["expanded"]:
            kept.setdefault(site["symbol"], []).append(index)
    out: list[tuple[int, str]] = []
    later: list[tuple[int, str]] = []

    def add(index, action, queue=out):
        if index not in forced and _forceable(sites[index]) \
                and (index, action) not in out and (index, action) not in later:
            queue.append((index, action))
    matcher = difflib.SequenceMatcher(a=ours, b=retail, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == "equal":
            continue
        for i in range(i1, i2):
            ordinal = ours[:i].count(ours[i])
            indices = kept.get(ours[i], [])
            if ordinal < len(indices):
                add(indices[ordinal], "E")
            for index in indices:
                add(index, "E", later)
        for name in retail[j1:j2]:
            ordinal = ours[:i1].count(name)
            calls = kept.get(name, [])
            expanded = [i for i, site in enumerate(sites)
                        if site["symbol"] == name and site["expanded"]]
            # An expansion with `ordinal` kept calls of the callee before it
            # sits where retail's extra call is.
            for index in sorted(expanded,
                                key=lambda i: abs(sum(k < i for k in calls) - ordinal)):
                add(index, "K")
    return (out + later)[:limit]


def distance(ours: list[str], retail: list[str], traced: set[str]) -> int:
    surplus, missing = divergence(ours, retail, traced)
    return len(surplus) + len(missing)


def divergence(ours: list[str], retail: list[str], traced: set[str]) -> tuple[list, list]:
    """(surplus, missing): traced callees we call and retail does not, and
    the reverse."""
    matcher = difflib.SequenceMatcher(a=ours, b=retail, autojunk=False)
    surplus, missing = [], []
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag in ("replace", "delete"):
            surplus += [name for name in ours[i1:i2] if name in traced]
        if tag in ("replace", "insert"):
            missing += [name for name in retail[j1:j2] if name in traced]
    return surplus, missing


def need(site: dict, action: str) -> str:
    cb, budget = site["cb"], site["budget"]
    if action == "E":
        if site["running"] > 35000:
            return "running size over 35000 at this site"
        if cb <= SMALL_FREE or budget >= cb:
            return "budget already suffices; C2 refused it from the call's context"
        return f"budget +{cb - budget} at this site (cost {cb}, budget {budget})"
    if cb <= SMALL_FREE:
        return f"cost {cb} is budget-exempt: only a body over 40 or a refusal keeps this call"
    return f"budget -{budget - cb + 1} at this site (cost {cb}, budget {budget})"


def reach(token: str, max_rules: int, width: int = 8, pair_width: int = 4) -> dict:
    from gruntz import graph
    from gruntz.compare.canonicalize import canonicalize_coff
    from gruntz.permute.tu_state_noise import objdiff_scores
    from gruntz.walls import inline_trace
    from gruntz.walls.diagnose import _locate

    located, why = _locate(token)
    if located is None:
        raise SystemExit(f"inline-reach: {why}")
    unit, symbol = located.unit, located.name
    src, flags = inline_trace.unit_source(unit)
    work = OUT / unit / f"0x{located.rva:06x}"
    work.mkdir(parents=True, exist_ok=True)
    retail = work / "retail.obj"
    retail.write_bytes(canonicalize_coff(
        (REPO / "build/objdiff/target-new" / f"{unit}.c.obj").read_bytes()).data)
    retail_calls = call_stream(retail, symbol)
    built = REPO / graph.BASE_DIR / f"{unit}.obj"

    def run(rules: list[str], tag: str) -> tuple[float, list[str], dict]:
        raw = work / f"{tag}.obj"
        text = inline_trace.compile_traced(src, flags, raw, rules=rules)
        root = next((r for r in inline_trace.parse(text)["roots"] if r["symbol"] == symbol), None)
        if root is None:
            raise SystemExit(f"inline-reach: the trace has no root {symbol}")
        canonical = work / f"{tag}.canonical.obj"
        canonical.write_bytes(canonicalize_coff(raw.read_bytes()).data)
        scores, _sizes, _counts, log = objdiff_scores(retail, canonical, symbol)
        if symbol not in scores:
            raise SystemExit(f"inline-reach: objdiff did not score {symbol}:\n{log[-1000:]}")
        return scores[symbol], call_stream(canonical, symbol), root

    score, ours, root = run([], "baseline")
    if inline_trace._comparable((work / "baseline.obj").read_bytes()) != \
            inline_trace._comparable(built.read_bytes()):
        raise SystemExit(f"inline-reach: the unforced compile differs from {built}; "
                         f"rebuild the unit (`gruntz match {unit}`) first")
    report = dict(function=symbol, unit=unit, rva=f"0x{located.rva:06x}", baseline=score,
                  score=score, rules=[], verdict=None)
    if not root["sites"]:
        report["verdict"] = "not inline: no candidate sites"
        return report
    rules: list[str] = []
    traced = {s["symbol"] for s in root["sites"]}
    gap = distance(ours, retail_calls, traced)
    tried = 0

    def attempt(base: list[str], at: dict, index: int, action: str):
        nonlocal tried
        site = at["sites"][index]
        callee = site["symbol"]
        occurrence = [i for i, s in enumerate(at["sites"])
                      if s["symbol"] == callee and _forceable(s)].index(index) + 1
        rule = f"{symbol}\t*\t{callee}\t{occurrence}\t{action}"
        tried += 1
        new_score, new_ours, new_root = run(base + [rule], f"try{tried}")
        new_gap = distance(new_ours, retail_calls,
                           traced | {s["symbol"] for s in new_root["sites"]})
        step = dict(callee=callee, occurrence=occurrence, action=action, owner=site["owner"],
                    depth=site["depth"], need=need(site, action), score=new_score)
        return (new_gap, -new_score), base + [rule], [step], new_score, new_ours, new_root

    def forced_in(at: dict) -> set[int]:
        return {index for index, site in enumerate(at["sites"]) if site.get("forced")}

    while gap and len(rules) < max_rules:
        trials = [attempt(rules, root, index, action) for index, action in
                  candidates(root, ours, retail_calls, forced_in(root), width)]
        better = [t for t in trials if t[0] < (gap, -score)]
        if not better:
            # An expansion can bring its own nested decision along: try each
            # candidate with the follow-ups its own trace suggests.
            for first in trials[:pair_width]:
                _key, base, steps, _s, first_ours, first_root = first
                for index, action in candidates(first_root, first_ours, retail_calls,
                                                forced_in(first_root), pair_width):
                    second = attempt(base, first_root, index, action)
                    if second[0] < (gap, -score):
                        better.append((second[0], second[1], steps + second[2], *second[3:]))
        if not better:
            break
        _key, rules, steps, score, ours, root = min(better, key=lambda t: t[0])
        traced |= {s["symbol"] for s in root["sites"]}
        gap = distance(ours, retail_calls, traced)
        report["rules"].extend(steps)
    report["compiles"] = tried + 1
    report["score"] = score
    traced = {s["symbol"] for s in root["sites"]}
    surplus, missing = divergence(ours, retail_calls, traced)
    if score >= 100.0:
        report["verdict"] = ("inline-budget wall: retail's decisions make it exact" if rules
                             else "exact without forcing")
    elif not report["rules"]:
        report["verdict"] = ("not inline: the traced calls already agree with retail"
                             if not surplus and not missing else
                             "not inline: no forced decision changes the calls")
    elif score > report["baseline"]:
        report["verdict"] = "partial: forcing moves toward retail; the rest is another class"
    else:
        report["verdict"] = "not inline: forcing does not improve the score"
    report["remaining"] = dict(surplus=surplus, missing=missing)
    (work / "reach.json").write_text(json.dumps(report, indent=1) + "\n")
    return report


def _reach_row(job: tuple[str, int, int]) -> dict:
    rva, max_rules, width = job
    try:
        return reach(rva, max_rules, width)
    except SystemExit as error:
        return dict(rva=rva, verdict=f"error: {error}")
    except Exception as error:  # one row's failure must not stop the batch
        return dict(rva=rva, verdict=f"error: {type(error).__name__}: {error}")


def reach_all(jobs: int, max_rules: int, width: int) -> int:
    """`reach` over every claimed function whose MAX is below 100."""
    from concurrent.futures import ProcessPoolExecutor
    from gruntz.core.paths import CONFIG

    rows = []
    for line in (CONFIG / "match_baseline.tsv").read_text().splitlines():
        fields = line.split("\t")
        if line.startswith("#") or len(fields) < 9:
            continue
        if float(fields[2]) < 100.0:
            rows.append(fields[6])
    OUT.mkdir(parents=True, exist_ok=True)
    out = OUT / "all.jsonl"
    verdicts: Counter = Counter()
    with out.open("w") as sink, ProcessPoolExecutor(jobs) as pool:
        for report in pool.map(_reach_row, [(rva, max_rules, width) for rva in rows]):
            sink.write(json.dumps(report) + "\n")
            sink.flush()
            verdicts[report["verdict"].split(":")[0]] += 1
    print(f"{len(rows)} functions below MAX 100 -> {out}")
    for verdict, n in verdicts.most_common():
        print(f"  {n:5d}  {verdict}")
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls inline-reach",
                                 description=__doc__.split("\n\n")[1])
    ap.add_argument("function", nargs="?", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--all", action="store_true", help="every function below MAX 100")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--max-rules", type=int, default=24)
    ap.add_argument("--width", type=int, default=8, help="candidate rules compiled per round")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    if args.all:
        return reach_all(args.jobs, args.max_rules, args.width)
    if not args.function:
        ap.error("name a function, or pass --all")
    report = reach(args.function, args.max_rules, args.width)
    if args.json:
        json.dump(report, sys.stdout, indent=1)
        print()
        return 0
    print(f"[inline-reach] {report['function']}  [{report['unit']}]  rva {report['rva']}")
    print(f"  score {report['baseline']:.2f} -> {report['score']:.2f}: {report['verdict']}")
    groups: list[list[dict]] = []
    for rule in report["rules"]:
        key = (rule["action"], rule["callee"], rule["owner"], rule["depth"])
        reason = rule["need"].split(" (")[0] if rule["need"].startswith("running") else rule["need"]
        if groups and groups[-1][0][0] == key and groups[-1][0][1] == reason:
            groups[-1].append(rule)
        else:
            groups.append([(key, reason), rule])
    for (key, reason), *rules in groups:
        action, callee, owner, depth = key
        which = (f"#{rules[0]['occurrence']}" if len(rules) == 1 else
                 f"{len(rules)} sites #{rules[0]['occurrence']}..#{rules[-1]['occurrence']}")
        print(f"  {action} {callee} {which} (in {owner}, depth {depth}) -> "
              f"{rules[-1]['score']:.2f}; {reason}")
    remaining = report.get("remaining") or {}
    for side, label in (("surplus", "ours only"), ("missing", "retail only")):
        counts = Counter(remaining.get(side, []))
        if counts:
            print(f"  calls still differing, {label}: "
                  + ", ".join(f"{name} x{n}" for name, n in counts.most_common()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
