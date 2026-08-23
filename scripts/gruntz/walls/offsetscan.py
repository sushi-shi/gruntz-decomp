"""gruntz.walls.offsetscan - the MEMBER-OFFSET sieve.

A memory operand's displacement IS the field the source names.  objdiff does
not mask it, so `mov ecx,[ebx+0x64]` against our `mov ecx,[ebx+0x68]` is not a
schedule coin and not a register rotation: it is a DIFFERENT MEMBER, and every
run of the game reads the wrong dword.  That makes this, with `signscan`, one
of the two channels whose hits are CORRECTNESS defects rather than spellings.

The defect hides from every other reading.  The instruction is the same
instruction, in the same block, with the same mnemonic, the same operand shape
and the same relocation set; only four bits of the ModRM displacement differ.
`diagnose` reports it as "bytes first differ at +0xNNN" with the class
REGALLOC/SCHEDULING, which is exactly the verdict that makes a reader stop.

WHY IT IS FOUND BY ALIGNMENT AND NOT BY A CENSUS.  A displacement MULTISET
delta drowns: 0x10 and 0x68 are among the commonest offsets in the image, so
a real 4-site swap sits inside dozens of unrelated occurrences and any
threshold that admits it admits noise.  What is decisive is the SAME POSITION:
align the two instruction streams on a key that masks every operand cl is
free to choose - the registers (regalloc rotates them), the immediates, the
displacements themselves - and then read the displacements back at the
positions the alignment called EQUAL.  A mismatch there is one instruction
that both sides emit, doing the same thing, to a different field.

WHAT IS EXCLUDED, AND WHY:

  * frame-relative operands.  `[esp+N]` is a stack slot, and a slot number is
    an output of cl's allocator, not a source fact - `framescan` owns that
    question.  `[ebp+N]` is excluded ONLY when that side really set up a frame
    pointer (`push ebp / mov ebp,esp` in the prologue); /O2 omits it in 669 of
    671 retail bodies, and in a frameless body EBP is an ordinary value
    register, so `lea eax,[ebp+0x4]` is a heap field.  `frame_regs` (shared
    with `escapescan`) decides per side.
  * every line carrying a RELOCATION.  An absolute `ds:0x...`, a global folded
    into `[eax+<table>]`, a `$L` block address: the displacement is a link-time
    address objdiff masks, the referent channel owns it, and `assert-relocs`
    already proves it.
  * the function's own switch/index table, which decodes as instructions -
    removed by `code_pair`'s byte-range filter, as `diagnose` removes it - and
    the indirect `jmp`/`call` through it, whose displacement is the table's
    link-time address.
  * a candidate whose two displacements are BOTH touched by BOTH sides inside
    the same aligned run.  Then the two accesses were merely SCHEDULED in a
    different order and the aligner paired them crosswise - storescan's
    channel.  This was the sieve's dominant false-positive population: one
    99.56% row produced 46 such pairs forming closed permutation cycles.  A
    wrong MEMBER, by contrast, is a field the other side never touches there.
  * a candidate within two instructions of the EDGE of its aligned run.  That
    is where the pairing slips: one side has an extra instruction and the tail
    walks off by one, so two unrelated accesses meet.  Three live rows read
    that way and every one was an extra instruction, not a different field.
  * an aligned pair whose BASE REGISTERS differ.  Two instructions indexing off
    different pointers are two different accesses the masked key happened to
    align; requiring the same base is what separates the reading from the
    alignment's own noise.  It bounds the sieve in exchange: a real wrong
    member whose base register rotated too does not reach the report.

WHAT IT STRUCTURALLY CANNOT SEE.  A wrong member whose offset happens to
coincide with the right one moves no byte.  A wrong member inside a block the
alignment could not match - an inline/call-set or CFG wall shifts the streams
far enough that whole regions read as `replace` - is dropped rather than
guessed at; the sieve reports how much of each side it could align, so a low
coverage figure is a warning that the reading is partial, not clean.  And an
aligned mismatch is a LEAD until the retail disassembly says which member the
displacement belongs to: two adjacent scalars of one class, or one class's
member against a base subobject's copy of the same pointer, look identical
here and only the class model separates them.

THE SHAPE THAT BUILT IT.  `CTriggerMgr::LoadTileArrivalFx` reached the goober
puddle through `CUserLogic::m_object` (+0x10) where retail uses
`CWapX::m_wwdObject` (+0x38) - `CGruntPuddle` derives from both and the two
members hold the same pointer, so nothing misbehaved - and read the gauge award
from +0x68 where retail reads +0x64.  That second one was a real bug:
`CGruntPuddle::Place` (byte-exact) homes its fourth argument at +0x64, and
`CTriggerMgr::PlacePuddle` passes `sprite->m_points` there with the same 25
default the caller primes, so the pickup credited the puddle's type id instead
of its points.  Five aligned displacement mismatches, no other signal.

    gruntz walls offsetscan [--todo] [--unit U] [--limit N] [--all] [--json]
    gruntz walls offsetscan <rva|name> ...   one row, every mismatch in context
    gruntz walls offsetscan --control        re-prove the verdict on the rows
                                             read by hand
"""

