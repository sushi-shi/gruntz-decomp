#!/usr/bin/env python3
"""int_return_type.py - functions we declare `i32` that retail declared `void`.

The INVERSE of `gruntz.audit.void_return_type`, and the harder half: a wrong `int`
return type has no tell of its own. When the success path is `return <callee
result>` nothing is materialised, so an `int` function disassembles exactly like a
`void` one - the only trace is a spurious `xor eax,eax` we emit and retail does
not, or one extra `ret` we keep that retail cross-jumped away.

**One object-side signature is not enough.** `CGameLevel::MoveRising` /
`MoveFalling` / `MoveToward` are genuine `int` returns whose value is already in
eax from the last callee, so retail sets eax at no `ret` either and they look
identical to a real hit. The discriminator is the CALL GRAPH: a `void` function's
callers must all DISCARD eax. So the sieve is the conjunction

  1. retail materialises nothing into eax at any `ret` (object pair), and
  2. every retail call site discards the result (forward eax-liveness from the
     `call`, chased through the ILT thunks, over the whole .text).

Condition 1 alone is what produces the `MoveRising` family; condition 2 is what
kills them.

Three object-side tiers, mirroring void_return_type:

**VALUE** - our base sets eax in the epilogue of at least one `ret` and retail sets
it at none. The plain case: we wrote `return 0;` / `return 1;` / `return m_x;`.
A VALUE row the call graph REJECTS is still worth reading: the return type is right
but we invented an exit path that materialises a constant where retail has none.

**GUARD** - neither side materialises anything, but our base has MORE `ret`s.
Declared `int` with `return 0;` after a pointer test, cl5 needs no `xor eax,eax`
(eax is already 0) and cannot cross-jump the guard onto the tail `return f(...)`;
declared `void` it merges every empty return onto the one trailing `ret`.

**NOVALUE** - nothing separates the two declarations in this function's own bytes.
Only the call graph votes. Still listed: a wrong return type is a modelling defect
whether or not it costs a byte here, and the spurious live value is what blocks a
shrink-wrap somewhere else.

## Calibration (measured 2026-08-08, whole tree)

Over the 784 functions that are declared `void` AND already 100% exact - i.e.
PROVEN void - the object-side detector fires **0** times. Over the 1515 that are
declared int and 100% exact it fires 1317 times (87%); the 13% it misses are
exactly the `return <callee result>` shape this sieve is about. So condition 1 is
necessary-but-not-sufficient with perfect specificity, and condition 2 does the
rest: of 10 condition-1 candidates, 9 are genuine ints.

**Read `%al`, not just `%eax`.** A `bool` (`_N`) return is materialised BYTE-wide
(`xor al,al` / `mov al,[esp+0x24]`), and an eax-only reader hands back the whole
`CButeMgr` `_N` family - a library with a published `bool` signature - as hits.

    python -m gruntz.audit.int_return_type            # the worklist
    python -m gruntz.audit.int_return_type --all      # + the rejects, with reasons
    python -m gruntz.audit.int_return_type --why SYM  # per-call-site verdicts
    python -m gruntz.audit.int_return_type --max N    # exit 1 above N (ratchet)

See docs/patterns/int-return-that-retail-never-sets-is-a-void.md.
"""
import argparse
import bisect
import csv
import json
import re
import sys

from gruntz.core import get_context
from gruntz.core.branches import code_stop, decode, obj_paths, rets
from gruntz.core.pe import ILT_HI, ILT_LO
from gruntz.audit._textdisasm import text_insns
from gruntz.audit.void_return_type import GEN_NAMES, REPORT

# ---------------------------------------------------------------- mangled names

# MSVC's function-encoding table, indexed by the letter right after `@@`.
# Non-static members carry a cv-qualifier (`A`..`D`) before the calling
# convention; statics and free functions do not.  Getting this from a table
# rather than an ad-hoc alternation matters: the return type is the token
# straight after the calling-convention letter, and mis-locating it by one
# reads a PARAMETER as the return type.
MEMBER_ACCESS = frozenset("ABEFGHIJMNOPQRUVWX")   # private/protected/public, virtual, thunk
FREE_ACCESS = frozenset("CDKLSTYZ")               # static members + globals
CALLCONV = frozenset("ABCDEFGHIJ")                # cdecl/pascal/thiscall/stdcall/fastcall


