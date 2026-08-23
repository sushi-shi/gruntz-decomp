"""gruntz.walls.storescan - the permuted-member-store-run sieve.

C2 schedules a straight-line member-store run ITSELF, so retail's EMITTED
store order is an OUTPUT of the scheduler, not a picture of the devs' source.
A reconstruction that transcribes the emitted order feeds C2 an input it never
had and C2 re-permutes THAT, so the two sides store the same fields in a
different order. docs/patterns/emitted-store-order-is-not-the-source-order.md
names the shape and the remedy (write the run as an object copy, or in the
class's declaration order).

For every paired sub-100 function this reads the SAME evidence objdiff scored
(the normalized pair under build/objdiff/compare-new) and reports the rows
where the two sides' member-store runs are PERMUTATIONS of each other:

    permuted      some run's offset SEQUENCE differs while its offset
                  MULTISET is identical - the separator from an ordinary
                  register rotation, which leaves the order alone
    counts equal  bytes, instructions, calls, branches, returns and
                  relocations all equal, so nothing but the order moved
    multisets eq  the five walls-semdiff multisets agree with no exclusive
                  key, i.e. the same fields get the same values

`--values` runs the companion screen instead: per member offset, the multiset
of IMMEDIATES stored there. Two offsets that exchanged their constants is a
live transposition bug the masked score barely sees, and that screen is how
you rule it out (it found none in the 578-row queue of 2026-08-23).

    gruntz walls storescan [--todo] [--unit U] [--min-run N] [--loose]
                           [--values] [--limit N] [--json]

FALSE-POSITIVE TAXONOMY - each was observed on a row the sieve flagged whose
store ORDER was not the defect:

  * shared-inline expansion   the run comes from an inline expanded at N call
                              sites; if any sibling expansion is EXACT the
                              order is already proven and the residue is a
                              lone immediate store filling a load-use shadow
                              (CGameLevel::SetCoordExtents, 5 exact siblings).
  * regalloc serialization    retail holds two constants in two registers at
                              once; ours funnels both through EAX, which
                              FORCES the group reorder. The permutation is a
                              consequence (CBattlezMapConfig ctor: retail
                              `mov eax,0xbb8` + `mov edx,0x7d0` live together).
  * frame/local-count wall    a `sub esp` delta shifts every slot and the
                              store swap rides on it (ResolveArrivalReposition).
  * vptr-stamp placement      the moved "store" is the `??_7` stamp, which is
                              not a source statement at all.

Cost: one objdump decode per side per row (~5 min for the full ~580-row todo
queue), so it is a sweep tool. `walls diagnose` and `walls semdiff` adjudicate
the rows it flags.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter, defaultdict

from gruntz.delink.coffx import Obj
from gruntz.walls import check_unit
from gruntz.walls.diagnose import _find_function, _locate
from gruntz.walls.semdiff import NORM, _decode, exclusive, features

#: `mov <sz> PTR [<base>(+idx*s)?(+0xNN)?],<src>` with base != esp - a stack
#: slot is a frame accident, a member displacement is the class model.
STORE = re.compile(
    r"^mov\s+(BYTE|WORD|DWORD) PTR "
    r"\[(e[a-d]x|e[sd]i|ebx|ebp)((?:\+e[a-z]{2}\*\d)?)([+-]0x[0-9a-f]+)?\],"
    r"(\S+)$")
IMM_SRC = re.compile(r"^(0x[0-9a-f]+|\d+)$")
#: a call, branch or return ends the basic block C2 scheduled, so it ends the run
BREAK = re.compile(r"^(call|j\w+|ret|loop|rep|leave|int3?)\b")
BYTES_ONLY = re.compile(r"^(?:[0-9a-f]{2} ?)+$")
BRANCH = re.compile(r"^j\w+$")

#: non-store instructions tolerated between two stores of one run. The
#: interleaved parameter loads are part of the signature, so the run must
#: survive them; a longer gap is a different piece of code.
SLACK = 6


def store_runs(lines, min_len: int = 3, slack: int = SLACK) -> list[dict]:
    """Maximal straight-line member-store runs through one base register."""
    runs: list[dict] = []
    cur: dict | None = None
    gap = 0

    def close():
        nonlocal cur
        if cur and len(cur["off"]) >= min_len:
            runs.append(cur)
        cur = None

    for ln in lines:
        asm = ln.asm
        if not asm or BYTES_ONLY.match(asm):
            continue
        if BREAK.match(asm):
            close()
            gap = 0
            continue
        m = STORE.match(asm)
        if not m:
            if cur:
                gap += 1
                if gap > slack:
                    close()
            continue
        _size, base, idx, off, src = m.groups()
        if cur is None or cur["base"] != base or idx:
            close()
            cur = {"base": base, "addr": ln.addr, "off": [], "src": []}
        cur["off"].append(off or "+0x0")
        cur["src"].append(src)
        gap = 0
    close()
    return runs


def permuted(base_runs, tgt_runs) -> list[dict]:
    """Runs that are permutations of each other but not equal.

    Runs are paired POSITIONALLY. If the run counts differ the block structure
    differs and this sieve does not apply to the row.
    """
    if len(base_runs) != len(tgt_runs):
        return []
    out = []
    for i, (a, b) in enumerate(zip(base_runs, tgt_runs)):
        if a["off"] != b["off"] and sorted(a["off"]) == sorted(b["off"]):
            out.append({"i": i, "addr": a["addr"], "n": len(a["off"]),
                        "base_reg": a["base"], "tgt_reg": b["base"],
                        "ours": a["off"], "retail": b["off"],
                        "ours_src": a["src"], "retail_src": b["src"]})
    return out


def counts(lines) -> tuple[int, int, int, int]:
    """(instructions, calls, branches, returns) of one decoded side."""
    n = calls = branches = rets = 0
    for ln in lines:
        asm = ln.asm
        if not asm or BYTES_ONLY.match(asm):
            continue
        n += 1
        op = asm.split()[0]
        if op == "call":
            calls += 1
        elif BRANCH.match(op):
            branches += 1
        elif op == "ret":
            rets += 1
    return n, calls, branches, rets


def _pair(token: str):
    """(binding, (lines, bytes, relocs) base, (lines, bytes, relocs) target)."""
    binding, why = _locate(token)
    if binding is None:
        raise SystemExit(f"[storescan] {why}")
    base = Obj(NORM / "base" / f"{binding.unit}.obj")
    tgt_path = NORM / "target" / f"{binding.unit}.c.obj"
    if not tgt_path.exists():
        tgt_path = NORM / "target" / f"{binding.unit}.obj"
    tgt = Obj(tgt_path)
    bb, brel, _ = _find_function(base, binding.name)
    tb, trel, _ = _find_function(tgt, binding.name)
    return (binding, (_decode(bb, brel), bb, brel), (_decode(tb, trel), tb, trel))


def scan_one(rva: str, min_run: int) -> dict:
    binding, (lb, bb, brel), (lt, tb, trel) = _pair(rva)
    hits = permuted(store_runs(lb, min_run), store_runs(lt, min_run))
    cb, ct = counts(lb), counts(lt)
    fb, ft = features(lb, binding.name), features(lt, binding.name)
    rec = {"hits": hits,
           "base_bytes": len(bb), "tgt_bytes": len(tb),
           "base_relocs": len(brel), "tgt_relocs": len(trel),
           "base_insns": cb[0], "tgt_insns": ct[0],
           "base_calls": cb[1], "tgt_calls": ct[1],
           "base_branches": cb[2], "tgt_branches": ct[2],
           "base_rets": cb[3], "tgt_rets": ct[3],
           "exclusive": len(exclusive(fb, ft)),
           "multisets_equal": all(fb[k] == ft[k] for k in fb)}
    rec["counts_equal"] = all(
        rec[f"base_{k}"] == rec[f"tgt_{k}"]
        for k in ("bytes", "relocs", "insns", "calls", "branches", "rets"))
    return rec


#: `mov <sz> PTR [<base>+0xNN],<imm>` - the value screen's input
IMM_STORE = re.compile(
    r"^mov\s+(BYTE|WORD|DWORD) PTR "
    r"\[(e[a-d]x|e[sd]i|ebx|ebp)((?:\+e[a-z]{2}\*\d)?)([+-]0x[0-9a-f]+)?\],"
    r"(0x[0-9a-f]+|\d+)$")


def imm_map(lines, self_name: str = "") -> dict[str, Counter]:
    """member offset -> multiset of immediates stored there."""
    out: dict[str, Counter] = defaultdict(Counter)
    for ln in lines:
        if self_name and ln.ref == self_name:
            continue
        m = IMM_STORE.match(ln.asm)
        if not m:
            continue
        size, _base, idx, off, val = m.groups()
        if idx:
            continue
        out[f"{size}{off or '+0x0'}"][int(val, 0)] += 1
    return out


def values_one(rva: str) -> dict:
    binding, (lb, _bb, _br), (lt, _tb, _tr) = _pair(rva)
    mb, mt = imm_map(lb, binding.name), imm_map(lt, binding.name)
    tot_b, tot_t = Counter(), Counter()
    for c in mb.values():
        tot_b += c
    for c in mt.values():
        tot_t += c
    diff = [{"off": k,
             "ours": sorted(mb[k].elements()),
             "retail": sorted(mt[k].elements())}
            for k in sorted(set(mb) | set(mt)) if mb[k] != mt[k]]
    return {"diff": diff, "total_equal": tot_b == tot_t}


def scan(rows, min_run: int, values: bool, progress=None) -> list[dict]:
    out = []
    for n, row in enumerate(rows, 1):
        rec = {k: row[k] for k in ("rva", "unit", "symbol", "cur", "hist_max")}
        try:
            rec.update(values_one(row["rva"]) if values
                       else scan_one(row["rva"], min_run))
        except BaseException as err:          # SystemExit from the locator too
            rec["error"] = str(err)[:110]
        out.append(rec)
        if progress and n % 50 == 0:
            print(f"  ... {n}/{len(rows)}", file=progress)
    return out


def _gold(r) -> bool:
    return r["counts_equal"] and r["multisets_equal"] and r["exclusive"] == 0


def report(rows, limit: int, loose: bool) -> None:
    ok = [r for r in rows if "hits" in r]
    perm = [r for r in ok if r["hits"]]
    strict = [r for r in perm if r["counts_equal"]]
    gold = [r for r in strict if _gold(r)]
    print(f"paired rows read: {len(ok)}   (errors {len(rows) - len(ok)})")
    print(f"  PERMUTED store run        : {len(perm):4d}")
    print(f"  ... and every count equal : {len(strict):4d}")
    print(f"  ... and multisets equal   : {len(gold):4d}"
          "   <== the pattern's shape")
    print()
    for r in sorted(perm if loose else strict,
                    key=lambda x: -x["cur"])[:limit]:
        tag = ("GOLD" if _gold(r) else "counts" if r["counts_equal"]
               else "loose")
        print(f"[{tag:6s}] {r['rva']} {r['cur']:6.2f} "
              f"(hist {r['hist_max']:6.2f}) {r['unit']}/{r['symbol'][:58]}")
        print(f"          bytes {r['base_bytes']}/{r['tgt_bytes']} "
              f"insn {r['base_insns']}/{r['tgt_insns']} "
              f"call {r['base_calls']}/{r['tgt_calls']} "
              f"br {r['base_branches']}/{r['tgt_branches']} "
              f"rel {r['base_relocs']}/{r['tgt_relocs']} "
              f"exclusive {r['exclusive']}")
        for h in r["hits"]:
            print(f"          run#{h['i']} n={h['n']} @0x{h['addr']:x} "
                  f"[{h['base_reg']}/{h['tgt_reg']}]")
            print(f"            ours   {' '.join(h['ours'])}")
            print(f"            retail {' '.join(h['retail'])}")


def report_values(rows, limit: int) -> None:
    ok = [r for r in rows if "diff" in r]
    swapped = [r for r in ok if r["diff"] and r["total_equal"]]
    other = [r for r in ok if r["diff"] and not r["total_equal"]]
    print(f"paired rows read: {len(ok)}   (errors {len(rows) - len(ok)})")
    print(f"  offsets differ, total immediate multiset IDENTICAL "
          f"(transposition candidates): {len(swapped)}")
    print(f"  offsets differ and totals differ (added/dropped constants): "
          f"{len(other)}")
    print("  a `lea`-folded base shifts every offset by the fold and is not a "
          "transposition - compare the SUM before believing a row.")
    print()
    for r in sorted(swapped, key=lambda x: -x["cur"])[:limit]:
        print(f"[SWAP?] {r['rva']} {r['cur']:6.2f} "
              f"{r['unit']}/{r['symbol'][:58]}")
        for d in r["diff"]:
            print(f"          {d['off']:16s} ours {d['ours']} "
                  f"retail {d['retail']}")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls storescan",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--todo", action="store_true",
                    help="the campaign queue rather than every sub-100 row")
    ap.add_argument("--below", type=float, default=100.0)
    ap.add_argument("--min-run", type=int, default=3,
                    help="shortest store run the sieve considers")
    ap.add_argument("--loose", action="store_true",
                    help="list permuted rows whose counts also differ")
    ap.add_argument("--values", action="store_true",
                    help="screen offset->immediate maps for a transposition")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)

    from gruntz.walls.inventory import build
    rows = build(check_unit(args.unit), args.below, args.todo)
    scanned = scan(rows, args.min_run, args.values,
                   progress=None if args.json else sys.stderr)
    if args.json:
        json.dump(scanned, sys.stdout)
        return 0
    if args.values:
        report_values(scanned, args.limit)
    else:
        report(scanned, args.limit, args.loose)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
