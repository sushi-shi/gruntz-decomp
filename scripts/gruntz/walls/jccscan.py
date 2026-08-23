"""gruntz.walls.jccscan - the CONDITION-CODE sieve.

A conditional branch's mnemonic IS the source comparison operator, and it is
the one operand class objdiff never masks: `je` versus `jl` versus `jb` is a
different question about the same two values.  So a function whose base and
target hold DIFFERENT MULTISETS of condition codes is making a different
comparison somewhere, and that is a source fact, not a schedule coin.

Three readings, in descending strength:

  SIGNED    ours `jl` where retail has `jb` (or jle/jbe, jg/ja, jge/jae) in
            equal numbers.  A signedness slip: one side compares the loop
            guard or index as int and the other as unsigned.  This one is a
            defect, not a spelling - the two disagree for half the domain.
  OPERATOR  one side holds equality codes (je/jne) where the other holds
            ordered ones (jl/jle/jg/jge/...).  Usually a `switch` written as
            an `||` chain: cl folds clustered `case` labels into range tests
            (`cmp 4 / jl def; cmp 5 / jle arm; cmp 8 / jne def` for {4,5,8})
            and emits three `cmp/je` for the chain.
  POLARITY  the same codes but transposed counts (ours +N je, retail +N jne).
            An arm-order or guard-polarity difference; see
            merged-call-arms-expose-the-if-else-order.md and
            allocate-check-then-body-is-the-then-block.md.

WHAT IT CANNOT SEE.  Equal multisets prove nothing: two `je` can still test
different things.  It counts codes, not operands.  And a branch cl folded away
entirely (a constant-folded guard) leaves no code to count, so a genuinely
missing comparison reads as a plain surplus on the other side.

Everything past the LAST `ret` is excluded.  A jump table and the alignment
padding after it decode as instructions, and `js`/`jo`/`jp` out of that region
are the sieve's whole false-positive population - `CPlay::LoadCursorSprites`
0xd0120 read as three retail-only `js` that were all jump-table payload.

    gruntz walls jccscan [--todo] [--unit U] [--below N] [--limit N] [--json]
    gruntz walls jccscan <rva|name> ...      one row, every site listed
"""

from __future__ import annotations

import argparse
import json
import sys
from collections import Counter

from gruntz.walls import check_unit
from gruntz.walls.semdiff import pair_lines

#: every x86 conditional branch cl 5.0 emits, canonical spelling
CC = ("je", "jne", "jl", "jle", "jg", "jge", "jb", "jbe", "ja", "jae",
      "js", "jns", "jo", "jno", "jp", "jnp")

#: signed code <-> its unsigned counterpart on the same relation
SIGNED = {"jl": "jb", "jle": "jbe", "jg": "ja", "jge": "jae",
          "jb": "jl", "jbe": "jle", "ja": "jg", "jae": "jge"}

EQ = {"je", "jne"}
ORD = {"jl", "jle", "jg", "jge", "jb", "jbe", "ja", "jae"}


def code_region(lines):
    """Up to and including the LAST `ret`; past that is jump-table or
    alignment payload objdump decodes as instructions."""
    last = max((i for i, x in enumerate(lines) if x.asm.startswith("ret")),
               default=len(lines) - 1)
    return lines[:last + 1]


def codes(lines) -> Counter:
    return Counter(x.asm.split()[0] for x in code_region(lines)
                   if x.asm.split()[0] in CC)


def classify(ours: Counter, retail: Counter) -> dict | None:
    if ours == retail:
        return None
    so = {k: ours[k] - retail.get(k, 0) for k in ours if ours[k] > retail.get(k, 0)}
    sr = {k: retail[k] - ours.get(k, 0) for k in retail if retail[k] > ours.get(k, 0)}
    signed = [(k, SIGNED[k]) for k in so
              if k in SIGNED and SIGNED[k] in sr and so[k] == sr[SIGNED[k]]]
    if signed:
        kind = "SIGNED"
    elif (set(so) & EQ and set(sr) & ORD) or (set(so) & ORD and set(sr) & EQ):
        kind = "OPERATOR"
    else:
        kind = "POLARITY"
    return {"kind": kind, "ours_extra": so, "retail_extra": sr,
            "signed": signed, "delta": sum(so.values()) + sum(sr.values())}