from __future__ import annotations

import argparse
import difflib
import json
import re
import sys
from collections import Counter

from gruntz.walls import check_unit
from gruntz.walls.escapescan import code_pair, frame_regs

#: one memory operand: an optional size prefix, a base register, an optional
#: scaled index, an optional displacement.  `ds:0x...` has no bracket and is
#: therefore not matched at all.
MEM = re.compile(r"\[(e[a-z]{2})(\*\d)?"
                 r"(?:\+(e[a-z]{2})(\*\d)?)?"
                 r"([+-]0x[0-9a-f]+)?\]")

REG = re.compile(r"\be[a-z]{2}\b|\b[a-d][lh]\b|\b[a-d]x\b|\b[sd]i\b|\b[sb]p\b")
IMM = re.compile(r"0x[0-9a-f]+")


def operand(asm: str) -> tuple[str, int] | None:
    """(base register, displacement) of this line's MEMBER operand, or None.

    An operand whose only register is SCALED (`[ecx*4+0x127c]`) is an array
    index off an absolute address - a jump table, a global - not a field of an
    object, and its displacement is a link-time address the referent channel
    owns.  Those leave here.
    """
    if asm.startswith(("jmp", "call")):
        return None
    m = MEM.search(asm)
    if not m or m.group(2):
        return None
    disp = m.group(5)
    return m.group(1), int(disp, 16) if disp else 0


def key(asm: str) -> str:
    """The instruction with everything cl is free to re-choose masked out:
    the registers, the immediates and the displacements.  What survives is
    the mnemonic and the operand SHAPE, which the source fixes."""
    return IMM.sub("I", REG.sub("R", asm))


def field_lines(lines) -> list[tuple[str, tuple[str, int] | None]]:
    """Each instruction as (alignment key, member operand or None)."""
    frame = set(frame_regs(lines))
    out = []
    for ln in lines:
        # A line carrying a RELOCATION has a link-time address folded into its
        # operand - a global, a jump/index table base - which objdiff masks and
        # which is not a field of anything.
        op = None if ln.ref else operand(ln.asm)
        if op and op[0] in frame:
            op = None
        out.append((key(ln.asm), op))
    return out


#: a permutation can also straddle a run boundary, so the run set is widened
#: by this many instructions on each side of it
MARGIN = 8

#: a mismatch this close to the edge of its aligned run is where the aligner
#: SLIPS - one side has an extra instruction and the pairing walks off by one
EDGE = 2


def _touched(side, lo: int, hi: int) -> set:
    """The member operands one side reads or writes inside one aligned run,
    widened by MARGIN so a permutation that straddles the boundary is still
    seen as one."""
    lo, hi = max(0, lo - MARGIN), min(len(side), hi + MARGIN)
    # DISPLACEMENTS only: the base register is precisely what regalloc rotates,
    # so keying the suppression on it would ask a question about allocation
    # instead of about which field the run touches.
    return {side[k][1][1] for k in range(lo, hi) if side[k][1]}