def return_token(sym):
    """The mangled return-type token of a function symbol, or None if `sym` is not
    a mangled function (extern "C", a data symbol, a `$L` label, ...)."""
    if not sym.startswith("?"):
        return None
    i = 0
    while True:
        i = sym.find("@@", i)
        if i < 0:
            return None
        j = i + 2
        if j < len(sym):
            a = sym[j]
            if (a in MEMBER_ACCESS and j + 3 < len(sym)
                    and sym[j + 1] in "ABCD" and sym[j + 2] in CALLCONV):
                return sym[j + 3:]
            if a in FREE_ACCESS and j + 2 < len(sym) and sym[j + 1] in CALLCONV:
                return sym[j + 2:]
        i = j


def returns_intish(sym):
    t = return_token(sym)
    if not t:
        return False
    return t.startswith("_N") or t[0] in "HIJK"


def returns_void(sym):
    t = return_token(sym)
    return bool(t) and t[0] == "X"


# ---------------------------------------------------------------- object side

# AT&T (llvm-objdump default, which `gruntz.core.branches.decode` speaks).
# `%al` MUST be in the set: a `bool` (`_N`) return is materialised BYTE-wide, so an
# eax-only reader calls `xor al,al / ret` "no value" and hands back
# `CButeMgr::Parse` - a library API with a published `bool` signature - as a hit.
ATT_RETREG = re.compile(r"%(?:eax|ax|al)\b")
RETREG = frozenset(("%eax", "%ax", "%al"))
CONST_SRC = re.compile(r"^\$")


def _eax_definer(insns, i):
    """Walk back from `insns[i]` (a `ret`) to whatever last put a value in the
    return register.

    Returns ('const', mn, op) / ('value', mn, op) / ('call', ...) / None.
    Stops at a `call` (the value is a callee's result, not something the function
    materialised) and at any jump (crossing a block boundary backwards).

    The walk is bounded by the BASIC BLOCK, not by a fixed instruction count: a
    fixed window is an off-by-one waiting to happen, and it was.
    `CBootyState::LoadGameAssetNamespaces` parks its `mov eax,0x1` exactly ten
    instructions ahead of the `ret` - one past a 10-deep window - so a window read
    a plainly-int function as materialising nothing."""
    for j in range(i - 1, -1, -1):
        _off, mn, op = insns[j]
        if mn.startswith("call"):
            return ("call", mn, op)
        if mn.startswith("j"):
            return None
        if not ATT_RETREG.search(op):
            continue
        dst = op.rsplit(",", 1)[-1].strip()
        src = op.rsplit(",", 1)[0].strip() if "," in op else ""
        if dst not in RETREG:
            return None                     # the reg is READ here - not a definition
        if mn.startswith("set"):
            return ("value", mn, op)        # `setcc %al` - a computed bool
        if mn.startswith(("mov", "lea", "xor", "sub", "pop", "and", "or")):
            if mn.startswith(("xor", "sub")) and src == dst:
                return ("const", mn, op)
            if CONST_SRC.match(src):
                return ("const", mn, op)
            if ATT_RETREG.search(src):
                return None                 # read-modify-write: a real result
            return ("value", mn, op)
        return None
    return None


def eax_at_rets(insns, stop):
    """[(ret_offset, kind, mn, op)] for every `ret` whose eax the function itself
    materialised (a constant or a load) rather than inheriting from a callee."""
    out = []
    for i, (off, mn, _op) in enumerate(insns):
        if not mn.startswith("ret") or (stop is not None and off >= stop):
            continue
        d = _eax_definer(insns, i)
        if d and d[0] in ("const", "value"):
            out.append((off, d[0], d[1], d[2]))
    return out


# ---------------------------------------------------------------- caller side

# objdump Intel syntax (what `_textdisasm` speaks).
INTEL_EAX = re.compile(r"\b(?:eax|ax|al|ah)\b")
HEXTGT = re.compile(r"^0x([0-9a-f]+)")
JCC = re.compile(r"^j(?!mp\b)[a-z]+$")


def _touches(mn, ops):
    """'read' / 'write' / None - what this instruction does to eax.

    'write' means eax is fully overwritten WITHOUT being read, which kills the
    callee's return value; 'read' means the value is consumed. Anything that
    mentions eax and is not a recognised pure write counts as a read, so an
    unmodelled instruction can only make the sieve stricter."""
    if mn.startswith("call"):
        return "read" if INTEL_EAX.search(ops) else "write"
    if not INTEL_EAX.search(ops):
        return None
    if mn in ("mov", "movzx", "movsx", "lea", "pop", "xor", "sub"):
        dst, _, src = ops.partition(",")
        dst, src = dst.strip(), src.strip()
        if dst != "eax":
            return "read"
        if mn == "pop":
            return "write"
        if mn in ("xor", "sub") and src == "eax":
            return "write"
        return "read" if INTEL_EAX.search(src) else "write"
    return "read"


