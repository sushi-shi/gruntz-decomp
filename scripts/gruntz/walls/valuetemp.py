"""gruntz.walls.valuetemp - the by-value struct temp sieve.

An inlined accessor returning a two-field struct BY VALUE materialises a frame
temp. The half whose field is READ gets folded into its consumer (cl re-loads
the source member instead), and the UNREAD half's store is emitted and left
dead. Two calls therefore leave two dead stores in adjacent frame slots, fed by
two adjacent member offsets through one base register:

    mov  <rA>,[<b>+N]     mov  <rB>,[<b>+N+4]
    mov  [esp+K],<rA>     mov  [esp+K+4],<rB>       both dead

  target-only   retail takes the pair by value where we read the members -
                give the class the accessor and consume it as an rvalue
                (`Coord LastTilePx() { return m_lastTilePx; }`, used as
                `LastTilePx().m_x`, NOT bound to a named local).
  base-only     the mirror: we copy a pair retail reads in place.

DEAD is decided by the EVENT ORDER on the slot: a store is dead when the next
event on its slot is another store (the RectContains form, where the real value
overwrites the temp) or nothing at all (the GetModeSize form). Two things make
the order the only workable rule. ESP IS TRACKED - a `push` between a store and
its read renumbers the slot. And an ADDRESS-TAKEN aggregate names only its BASE,
so a per-dword read scan calls its interior fields dead; an `lea` therefore
observes the whole object it points at, and it observes it AT ITS OWN INDEX -
the RectContains temp escapes the same slot, just after its successor killed it.

`--calibrate` is the negative control this sieve exists to have: it reports the
rows on 100.00% functions, where the two sides ARE the same bytes and any row is
a detector bug. A clean zero from a detector nobody has seen fire is not a
result - `--control` re-proves it against the two rows the mechanism was
established on.

    gruntz walls valuetemp [--unit U] [--fn SUB] [--calibrate] [--control]
"""

from __future__ import annotations

import argparse
import re
import sys

from gruntz.delink.coffx import Obj
from gruntz.walls import check_unit, pairscan

STORE = re.compile(r"^(?:DWORD PTR )?\[esp(?:\+0x([0-9a-f]+))?\],(e[a-z][a-z])$")
LOAD = re.compile(r"^(e[a-z][a-z]),(?:DWORD PTR )?\[(e[a-z][a-z])\+0x([0-9a-f]+)\]$")
ESPREF = re.compile(r"\[esp(?:\+0x([0-9a-f]+))?\]")
LEA = re.compile(r"^e[a-z][a-z],\[esp(?:\+0x([0-9a-f]+))?\]$")
# An address-taken frame object is live THROUGHOUT, and only its BASE appears in
# the operand - so a per-dword read scan calls its interior fields dead. That is
# the sieve's one false-positive class (CBattlezMapConfig::ScanRegion built two
# adjacent RECTs, `lea`'d both and pushed one; the sieve read the second RECT's
# right/bottom stores as a Coord temp). An address-take therefore covers a whole
# object: up to the next address-taken base, capped at the widest aggregate this
# codebase passes that way.
WIDEST_AGGREGATE = 0x10          # RECT; Coord is 8
# (unit, symbol, member offset, which sides must carry it). The first two took
# the accessor and agree; ApplyTriggerA took it too but cl still gives its two
# calls SEPARATE slots where retail shares one pair, so it stays target-only -
# which exercises the disagreement path the sweep exists to report.
CONTROL = (("gruntsteps", "?RectContains@CGrunt@@QAEHHH@Z", 0x17C, "bt"),
           ("gruntsteps", "?RectContainsGated@CGrunt@@QAEHHH@Z", 0x17C, "bt"),
           ("triggermgrgrid", "?ApplyTriggerA@CTriggerMgr@@QAEHHHHH@Z", 0x17C, "t"))
LOOKBACK = 40


def _esp_trace(ins):
    """[(mnemonic, operands, delta)] - esp's offset below its value at entry."""
    out, d = [], 0
    for _off, mn, ops in ins:
        out.append((mn, ops, d))
        if mn == "push":
            d += 4
        elif mn == "pop":
            d -= 4
        elif mn in ("sub", "add") and ops.startswith("esp,0x"):
            n = int(ops.split("0x", 1)[1], 16)
            d += n if mn == "sub" else -n
    return out


def _source_of(ins, i, reg):
    """The member (base register, offset) `reg` held at index i, if a load set it."""
    for j in range(i - 1, max(-1, i - LOOKBACK), -1):
        _off, mn, ops = ins[j]
        if mn == "mov":
            m = LOAD.match(ops)
            if m and m.group(1) == reg:
                return m.group(2), int(m.group(3), 16)
        if ops.startswith(f"{reg},") or ops == reg:
            return None          # some other definition of reg
    return None


def _escape_slots(base: int, bases: list[int]) -> range:
    """The dword slots one `lea`-taken frame address keeps live: up to the next
    address-taken base, capped at the widest aggregate passed that way."""
    nxt = next((b for b in bases if b > base), base + WIDEST_AGGREGATE)
    return range(base, min(nxt, base + WIDEST_AGGREGATE), 4)