def mismatches(base, target) -> tuple[list[dict], int, int, int, int]:
    """Aligned positions whose member displacements differ, how many
    instructions the alignment matched, and how many candidates were
    suppressed as an order swap or as an alignment slip at a run edge."""
    fb, ft = field_lines(base), field_lines(target)
    sm = difflib.SequenceMatcher(a=[k for k, _ in fb], b=[k for k, _ in ft],
                                 autojunk=False)
    out, aligned, swapped, edged = [], 0, 0, 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag != "equal":
            continue
        aligned += i2 - i1
        ours_run, retail_run = _touched(fb, i1, i2), _touched(ft, j1, j2)
        for i, j in zip(range(i1, i2), range(j1, j2)):
            ob, ot = fb[i][1], ft[j][1]
            # The base register must AGREE.  Two aligned instructions indexing
            # off DIFFERENT pointers are two different accesses the masked key
            # happened to align, not one access to two fields; requiring the
            # same base is what separates the reading from the alignment's own
            # noise.  It also bounds the sieve: a real wrong member whose base
            # register rotated as well is invisible here.
            if not (ob and ot and ob[0] == ot[0] and ob[1] != ot[1]):
                continue
            # Both sides touch BOTH fields somewhere in the SAME aligned run:
            # the accesses were SCHEDULED in a different order and the aligner
            # paired them crosswise.  That is a store/load order question -
            # storescan's channel - not a wrong member, and a long store run
            # permutes far enough that a positional window cannot see it (46
            # such pairs in one 99.56% row, forming closed cycles).  A wrong
            # MEMBER shows as a field one side never touches in that run.
            if ot[1] in ours_run and ob[1] in retail_run:
                swapped += 1
                continue
            # An aligned run's EDGE is where the pairing slips: the side with
            # an extra instruction walks the whole tail off by one, and the two
            # unrelated accesses that meet there read as one wrong member.
            # Measured on three live rows (CGruntSpawnConfig::Clear,
            # CInGameIcon::PlaceAt, CRollingBall's ctor), every one an extra
            # instruction on our side rather than a different field.
            if min(i - i1, i2 - 1 - i, j - j1, j2 - 1 - j) < EDGE:
                edged += 1
                continue
            out.append({"ours": base[i].asm, "retail": target[j].asm,
                        "ours_disp": ob[1], "retail_disp": ot[1],
                        "shape": fb[i][0]})
    return out, aligned, min(len(fb), len(ft)), swapped, edged


#: a run of this many mismatches sharing ONE masked instruction shape is a
#: SET question, not a positional one
RUN = 5


def kind(bad: list[dict]) -> str:
    """`run` when most mismatches share one masked instruction shape.

    A long run of literally identical instructions - `mov [esi+X],edi` with
    EDI zero, the ctor's field clearing - carries no information about which
    store pairs with which, so the alignment zips two lists and every pair
    reads as a mismatch.  The real question there is which FIELDS each side
    clears, a set difference; `field` rows are the ones where the mismatch is
    isolated and the position means something.
    """
    if not bad:
        return "clean"
    top = max(Counter(m["shape"] for m in bad).values())
    return "run" if top >= RUN else "field"


def scan_one(token: str) -> dict:
    binding, base, target = code_pair(token)
    bad, aligned, total, swapped, edged = mismatches(base, target)
    swaps: dict[str, int] = {}
    for m in bad:
        swaps[f"{m['ours_disp']:#x}->{m['retail_disp']:#x}"] = \
            swaps.get(f"{m['ours_disp']:#x}->{m['retail_disp']:#x}", 0) + 1
    return {"nbad": len(bad), "swaps": swaps, "sites": bad,
            "order_swaps": swapped, "edge_slips": edged,
            "kind": kind(bad),
            "coverage": round(100.0 * aligned / total, 1) if total else 0.0}


def scan(rows: list[dict], progress=None) -> list[dict]:
    out = []
    for n, row in enumerate(rows, 1):
        rec = {k: row[k] for k in ("rva", "unit", "symbol", "cur", "hist_max")}
        try:
            rec.update(scan_one(row["rva"]))
        except BaseException as err:
            rec["error"] = str(err)[:110]
        out.append(rec)
        if progress and n % 100 == 0:
            print(f"  ... {n}/{len(rows)}", file=progress)
    return out


def report(rows: list[dict], limit: int, show_all: bool) -> None:
    ok = [r for r in rows if "nbad" in r]
    hit = [r for r in ok if r["nbad"]]
    field = [r for r in hit if r["kind"] == "field"]
    print(f"paired rows read: {len(ok)}   (errors {len(rows) - len(ok)})")
    print(f"  an isolated member offset differs : {len(field):4d}")
    print(f"  a whole clearing/copy RUN differs : {len(hit) - len(field):4d}"
          f"   (a field SET question, read the two sets)")
    print(f"  clean                             : {len(ok) - len(hit):4d}")
    print()
    print("coverage = the share of the shorter side the alignment matched; a "
          "low figure means the streams diverge structurally and this reading "
          "is partial.")
    print(f"{'rva':>10} {'cur':>7} {'n':>4} {'cov':>6}  unit/symbol")
    for r in sorted(hit if not show_all else ok,
                    key=lambda x: (x["kind"] != "field", x["cur"]))[:limit]:
        print(f"{r['rva']:>10} {r['cur']:7.2f} {r['nbad']:4d} "
              f"{r['coverage']:5.1f}% {r['kind']:>5}  "
              f"{r['unit']}/{r['symbol'][:40]}")
        for swap, n in sorted(r["swaps"].items(), key=lambda kv: -kv[1]):
            print(f"{'':>12}{swap:<20} x{n}")


