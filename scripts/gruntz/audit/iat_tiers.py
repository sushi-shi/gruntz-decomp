#!/usr/bin/env python3
"""iat_tiers.py - TRANSITIVE import reachability: how much of the engine leaves the process?

This walks the CALL GRAPH from every function and asks: following every edge, do we ever
reach an IAT thunk? That is a different question from `recomp_islands`, which classifies a
function by its OWN relocations.

It was written to size a snapshot-and-replay oracle. **That oracle has been removed**
(2026-07-28 user ruling - it launched game windows; see `recomp/README.md`), so the tiers
no longer gate anything. The census is kept because it is a durable STATIC FACT about the
binary, useful well beyond its original purpose: it establishes that two thirds of the
engine never touches the IAT at all, because the CRT and MFC are statically linked here.
That tells you which code is pure computation over its own data - which is exactly the code
whose behaviour is determined by the bytes and nothing else.

The answer is much better than the direct classification suggests, for one structural
reason: **the CRT and MFC are STATICALLY LINKED in this binary.** malloc, memcpy, sprintf,
CString, the CObList/CMap containers - all of that lives inside the image and is NOT an
import. The IAT is reached only by genuine Win32/DirectX calls: DirectDraw, DirectSound,
GDI, kernel32 file I/O, user32.

  CLEAN    the whole transitive closure is import-free. Snapshot, restore, call, diff -
           no stubs of any kind needed.
  UNKNOWN  the closure reaches a statically-linked CRT/MFC entry point whose body is not
           in our delinked target set, so we cannot see past it. Dominated by allocation
           (operator new/delete, CString, container inserts). Under a RESTORED heap these
           are deterministic - the allocator replays its own snapshotted free lists - so
           most of this tier is reachable in practice; the ~200 distinct entry points are
           exactly the stub surface.
  IMPORT   the closure reaches a real Win32/DirectX import. Reachable only by STUBBING the
           IAT: record each import call (name, args, result, bytes written) during capture,
           replay the recorded sequence, and make the CALL SEQUENCE part of the compared
           observable alongside memory. That also catches "called DirectDraw with the wrong
           rect", which a pure memory diff would miss.

TWO PARSING TRAPS, both of which produced confident wrong numbers before being caught:

1. **A reloc is a call edge only when it patches a call/jmp.** Keying off the symbol NAME
   instead (treating anything that is not `?name@@3` as a callee) misclassifies every
   C-style global (`_g_gameReg`), vtable (`??_7...@@6B@`) and string literal (`??_C@...`)
   as a function call. That inflated UNKNOWN to 39%.
2. **llvm-objdump's mnemonic is tab column 1, not the last column.** Taking the last column
   grabs the OPERAND, so no mnemonic ever matched "call", every edge set came out empty,
   and the transitive walk silently degenerated to a direct-reloc check - which still
   printed a plausible-looking 88.4% CLEAN. A metric that cannot fail loudly will lie.

    python -m gruntz.audit.iat_tiers             # tier counts + the CLEAN worklist
    python -m gruntz.audit.iat_tiers --summary   # counts only
    python -m gruntz.audit.iat_tiers --stubs     # the CRT/MFC stub surface, by call count
"""
import argparse
import collections
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
TARGET = REPO / "build/objdiff/target"

SYM = re.compile(r"^[0-9a-f]+ <(.+)>:")
RELOC = re.compile(r"IMAGE_REL_\w+\s+(\S+)")
IMP = re.compile(r"^__imp_")
CALL_MNEM = ("call", "calll", "jmp", "jmpl")