def _forward_eax(insn, order, start, lo, hi, caller_void):
    """Is the callee's eax dead on every path out of the call at `start`?

    Returns 'dead' / 'used' / 'unknown'. Both edges of a conditional jump are
    explored, so a value consumed only on one arm still reads as 'used'. A `ret`
    resolves against the CALLER's own return type: a `void` caller that never
    touched eax genuinely discards it, while an `int` caller is FORWARDING the
    value - which is exactly the `MoveRising` shape and refutes the theory."""
    k = bisect.bisect_right(order, start)
    if k >= len(order):
        return "unknown"
    work, seen, steps = [order[k]], set(), 0
    while work:
        a = work.pop()
        if a in seen:
            continue
        seen.add(a)
        steps += 1
        if steps > 400:
            return "unknown"
        if not (lo <= a < hi) or a not in insn:
            return "unknown"
        mn, ops = insn[a]
        if mn.startswith("ret"):
            if caller_void is True:
                continue                   # void caller, eax genuinely dropped
            return "used" if caller_void is False else "unknown"
        eff = _touches(mn, ops)
        if eff == "read":
            return "used"
        if eff == "write":
            continue                       # clobbered before any use on this path
        if mn == "jmp" or JCC.match(mn):
            m = HEXTGT.match(ops)
            if not m:
                return "unknown"           # indirect jump / jump table
            t = int(m.group(1), 16)
            if not (lo <= t < hi):
                return "unknown"           # tail call or thunk: eax escapes
            work.append(t)
            if mn == "jmp":
                continue                   # no fallthrough edge
        k = bisect.bisect_right(order, a)
        if k >= len(order):
            return "unknown"
        work.append(order[k])
    return "dead"


def call_sites(ctx, target):
    """[(site, kind)] - every direct rel32 call/jmp reaching `target`, with the ILT
    thunk band expanded so a thunked call is attributed to its real caller."""
    idx = ctx.pe.call_index
    out, seen = [], set()

    def walk(rva, depth):
        if rva in seen or depth > 4:
            return
        seen.add(rva)
        for site, op in idx.get(rva, []):
            if ILT_LO <= site < ILT_HI:
                walk(site, depth + 1)      # a thunk: chase its own callers
            else:
                out.append((site, op))
    walk(target, 0)
    return out


def indirect_refs(ctx, target):
    """How many times the function's address appears as DATA - a vtable slot or a
    function-pointer table. A non-zero count means the rel32 scan is structurally
    incomplete for it (the calls are `call [reg+off]`), so "no caller" is not
    evidence of anything and the slot's return type is fixed by the base class."""
    from gruntz.sema.xref import data_refs
    return len(data_refs(ctx, target))


def caller_verdicts(ctx, target):
    """[(site, owner_rva, owner_name, verdict)] over every retail call site."""
    insn = text_insns()
    order = sorted(insn)
    db = ctx.symbols
    res = []
    for site, op in call_sites(ctx, target):
        if site not in insn:
            res.append((site, None, "(mid-instruction / undecoded)", "unknown"))
            continue
        if op == 0xE9:
            res.append((site, db.owner(site), "(tail jmp)", "unknown"))
            continue
        o = db.owner(site)
        if o is None:
            lo, hi, cv, nm = site, site + 0x100, None, "(unrecovered fn)"
        else:
            nm = db.name_of(o)[0]
            sz = db.fsize.get(o) or 0x400
            lo, hi = o, o + sz
            cv = True if returns_void(nm) else (False if returns_intish(nm) else None)
        res.append((site, o, nm, _forward_eax(insn, order, site, lo, hi, cv)))
    return res


# ---------------------------------------------------------------- sweep

def rva_index():
    """{mangled name: '0x000xxxxx'} out of the generated symbol table."""
    idx = {}
    if GEN_NAMES.is_file():
        for r in csv.DictReader(GEN_NAMES.open()):
            rva = r["rva"]
            idx.setdefault(r["name"], rva if rva.startswith("0x") else "0x" + rva)
    return idx