def temps(ins):
    """{member offset N} for each dead adjacent store pair fed by [b+N],[b+N+4].

    Liveness is decided by the EVENT ORDER on each slot, not by whole-function
    sets: a store is dead when the next event on its slot is another store (or
    nothing). Order is what separates the two forms - the RectContains form
    overwrites the slot before its address is taken, so the address-take never
    observes the temp, while ScanRegion's `lea`+push comes straight after its
    stores and observes them."""
    tr = _esp_trace(ins)
    events: dict[int, list[tuple[int, str]]] = {}
    src_of: dict[int, tuple[str, int]] = {}
    leas: list[tuple[int, int]] = []
    for i, (mn, ops, d) in enumerate(tr):
        m = STORE.match(ops) if mn == "mov" else None
        if m is not None:
            slot = (int(m.group(1), 16) if m.group(1) else 0) - d
            events.setdefault(slot, []).append((i, "w"))
            src = _source_of(ins, i, m.group(2))
            if src and slot not in src_of:
                src_of[slot] = src
            continue
        if mn == "lea" and (g := LEA.match(ops)):
            leas.append((i, (int(g.group(1), 16) if g.group(1) else 0) - d))
        for g in ESPREF.finditer(ops):
            events.setdefault((int(g.group(1), 16) if g.group(1) else 0) - d,
                              []).append((i, "r"))
    bases = sorted({b for _i, b in leas})
    for i, base in leas:
        for slot in _escape_slots(base, bases):
            events.setdefault(slot, []).append((i, "r"))

    dead: dict[int, tuple[str, int]] = {}
    for slot, evs in events.items():
        evs.sort()
        first_store = next((n for n, (_i, k) in enumerate(evs) if k == "w"), None)
        if first_store is None or slot not in src_of:
            continue
        after = evs[first_store + 1:]
        if not after or after[0][1] == "w":
            dead[slot] = src_of[slot]
    out = set()
    for k, (b, n) in dead.items():
        hi = dead.get(k + 4)
        if hi and hi[0] == b and hi[1] == n + 4:
            out.add(n)
        lo = dead.get(k - 4)
        if lo and lo[0] == b and lo[1] == n - 4:
            out.add(n - 4)
    return out


def _sides(bobj, tobj, bf, tf, sym):
    bsec, bs, be = bf[sym]
    tsec, ts, te = tf[sym]
    return (temps(pairscan.insns(bobj, bsec, bs, be)),
            temps(pairscan.insns(tobj, tsec, ts, te)))


def scan(unit_filter=None, fn_filter=None, calibrate=False):
    sc, live = pairscan.scores()
    agree = 0
    miss, extra = [], []
    for unit, (base, target) in sorted(
            pairscan.require_pairs({unit_filter} if unit_filter else None).items()):
        if live and unit not in live:
            continue
        try:
            bobj, tobj = Obj(base), Obj(target)
        except (ValueError, OSError):
            continue
        bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
        for sym in sorted(set(bf) & set(tf)):
            if fn_filter and fn_filter not in sym:
                continue
            pct = sc.get((unit, sym))
            if pct is None:
                continue
            if calibrate != (pct >= 100.0):
                continue
            b, t = _sides(bobj, tobj, bf, tf, sym)
            if b == t:
                agree += 1 if t else 0
                continue
            if t - b:
                miss.append((pct, unit, sym, sorted(t - b)))
            if b - t:
                extra.append((pct, unit, sym, sorted(b - t)))
    return agree, miss, extra


def _show(title, rows):
    print(f"\n{title}: {len(rows)}")
    for pct, unit, sym, offs in sorted(rows):
        print(f"{pct:7.2f}  {unit:<22} "
              f"+{'/+'.join(hex(o) for o in offs):<16} {sym}")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument("--unit")
    ap.add_argument("--fn", help="substring of the mangled name")
    ap.add_argument("--calibrate", action="store_true",
                    help="rows on 100%% functions - the detector-bug rate")
    ap.add_argument("--control", action="store_true",
                    help="re-prove the detector on the established rows")
    a = ap.parse_args(argv)
    unit = check_unit(a.unit)

    if a.control:
        bad = 0
        pairs = pairscan.require_pairs({u for u, _s, _o, _w in CONTROL})
        for u, sym, want, sides in CONTROL:
            bobj, tobj = (Obj(p) for p in pairs[u])
            bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
            if sym not in bf or sym not in tf:
                print(f"MISSING  {u}/{sym}")
                bad += 1
                continue
            b, t = _sides(bobj, tobj, bf, tf, sym)
            ok = ((want in b) == ("b" in sides)) and ((want in t) == ("t" in sides))
            bad += not ok
            print(f"{'ok  ' if ok else 'FAIL'}  {u:<16} +{want:#x} on {sides:<2}  "
                  f"base={[hex(x) for x in sorted(b)]} "
                  f"target={[hex(x) for x in sorted(t)]}  {sym}")
        print("\nthe detector fires on every established row"
              if not bad else f"\n{bad} control(s) FAILED - the detector is "
                              "measuring nothing; fix it before reading a sweep")
        return 1 if bad else 0

    agree, miss, extra = scan(unit, a.fn, a.calibrate)
    if a.calibrate:
        print(f"CALIBRATION over 100.00% rows - every row below is a detector bug")
    print(f"carrying the temp on BOTH sides at the same member: {agree}")
    _show("TARGET-ONLY (retail takes the pair by value, we do not)", miss)
    _show("BASE-ONLY (we take it by value, retail does not)", extra)
    return 0


if __name__ == "__main__":
    sys.exit(main())
