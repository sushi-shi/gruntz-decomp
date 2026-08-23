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

COMPLEMENT FOLDING is what makes the third reading usable.  `je` against `jne`,
`jl` against `jge`, `jle` against `jg` are one branch asked the other way, which
at source level is an arm order far more often than a different predicate.  So
the surpluses are cancelled pairwise FIRST and the verdict is taken on what is
LEFT: a row that folds to nothing is pure polarity, and a row with residue is
holding a comparison the other side has not got.  The fold also demotes rows the
raw multiset over-reads - `CBattlezMapConfig::ValidateUnitPath` 0x29b40 and
`CTriggerMgr::SpawnGrunt` 0x7c110 each read OPERATOR on a bare je/jne plus jle/jg
surplus and are two arm orders, nothing more.

WHAT IT CANNOT SEE.  Equal multisets prove nothing: two `je` can still test
different things.  It counts codes, not operands.  And a branch cl folded away
entirely (a constant-folded guard) leaves no code to count, so a genuinely
missing comparison reads as a plain surplus on the other side.

NOR IS A ONE-BRANCH RESIDUE PROOF OF A SOURCE DIFFERENCE.  The eight
`CButeMgr::Set*` rows are banked at best 100 on unchanged source and every one of
them currently shows retail holding one `je` we do not - a branch count moves
under TU composition alone.  Read the residue as a lead, then check the ledger.

WHERE THE CODE ENDS.  A switch table and the alignment padding after it decode
as instructions, and that payload is the sieve's whole false-positive
population - `CPlay::LoadCursorSprites` 0xd0120 read as three retail-only `js`
that were all table bytes.  Cutting at the LAST `ret` is NOT enough: a table
byte 0xc3 decodes as `ret`, and in `CGruntzMgr::HandleCommand` 0x862f0 (seven
tables) that phantom `ret` re-admitted ~450 lines of payload and read the row
as OPERATOR d=187 when the code is POLARITY d=8.

The table base is stated by the code itself.  A switch dispatch is
`mov cl,BYTE PTR [eax+0xTTTT]` / `jmp DWORD PTR [ecx*4+0xTTTT]` carrying a
relocation to the function's OWN symbol, and 0xTTTT is where the data starts.
So cut at the lowest such base, then at the last `ret` before it.  Verified on
0x862f0: past the lowest base both sides hold zero external referents and zero
real calls, i.e. it is entirely data.

    gruntz walls jccscan [--todo] [--unit U] [--below N] [--limit N] [--json]
    gruntz walls jccscan <rva|name> ...      one row, every site listed
"""

from __future__ import annotations

import argparse
import json
import re
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

#: code <-> the code that tests the NEGATION of the same question
COMPLEMENT = {"je": "jne", "jl": "jge", "jle": "jg", "jb": "jae",
              "jbe": "ja", "js": "jns", "jo": "jno", "jp": "jnp"}
COMPLEMENT.update({v: k for k, v in COMPLEMENT.items()})


def fold_complements(ours: dict, retail: dict):
    """Cancel each `jcc`/`jncc` pair across the two sides and return the
    flips plus what is LEFT.

    A complement pair is one branch asking the same question the other way,
    which at source level is an arm order (`if (x) A else B` written the
    other way round) far more often than a different predicate.  Cancelling
    them first is what makes the residue readable: a row that folds to
    nothing is pure polarity, and a row that does not is holding a
    comparison the other side has not got."""
    ours, retail = dict(ours), dict(retail)
    flips = []
    for k in list(ours):
        c = COMPLEMENT.get(k)
        if c and retail.get(c):
            n = min(ours[k], retail[c])
            flips.append((k, c, n))
            ours[k] -= n
            retail[c] -= n
    return (flips,
            {k: v for k, v in ours.items() if v},
            {k: v for k, v in retail.items() if v})


#: a switch dispatch reading its own table: the displacement is the table base
TABLE = re.compile(r"^(?:jmp|mov)\b.*\[[^]]*\+0x([0-9a-f]{3,})\]")


def code_region(lines, self_name: str = ""):
    """Up to the last `ret` BEFORE the function's own switch tables.

    The tables are located from the dispatch instructions that read them (a
    self-referent `jmp [reg*4+0xTTTT]` / `mov r8,BYTE PTR [reg+0xTTTT]`), not
    from the last `ret` alone - a 0xc3 table byte decodes as `ret`."""
    base = min((int(m.group(1), 16) for x in lines if x.ref == self_name
                for m in [TABLE.match(x.asm)] if m), default=None)
    if base is not None:
        lines = [x for x in lines if x.addr < base]
    last = max((i for i, x in enumerate(lines) if x.asm.startswith("ret")),
               default=len(lines) - 1)
    return lines[:last + 1]


def codes(lines, self_name: str = "") -> Counter:
    return Counter(x.asm.split()[0] for x in code_region(lines, self_name)
                   if x.asm.split()[0] in CC)


def classify(ours: Counter, retail: Counter) -> dict | None:
    if ours == retail:
        return None
    so = {k: ours[k] - retail.get(k, 0) for k in ours if ours[k] > retail.get(k, 0)}
    sr = {k: retail[k] - ours.get(k, 0) for k in retail if retail[k] > ours.get(k, 0)}
    flips, ro, rr = fold_complements(so, sr)
    signed = [(k, SIGNED[k]) for k in ro
              if k in SIGNED and SIGNED[k] in rr and ro[k] == rr[SIGNED[k]]]
    if signed:
        kind = "SIGNED"
    elif (set(ro) & EQ and set(rr) & ORD) or (set(ro) & ORD and set(rr) & EQ):
        kind = "OPERATOR"
    else:
        kind = "POLARITY"
    return {"kind": kind, "ours_extra": so, "retail_extra": sr,
            "flips": flips, "residue_ours": ro, "residue_retail": rr,
            "balanced": sum(so.values()) == sum(sr.values()),
            "signed": signed, "delta": sum(so.values()) + sum(sr.values())}


def scan_one(rva: str) -> dict:
    binding, base, target = pair_lines(rva)
    rec = classify(codes(base, binding.name), codes(target, binding.name))
    return {"agree": rec is None, **(rec or {})}


def detail(token: str, limit: int = 12) -> None:
    binding, base, target = pair_lines(token)
    ours = codes(base, binding.name)
    retail = codes(target, binding.name)
    rec = classify(ours, retail)
    print(f"== {binding.unit}/{binding.name}")
    if rec is None:
        print("   condition-code multisets AGREE (this sieve has nothing to say;"
              " equal counts are not proof)")
        return
    print(f"   {rec['kind']}   ours {dict(ours)}")
    print(f"   {'':8}retail {dict(retail)}")
    if rec["flips"]:
        print("   complement flips (arm order, or the predicate written the "
              "other way): "
              + ", ".join(f"ours {a} / retail {b}" + (f" x{n}" if n > 1 else "")
                          for a, b, n in rec["flips"]))
    if rec["residue_ours"] or rec["residue_retail"]:
        print(f"   residue after folding: ours {rec['residue_ours']} "
              f"retail {rec['residue_retail']}"
              "   <- a comparison one side has and the other has not")
    if rec["signed"]:
        print("   signed/unsigned pairs: "
              + ", ".join(f"ours {a} vs retail {b}" for a, b in rec["signed"]))
    want = set(rec["ours_extra"]) | set(rec["retail_extra"])
    for nm, lines in (("ours  ", base), ("retail", target)):
        region = code_region(lines, binding.name)
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