def build_graph():
    """fn -> callees, plus the set of functions with a DIRECT import reloc."""
    objs = sorted(TARGET.rglob("*.obj")) + sorted(TARGET.rglob("*.o"))
    if not objs:
        raise SystemExit("[iat-tiers] no target objects - run `gruntz build`")
    edges = collections.defaultdict(set)
    defined, direct_imp = set(), set()
    for o in objs:
        try:
            out = subprocess.run(["llvm-objdump", "-dr", str(o)],
                                 capture_output=True, text=True, timeout=180).stdout
        except Exception as exc:                      # a single unreadable obj is not fatal
            print("[iat-tiers] skip %s: %s" % (o.name, exc), file=sys.stderr)
            continue
        cur, last_mnem = None, ""
        for line in out.split("\n"):
            m = SYM.match(line)
            if m:
                cur, last_mnem = m.group(1), ""
                defined.add(cur)
                continue
            if cur is None:
                continue
            r = RELOC.search(line)
            if r:
                tgt = r.group(1)
                if IMP.match(tgt):
                    direct_imp.add(cur)
                elif last_mnem in CALL_MNEM:          # trap 1: only call/jmp is an edge
                    edges[cur].add(tgt)
                continue
            parts = line.split("\t")
            if len(parts) >= 2:
                last_mnem = parts[1].strip()          # trap 2: mnemonic is column 1
    return edges, defined, direct_imp


def classify(edges, defined, direct_imp):
    memo = {}

    def walk(f, stack):
        if f in memo:
            return memo[f]
        if f in direct_imp:
            memo[f] = "IMPORT"
            return "IMPORT"
        if f not in defined:
            return "UNKNOWN"          # no body in our set - deliberately NOT memoized
        if f in stack:
            return "CLEAN"            # a cycle contributes nothing by itself
        stack.add(f)
        verdict = "CLEAN"
        for c in edges.get(f, ()):
            r = walk(c, stack)
            if r == "IMPORT":
                verdict = "IMPORT"
                break
            if r == "UNKNOWN":
                verdict = "UNKNOWN"
        stack.discard(f)
        memo[f] = verdict
        return verdict

    sys.setrecursionlimit(200000)
    return {f: walk(f, set()) for f in sorted(defined)}


def scores():
    rep = json.loads((REPO / "build/objdiff/report.json").read_text())
    out = {}
    for u in rep["units"]:
        for fn in u.get("functions", []):
            out[fn["name"]] = (fn.get("fuzzy_match_percent", 0.0),
                               int(fn.get("size", "0") or 0))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--summary", action="store_true", help="tier counts only")
    ap.add_argument("--stubs", action="store_true",
                    help="the CRT/MFC entry points that would have to be stubbed")
    ap.add_argument("--min-size", type=lambda s: int(s, 0), default=0x40,
                    help="worklist floor in bytes (default 0x40)")
    a = ap.parse_args()

    edges, defined, direct_imp = build_graph()
    verdicts = classify(edges, defined, direct_imp)
    tiers = collections.Counter(verdicts.values())
    total = len(defined)

    print("transitive IAT reachability over %d functions "
          "(%d have a DIRECT import reloc):" % (total, len(direct_imp)))
    for tier in ("CLEAN", "UNKNOWN", "IMPORT"):
        n = tiers.get(tier, 0)
        print("   %-8s %5d  (%4.1f%%)" % (tier, n, 100.0 * n / total))

    if a.stubs:
        unresolved = collections.Counter()
        for f, cs in edges.items():
            for c in cs:
                if c not in defined:
                    unresolved[c] += 1
        print("\nstub surface: %d distinct CRT/MFC entry points over %d call sites"
              % (len(unresolved), sum(unresolved.values())))
        for c, n in unresolved.most_common(40):
            print("   %5d  %s" % (n, c[:78]))
        return 0

    if not a.summary:
        pct = scores()
        rows = [(pct[f][0], pct[f][1], f) for f, v in verdicts.items()
                if v == "CLEAN" and f in pct
                and pct[f][0] < 100.0 and pct[f][1] >= a.min_size]
        rows.sort(key=lambda r: -r[1])
        print("\nCLEAN and not-yet-exact and >= 0x%x bytes: %d "
              "(no stubs needed - snapshot, restore, call, diff)" % (a.min_size, len(rows)))
        for p, s, f in rows[:40]:
            print("   %5.1f%%  0x%-5x  %s" % (p, s, f[:88]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