def detail(token: str) -> None:
    binding, base, target = code_pair(token)
    bad, aligned, total, swapped, edged = mismatches(base, target)
    cov = round(100.0 * aligned / total, 1) if total else 0.0
    print(f"== {binding.unit}/{binding.name}   aligned {aligned}/{total} "
          f"({cov}%)   suppressed: {swapped} order, {edged} run-edge")
    if not bad:
        print("   no aligned member offset differs")
    for m in bad:
        print(f"   ours   {m['ours']}")
        print(f"   retail {m['retail']}")


#: hand-verified rows, each re-derived from the disassembly.  A sieve nobody
#: has seen FIRE is a green light, not a check.
CONTROL = {
    "0x00075e90": (False,
                   "NEGATIVE: CTriggerMgr::LoadTileArrivalFx is the row that "
                   "built this sieve - it held +0x10 for CWapX::m_wwdObject "
                   "(+0x38) and +0x68 for the gauge award retail reads at "
                   "+0x64.  Both are fixed, so it must stay silent"),
}


def _hermetic() -> int:
    """The POSITIVE, kept as a fixture because the only live one is the row
    this sieve CLOSED.  These are the exact retail and base bytes of
    CTriggerMgr::LoadTileArrivalFx's puddle block; the end-to-end proof is
    stronger and is recorded in docs/patterns: the defect was re-introduced
    into the source, rebuilt, and the sweep named both displacements in one
    line each before the fix was restored."""
    from gruntz.walls.semdiff import Line
    pad = ["nop"] * EDGE
    mk = lambda a: [Line(i * 4, t, None) for i, t in enumerate(pad + a + pad)]
    ours = mk(["mov ecx,DWORD PTR [ebx+0x68]", "push ecx", "call 0x0",
               "mov ebx,DWORD PTR [ebx+0x10]", "push ebx"])
    retail = mk(["mov edx,DWORD PTR [ebx+0x64]", "push edx", "call 0x0",
                 "mov ebx,DWORD PTR [ebx+0x38]", "push ebx"])
    got = {(m["ours_disp"], m["retail_disp"])
           for m in mismatches(ours, retail)[0]}
    want = {(0x68, 0x64), (0x10, 0x38)}
    ok = got == want
    print(f"{'FIRES ' if ok else 'BROKEN'} hermetic  "
          f"{'ok' if ok else f'got {got}, want {want}'}")
    print("      POSITIVE: the puddle block's own bytes - the gauge award at "
          "+0x68 for retail's +0x64, and CUserLogic::m_object (+0x10) for "
          "CWapX::m_wwdObject (+0x38)")
    return 0 if ok else 1


def control() -> int:
    bad = _hermetic()
    for rva, (expect, why) in CONTROL.items():
        try:
            rec = scan_one(rva)
        except BaseException as err:
            print(f"FAIL {rva}: {err}")
            bad += 1
            continue
        fires = bool(rec["nbad"])
        ok = fires == expect
        print(f"{'FIRES ' if fires else 'SILENT'} {rva}  "
              f"{'ok' if ok else 'UNEXPECTED'}  {rec['swaps']}")
        print(f"      {why}")
        bad += 0 if ok else 1
    if bad:
        print("\na control changed verdict: the row was fixed or regressed, or "
              "the detector did - read it, then re-pick")
    return 1 if bad else 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls offsetscan",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("rows", nargs="*", metavar="rva",
                    help="adjudicate these rows instead of sweeping")
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--todo", action="store_true")
    ap.add_argument("--below", type=float, default=100.0)
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--control", action="store_true",
                    help="re-prove the detector's verdict on hand-read rows")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)

    if args.control:
        return control()
    if args.rows:
        for token in args.rows:
            detail(token)
        return 0
    from gruntz.walls.inventory import build
    rows = build(check_unit(args.unit), args.below, args.todo)
    scanned = scan(rows, progress=None if args.json else sys.stderr)
    if args.json:
        json.dump(scanned, sys.stdout)
        return 0
    report(scanned, args.limit, args.all)
    return 0