def object_side():
    """The condition-1 candidates: declared int, retail materialises no eax."""
    report = json.loads(REPORT.read_text())
    cands = []
    for u in report.get("units") or []:
        unit = u.get("name")
        fns = [f for f in (u.get("functions") or [])
               if returns_intish(f.get("name") or "")
               and float(f.get("fuzzy_match_percent") or 0.0) < 99.995]
        if not fns:
            continue
        bobj, tobj = obj_paths(unit)
        if not (bobj.is_file() and tobj.is_file()):
            continue
        bs, ts = decode(bobj), decode(tobj)
        for f in fns:
            name = f.get("name")
            bi, ti = bs.get(name), ts.get(name)
            if not bi or not ti:
                continue
            bstop, tstop = code_stop(bi), code_stop(ti)
            beax, teax = eax_at_rets(bi, bstop), eax_at_rets(ti, tstop)
            if teax:
                continue                    # retail DOES materialise a value: int
            rb, rt = rets(bi, bstop), rets(ti, tstop)
            if beax:
                kind = "VALUE"
            elif rb > rt:
                kind = "GUARD"
            else:
                # Neither side materialises anything and the `ret` counts agree:
                # the two declarations are codegen-indistinguishable HERE, so only
                # the call graph can vote. Kept because a wrong return type is a
                # modelling defect whether or not it costs a byte - and because
                # the extra live value is what blocks a shrink-wrap elsewhere.
                kind = "NOVALUE"
            cands.append({
                "unit": unit, "name": name, "kind": kind,
                "pct": float(f.get("fuzzy_match_percent") or 0.0),
                "rets": (rb, rt), "eax": beax,
            })
    return cands


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--all", action="store_true",
                    help="also list the condition-1 candidates the call graph rejects")
    ap.add_argument("--why", metavar="SYM",
                    help="per-call-site eax verdicts for one mangled symbol")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 if the hit count exceeds N (ratchet)")
    a = ap.parse_args()

    if not REPORT.is_file():
        print("int-return-type: no build/objdiff/report.json - run `gruntz build`")
        return 2

    ctx = get_context()
    rvas = rva_index()

    if a.why:
        rva = rvas.get(a.why)
        if rva is None:
            print("int-return-type: %s is not in build/gen/symbol_names.csv" % a.why)
            return 2
        print("%s  %s" % (a.why, rva))
        for site, o, nm, v in caller_verdicts(ctx, int(rva, 16)):
            print("   %-8s @0x%08x  in %s" % (v, site, nm))
        return 0

    cands = object_side()
    hits, rejects = [], []
    for c in cands:
        rva = rvas.get(c["name"])
        c["rva"] = rva
        if rva is None:
            c["verdicts"] = []
            rejects.append(c)
            continue
        v = caller_verdicts(ctx, int(rva, 16))
        c["verdicts"] = v
        kinds = set(k for _s, _o, _n, k in v)
        c["callers"] = len(v)
        # A function with no rel32 caller (a virtual reached only through its
        # vtable slot) has nothing constraining its return type - the call graph
        # cannot vote, so it is not a hit.
        (hits if v and kinds <= {"dead"} else rejects).append(c)

    tally = " ".join("%s %d" % (k, sum(1 for h in hits if h["kind"] == k))
                     for k in ("VALUE", "GUARD", "NOVALUE"))
    print("int-return-type: %d function(s) declared int that retail returns nothing "
          "from  |  %s   (%d condition-1 candidate(s), %d rejected by the call graph)"
          % (len(hits), tally, len(hits) + len(rejects), len(rejects)))
    print("   flip the declaration to `void` and drop every `return <expr>;` - see "
          "docs/patterns/int-return-that-retail-never-sets-is-a-void.md")
    for h in sorted(hits, key=lambda h: (h["kind"], -h["pct"])):
        print("   %-7s %6.2f%%  %-14s %-10s %s"
              % (h["kind"], h["pct"], h["unit"], h["rva"] or "?", h["name"]))
        det = (", ".join("0x%x %s %s" % (o, m, op) for o, _k, m, op in h["eax"][:3])
               if h["eax"] else "no eax at any ret on either side")
        print("               rets %d->%d, %d caller(s) all discard:  %s"
              % (h["rets"][0], h["rets"][1], h["callers"], det))
    if a.all:
        print("   -- rejected (a caller consumes eax, the disposition is "
              "undetermined, or nothing calls it).")
        print("      a rejected VALUE row is still a lead: the return type is right "
              "but we materialise a constant on an exit path retail does not have.")
        for r in sorted(rejects, key=lambda r: -r["pct"]):
            bad = sorted(set(k for _s, _o, _n, k in r["verdicts"]) - {"dead"})
            if not bad:
                n = indirect_refs(ctx, int(r["rva"], 16)) if r["rva"] else 0
                bad = ["no rva"] if not r["rva"] else \
                    ["%d vtable/fn-ptr slot(s) - dispatched indirectly, the base "
                     "class fixes the type" % n] if n else ["no caller at all"]
            print("   %-7s %6.2f%%  %-14s %-10s %s   [%s]"
                  % (r["kind"], r["pct"], r["unit"], r["rva"] or "?", r["name"],
                     ",".join(bad)))

    if a.max is not None and len(hits) > a.max:
        print("int-return-type: %d hits exceeds the %d ratchet" % (len(hits), a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
