"""gruntz.walls.thisscan - the dropped-receiver (`this`) sieve.

A `__thiscall` member and a free `__stdcall` with the SAME stack arguments
compile to the same callee bytes: the receiver travels in ECX, so it costs the
callee nothing. `CLatencyList::GetSelItemData` scored 100% EXACT while modelled
as a free `__stdcall`, and stayed exact after it became a member. The defect is
therefore invisible in the callee and invisible to every test `walls diagnose`
runs (call-set, CFG, frame): only a CALLER shows it, as an ECX load retail
emits and we do not.

    retail   mov ecx,DWORD PTR [ecx+0x60]   ; m_slotList - never read again
             call ?GetSelItemData@...
    ours     call ?GetSelItemData@...

cl 5.0 does not emit dead loads, so a load whose value nothing consumes before
the call IS a receiver. This sieve reads the normalized pair objdiff scored and
reports, per call site, the callees retail gives a receiver and we do not.

    gruntz walls thisscan [--todo] [--unit U] [--below P] [--above P]
                          [--inverse] [--all-callees] [--receivers] [--arity]
                          [--limit N] [--json]

The default screen requires ALL FOUR of:

  dead        nothing between the ECX definition and the call reads ECX. This
              is the discriminator, not a refinement: retail routinely
              materialises a pushed ARGUMENT through ECX (`mov ecx,[ebx];
              push eax; push ecx; call CellTargetable`) where we use EDX, and
              every such row is a register-name rotation, not a receiver.
  ours-lacks  no ECX definition reaches our call site. Our side is scanned
              with a WEAKER rule that walks back past branch boundaries to the
              previous `call` (which clobbers ECX anyway), because a receiver
              we materialise before a guard branch and retail re-materialises
              after it is not a missing receiver.
  counts      both sides call the callee the same number of times, and retail
              gives a receiver at EVERY one. A receiver at some sites only is
              a value that happens to live in ECX.
  free callee the callee is not already mangled `__thiscall`. A hit on a
              member is a wrong-OBJECT question (or, usually, this sieve
              failing to see our receiver), not a dropped-`this` one.

`--inverse` runs the mirror: WE pass a receiver retail does not, i.e. a free
function modelled as a member.

Two adjacent screens share the machinery, both for the same reason - the callee
cannot witness the defect:

  --receivers  BOTH sides pass a receiver, but from a different member or
               global: a wrong-OBJECT bug. The callee must be mangled
               `__thiscall`, because against a free callee ECX is an ordinary
               scratch register and the "receiver" is whichever step of a
               pointer chain landed there.
  --arity      the caller's `__cdecl` cleanup (`add esp,N`, which cl also
               spells `pop ecx`) against retail's. A `__cdecl` callee does not
               clean up, so a trailing argument it never reads is invisible in
               ITS bytes exactly as a receiver is - the caller is again the
               only witness. cl merges the cleanup of adjacent calls, so read
               the surrounding run before believing a row.

FALSE-POSITIVE TAXONOMY - each was observed on a flagged row whose model was
right, and each is what one of the four filters above removes:

  * argument through ECX     retail pushes the value it just loaded into ECX;
                             we load the same value into EDX/EAX. Killed by
                             `dead` (`?ActiveWait@@YAXI@Z` from RetireScene,
                             `?CellTargetable@@YAHHH@Z` from
                             StepGooSuckerBehavior, `?RotateRasterize@@...`
                             from ImageRotateBlit - all three consumed).
  * receiver before a guard  ours holds the receiver in ECX from before the
                             `je` that retail re-loads after. Killed by the
                             weaker our-side rule (`?Reset@CLightFxMgr@@QAEXXZ`
                             from CGruntzMgr::Close,
                             `?HitClick@CActionOptionsMenuBar@@QAEHHH@Z` from
                             CTriggerMgr::PlaceObjectFull).
  * `pop ecx`                cl 5.0 spells `add esp,4` as `pop ecx`. Never a
                             receiver; excluded at the source-kind test.
  * CRT / operator new/delete `_fopen`, `_srand`, `??2`, `??3` cannot be
                             members; the free-callee filter keeps them out of
                             the default screen but they are what
                             `--all-callees` prints.

The 2026-08-23 sweep of the 675-row sub-100 queue: 243 asymmetric ECX sites
over 49 callees unfiltered, 2 after the filters, and BOTH were real -
`CSymParser::UnpackTag` (the mirror of the already-member
`CSymParser::PackTag`, and retail hands it ParseRecords' own spilled `this`)
and `CGameLevel::InflateMainBlock` (retail hands it LoadWwd's `this` in EBP).
Neither callee's bytes moved; both callers gained the load. `--inverse`,
`--receivers` and `--arity` were all zero in the same sweep.

The 222 skipped rows are EH funclets and are not recoverable here: cl 5.0 emits
them inside the parent's COMDAT with no symbol of their own, and only the
delinker gives the target side `__ehunwind$<parent>$N` names, so there is
nothing to pair them against. Their calls are destructors, which are members on
both sides by construction.
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
from gruntz.walls.semdiff import NORM, _decode

BYTES_ONLY = re.compile(r"^(?:[0-9a-f]{2} ?)+$")
ECX_TOK = re.compile(r"\b(ecx|cx|cl|ch)\b")
BRANCH = re.compile(r"^j\w+$")
DIRECT_CALL = re.compile(r"^call\s+0x[0-9a-f]+$")
#: `mov ecx,[<base>+0xNN]` with base a non-frame register: a member read
MEMBER_LOAD = re.compile(
    r"^mov\s+ecx,(?:DWORD PTR )?\[(e[a-d]x|e[sd]i|ebx|ecx)"
    r"(?:\+e[a-z]{2}\*\d)?([+-]0x[0-9a-f]+)?\]$")
FRAME_LOAD = re.compile(
    r"^mov\s+ecx,(?:DWORD PTR )?\[(esp|ebp)([+-]0x[0-9a-f]+)?\]$")
ABS_LOAD = re.compile(r"^mov\s+ecx,(?:DWORD PTR )?(?:ds:)?0x[0-9a-f]+$")
LEA_LOCAL = re.compile(r"^lea\s+ecx,\[(esp|ebp)([+-]0x[0-9a-f]+)?\]$")
LEA_OTHER = re.compile(r"^lea\s+ecx,")
REG_COPY = re.compile(r"^mov\s+ecx,(e[a-d]x|e[sd]i|ebx|ebp)$")
#: cl 5.0 spells `add esp,4` as `pop ecx`. NEVER a receiver.
POP_ECX = re.compile(r"^pop\s+ecx$")
#: an MSVC5 `__thiscall` member mangling - `@` + access + cv + the E convention
THISCALL = re.compile(r"@[QAIUEMB][ABCD]E")

#: instructions back from the call the retail-side scan considers; our side
#: gets three times this because it walks whole blocks
WINDOW = 24
#: ECX sources that name a concrete object, so a hit says WHICH class
STRONG = ("member", "global", "lea-local")


def kind_of(asm: str) -> str:
    """What the ECX definition reads - `pop` is the one that is never a `this`."""
    if POP_ECX.match(asm):
        return "pop"
    if MEMBER_LOAD.match(asm):
        return "member"
    if ABS_LOAD.match(asm):
        return "global"
    if FRAME_LOAD.match(asm):
        return "frame"
    if LEA_LOCAL.match(asm):
        return "lea-local"
    if LEA_OTHER.match(asm):
        return "lea"
    if REG_COPY.match(asm):
        return "regcopy"
    if asm.startswith(("xor ecx,ecx", "sub ecx,ecx")):
        return "zero"
    return "other"


def writes_ecx(asm: str) -> bool:
    parts = asm.split(None, 1)
    if parts[0] in ("cmp", "test", "push", "call", "ret", "nop", "int3"):
        return False
    if len(parts) < 2:
        return False
    return parts[1].split(",")[0].strip() in ("ecx", "cx", "cl", "ch")


def reads_ecx(asm: str) -> bool:
    """ECX appears as a VALUE - a source operand or an address base."""
    parts = asm.split(None, 1)
    if len(parts) < 2:
        return bool(ECX_TOK.search(asm))
    op, rest = parts
    ops = [o.strip() for o in rest.split(",")]
    if op in ("cmp", "test", "push", "xchg"):
        return bool(ECX_TOK.search(rest))
    if writes_ecx(asm):
        if op in ("mov", "lea", "movsx", "movzx", "pop", "set"):
            return any(ECX_TOK.search(s) for s in ops[1:])
        if asm.startswith(("xor ecx,ecx", "sub ecx,ecx")):
            return False        # an idempotent zero, not a use of the old value
        return True
    return bool(ECX_TOK.search(rest))


def branch_targets(lines) -> set[int]:
    out = set()
    for ln in lines:
        asm = ln.asm
        if not asm or not BRANCH.match(asm.split()[0]):
            continue
        m = re.search(r"\b0x([0-9a-f]+)$", asm)
        if m:
            out.add(int(m.group(1), 16))
    return out


def receiver(lines, i: int, targets: set[int], loose: bool = False):
    """The ECX definition reaching call `i`, or None.

    `loose` is the our-side rule: walk back past branch boundaries to the
    previous `call`, so a receiver materialised before a guard branch counts.
    """
    consumed = False
    steps = 0
    limit = WINDOW * 3 if loose else WINDOW
    for j in range(i - 1, -1, -1):
        asm = lines[j].asm
        if not asm or BYTES_ONLY.match(asm):
            continue
        op = asm.split()[0]
        if op in ("call", "ret", "leave"):
            return None                        # ECX is volatile across a call
        if not loose and BRANCH.match(op):
            return None
        if writes_ecx(asm):
            k = kind_of(asm)
            return None if k == "pop" else {
                "kind": k, "asm": asm, "dist": steps, "consumed": consumed,
                "ref": lines[j].ref or ""}
        if reads_ecx(asm):
            consumed = True
        steps += 1
        if steps >= limit or (not loose and lines[j].addr in targets):
            return None
    return None


def call_sites(lines, loose: bool = False) -> list[dict]:
    targets = branch_targets(lines)
    out = []
    for i, ln in enumerate(lines):
        asm = ln.asm
        if not asm or not asm.startswith("call"):
            continue
        out.append({"addr": ln.addr, "ref": ln.ref or "",
                    "direct": bool(DIRECT_CALL.match(asm)),
                    "recv": receiver(lines, i, targets, loose)})
    return out


class _Pin:
    """A (unit, name) the Model cannot resolve - an EH funclet, whose symbol
    carries no rva of its own."""

    def __init__(self, unit, name):
        self.unit, self.name = unit, name


def _pair(token, unit: str | None = None):
    if unit is not None:
        binding = _Pin(unit, token)
    else:
        binding, why = _locate(token)
        if binding is None:
            raise SystemExit(f"[thisscan] {why}")
    base = Obj(NORM / "base" / f"{binding.unit}.obj")
    tp = NORM / "target" / f"{binding.unit}.c.obj"
    if not tp.exists():
        tp = NORM / "target" / f"{binding.unit}.obj"
    bb, brel, _ = _find_function(base, binding.name)
    tb, trel, _ = _find_function(Obj(tp), binding.name)
    return binding, _decode(bb, brel), _decode(tb, trel)


#: a register name is an allocation accident; the DISPLACEMENT is the model
REGNAME = re.compile(r"\b(?:e[a-d]x|e[sd]i|ebx|ebp)\b")


def _shape(asm: str) -> str:
    """The ECX definition with register names cancelled - `mov ecx,[esi+0x60]`
    and `mov ecx,[edi+0x60]` are the same receiver, `[esi+0x54]` is not."""
    return REGNAME.sub("r", asm.replace("ecx", "@", 1)).replace("@", "ecx", 1)


def _census(cs):
    """(calls, calls-with-a-receiver, receivers) per direct callee."""
    tot, rec, det = Counter(), Counter(), defaultdict(list)
    for c in cs:
        if not c["direct"] or not c["ref"]:
            continue
        tot[c["ref"]] += 1
        if c["recv"]:
            rec[c["ref"]] += 1
            det[c["ref"]].append(c["recv"])
    return tot, rec, det


def scan_one(token: str, unit: str | None = None, inverse: bool = False) -> dict:
    binding, lb, lt = _pair(token, unit)
    # the side that must show the receiver gets the STRICT rule, the side that
    # must lack it gets the weaker one
    ours = call_sites(lb, loose=not inverse)
    retail = call_sites(lt, loose=inverse)
    give, lack = ((retail, ours) if not inverse else (ours, retail))
    tg, rg, dg = _census(give)
    tl, rl, _dl = _census(lack)

    hits, asym = [], []
    for ref in sorted(tg):
        if rg[ref] == 0 or rg[ref] <= rl[ref]:
            continue
        rs = dg[ref]
        rec = {"callee": ref, "n": tg[ref], "other_n": tl[ref],
               "recv": rg[ref], "other_recv": rl[ref],
               "kinds": sorted({r["kind"] for r in rs}),
               "strong": sum(1 for r in rs if r["kind"] in STRONG),
               "dead": all(not r["consumed"] for r in rs),
               "maxdist": max(r["dist"] for r in rs),
               "asm": sorted({r["asm"] for r in rs}),
               "thiscall": bool(THISCALL.search(ref))}
        asym.append(rec)
        if (tg[ref] == tl[ref] and rl[ref] == 0 and rg[ref] == tg[ref]
                and rec["dead"] and not rec["thiscall"]):
            hits.append(rec)
    seq_o = [c["ref"] for c in ours if c["direct"]]
    seq_r = [c["ref"] for c in retail if c["direct"]]
    return {"hits": hits, "asym": asym, "ordered": seq_o == seq_r and bool(seq_o),
            "calls": len(seq_o),
            # the mismatch screen needs the SAME rule on both sides - the
            # asymmetric pair above would read a further-back our-side
            # definition as a different object
            "mismatch": _receiver_mismatch(call_sites(lb), call_sites(lt),
                                           seq_o == seq_r),
            "arity": _arity_mismatch(lb, lt, seq_o == seq_r)}


#: caller-side cleanup of a `__cdecl` call. cl 5.0 spells `add esp,4` as
#: `pop ecx`, and merges the cleanup of adjacent calls.
CLEANUP = re.compile(r"^add\s+esp,(0x[0-9a-f]+)$")


def _cleanups(lines) -> list[tuple[str, int | None]]:
    """(callee, bytes the CALLER pops) for each direct call, in order.

    None where the next instruction is not a cleanup - a `__stdcall` callee, a
    merged cleanup, or a deferred one.
    """
    out = []
    for i, ln in enumerate(lines):
        if not ln.asm or not DIRECT_CALL.match(ln.asm):
            continue
        nxt = None
        for j in range(i + 1, len(lines)):
            a = lines[j].asm
            if not a or BYTES_ONLY.match(a):
                continue
            m = CLEANUP.match(a)
            if m:
                nxt = int(m.group(1), 16)
            elif a == "pop ecx":
                nxt = 4
            break
        out.append((ln.ref or "", nxt))
    return out


def _arity_mismatch(lb, lt, ordered) -> list[dict]:
    """`__cdecl` argument-count defects.

    A `__cdecl` callee does not clean up, so a trailing argument it never reads
    is invisible in ITS bytes - the caller's `add esp,N` is the only witness,
    exactly as ECX is the only witness of a dropped receiver.
    """
    if not ordered:
        return []
    out = []
    for (ra, ca), (rb, cb) in zip(_cleanups(lb), _cleanups(lt)):
        if ca is None or cb is None or ca == cb:
            continue
        out.append({"callee": ra, "ours": ca, "retail": cb})
    return out


def _receiver_mismatch(ours, retail, ordered) -> list[dict]:
    """Call sites where BOTH sides pass a receiver but from a DIFFERENT member
    or global - a wrong-object bug, not a missing-argument one.

    The callee must be mangled `__thiscall`. Against a FREE callee ECX is an
    ordinary scratch register, so the "receiver" is whichever step of a pointer
    chain happened to land there - all five rows of the first sweep were that
    (`[g_gameReg+0x30]` then `[+0x24]` then `[+0x5c]`, with the two sides
    stopping at different links). Only member-vs-member and global-vs-global
    are reported: a register copy (`mov ecx,esi`) names no object.
    """
    if not ordered:
        return []
    po = [c for c in ours if c["direct"]]
    pr = [c for c in retail if c["direct"]]
    out = []
    for a, b in zip(po, pr):
        ra, rb = a["recv"], b["recv"]
        if not ra or not rb or ra["kind"] != rb["kind"]:
            continue
        if not THISCALL.search(a["ref"]):
            continue
        if ra["kind"] == "member" and _shape(ra["asm"]) != _shape(rb["asm"]):
            out.append({"callee": a["ref"], "kind": "member",
                        "ours": _shape(ra["asm"]), "retail": _shape(rb["asm"])})
        elif ra["kind"] == "global" and ra["ref"] != rb["ref"]:
            out.append({"callee": a["ref"], "kind": "global",
                        "ours": ra["ref"] or "(none)",
                        "retail": rb["ref"] or "(none)"})
    return out


def scan(rows, inverse: bool, progress=None) -> list[dict]:
    out = []
    for n, row in enumerate(rows, 1):
        rec = {k: row[k] for k in ("rva", "unit", "symbol", "cur", "hist_max")}
        try:
            try:
                rec.update(scan_one(row["rva"], inverse=inverse))
            except SystemExit:
                rec.update(scan_one(row["symbol"], row["unit"], inverse))
        except BaseException as err:      # an unpairable funclet, mostly
            rec["skipped"] = str(err)[:110]
        out.append(rec)
        if progress and n % 100 == 0:
            print(f"  ... {n}/{len(rows)}", file=progress)
    return out


def report_mismatch(rows, limit: int) -> None:
    ok = [r for r in rows if "mismatch" in r]
    hit = [r for r in ok if r["mismatch"]]
    sites = sum(len(r["mismatch"]) for r in hit)
    print(f"paired rows read: {len(ok)}   "
          f"(skipped {len(rows) - len(ok)}: EH funclets, which cl emits "
          "inside the parent COMDAT with no symbol of their own)")
    print(f"  callers passing a receiver from a DIFFERENT member/global : "
          f"{len(hit)}")
    print(f"  call SITES                                               : "
          f"{sites}")
    print("  a `lea`-folded base shifts the displacement without changing the "
          "object - compare the SUM before believing a row.")
    print()
    for r in sorted(hit, key=lambda x: -x["cur"])[:limit]:
        print(f"{r['rva']} {r['cur']:6.2f} (hist {r['hist_max'] or 0:6.2f}) "
              f"{r['unit']}/{r['symbol'][:64]}")
        for m in r["mismatch"]:
            print(f"    -> {m['callee'][:74]}")
            print(f"       {m['kind']}  ours {m['ours']}   retail {m['retail']}")


def report_arity(rows, limit: int) -> None:
    ok = [r for r in rows if "arity" in r]
    hit = [r for r in ok if r["arity"]]
    sites = sum(len(r["arity"]) for r in hit)
    print(f"paired rows read: {len(ok)}   "
          f"(skipped {len(rows) - len(ok)}: EH funclets, which cl emits "
          "inside the parent COMDAT with no symbol of their own)")
    print(f"  callers whose `__cdecl` cleanup differs from retail's : {len(hit)}")
    print(f"  call SITES                                            : {sites}")
    print("  cl merges the cleanup of adjacent calls and can defer one, so read "
          "the surrounding\n  run before believing a row is an argument-count "
          "defect.")
    print()
    for r in sorted(hit, key=lambda x: -x["cur"])[:limit]:
        print(f"{r['rva']} {r['cur']:6.2f} (hist {r['hist_max'] or 0:6.2f}) "
              f"{r['unit']}/{r['symbol'][:64]}")
        for m in r["arity"]:
            print(f"    -> {m['callee'][:74]}")
            print(f"       ours pops 0x{m['ours']:x}   retail pops "
                  f"0x{m['retail']:x}")


def report(rows, limit: int, inverse: bool, all_callees: bool) -> None:
    ok = [r for r in rows if "hits" in r]
    key = "asym" if all_callees else "hits"
    hit = [r for r in ok if r[key]]
    sites = sum(h["n"] for r in hit for h in r[key])
    dead = sum(h["n"] for r in hit for h in r[key] if h["dead"])
    strong = sum(h["strong"] for r in hit for h in r[key])
    free = sum(h["n"] for r in hit for h in r[key] if not h["thiscall"])
    callees = {h["callee"] for r in hit for h in r[key]}
    who = "WE pass" if inverse else "retail passes"
    print(f"paired rows read: {len(ok)}   "
          f"(skipped {len(rows) - len(ok)}: EH funclets, which cl emits "
          "inside the parent COMDAT with no symbol of their own)")
    print(f"  callers where {who} a receiver the other side lacks")
    print(f"    callers                                 : {len(hit):4d}")
    print(f"    call SITES                              : {sites:4d}")
    print(f"    ... ECX value never consumed            : {dead:4d}")
    print(f"    ... ECX from a member/global/local      : {strong:4d}")
    print(f"    ... callee not already thiscall         : {free:4d}")
    print(f"    distinct CALLEES named                  : {len(callees):4d}")
    print()
    for r in sorted(hit, key=lambda x: -x["cur"])[:limit]:
        print(f"{r['rva']} {r['cur']:6.2f} (hist {r['hist_max'] or 0:6.2f}) "
              f"{r['unit']}/{r['symbol'][:64]}"
              + ("" if r["ordered"] else "   [call order differs]"))
        for h in r[key]:
            print(f"    -> {'[member]' if h['thiscall'] else '[FREE]  '} "
                  f"{h['callee'][:74]}")
            print(f"       n={h['n']}/{h['other_n']} recv={h['recv']}/"
                  f"{h['other_recv']} kind={','.join(h['kinds'])} "
                  f"dead={h['dead']} dist<={h['maxdist']}  "
                  f"{' | '.join(h['asm'])}")
    if not all_callees:
        return
    print("\nCALLEE census (distinct callers naming each):")
    per = Counter()
    for r in hit:
        for h in r[key]:
            per[h["callee"]] += 1
    for c, n in per.most_common(80):
        print(f"  {n:3d}  {c}")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls thisscan",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--todo", action="store_true",
                    help="the campaign queue rather than every sub-100 row")
    ap.add_argument("--below", type=float, default=100.0)
    ap.add_argument("--above", type=float, default=-1.0,
                    help="calibration: exact rows are byte-identical, so any "
                         "hit at --above 100 --below 100.01 is a detector bug")
    ap.add_argument("--inverse", action="store_true",
                    help="the mirror: WE pass a receiver retail does not")
    ap.add_argument("--all-callees", action="store_true",
                    help="print every asymmetric callee, filters off")
    ap.add_argument("--arity", action="store_true",
                    help="the `__cdecl` argument-count screen: our caller's "
                         "`add esp,N` against retail's. A trailing argument a "
                         "__cdecl callee ignores is invisible in ITS bytes")
    ap.add_argument("--receivers", action="store_true",
                    help="the adjacent screen: BOTH sides pass a receiver, but "
                         "from a different member or global (a wrong-OBJECT "
                         "bug rather than a missing-argument one)")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--one", help="a single rva or name")
    args = ap.parse_args(argv)

    if args.one:
        json.dump(scan_one(args.one, inverse=args.inverse), sys.stdout, indent=1)
        print()
        return 0
    from gruntz.walls.inventory import build
    rows = build(check_unit(args.unit), args.below, args.todo)
    if args.above >= 0:
        rows = [r for r in rows if r["cur"] >= args.above]
    scanned = scan(rows, args.inverse, None if args.json else sys.stderr)
    if args.json:
        json.dump(scanned, sys.stdout)
        return 0
    if args.arity:
        report_arity(scanned, args.limit)
    elif args.receivers:
        report_mismatch(scanned, args.limit)
    else:
        report(scanned, args.limit, args.inverse, args.all_callees)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
