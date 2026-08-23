"""gruntz.walls.residue - NAME the masked residual of every paired wall.

`walls framescan` proved the frame-size vein is drained and left ~500 rows
whose frame already EQUALS retail's. This sieve says what is left in each of
them, by reducing the diff twice before it classifies:

    1. POSITION cancellation - an instruction that appears on both sides in
       the residual only MOVED, and a move is a schedule choice.
    2. REGISTER stripping - cl 5.0 picks registers off a rotation cursor, so
       `mov ebx,[esi+0x84]` and `mov eax,[esi+0x84]` compute the same thing.

What survives both is the part no allocation or schedule choice explains, and
the classifier names it from the NET residual, most-actionable first:

    immediate      a constant differs                  possible semantic bug
    displacement   a member offset differs             possible layout bug
    referent       one side names a symbol the other   possible identity bug
                   never names anywhere
    selection      the mnemonic multiset differs       instruction selection
    operand        same mnemonics, different operands  term order / CSE
    arm-copy       target has callee-saved `mov r,r`   the arm-result temp is
                   the base lacks                      MISSING (register case)
    extra-copy     the inverse                         a temp we invented
    missing-store  target repeats a member store       the arm-result temp is
    dup-store      base repeats one                    PRESENT and should not
                                                       be (memory case)
    regname        register rotation only              R1/R2, park
    schedule       a pure permutation                  park
    none           nothing survives the mask

The arm-result classes implement the detection signature of
docs/patterns/arm-result-temp-controls-copies-and-shared-store.md; `--arm`
answers the same question directly over the WHOLE stream (every member store
and every callee-saved register copy, not just the ones inside a diff chunk),
which is the sensitive form.

Five encoding mirrors are normalized away, because each was measured
mislabelling real rows: the addend of a RELOCATED call or jump (position
state - the referent is compared separately), cl's 2-byte `and al,imm8`
form of `and eax,0xffffff00|imm8`, `add r,-K` against `sub r,K`, the forced
zero displacement of an EBP-based or base-less memory operand, and a
referent NEITHER side has a name for (`$anon_data_<sha>` against `FUN_<va>`
- a `$E` dynamic-init helper). The referent test also compares symbol SETS
over the whole stream, so a CSE'd second load of a global both sides name
is not an identity row.

    gruntz walls residue [--todo] [--unit U] [--max-resid N] [--kind K,...]
    gruntz walls residue --arm [...]        the arm-result worklist
    gruntz walls residue --show <rva|name>  one row, NET and EXACT residual

Cost: one objdump decode per side per row (~4 min for the ~580-row todo
queue), so it is a sweep tool. `walls diagnose` and `walls semdiff`
adjudicate the rows it flags.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from difflib import SequenceMatcher

from gruntz.walls import check_unit
from gruntz.walls.framescan import ESP_DISP, FRAME_IMM, LOCAL_BRANCH, frame
from gruntz.walls.semdiff import BYTES_ONLY, pair_lines

CALLEE_SAVED = ("esi", "edi", "ebx", "ebp")
REGMOV = re.compile(r"^mov\s+(e[a-z][a-z]),(e[a-z][a-z])$")
STORE = re.compile(r"^mov\s+(?:BYTE|WORD|DWORD)\s+PTR\s+\[(?!esp)([^\]]+)\],")
IMM = re.compile(r"(?<![\w\[+-])(0x[0-9a-f]{2,}|\b\d{3,}\b)")
#: read off REGISTER-STRIPPED text, where every register reads `r`
DISP = re.compile(r"\[r(?:\*\d)?(?:\+r(?:\*\d)?)?([+-]0x[0-9a-f]+)?\]")
REG = re.compile(r"\b(eax|ecx|edx|ebx|esi|edi|ebp|al|cl|dl|bl|ah|ch|dh|bh"
                 r"|ax|cx|dx|bx|si|di|bp)\b")

#: cl takes the 2-byte accumulator form when the value is in EAX and the
#: immediate's high bits are all ones, so `and al,0xe0` IS `and ecx,
#: 0xffffffe0` under a different register pick. Un-normalized, this single
#: mirror mislabelled seven ctors as an immediate difference.
ACC8 = re.compile(r"^(and|or|xor)\s+([abcd])l,0x([0-9a-f]{1,2})$")
ACC8_HIGH = {"and": "ffffff", "or": "000000", "xor": "000000"}

#: `add eax,0xffffffe0` IS `sub eax,0x20` - same three bytes' worth of work,
#: same value, and cl picks between them on its own (measured going BOTH ways
#: against retail in one tree: CSpotLight's ctor has the add where retail has
#: the sub, CStaticHazard's has the sub where retail has the add). Left alone,
#: the pair reads as two different constants.
ADDNEG = re.compile(r"^(add|sub)\s+(e[a-z][a-z]),0x([0-9a-f]{8})$")

#: a referent NEITHER side has a name for. objdiff calls our obj's unnamed
#: symbol `$anon_data_<sha>_N`; the delinker, with no claim on the address,
#: calls retail's `FUN_<va>` / `DAT_<va>` / `$gap_<va>`. Eleven of the fifteen
#: `referent` rows were one address under two non-names - every one a `$E`
#: dynamic-initializer helper, which the label rules deliberately never name.
UNNAMED = re.compile(r"^(FUN_|DAT_|\$gap_|\$anon_data_)")


def strip_regs(asm: str) -> str:
    return REG.sub("r", asm)


def mirror(asm: str) -> str:
    m = ACC8.match(asm)
    if m:
        return (f"{m.group(1)} e{m.group(2)}x,"
                f"0x{ACC8_HIGH[m.group(1)]}{int(m.group(3), 16):02x}")
    m = ADDNEG.match(asm)
    if m and int(m.group(3), 16) >= 0x80000000:
        flip = "sub" if m.group(1) == "add" else "add"
        return f"{flip} {m.group(2)},0x{0x100000000 - int(m.group(3), 16):x}"
    return asm


def masked(lines, self_name: str = "") -> list[str]:
    """framescan's mask (esp displacements, branch targets, the frame
    immediate) plus the relocated-call addend and the accumulator mirror.

    A function's own jump/index table decodes as junk instructions carrying
    huge displacements and a relocation back to the function; every line that
    names ITSELF is dropped, the same filter `walls semdiff` applies. Left in,
    one table produced 500 lines of "residual" on a single row."""
    out = []
    for ln in lines:
        if self_name and ln.ref == self_name:
            continue
        asm = ESP_DISP.sub("[esp+?]", ln.asm)
        if not asm or BYTES_ONLY.match(asm):
            continue
        if LOCAL_BRANCH.match(asm):
            asm = asm.split()[0] + " L"
        if FRAME_IMM.match(asm):
            asm = "FRAME"
        ref = "?unnamed" if ln.ref and UNNAMED.match(ln.ref) else ln.ref
        out.append(mirror(asm) + (f"|{ref}" if ref else ""))
    return out


def residual_of(base_m: list[str], tgt_m: list[str]):
    ops = SequenceMatcher(None, base_m, tgt_m, autojunk=False).get_opcodes()
    chunks, resid = [], 0
    for op, i1, i2, j1, j2 in ops:
        if op == "equal":
            continue
        resid += max(i2 - i1, j2 - j1)
        chunks.append({"op": op, "b": base_m[i1:i2], "t": tgt_m[j1:j2],
                       "bi": i1, "ti": j1})
    return resid, chunks


def bare(asm: str) -> str:
    """The instruction without its relocation referent."""
    return asm.split("|")[0]


def reg_key(asm: str) -> str:
    """Register-stripped, but the REFERENT is kept: two loads of different
    globals are not the same tuple however the registers land.

    The forced zero displacement folds here and only here: which base register
    cl picked is exactly what this key is meant to erase, and `[ebp+0x0]`
    addresses what `[ecx]` addresses."""
    head, sep, ref = asm.partition("|")
    return strip_regs(head).replace("+0x0]", "]") + sep + ref


def net_residual(chunks):
    """(base lines, target lines, exact-only, net-after-register-stripping)."""
    bo = [x for c in chunks for x in c["b"]]
    to = [x for c in chunks for x in c["t"]]
    exact_b, exact_t = Counter(bo) - Counter(to), Counter(to) - Counter(bo)
    kb = Counter(reg_key(x) for x in exact_b.elements())
    kt = Counter(reg_key(x) for x in exact_t.elements())
    return bo, to, exact_b, exact_t, kb - kt, kt - kb


def disp(d: str) -> str:
    """An x86 memory operand with EBP as its base, or with a scaled index and
    no base at all, CANNOT encode without a displacement byte/dword, so cl
    writes `[ebp+0x0]` / `[eax*8+0x0]` for the same address `[ecx]` / `[eax*8]`
    encodes bare. Register-stripping erases which base it was, so an explicit
    zero reads as a member-offset difference: a quarter of the `displacement`
    bucket was this one encoding."""
    return "" if d in ("+0x0", "-0x0") else d


def classify(chunks, base_m, tgt_m):
    bo, to, exact_b, exact_t, net_b, net_t = net_residual(chunks)
    if not bo and not to:
        return "none", ""
    if not exact_b and not exact_t:
        return "schedule", f"{len(bo)} insns permuted"
    if not net_b and not net_t:
        return "regname", (f"{sum(exact_b.values())}/{sum(exact_t.values())}"
                           f" insns, register rotation")

    deficit = sum(net_t.values()) - sum(net_b.values())
    t_copies = [x for x in net_t.elements() if x == "mov r,r"]
    b_copies = [x for x in net_b.elements() if x == "mov r,r"]
    only_b = [bare(x) for x in net_b.elements()]
    only_t = [bare(x) for x in net_t.elements()]
    raw = lambda c: sorted({x for x in c.elements()                # noqa: E731
                            if (m := REGMOV.match(bare(x)))
                            and m.group(1) in CALLEE_SAVED})
    if deficit > 0 and len(t_copies) >= deficit and not b_copies:
        return "arm-copy", f"+{deficit}: target has {raw(exact_t)}"
    if deficit < 0 and len(b_copies) >= -deficit and not t_copies:
        return "extra-copy", f"{deficit}: base has {raw(exact_b)}"

    b_stores = [x for x in only_b if STORE.match(x)]
    t_stores = [x for x in only_t if STORE.match(x)]
    if deficit < 0 and len(b_stores) >= -deficit and not t_stores:
        dup = sorted({x for x in exact_b.elements()
                      if STORE.match(bare(x)) and base_m.count(x) > 1})
        if dup:
            return "dup-store", f"{deficit}: base repeats {dup}"
        return "extra-store", f"{deficit}: base-only {sorted(set(b_stores))}"
    if deficit > 0 and len(t_stores) >= deficit and not b_stores:
        return "missing-store", f"+{deficit}: target-only {sorted(set(t_stores))}"

    bi = Counter(m for x in only_b for m in IMM.findall(x))
    ti = Counter(m for x in only_t for m in IMM.findall(x))
    if bi != ti:
        return "immediate", (f"base {sorted((bi - ti).elements())} vs target "
                             f"{sorted((ti - bi).elements())}")
    bd = Counter(disp(d) for x in only_b for d in DISP.findall(x))
    td = Counter(disp(d) for x in only_t for d in DISP.findall(x))
    if bd != td:
        return "displacement", (f"base {sorted((bd - td).elements())} vs target"
                                f" {sorted((td - bd).elements())}")
    br = Counter(x.split("|", 1)[1] for x in exact_b.elements() if "|" in x)
    tr = Counter(x.split("|", 1)[1] for x in exact_t.elements() if "|" in x)
    # A COUNT difference on a symbol both sides reference is CSE or
    # rematerialization, not an identity question - only a symbol the other
    # side never names anywhere is a claim to check.
    seen_b = {x.split("|", 1)[1] for x in base_m if "|" in x}
    seen_t = {x.split("|", 1)[1] for x in tgt_m if "|" in x}
    only_br = sorted(r for r in (br - tr) if r not in seen_t)
    only_tr = sorted(r for r in (tr - br) if r not in seen_b)
    if only_br or only_tr:
        return "referent", f"base {only_br} vs target {only_tr}"
    if Counter(x.split()[0] for x in only_b) != \
       Counter(x.split()[0] for x in only_t):
        return "selection", f"{sorted(only_b)} vs {sorted(only_t)}"
    return "operand", f"{sorted(only_b)} vs {sorted(only_t)}"


def store_census(stream):
    """Member stores keyed on the DESTINATION only, so `mov [esi+0x4c],ecx`
    and `mov [esi+0x4c],edx` are one store. A count that differs between the
    sides is the arm-result MEMORY signature."""
    c = Counter()
    for x in stream:
        m = STORE.match(bare(x))
        if m:
            c[strip_regs(m.group(1))] += 1
    return c


def copy_census(stream):
    return sum(1 for x in stream
               if (m := REGMOV.match(bare(x))) and m.group(1) in CALLEE_SAVED)


def scan_one(rva: str) -> dict:
    binding, base, target = pair_lines(rva)
    base_res, base_push = frame(base)
    tgt_res, tgt_push = frame(target)
    mb, mt = masked(base, binding.name), masked(target, binding.name)
    residual, chunks = residual_of(mb, mt)
    kind, note = classify(chunks, mb, mt)
    sb, st = store_census(mb), store_census(mt)
    return {"base_frame": base_res, "tgt_frame": tgt_res,
            "base_push": base_push, "tgt_push": tgt_push,
            "base_insns": len(mb), "tgt_insns": len(mt),
            "residual": residual, "kind": kind, "note": note[:220],
            "store_delta": {k: [sb[k], st[k]]
                            for k in set(sb) | set(st) if sb[k] != st[k]},
            "base_copies": copy_census(mb), "tgt_copies": copy_census(mt),
            "chunks": chunks}


def scan(rows, progress=None):
    out = []
    for n, row in enumerate(rows, 1):
        rec = {k: row.get(k) for k in ("rva", "unit", "symbol", "cur",
                                       "hist_max")}
        try:
            rec.update(scan_one(row["rva"]))
        except BaseException as err:              # SystemExit from the locator
            rec["error"] = str(err)[:120]
        out.append(rec)
        if progress and n % 50 == 0:
            print(f"  ... {n}/{len(rows)}", file=progress)
    return out


ORDER = ["immediate", "displacement", "referent", "selection", "operand",
         "arm-copy", "dup-store", "missing-store", "extra-store",
         "extra-copy", "regname", "schedule", "none"]


def report(rows, max_resid, kinds, limit):
    ok = [r for r in rows if "kind" in r]
    eq = [r for r in ok if r["base_frame"] == r["tgt_frame"]]
    sel = [r for r in eq if r["residual"] <= max_resid]
    print(f"paired rows read: {len(ok)}   (errors {len(rows) - len(ok)})")
    print(f"  frame EQUAL to retail's : {len(eq)}")
    print(f"  of those, residual <= {max_resid} : {len(sel)}")
    print()
    counts = Counter(r["kind"] for r in sel)
    for k in ORDER:
        if counts[k]:
            print(f"  {k:<14} {counts[k]:4d}")
    print()
    shown = [r for r in sel if not kinds or r["kind"] in kinds]
    shown.sort(key=lambda r: (ORDER.index(r["kind"]) if r["kind"] in ORDER
                              else 99, r["residual"], -r["cur"]))
    print(f"{'rva':>10} {'cur':>7} {'res':>4} {'nB':>5} {'nT':>5} "
          f"{'kind':<14} unit/symbol")
    for r in shown[:limit]:
        print(f"{r['rva']:>10} {r['cur']:7.2f} {r['residual']:4d} "
              f"{r['base_insns']:5d} {r['tgt_insns']:5d} {r['kind']:<14} "
              f"{r['unit']}/{r['symbol'][:44]}")
        if r["note"]:
            print(f"           {r['note']}")


def arm_report(rows, limit):
    """The arm-result-temp worklist, measured over the WHOLE stream."""
    ok = [r for r in rows if "kind" in r]
    mem = [r for r in ok if r["store_delta"]]
    reg = [r for r in ok if r["base_copies"] != r["tgt_copies"]]
    print(f"paired rows read: {len(ok)}")
    print(f"  MEMORY case   (a member store one side duplicates) : {len(mem)}")
    print(f"  REGISTER case (callee-saved copy count differs)    : {len(reg)}")
    print()
    print("== MEMORY: `target > base` means retail writes the member PER ARM "
          "and we share it ==")
    for r in sorted(mem, key=lambda x: -x["cur"])[:limit]:
        total = sum(v[1] - v[0] for v in r["store_delta"].values())
        print(f"{r['rva']:>10} {r['cur']:7.2f} "
              f"{'T+' if total > 0 else 'B+'}{abs(total):<3d} "
              f"{r['unit']}/{r['symbol'][:46]}")
        for k, v in sorted(r["store_delta"].items()):
            print(f"           {k:<22} base {v[0]}  target {v[1]}")
    print()
    print("== REGISTER: `target > base` means the arm temp cl would copy to a "
          "callee-saved register is MISSING ==")
    for r in sorted(reg, key=lambda x: -x["cur"])[:limit]:
        print(f"{r['rva']:>10} {r['cur']:7.2f} base {r['base_copies']:3d} "
              f"target {r['tgt_copies']:3d}  {r['kind']:<13} "
              f"{r['unit']}/{r['symbol'][:44]}")


def show(rva: str) -> None:
    rec = scan_one(rva)
    print(f"{rva}  frame {rec['base_frame']}/{rec['tgt_frame']}  push "
          f"{rec['base_push']}/{rec['tgt_push']}  insns "
          f"{rec['base_insns']}/{rec['tgt_insns']}")
    print(f"  residual {rec['residual']}  kind {rec['kind']}")
    print(f"  {rec['note']}")
    _bo, _to, eb, et, nb, nt = net_residual(rec["chunks"])
    print("  -- NET (position-cancelled, register-stripped) --")
    for x in sorted(nb.elements()):
        print(f"    b< {x}")
    for x in sorted(nt.elements()):
        print(f"    t> {x}")
    print("  -- EXACT-ONLY --")
    for x in sorted(eb.elements()):
        print(f"    B< {x}")
    for x in sorted(et.elements()):
        print(f"    T> {x}")
    if rec["store_delta"]:
        print("  -- member stores whose COUNT differs --")
        for k, v in sorted(rec["store_delta"].items()):
            print(f"    {k:<22} base {v[0]}  target {v[1]}")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls residue",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("--show", metavar="RVA", help="one row, in full")
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--todo", action="store_true",
                    help="the campaign queue rather than every sub-100 row")
    ap.add_argument("--below", type=float, default=100.0)
    ap.add_argument("--max-resid", type=int, default=10 ** 6)
    ap.add_argument("--kind", help="comma-separated kinds to list")
    ap.add_argument("--arm", action="store_true",
                    help="the arm-result-temp worklist instead")
    ap.add_argument("--limit", type=int, default=60)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)

    if args.show:
        show(args.show)
        return 0

    from gruntz.walls.inventory import build
    rows = build(check_unit(args.unit), args.below, args.todo)
    scanned = scan(rows, progress=None if args.json else sys.stderr)
    if args.json:
        json.dump(scanned, sys.stdout)
        return 0
    if args.arm:
        arm_report(scanned, args.limit)
        return 0
    report(scanned, args.max_resid,
           set(args.kind.split(",")) if args.kind else None, args.limit)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