def scan_one(rva: str) -> dict:
    binding, base, target = pair_lines(rva)
    rec = classify(codes(base), codes(target))
    return {"agree": rec is None, **(rec or {})}


def detail(token: str, limit: int = 12) -> None:
    binding, base, target = pair_lines(token)
    ours, retail = codes(base), codes(target)
    rec = classify(ours, retail)
    print(f"== {binding.unit}/{binding.name}")
    if rec is None:
        print("   condition-code multisets AGREE (this sieve has nothing to say;"
              " equal counts are not proof)")
        return
    print(f"   {rec['kind']}   ours {dict(ours)}")
    print(f"   {'':8}retail {dict(retail)}")
    if rec["signed"]:
        print("   signed/unsigned pairs: "
              + ", ".join(f"ours {a} vs retail {b}" for a, b in rec["signed"]))
    want = set(rec["ours_extra"]) | set(rec["retail_extra"])
    for nm, lines in (("ours  ", base), ("retail", target)):
        region = code_region(lines)
        sites = [i for i, x in enumerate(region) if x.asm.split()[0] in want]
        for i in sites[:limit]:
            ctx = " ; ".join(y.asm for y in region[max(0, i - 2):i + 1])
            print(f"   {nm} @{region[i].addr:04x}  {ctx}")
        if len(sites) > limit:
            print(f"   {nm} ... {len(sites) - limit} more site(s) "
                  f"(--limit to widen; a surplus spread over many sites is a"
                  f" whole-function polarity question, not one guard)")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls jccscan",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("rows", nargs="*", metavar="rva")
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--todo", action="store_true")
    ap.add_argument("--below", type=float, default=100.0)
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)

    if args.rows:
        for token in args.rows:
            detail(token, args.limit)
        return 0

    from gruntz.walls.inventory import build
    rows = build(check_unit(args.unit), args.below, args.todo)
    out = []
    for n, row in enumerate(rows, 1):
        rec = {k: row[k] for k in ("rva", "unit", "symbol", "cur", "hist_max")}
        try:
            rec.update(scan_one(row["rva"]))
        except BaseException as err:
            rec["error"] = str(err)[:110]
        out.append(rec)
        if not args.json and n % 100 == 0:
            print(f"  ... {n}/{len(rows)}", file=sys.stderr)

    if args.json:
        json.dump(out, sys.stdout)
        return 0

    hits = [r for r in out if r.get("agree") is False]
    by = Counter(r["kind"] for r in hits)
    print(f"rows read: {sum(1 for r in out if 'error' not in r)}"
          f"   (errors {sum(1 for r in out if 'error' in r)})")
    print(f"  condition-code multiset DIFFERS : {len(hits):4d}"
          f"   SIGNED {by['SIGNED']}  OPERATOR {by['OPERATOR']}"
          f"  POLARITY {by['POLARITY']}")
    print(f"  agree                           : "
          f"{sum(1 for r in out if r.get('agree')):4d}"
          f"   (agreement is NOT evidence - it counts codes, not operands)")
    print()
    order = {"SIGNED": 0, "OPERATOR": 1, "POLARITY": 2}
    for r in sorted(hits, key=lambda x: (order[x["kind"]], -x["delta"]))[:args.limit]:
        print(f"{r['rva']:>10} {r['cur']:7.2f} {r['kind']:<9} d={r['delta']:<3d} "
              f"ours+{r['ours_extra']} retail+{r['retail_extra']}  "
              f"{r['unit']}/{r['symbol'][:40]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
